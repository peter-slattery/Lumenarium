//
// File: gs_meta.cpp
// Author: Peter Slattery
// Creation Date: 2020-01-19
//
//
// Usage
//
// GSMetaTag(<tag name>) to give commands to the meta layer
//
// Tag Values
// 
// breakpoint 
//   will cause the meta layer to break in the debugger when it reaches
//   that point in processing the file
//   TODO: specify which stage you want it to break at

#ifndef GS_META_CPP

#include <windows.h>
#include <stdio.h>

#include <gs_language.h>
#include <gs_bucket.h>
#include "..\src\gs_platform.h"
#include <gs_memory_arena.h>
#include <gs_string.h>

#include "gs_meta_lexer.h"
#include "gs_meta_error.h"

#include "foldhaus_meta_type_table.h"

struct source_code_file
{
    string Path;
    s32 FileSize;
    string Contents;
    
    s32 FirstTokenIndex;
    s32 LastTokenIndex;
};

struct token_iter
{
    gs_bucket<token>* Tokens;
    token* TokenAt;
    s32 TokenAtIndex;
    s32 FirstToken;
    s32 LastToken;
    
#define TOKEN_ITER_SNAPSHOTS_MAX 64
    u32 SnapshotsUsed;
    u32 Snapshots[TOKEN_ITER_SNAPSHOTS_MAX];
    
    errors* Errors;
};

struct gsm_profiler_scope
{
    s64 StartTime;
    s64 EndTime;
    r32 Seconds;
    r32 LongestSeconds;
    u32 CallCount;
    string Category;
    string Identifier;
};

struct gsm_profiler_category
{
    string Identifier;
    r32 TotalTime;
    u32 SubscopesCount;
    gsm_profiler_scope* LongestSubscope;
};

struct gsm_profiler
{
    gs_bucket<gsm_profiler_scope> Scopes;
    gs_bucket<gsm_profiler_category> Categories;
};

struct gs_meta_preprocessor
{
    errors Errors;
    
    gs_bucket<source_code_file> SourceFiles;
    gs_bucket<token> Tokens;
    
    gs_bucket<token> TagList;
    
    type_table TypeTable;
    
    // Performance
    s64 PreprocessorStartTime;
    s64 TokenizeTime;
    s64 PreprocTime;
    s64 FixupTime;
    s64 PreprocessorEndTime;
    
    gsm_profiler Profiler;
};

// ------------------------
//   Timing / Performance
// ------------------------

internal s64
GetWallClock ()
{
    LARGE_INTEGER Time;
    if (!QueryPerformanceCounter(&Time))
    {
        s32 Error = GetLastError();
        InvalidCodePath;
    }
    return (s64)Time.QuadPart;
}

internal s64
GetPerformanceFrequency ()
{
    LARGE_INTEGER Frequency;
    if (!QueryPerformanceFrequency(&Frequency))
    {
        s32 Error = GetLastError();
        InvalidCodePath;
    }
    return (s64)Frequency.QuadPart;
}
internal r32
GetSecondsElapsed(s64 CyclesCount)
{
    s64 Frequency = GetPerformanceFrequency();
    r32 SecondsElapsed = (r32)(CyclesCount) / (r32)(Frequency);
    return SecondsElapsed;
}

internal r32
GetSecondsElapsed(s64 StartCycles, s64 EndCycles)
{
    return GetSecondsElapsed(EndCycles - StartCycles);
}

internal gsm_profiler_scope*
FindMatchingScope(gsm_profiler* Profiler, string Category, string Identifier)
{
    gsm_profiler_scope* Result = 0;
    for (u32 i = 0; i < Profiler->Scopes.Used; i++)
    {
        gsm_profiler_scope* Scope = Profiler->Scopes.GetElementAtIndex(i);
        if (StringsEqual(Scope->Identifier, Identifier) &&
            StringsEqual(Scope->Category, Category))
        {
            Result = Scope;
            break;
        }
    }
    return Result;
}

internal gsm_profiler_scope*
BeginScope(gsm_profiler* Profiler, string Category, string Identifier)
{
    gsm_profiler_scope* Scope = FindMatchingScope(Profiler, Category, Identifier);
    if (!Scope)
    {
        Scope = Profiler->Scopes.TakeElement();
        *Scope = {};
    }
    Scope->Category = Category;
    Scope->Identifier = Identifier;
    Scope->StartTime = GetWallClock();
    return Scope;
}

internal gsm_profiler_scope*
BeginScope(gsm_profiler* Profiler, char* Category, char* Identifier)
{
    return BeginScope(Profiler, MakeStringLiteral(Category), MakeStringLiteral(Identifier));
}

internal void
EndScope(gsm_profiler_scope* Scope)
{
    Scope->EndTime = GetWallClock();
    r32 Seconds = GetSecondsElapsed(Scope->StartTime, Scope->EndTime);
    Scope->Seconds += Seconds;
    if (Seconds > Scope->LongestSeconds)
    {
        Scope->LongestSeconds = Seconds;
    }
    Scope->CallCount++;
}

internal gsm_profiler_category*
GetCategory(string Identifier, gsm_profiler* Profiler)
{
    gsm_profiler_category* Result = 0;
    
    for (u32 i = 0; i < Profiler->Categories.Used; i++)
    {
        gsm_profiler_category* Category = Profiler->Categories.GetElementAtIndex(i);
        if (StringsEqual(Identifier, Category->Identifier))
        {
            Result = Category;
            break;
        }
    }
    
    if (Result == 0)
    {
        Result = Profiler->Categories.TakeElement();
        *Result = {};
        Result->Identifier = Identifier;
    }
    
    return Result;
}

internal void
FinishProfiler(gsm_profiler* Profiler)
{
    for (u32 i = 0; i < Profiler->Scopes.Used; i++)
    {
        gsm_profiler_scope* Scope = Profiler->Scopes.GetElementAtIndex(i);
        gsm_profiler_category* Category = GetCategory(Scope->Category, Profiler);
        Category->TotalTime += Scope->Seconds;
        Category->SubscopesCount++;
        
        if (!Category->LongestSubscope ||
            Scope->Seconds > Category->LongestSubscope->Seconds)
        {
            Category->LongestSubscope = Scope;
        }
    }
}

internal void
PrintAllCategories(gsm_profiler* Profiler)
{
    for (u32 i = 0; i < Profiler->Categories.Used; i++)
    {
        gsm_profiler_category* Category = Profiler->Categories.GetElementAtIndex(i);
        printf("Category: %.*s Total Time: %.*f Count: %d\n",
               StringExpand(Category->Identifier),
               6, Category->TotalTime,
               Category->SubscopesCount);
        printf("    Longest Scope: %.*s Total Time: %.*f Longest Time: %.*f Call Count: %d\n",
               StringExpand(Category->LongestSubscope->Identifier),
               6, Category->LongestSubscope->Seconds,
               6, Category->LongestSubscope->LongestSeconds,
               Category->LongestSubscope->CallCount);
        
        if (StringsEqual(Category->Identifier, MakeStringLiteral("parse")))
        {
            for (u32 j = 0; j < Profiler->Scopes.Used; j++)
            {
                gsm_profiler_scope* Scope = Profiler->Scopes.GetElementAtIndex(j);
                if (StringsEqual(Scope->Category, Category->Identifier))
                {
                    printf("    Time: %.*f Call Count: %d Scope: %.*s\n",
                           6, Scope->Seconds,
                           Scope->CallCount,
                           StringExpand(Scope->Identifier));
                }
            }
        }
    }
}

// ------------------------
//   Token Iterator
// ------------------------

internal token*
NextToken (token_iter* Iter)
{
    if (Iter->TokenAtIndex < Iter->LastToken)
    {
        Iter->TokenAtIndex++;
        Iter->TokenAt = Iter->Tokens->GetElementAtIndex(Iter->TokenAtIndex);
    }
    
    return Iter->TokenAt;
}

internal b32
TokenAtEquals(token_iter* Iter, char* String)
{
    b32 Result = false;
    if (StringEqualsCharArray(Iter->TokenAt->Text, String))
    {
        Result = true;
        NextToken(Iter);
    }
    return Result;
}

internal b32
TokenAtEquals(token_iter* Iter, token_type Type)
{
    b32 Result = false;
    if (Iter->TokenAt->Type == Type)
    {
        Result = true;
        NextToken(Iter);
    }
    return Result;
}

internal b32
TokenAtEquals(token_iter* Iter, token_type Type, token* Token)
{
    b32 Result = false;
    if (Iter->TokenAt->Type == Type)
    {
        Result = true;
        *Token = *Iter->TokenAt;
        NextToken(Iter);
    }
    return Result;
}

internal void
PushSnapshot (token_iter* Iter)
{
    Iter->Snapshots[Iter->SnapshotsUsed++] = Iter->TokenAtIndex;
}

internal void
PopSnapshot (token_iter* Iter)
{
    if (Iter->SnapshotsUsed > 0)
    {
        Iter->SnapshotsUsed -= 1;
    }
}

internal void
ApplySnapshot (token_iter* Iter)
{
    u32 SnapshotIndex = Iter->SnapshotsUsed;
    u32 SnapshotPoint = Iter->Snapshots[SnapshotIndex];
    Iter->TokenAtIndex = SnapshotPoint;
    Iter->TokenAt = Iter->Tokens->GetElementAtIndex(SnapshotPoint);
}

internal void
ApplySnapshotIfNotParsedAndPop(b32 ParseSuccess, token_iter* Iter)
{
    PopSnapshot(Iter);
    if (!ParseSuccess)
    {
        ApplySnapshot(Iter);
    }
}


internal s32
GetFileSize (char* FileName)
{
    s32 Result = 0;
    
    FILE* ReadFile = fopen(FileName, "r");
    if (ReadFile)
    {
        fseek(ReadFile, 0, SEEK_END);
        size_t FileSize = ftell(ReadFile);
        fseek(ReadFile, 0, SEEK_SET);
        
        Result = (s32)FileSize;
        fclose(ReadFile);
    }
    
    return Result;
}

// -------------------------
//   Source File Handling
// -------------------------

internal s32
ReadEntireFileAndNullTerminate (source_code_file* File, errors* Errors)
{
    s32 LengthRead = 0;
    
    FILE* ReadFile = fopen(File->Path.Memory, "r");
    if (ReadFile)
    {
        fseek(ReadFile, 0, SEEK_END);
        size_t FileSize = ftell(ReadFile);
        fseek(ReadFile, 0, SEEK_SET);
        
        Assert(File->Contents.Memory == 0);
        File->Contents.Max = (s32)FileSize + 1;
        File->Contents.Memory = (char*)malloc(File->Contents.Max);
        
        size_t ReadSize = fread(File->Contents.Memory, 1, FileSize, ReadFile);
        File->Contents.Memory[FileSize] = 0;
        File->Contents.Length = (s32)ReadSize;
        
        LengthRead = (s32)ReadSize + 1;
        fclose(ReadFile);
    }
    else
    {
        PushFError(Errors, "Could Not Read File: %S", File->Path);
    }
    
    return LengthRead;
}

internal b32
FileAlreadyInSource(string Path, gs_bucket<source_code_file> SourceFiles)
{
    b32 Result = false;
    
    for (u32 i = 0; i < SourceFiles.Used; i++)
    {
        source_code_file* File = SourceFiles.GetElementAtIndex(i);
        if (StringsEqual(File->Path, Path))
        {
            Result = true;
            break;
        }
    }
    
    return Result;
}

internal void
AddFileToSource(string RelativePath, gs_bucket<source_code_file>* SourceFiles, errors* Errors)
{
    source_code_file File = {0};
    
    File.FirstTokenIndex = -1;
    File.LastTokenIndex = -1;
    
    u32 PathLength = RelativePath.Length + 1;
    File.Path = MakeString((char*)malloc(sizeof(char) * PathLength), 0, PathLength);
    CopyStringTo(RelativePath, &File.Path);
    NullTerminate(&File.Path);
    
    File.FileSize = ReadEntireFileAndNullTerminate(&File, Errors);
    
    if (File.FileSize > 0)
    {
        SourceFiles->PushElementOnBucket(File);
    }
    else
    {
        PushFError(Errors, "Error: Could not load file %S\n", RelativePath);
    }
}

internal void
TokenizeFile (source_code_file* File, gs_bucket<token>* Tokens)
{
    tokenizer Tokenizer = {};
    Tokenizer.At = File->Contents.Memory;
    Tokenizer.Memory = File->Contents.Memory;
    Tokenizer.MemoryLength = File->Contents.Max;
    
    token* LastToken = 0;
    while(AtValidPosition(Tokenizer))
    {
        token NewToken = GetNextToken(&Tokenizer);
        u32 TokenIndex = Tokens->PushElementOnBucket(NewToken);
        if (File->FirstTokenIndex < 0)
        {
            File->FirstTokenIndex = (s32)TokenIndex;
        }
    }
    
    File->LastTokenIndex = Tokens->Used - 1;
}

// ------------------------
//         Parsing
// ------------------------

internal b32
ParseMetaTag(token_iter* Iter, gs_meta_preprocessor* Meta)
{
    gsm_profiler_scope* ProfilerScope = BeginScope(&Meta->Profiler, 
                                                   MakeStringLiteral("parse"),
                                                   MakeStringLiteral("ParseMetaTag"));
    b32 Result = false;
    PushSnapshot(Iter);
    
    if (TokenAtEquals(Iter, "GSMetaTag") &&
        TokenAtEquals(Iter, "("))
    {
        token MetaIdentifier = {0};
        if (TokenAtEquals(Iter, Token_Identifier, &MetaIdentifier))
        {
            Meta->TagList.PushElementOnBucket(MetaIdentifier);
            if (StringsEqual(MetaIdentifier.Text, MakeStringLiteral("breakpoint")))
            {
                // NOTE(Peter): This is not a temporary breakpoint. It is 
                // used to be able to break the meta program at specific points
                // throughout execution
                __debugbreak();
            }
            
            if (TokenAtEquals(Iter, ")") &&
                TokenAtEquals(Iter, ";"))
            {
                Result = true;
            }
        }
    }
    
    ApplySnapshotIfNotParsedAndPop(Result, Iter);
    EndScope(ProfilerScope);
    return Result;
}

internal b32
ParseSignedness (token_iter* Iter)
{
    // NOTE(Peter): This doesn't really do much at the moment, but 
    // I want all signedness parsing to happen in one place in case
    // we ever need to do anything with it.
    
    b32 Result = false;
    
    if (TokenAtEquals(Iter, "unsigned") ||
        TokenAtEquals(Iter, "signed"))
    {
        Result = true;
    }
    
    return Result;
}

internal b32
ShortInt (token_iter* Iter, type_table_handle* TypeHandleOut, type_table TypeTable)
{
    b32 Result = false;
    PushSnapshot(Iter);
    
    ParseSignedness(Iter);
    if (TokenAtEquals(Iter, "short"))
    {
        Result = true;
        if (TokenAtEquals(Iter, "int"))
        {
            Result = true;
        }
    }
    
    ApplySnapshotIfNotParsedAndPop(Result, Iter);
    if (Result) 
    { 
        *TypeHandleOut = GetTypeHandle(MakeStringLiteral("short int"), TypeTable);
    }
    return Result;
}

internal b32
Int (token_iter* Iter, type_table_handle* TypeHandleOut, type_table TypeTable)
{
    b32 Result = false;
    PushSnapshot(Iter);
    
    ParseSignedness(Iter);
    if (TokenAtEquals(Iter, "int"))
    {
        Result = true;
    }
    
    ApplySnapshotIfNotParsedAndPop(Result, Iter);
    if (Result) 
    { 
        *TypeHandleOut = GetTypeHandle(MakeStringLiteral("int"), TypeTable);
    }
    return Result;
}

internal b32
LongInt (token_iter* Iter, type_table_handle* TypeHandleOut, type_table TypeTable)
{
    b32 Result = false;
    PushSnapshot(Iter);
    
    ParseSignedness(Iter);
    if (TokenAtEquals(Iter, "long"))
    {
        Result = true;
        if (TokenAtEquals(Iter, "int"))
        {
            Result = true;
        }
    }
    
    ApplySnapshotIfNotParsedAndPop(Result, Iter);
    if (Result) 
    { 
        *TypeHandleOut = GetTypeHandle(MakeStringLiteral("long int"), TypeTable);
    }
    return Result;
}

internal b32
LongLongInt (token_iter* Iter, type_table_handle* TypeHandleOut, type_table TypeTable)
{
    b32 Result = false;
    PushSnapshot(Iter);
    
    ParseSignedness(Iter);
    if (TokenAtEquals(Iter, "long"))
    {
        if (TokenAtEquals(Iter, "long"))
        {
            Result = true;
            if (TokenAtEquals(Iter, "int"))
            {
                Result = true;
            }
        }
    }
    
    ApplySnapshotIfNotParsedAndPop(Result, Iter);
    if (Result) 
    { 
        *TypeHandleOut = GetTypeHandle(MakeStringLiteral("long long int"), TypeTable);
    }
    return Result;
}

internal b32
ParseChar(token_iter* Iter, type_table_handle* TypeHandleOut, type_table TypeTable)
{
    b32 Result = false;
    PushSnapshot(Iter);
    
    ParseSignedness(Iter);
    if (TokenAtEquals(Iter, "char"))
    {
        Result = true;
        *TypeHandleOut = GetTypeHandle(MakeStringLiteral("char"), TypeTable);
    }
    else if (TokenAtEquals(Iter, "wchar_t"))
    {
        Result = true;
        *TypeHandleOut = GetTypeHandle(MakeStringLiteral("wchar_t"), TypeTable);
    }
    
    ApplySnapshotIfNotParsedAndPop(Result, Iter);
    return Result;
}

internal b32
ParseBool(token_iter* Iter, type_table_handle* TypeHandleOut, type_table TypeTable)
{
    b32 Result = false;
    PushSnapshot(Iter);
    
    if (TokenAtEquals(Iter, "bool"))
    {
        Result = true;
        *TypeHandleOut = GetTypeHandle(MakeStringLiteral("bool"), TypeTable);
    }
    
    ApplySnapshotIfNotParsedAndPop(Result, Iter);
    return Result;
}

internal b32
ParseFloat(token_iter* Iter, type_table_handle* TypeHandleOut, type_table TypeTable)
{
    b32 Result = false;
    PushSnapshot(Iter);
    
    if (TokenAtEquals(Iter, "float"))
    {
        Result = true;
        *TypeHandleOut = GetTypeHandle(MakeStringLiteral("float"), TypeTable);
    }
    
    ApplySnapshotIfNotParsedAndPop(Result, Iter);
    return Result;
}

internal b32
ParseDouble(token_iter* Iter, type_table_handle* TypeHandleOut, type_table TypeTable)
{
    b32 Result = false;
    PushSnapshot(Iter);
    
    if (TokenAtEquals(Iter, "double"))
    {
        Result = true;
        *TypeHandleOut = GetTypeHandle(MakeStringLiteral("double"), TypeTable);
    }
    
    ApplySnapshotIfNotParsedAndPop(Result, Iter);
    return Result;
}

// :UndeclaredType
// NOTE(Peter): If TypeIndexOut is -1, you need to call NextToken after this
// function to advance past the type identifier.
internal b32
ParseType(token_iter* Iter, gs_meta_preprocessor* Meta, type_table_handle* TypeHandleOut)
{
    gsm_profiler_scope* ProfilerScope = BeginScope(&Meta->Profiler, 
                                                   MakeStringLiteral("parse"),
                                                   MakeStringLiteral("ParseType"));
    b32 Result = false;
    *TypeHandleOut = InvalidTypeTableHandle;
    PushSnapshot(Iter);
    
    if (ParseChar(Iter, TypeHandleOut, Meta->TypeTable) ||
        ParseBool(Iter, TypeHandleOut, Meta->TypeTable) ||
        LongLongInt(Iter, TypeHandleOut, Meta->TypeTable) ||
        LongInt(Iter, TypeHandleOut, Meta->TypeTable) ||
        ShortInt(Iter, TypeHandleOut, Meta->TypeTable) ||
        Int(Iter, TypeHandleOut, Meta->TypeTable) ||
        ParseFloat(Iter, TypeHandleOut, Meta->TypeTable) ||
        ParseDouble(Iter, TypeHandleOut, Meta->TypeTable))
    {
        Result = true;
    } 
    else if (StringsEqual(Iter->TokenAt->Text, MakeStringLiteral("void")))
    {
        NextToken(Iter);
        Result = true;
        *TypeHandleOut = GetTypeHandle(MakeStringLiteral("void"), Meta->TypeTable);
    }
    else 
    {
        gsm_profiler_scope* ProfileInnerScope = BeginScope(&Meta->Profiler, 
                                                           MakeStringLiteral("parse"),
                                                           MakeStringLiteral("ParseTypeInner"));
        
        *TypeHandleOut = GetTypeHandle(Iter->TokenAt->Text, Meta->TypeTable);
        if (TypeHandleIsValid(*TypeHandleOut))
        {
            Result = true;
            NextToken(Iter);
        }
        else if(Iter->TokenAt->Type == Token_Identifier)
        {
            Result = true;
            // NOTE(Peter): In this case, we believe we are at a type identifier,
            // however, it hasn't been declared yet. This is due to the fact that we 
            // tokenize files, then parse them, then import the files they include, and
            // then begin tokenizing, parsing, etc for those files. 
            // In the case that we get an as-of-yet undeclared type, we leave it
            // up to the calling site to determine what to do with that information
            // :UndeclaredType
            *TypeHandleOut = InvalidTypeTableHandle;
        }
        
        EndScope(ProfileInnerScope);
    }
    
    ApplySnapshotIfNotParsedAndPop(Result, Iter);
    EndScope(ProfilerScope);
    return Result;
}

internal b32
ParsePointer (token_iter* Iter)
{
    b32 Result = false;
    if (TokenAtEquals(Iter, "*"))
    {
        Result = true;
    }
    return Result;
}

internal b32
ParseConstVolatile (token_iter* Iter)
{
    b32 Result = false;
    PushSnapshot(Iter);
    
    if (TokenAtEquals(Iter, "volatile") ||
        TokenAtEquals(Iter, "const"))
    {
        Result = true;
    }
    
    ApplySnapshotIfNotParsedAndPop(Result, Iter);
    return Result;
}

internal b32
ParseVariableDecl(token_iter* Iter, gs_bucket<variable_decl>* VariableList, gs_meta_preprocessor* Meta)
{
    gsm_profiler_scope* ProfilerScope = BeginScope(&Meta->Profiler, 
                                                   MakeStringLiteral("parse"),
                                                   MakeStringLiteral("ParseVariableDecl"));
    b32 Result = false;
    PushSnapshot(Iter);
    
    if (ParseConstVolatile(Iter))
    {
        // NOTE(Peter): we don't do anything with this atm
        // dont have a reason to just yet
        // :UnusedConstVolatile
    }
    
    type_table_handle TypeHandle = InvalidTypeTableHandle;
    if (ParseType(Iter, Meta, &TypeHandle))
    {
        // :UndeclaredType
        if (!TypeHandleIsValid(TypeHandle))
        {
            TypeHandle = PushUndeclaredType(Iter->TokenAt->Text, &Meta->TypeTable);
            NextToken(Iter);
        }
        
        b32 IsPointer = ParsePointer(Iter);
        
        if (ParseConstVolatile(Iter))
        {
            // :UnusedConstVolatile
        }
        
        for(;;) {
            token IdentifierToken = {};
            if (TokenAtEquals(Iter, Token_Identifier, &IdentifierToken))
            {
                // Array Notationg ie r32 x[2];
                // NOTE(Peter): True initially because if there is no array notation, we 
                // are still ok to proceed
                b32 ArrayParseSuccess = true;
                u32 ArrayCount = 0;
                if (TokenAtEquals(Iter, "["))
                {
                    // NOTE(Peter): Once we get to this point, we have to complete the entire
                    // array notation before we have successfully parsed, hence setting 
                    // ArrayParseSucces to false here.
                    ArrayParseSuccess = false;
                    token NumberToken = {};
                    if (TokenAtEquals(Iter, Token_Number, &NumberToken))
                    {
                        parse_result ParseArrayCount = ParseUnsignedInt(StringExpand(NumberToken.Text));
                        ArrayCount = ParseArrayCount.UnsignedIntValue;
                    }
                    else
                    {
                        // TODO(Peter): Actually handle const expr for arrays
                        while (!StringsEqual(Iter->TokenAt->Text, MakeStringLiteral("]")))
                        {
                            NextToken(Iter);
                        }
                    }
                    
                    if (TokenAtEquals(Iter, "]"))
                    {
                        ArrayParseSuccess = true;
                    }
                }
                
                if (ArrayParseSuccess)
                {
                    Result = true;
                    
                    variable_decl* Decl = VariableList->TakeElement();
                    *Decl = {};
                    Decl->Identifier = IdentifierToken.Text;
                    Decl->TypeHandle = TypeHandle;
                    Decl->Pointer = IsPointer;
                    Decl->ArrayCount = ArrayCount;
                    CopyMetaTagsAndClear(&Meta->TagList, &Decl->MetaTags);
                }
            }
            
            if (StringsEqual(Iter->TokenAt->Text, MakeStringLiteral(",")))
            {
                // NOTE(Peter): There are two ways we enter this case
                // 1. We are parsing a declaration list ie. r32 x, y, z;
                // 2. We are parsing a function parameter list ie void proc(r32 x, u32 y)
                //    In this instance, we could still be parsing a declaration list
                //    ie. this is valid: void proc(r32 x, y, double z)
                
                // This first snapshot is so we can rewind to before the comma in the event that
                // we are parsing a function parameter list
                PushSnapshot(Iter);
                NextToken(Iter);
                
                // This second snapshot is so we can rewind to just _after_ the comma
                // and continue parsing in the event that we are in a declaration list
                PushSnapshot(Iter);
                
                if (TokenAtEquals(Iter, Token_Identifier) &&
                    (TokenAtEquals(Iter, ",") || TokenAtEquals(Iter, ";")))
                {
                    // We are in a declaration list (case 1)
                    ApplySnapshotIfNotParsedAndPop(false, Iter);
                    PopSnapshot(Iter); // We don't need the first snapshot in this case
                }
                else
                {
                    // We are in a function parameter list (case 2)
                    ApplySnapshotIfNotParsedAndPop(false, Iter);
                    ApplySnapshotIfNotParsedAndPop(false, Iter);
                    break;
                }
            }
            else
            {
                break;
            }
        }
    }
    
    ApplySnapshotIfNotParsedAndPop(Result, Iter);
    EndScope(ProfilerScope);
    return Result;
}

internal b32
StructOrUnion(token_iter* Iter, type_definition_type* Type)
{
    b32 Result = false;
    if (TokenAtEquals(Iter, "struct"))
    {
        Result = true;
        *Type = TypeDef_Struct;
    }
    else if (TokenAtEquals(Iter, "union"))
    {
        Result = true;
        *Type = TypeDef_Union;
    }
    return Result;
}

// NOTE(Peter): ContainingStruct will be 0 in all cases except when the struct or union
// is anonymous. In those cases, it MUST be the struct or union
// containing the anonymous struct/union
internal b32
ParseStruct(token_iter* Iter, type_table_handle* StructTypeHandleOut, gs_meta_preprocessor* Meta, type_definition*  ContainingStruct = 0)
{
    gsm_profiler_scope* ProfilerScope = BeginScope(&Meta->Profiler, 
                                                   MakeStringLiteral("parse"),
                                                   MakeStringLiteral("ParseStruct"));
    b32 Result = false;
    *StructTypeHandleOut = InvalidTypeTableHandle;
    
    PushSnapshot(Iter);
    
    type_definition_type DeclType;
    if (StructOrUnion(Iter, &DeclType))
    {
        token IdentifierToken = {};
        if (TokenAtEquals(Iter, Token_Identifier, &IdentifierToken)) {}
        
        // TODO(Peter): Handle name coming after the struct
        if (TokenAtEquals(Iter, "{"))
        {
            type_definition StructDecl = {};
            if (IdentifierToken.Text.Length > 0)
            {
                StructDecl.Identifier = IdentifierToken.Text;
                StructDecl.Struct.IsAnonymous = false;
            }
            else
            {
                Assert(ContainingStruct);
                Assert(ContainingStruct->Identifier.Length > 0);
                // NOTE(Peter): I'm not sure this is neccessary, but I don't know what
                // cases that its not true would be so I'm asserting just to find out
                Assert(ContainingStruct->Type == TypeDef_Union ||
                       ContainingStruct->Type == TypeDef_Struct);
                
                string AnonStructIdentifier = {};
                AnonStructIdentifier.Max = 256;
                AnonStructIdentifier.Memory = (char*)malloc(sizeof(char) * AnonStructIdentifier.Max);
                
                PrintF(&AnonStructIdentifier, "%S_%d", ContainingStruct->Identifier, ContainingStruct->Struct.MemberDecls.Used);
                
                StructDecl.Identifier = AnonStructIdentifier;
                StructDecl.Struct.IsAnonymous = true;
            }
            
            StructDecl.Type = DeclType;
            CopyMetaTagsAndClear(&Meta->TagList, &StructDecl.MetaTags);
            
            while (!TokenAtEquals(Iter, "}"))
            {
                type_table_handle MemberStructTypeHandle = InvalidTypeTableHandle;
                variable_decl MemberDecl = {};
                if (ParseMetaTag(Iter, Meta))
                {
                    
                }
                else if (ParseVariableDecl(Iter, &StructDecl.Struct.MemberDecls, Meta))
                {
                    if (!TokenAtEquals(Iter, ";"))
                    {
                        PushFError(Iter->Errors, "No semicolon after struct member variable declaration. %S", StructDecl.Identifier);
                    }
                }
                else if (ParseStruct(Iter, &MemberStructTypeHandle, Meta, &StructDecl))
                {
                    // NOTE(Peter): Pretty sure, since we just parsed the struct, that
                    // MemberStructTypeIndex should never be Invalid (unknown type). 
                    // Putting this Assert here for now, but remove if there's a valid
                    // reason that you might not be able to find a struct just parsed at
                    // this point.
                    Assert(TypeHandleIsValid(MemberStructTypeHandle)); 
                    
                    MemberDecl.TypeHandle = MemberStructTypeHandle;
                    StructDecl.Struct.MemberDecls.PushElementOnBucket(MemberDecl);
                }
                else
                {
                    // NOTE(Peter): One of the things that falls through here is
                    // cpp template stuff. Eventually, we should be able to use
                    // this meta layer to get rid of them all together, and then
                    // we can just disallow CPP templates
                    NextToken(Iter);
                }
            }
            
            if (TokenAtEquals(Iter, ";"))
            {
                Result = true;
                *StructTypeHandleOut = PushTypeDefOnTypeTable(StructDecl, &Meta->TypeTable);
            }
        }
    }
    
    ApplySnapshotIfNotParsedAndPop(Result, Iter);
    EndScope(ProfilerScope);
    return Result;
}

// ( type *? identifier, ... )
internal b32
ParseFunctionParameterList (token_iter* Iter, type_definition* FunctionPtrDecl, gs_meta_preprocessor* Meta)
{
    b32 Result = false;
    PushSnapshot(Iter);
    
    if (TokenAtEquals(Iter, "("))
    {
        Result = true;
        
        while(!StringsEqual(Iter->TokenAt->Text, MakeStringLiteral(")")))
        {
            if (ParseVariableDecl(Iter, &FunctionPtrDecl->FunctionPtr.Parameters, Meta))
            {
                if (TokenAtEquals(Iter, Token_Comma))
                {
                }
                else if (!StringsEqual(Iter->TokenAt->Text, MakeStringLiteral(")")))
                {
                    Result = false;
                    break;
                }
            }
        }
        
        if (TokenAtEquals(Iter, ")"))
        {
            Result = true;
        }
    }
    
    ApplySnapshotIfNotParsedAndPop(Result, Iter);
    return Result;
}

internal b32
ParseFunctionDeclaration (token_iter* Iter, token* Identifier, gs_meta_preprocessor* Meta)
{
    b32 Result = false;
    PushSnapshot(Iter);
    
    type_table_handle ReturnTypeHandle = InvalidTypeTableHandle;
    if (ParseType(Iter, Meta, &ReturnTypeHandle))
    {
        if (!TypeHandleIsValid(ReturnTypeHandle)) 
        { 
            ReturnTypeHandle = PushUndeclaredType(Iter->TokenAt->Text, &Meta->TypeTable);
            NextToken(Iter); 
        }
        
        b32 IsPointer = ParsePointer(Iter);
        
        if (TokenAtEquals(Iter, Token_Identifier, Identifier))
        {
            type_definition FunctionPtr = {};
            FunctionPtr.Identifier = Identifier->Text;
            FunctionPtr.Size = sizeof(void*);
            CopyMetaTagsAndClear(&Meta->TagList, &FunctionPtr.MetaTags);
            FunctionPtr.Type = TypeDef_FunctionPointer;
            FunctionPtr.Pointer = true;
            FunctionPtr.FunctionPtr = {};
            FunctionPtr.FunctionPtr.ReturnTypeHandle = ReturnTypeHandle;
            
            if (ParseFunctionParameterList(Iter, &FunctionPtr, Meta))
            {
                if (TokenAtEquals(Iter, ";"))
                {
                    Result = true;
                    PushTypeDefOnTypeTable(FunctionPtr, &Meta->TypeTable);
                }
            }
        }
    }
    
    ApplySnapshotIfNotParsedAndPop(Result, Iter);
    if (!Result) 
    { 
        *Identifier = {0};
    }
    return Result;
}

internal b32 
ParseTypedef(token_iter* Iter, gs_meta_preprocessor* Meta)
{
    gsm_profiler_scope* ProfilerScope = BeginScope(&Meta->Profiler, 
                                                   MakeStringLiteral("parse"),
                                                   MakeStringLiteral("ParseTypedef"));
    b32 Result = false;
    PushSnapshot(Iter);
    
    if (TokenAtEquals(Iter, "typedef"))
    {
        token TypeToken = {0};
        type_table_handle TypeHandle = InvalidTypeTableHandle;
        if (TokenAtEquals(Iter, "struct") &&
            ParseStruct(Iter, &TypeHandle, Meta))
        {
            Result = true;
        }
        else if (ParseFunctionDeclaration(Iter, &TypeToken, Meta))
        {
            Result = true;
        }
        else if (ParseType(Iter, Meta, &TypeHandle))
        {
            if (!TypeHandleIsValid(TypeHandle))
            {
                TypeHandle = PushUndeclaredType(Iter->TokenAt->Text, &Meta->TypeTable);
                NextToken(Iter);
            }
            
            b32 IsPointer = ParsePointer(Iter);
            
            type_definition* BasisType = GetTypeDefinition(TypeHandle, Meta->TypeTable);
            
            type_definition NewType = {};
            NewType.Size = BasisType->Size;
            CopyMetaTagsAndClear(&Meta->TagList, &NewType.MetaTags);
            NewType.Type = BasisType->Type;
            if (NewType.Type == TypeDef_Struct || 
                NewType.Type == TypeDef_Union)
            {
                NewType.Struct = BasisType->Struct;
            }
            NewType.Pointer = BasisType->Pointer || IsPointer;
            
            token IdentifierToken = {};
            if (TokenAtEquals(Iter, Token_Identifier, &IdentifierToken))
            {
                NewType.Identifier = IdentifierToken.Text;
                PushTypeDefOnTypeTable(NewType, &Meta->TypeTable);
                Result = true;
            }
        }
        else
        {
            string* Error = TakeError(Iter->Errors);
            PrintF(Error, "unhandled typedef ");
            while (!TokenAtEquals(Iter, ";"))
            {
                PrintF(Error, "%S ", Iter->TokenAt->Text);
                NextToken(Iter);
            }
            PrintF(Error, "\n");
        }
    }
    
    
    ApplySnapshotIfNotParsedAndPop(Result, Iter);
    EndScope(ProfilerScope);
    return Result;
}

internal b32
ParseEnum (token_iter* Iter, gs_meta_preprocessor* Meta)
{
    gsm_profiler_scope* ProfilerScope = BeginScope(&Meta->Profiler, 
                                                   MakeStringLiteral("parse"),
                                                   MakeStringLiteral("ParseEnum"));
    b32 Result = false;
    PushSnapshot(Iter);
    
    if (TokenAtEquals(Iter, "enum"))
    {
        token IdentifierToken = {};
        if (TokenAtEquals(Iter, Token_Identifier, &IdentifierToken))
        {
            type_definition EnumDecl = {};
            EnumDecl.Identifier = IdentifierToken.Text;
            EnumDecl.Size = sizeof(u32);
            CopyMetaTagsAndClear(&Meta->TagList, &EnumDecl.MetaTags);
            EnumDecl.Type = TypeDef_Enum;
            
            if (TokenAtEquals(Iter, "{"))
            {
                u32 EnumAcc = 0;
                
                while (!StringsEqual(Iter->TokenAt->Text, MakeStringLiteral("}")))
                {
                    token EnumIdentifierToken = {};
                    if (TokenAtEquals(Iter, Token_Identifier, &EnumIdentifierToken))
                    {
                        if (TokenAtEquals(Iter, "="))
                        {
                            // TODO(Peter): TempValue is just here until we handle all
                            // const expr that could define an enum value. Its there so 
                            // that if the first token of an expression is a number, 
                            // we can avoid using anything from the expression.
                            u32 TempValue = EnumAcc;
                            token NumberToken = {};
                            if (TokenAtEquals(Iter, Token_Number, &NumberToken))
                            {
                                parse_result ParsedExpr = ParseSignedInt(StringExpand(NumberToken.Text));
                                TempValue = ParsedExpr.SignedIntValue;
                            }
                            
                            // TODO(Peter): Handle setting enums equal to other kinds
                            // of const exprs.
                            // We're skipping a whole bunch of stuff now
                            while (!(StringsEqual(Iter->TokenAt->Text, MakeStringLiteral(",")) ||
                                     StringsEqual(Iter->TokenAt->Text, MakeStringLiteral("}"))))
                            {
                                TempValue = EnumAcc;
                                NextToken(Iter);
                            }
                            
                            EnumAcc = TempValue;
                        }
                        
                        s32 EnumValue = EnumAcc++;
                        if (TokenAtEquals(Iter, ",") ||
                            StringsEqual(Iter->TokenAt->Text, MakeStringLiteral("}")))
                        {
                            EnumDecl.Enum.Identifiers.PushElementOnBucket(EnumIdentifierToken.Text);
                            EnumDecl.Enum.Values.PushElementOnBucket(EnumValue);
                        }
                        else if (!StringsEqual(Iter->TokenAt->Text, MakeStringLiteral("}")))
                        {
                            Result = false;
                            break;
                        }
                    }
                }
                
                if (TokenAtEquals(Iter, "}") &&
                    TokenAtEquals(Iter, ";"))
                {
                    PushTypeDefOnTypeTable(EnumDecl, &Meta->TypeTable);
                    Result = true;
                }
            }
        }
    }
    
    ApplySnapshotIfNotParsedAndPop(Result, Iter);
    EndScope(ProfilerScope);
    return Result;
}

internal b32
ParseFunction (token_iter* Iter, gs_meta_preprocessor* Meta)
{
    gsm_profiler_scope* ProfilerScope = BeginScope(&Meta->Profiler, 
                                                   MakeStringLiteral("parse"),
                                                   MakeStringLiteral("ParseFunction"));
    b32 Result = false;
    PushSnapshot(Iter);
    
    type_table_handle ReturnTypeHandle = InvalidTypeTableHandle;
    if (ParseType(Iter, Meta, &ReturnTypeHandle))
    {
        token IdentifierToken = {};
        if (TokenAtEquals(Iter, Token_Identifier, &IdentifierToken) &&
            TokenAtEquals(Iter, "("))
        {
            gsm_profiler_scope* ProfilerInnerScope = BeginScope(&Meta->Profiler, 
                                                                MakeStringLiteral("parse"),
                                                                MakeStringLiteral("ParseFunctionInner"));
            type_definition FunctionDecl = {};
            FunctionDecl.Identifier = IdentifierToken.Text;
            FunctionDecl.Function.ReturnTypeHandle = ReturnTypeHandle;
            CopyMetaTagsAndClear(&Meta->TagList, &FunctionDecl.MetaTags);
            FunctionDecl.Type = TypeDef_Function;
            FunctionDecl.Function.Parameters = {};
            
            while (!StringsEqual(Iter->TokenAt->Text, MakeStringLiteral(")")))
            {
                if (ParseVariableDecl(Iter, &FunctionDecl.Function.Parameters, Meta))
                {
                    
                }
                
                if(!TokenAtEquals(Iter, ","))
                {
                    break;
                }
            }
            
            if (TokenAtEquals(Iter, ")"))
            {
                Result = true;
                PushTypeDefOnTypeTable(FunctionDecl, &Meta->TypeTable);
            }
            EndScope(ProfilerInnerScope);
        }
    }
    
    ApplySnapshotIfNotParsedAndPop(Result, Iter);
    EndScope(ProfilerScope);
    return Result;
}

internal void
PrintIndent (u32 Indent)
{
    for (u32 i = 0; i < Indent; i++)
    {
        printf("    ");
    }
}

internal void PrintStructDecl (type_definition* StructDecl, type_table TypeTable, u32 Indent);

internal void
PrintVariableDecl (variable_decl Member, type_table TypeTable, u32 Indent = 0)
{
    type_definition* MemberTypeDef = GetTypeDefinition(Member.TypeHandle, TypeTable);
    if ((MemberTypeDef->Type == TypeDef_Struct || MemberTypeDef->Type == TypeDef_Union)
        && MemberTypeDef->Identifier.Length == 0)
    {
        PrintStructDecl(MemberTypeDef, TypeTable, Indent);
    }
    else
    {
        PrintIndent(Indent);
        if (!TypeHandleIsValid(Member.TypeHandle))
        {
            printf("???? ");
        }
        printf("%.*s ", StringExpand(MemberTypeDef->Identifier));
    }
    
    if (Member.Pointer)
    {
        printf("* ");
    }
    
    printf("%.*s", StringExpand(Member.Identifier));
    
    if (Member.ArrayCount > 0)
    {
        printf("[%d]", Member.ArrayCount);
    }
}

internal void
PrintStructDecl (type_definition* StructDecl, type_table TypeTable, u32 Indent = 0)
{
    Assert(StructDecl->Type == TypeDef_Struct ||
           StructDecl->Type == TypeDef_Union);
    
    PrintIndent(Indent);
    if (StructDecl->Type == TypeDef_Struct)
    {
        printf("struct ");
    }
    else if (StructDecl->Type == TypeDef_Union)
    {
        printf("union ");
    }
    else { InvalidCodePath; }
    
    if (StructDecl->Identifier.Length > 0)
    {
        printf("%.*s ", StringExpand(StructDecl->Identifier));
    }
    printf("{\n");
    
    for (u32 MemberIndex = 0; MemberIndex < StructDecl->Struct.MemberDecls.Used; MemberIndex++)
    {
        variable_decl* Member = StructDecl->Struct.MemberDecls.GetElementAtIndex(MemberIndex);
        PrintVariableDecl(*Member, TypeTable, Indent + 1);
        printf(";\n");
    }
    PrintIndent(Indent);
    printf("} ( size = %d ) ", StructDecl->Size);
}

internal void
PrintFunctionPtrDecl (type_definition* FnPtrDecl, type_table TypeTable)
{
    type_definition* ReturnType = GetTypeDefinition(FnPtrDecl->FunctionPtr.ReturnTypeHandle, TypeTable);
    printf("%.*s ", StringExpand(ReturnType->Identifier));
    
    if (FnPtrDecl->Identifier.Length > 0)
    {
        printf("%.*s ", StringExpand(FnPtrDecl->Identifier));
    }
    printf("(");
    
    for (u32 MemberIndex = 0; MemberIndex < FnPtrDecl->FunctionPtr.Parameters.Used; MemberIndex++)
    {
        variable_decl* Param = FnPtrDecl->FunctionPtr.Parameters.GetElementAtIndex(MemberIndex);
        PrintVariableDecl(*Param, TypeTable, 0);
        printf(", ");
    }
    
    printf(");");
}

internal gs_meta_preprocessor
PreprocessProgram (char* SourceFile)
{
    gs_meta_preprocessor Meta = {};
    
    gsm_profiler_scope* TotalScope = BeginScope(&Meta.Profiler, "total", "total");
    
    Meta.PreprocessorStartTime = GetWallClock();
    
    PopulateTableWithDefaultCPPTypes(&Meta.TypeTable);
    
    string CurrentWorkingDirectory = MakeString((char*)malloc(1024), 0, 1024);
    
    string RootFile = MakeString(SourceFile);
    AddFileToSource(RootFile, &Meta.SourceFiles, &Meta.Errors);
    
    s32 LastSlash = ReverseSearchForCharInSet(RootFile, "\\/");
    if (LastSlash <= 0)
    {
        PushFError(&Meta.Errors, "%S: File path invalid.", RootFile);
        return Meta;
    }
    
    string RootPath = Substring(RootFile, 0, LastSlash + 1);
    CopyStringTo(RootPath, &CurrentWorkingDirectory);
    
    for (u32 SourceFileIdx = 0; SourceFileIdx < Meta.SourceFiles.Used; SourceFileIdx++)
    {
        source_code_file* File = Meta.SourceFiles.GetElementAtIndex(SourceFileIdx);
        
        gsm_profiler_scope* FileScope = BeginScope(&Meta.Profiler, 
                                                   MakeStringLiteral("file"),
                                                   File->Path);
        
        gsm_profiler_scope* TokenizeScope = BeginScope(&Meta.Profiler, 
                                                       MakeStringLiteral("tokenize"),
                                                       File->Path);
        TokenizeFile(File, &Meta.Tokens);
        EndScope(TokenizeScope);
        
        gsm_profiler_scope* PreprocScope = BeginScope(&Meta.Profiler, 
                                                      MakeStringLiteral("preproc"),
                                                      File->Path);
        token_iter Iter = {};
        Iter.Tokens = &Meta.Tokens;
        Iter.FirstToken = File->FirstTokenIndex;
        Iter.LastToken = File->LastTokenIndex;
        Iter.TokenAtIndex = Iter.FirstToken;
        Iter.TokenAt = Meta.Tokens.GetElementAtIndex(Iter.TokenAtIndex);
        Iter.Errors = &Meta.Errors;
        
        while (Iter.TokenAtIndex < Iter.LastToken)
        {
            b32 ParseSuccess = false;
            
            type_table_handle TypeHandle = InvalidTypeTableHandle;
            if (TokenAtEquals(&Iter, "#include"))
            {
                token* IncludeFile = Iter.TokenAt;
                
                // NOTE(Peter): For now we aren't going in and preprocessing the header files
                // we include from the system
                // Token_Operator is used to check if the include is of the form '#include <header.h>'
                // and skip it. 
                // TODO(Peter): This is only a rough approximation of ignoring system headers
                // TODO(Peter): We should actually see what parsing system headers would entail
                if (IncludeFile->Type != Token_Operator)
                {
                    string TempFilePath = IncludeFile->Text;
                    gsm_profiler_scope* IncludeScope = BeginScope(&Meta.Profiler, 
                                                                  MakeStringLiteral("include"),
                                                                  TempFilePath);
                    
                    // NOTE(Peter): if the path is NOT absolute ie "C:\etc
                    if (!(IsAlpha(TempFilePath.Memory[0]) && 
                          TempFilePath.Memory[1] == ':' && 
                          TempFilePath.Memory[2] == '\\'))
                    {
                        TempFilePath = CurrentWorkingDirectory;
                        ConcatString(IncludeFile->Text, &TempFilePath);
                        NullTerminate(&TempFilePath);
                    }
                    
                    ParseSuccess = true;
                    if (!FileAlreadyInSource(TempFilePath, Meta.SourceFiles))
                    {
                        AddFileToSource(TempFilePath, &Meta.SourceFiles, &Meta.Errors);
                    }
                    EndScope(IncludeScope);
                }
            }
            else if(ParseMetaTag(&Iter, &Meta))
            {
                ParseSuccess = true;
            }
            else if (ParseEnum(&Iter, &Meta))
            {
                ParseSuccess = true;
            }
            else if (ParseStruct(&Iter, &TypeHandle, &Meta))
            {
                ParseSuccess = true;
            }
            else if (ParseTypedef(&Iter, &Meta))
            {
                ParseSuccess = true;
            }
            else if (ParseFunction(&Iter, &Meta))
            {
                ParseSuccess = true;
            }
            
            if (!ParseSuccess)
            {
                NextToken(&Iter);
            }
        }
        EndScope(PreprocScope);
        EndScope(FileScope);
    }
    
    // Type Table Fixup
    gsm_profiler_scope* FixupScope = BeginScope(&Meta.Profiler, "fixup", "fixup");
    
    for (u32 b = 0; b < Meta.TypeTable.TypeBucketsCount; b++)
    {
        type_table_hash_bucket Bucket = Meta.TypeTable.Types[b];
        for (u32 i = 0; i < TYPE_TABLE_BUCKET_MAX; i++)
        {
            if (Bucket.Keys[i] > 0)
            {
                type_table_handle Handle = {};
                Handle.BucketIndex = b;
                Handle.IndexInBucket = i;
                
                type_definition* TypeDef = GetTypeDefinitionUnsafe(Handle, Meta.TypeTable);
                if (TypeDef)
                {
                    if (TypeDef->Type == TypeDef_Struct)
                    {
                        FixUpStructSize(Handle, Meta.TypeTable, &Meta.Errors);
                    }
                    else if (TypeDef->Type == TypeDef_Union)
                    {
                        FixUpUnionSize(Handle, Meta.TypeTable, &Meta.Errors);
                    }
                }
            }
        }
    }
    
    EndScope(FixupScope);
    EndScope(TotalScope);
    return Meta;
}

internal void
FinishMetaprogram(gs_meta_preprocessor* Meta)
{
    FinishProfiler(&Meta->Profiler);
    
    PrintAllErrors(Meta->Errors);
    
    printf("\nMetaprogram Performance:\n");
    PrintAllCategories(&Meta->Profiler);
}

#define GS_META_CPP
#endif // GS_META_CPP