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

#include <windows.h>
#include <stdio.h>

#include <gs_language.h>
#include <gs_bucket.h>
#include "..\src\gs_platform.h"
#include <gs_memory_arena.h>
#if 1
#include "..\src\gs_string.h"
#else
#include <gs_string.h>
#endif
#include <gs_string_builder.h>

#include "gs_meta_error.h"
#include "gs_meta_lexer.h"

struct error_buffer
{
    char* Backbuffer;
    string* Contents;
};

#define ERROR_MAX_LENGTH 256
#define ERROR_BUFFER_SIZE 256
struct errors
{
    error_buffer* Buffers;
    u32 BuffersCount;
    u32 Used;
};

internal void 
PushFError (errors* Errors, char* Format, ...)
{
    if (Errors->Used >= (Errors->BuffersCount * ERROR_BUFFER_SIZE))
    {
        Errors->BuffersCount += 1;
        Errors->Buffers = (error_buffer*)realloc(Errors->Buffers, sizeof(error_buffer*) * Errors->BuffersCount);
        
        error_buffer* NewBuffer = Errors->Buffers + (Errors->BuffersCount - 1);
        NewBuffer->Backbuffer = (char*)malloc(sizeof(char) * ERROR_MAX_LENGTH * ERROR_BUFFER_SIZE);
        NewBuffer->Contents = (string*)malloc(sizeof(string) * ERROR_BUFFER_SIZE);
        
        for (u32 i = 0; i < ERROR_BUFFER_SIZE; i++)
        {
            NewBuffer->Contents[i].Memory = NewBuffer->Backbuffer + (i * ERROR_MAX_LENGTH);
            NewBuffer->Contents[i].Max = ERROR_MAX_LENGTH;
            NewBuffer->Contents[i].Length = 0;
        }
    }
    
    u32 NewErrorIndex = Errors->Used++;
    u32 BufferIndex = NewErrorIndex / ERROR_BUFFER_SIZE;
    u32 IndexInBuffer = NewErrorIndex % ERROR_BUFFER_SIZE;
    string* NewError = Errors->Buffers[BufferIndex].Contents + IndexInBuffer;
    
    va_list Args;
    va_start(Args, Format);
    NewError->Length = PrintFArgsList(NewError->Memory, NewError->Max, Format, Args);
    va_end(Args);
}

internal string*
TakeError (errors* Errors)
{
    u32 NewErrorIndex = Errors->Used++;
    u32 BufferIndex = NewErrorIndex / ERROR_BUFFER_SIZE;
    u32 IndexInBuffer = NewErrorIndex % ERROR_BUFFER_SIZE;
    string* NewError = Errors->Buffers[BufferIndex].Contents + IndexInBuffer;
    return NewError;
}

internal void
PrintAllErrors (errors Errors)
{
    for (u32 i = 0; i < Errors.Used; i++)
    {
        u32 BufferIndex = i / ERROR_BUFFER_SIZE;
        u32 IndexInBuffer = i % ERROR_BUFFER_SIZE;
        string Error = Errors.Buffers[BufferIndex].Contents[IndexInBuffer];
        printf("%.*s\n", StringExpand(Error));
    }
}

#include "foldhaus_meta_type_table.h"
error_list GlobalErrorList = {};

PLATFORM_ALLOC(StdAlloc)
{
    u8* Result = (u8*)malloc(Size);
    return Result;
}

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

internal s32
ReadEntireFileAndNullTerminate (source_code_file* File)
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
        LogError(&GlobalErrorList, "Could Not Read File: %.*s", StringExpand(File->Path));
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
GetSecondsElapsed(s64 StartCycles, s64 EndCycles)
{
    s64 Frequency = GetPerformanceFrequency();
    r32 SecondsElapsed = (r32)(EndCycles - StartCycles) / (r32)(Frequency);
    return SecondsElapsed;
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
    
    File.FileSize = ReadEntireFileAndNullTerminate(&File);
    
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

internal b32
ParseMetaTag(token_iter* Iter, gs_bucket<token>* TagList)
{
    b32 Result = false;
    PushSnapshot(Iter);
    
    if (TokenAtEquals(Iter, "GSMetaTag") &&
        TokenAtEquals(Iter, "("))
    {
        token MetaIdentifier = {0};
        if (TokenAtEquals(Iter, Token_Identifier, &MetaIdentifier))
        {
            TagList->PushElementOnBucket(MetaIdentifier);
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
    return Result;
}

internal b32
ShortInt (token_iter* Iter, s32* TypeIndexOut, type_table TypeTable)
{
    b32 Result = false;
    PushSnapshot(Iter);
    
    if (TokenAtEquals(Iter, "unsigned") ||
        TokenAtEquals(Iter, "signed"))
    {
    }
    
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
        *TypeIndexOut = GetIndexOfType(MakeStringLiteral("short int"), TypeTable);
    }
    return Result;
}

internal b32
Int (token_iter* Iter, s32* TypeIndexOut, type_table TypeTable)
{
    b32 Result = false;
    PushSnapshot(Iter);
    
    if (TokenAtEquals(Iter, "unsigned") ||
        TokenAtEquals(Iter, "signed"))
    {
    }
    
    if (TokenAtEquals(Iter, "int"))
    {
        Result = true;
    }
    
    ApplySnapshotIfNotParsedAndPop(Result, Iter);
    if (Result) 
    { 
        *TypeIndexOut = GetIndexOfType(MakeStringLiteral("int"), TypeTable);
    }
    return Result;
}

internal b32
LongInt (token_iter* Iter, s32* TypeIndexOut, type_table TypeTable)
{
    b32 Result = false;
    PushSnapshot(Iter);
    
    if (TokenAtEquals(Iter, "unsigned") ||
        TokenAtEquals(Iter, "signed"))
    {
    }
    
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
        *TypeIndexOut = GetIndexOfType(MakeStringLiteral("long int"), TypeTable);
    }
    return Result;
}

internal b32
LongLongInt (token_iter* Iter, s32* TypeIndexOut, type_table TypeTable)
{
    b32 Result = false;
    PushSnapshot(Iter);
    
    if (TokenAtEquals(Iter, "unsigned") ||
        TokenAtEquals(Iter, "signed"))
    {
        
    }
    
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
        *TypeIndexOut = GetIndexOfType(MakeStringLiteral("long long int"), TypeTable);
    }
    return Result;
}

internal b32
ParseChar(token_iter* Iter, s32* TypeIndexOut, type_table TypeTable)
{
    b32 Result = false;
    PushSnapshot(Iter);
    
    if (TokenAtEquals(Iter, "unsigned") ||
        TokenAtEquals(Iter, "signed"))
    {
        
    }
    
    if (TokenAtEquals(Iter, "char"))
    {
        Result = true;
    }
    
    ApplySnapshotIfNotParsedAndPop(Result, Iter);
    if (Result) 
    { 
        *TypeIndexOut = GetIndexOfType(MakeStringLiteral("char"), TypeTable);
    }
    return Result;
}

internal b32
ParseBool(token_iter* Iter, s32* TypeIndexOut, type_table TypeTable)
{
    b32 Result = false;
    PushSnapshot(Iter);
    
    if (TokenAtEquals(Iter, "bool"))
    {
        Result = true;
        *TypeIndexOut = GetIndexOfType(MakeStringLiteral("bool"), TypeTable);
    }
    
    ApplySnapshotIfNotParsedAndPop(Result, Iter);
    return Result;
}

internal b32
ParseFloat(token_iter* Iter, s32* TypeIndexOut, type_table TypeTable)
{
    b32 Result = false;
    PushSnapshot(Iter);
    
    if (TokenAtEquals(Iter, "float"))
    {
        Result = true;
        *TypeIndexOut= GetIndexOfType(MakeStringLiteral("float"), TypeTable);
    }
    
    ApplySnapshotIfNotParsedAndPop(Result, Iter);
    return Result;
}

internal b32
ParseDouble(token_iter* Iter, s32* TypeIndexOut, type_table TypeTable)
{
    b32 Result = false;
    PushSnapshot(Iter);
    
    if (TokenAtEquals(Iter, "double"))
    {
        Result = true;
        *TypeIndexOut = GetIndexOfType(MakeStringLiteral("double"), TypeTable);
    }
    
    ApplySnapshotIfNotParsedAndPop(Result, Iter);
    return Result;
}

// :UndeclaredType
// NOTE(Peter): If TypeIndexOut is -1, you need to call NextToken after this
// function to advance past the type identifier.
internal b32
ParseType(token_iter* Iter, type_table* TypeTable, s32* TypeIndexOut)
{
    b32 Result = false;
    *TypeIndexOut = -1;
    PushSnapshot(Iter);
    
    // TODO(Peter): Store signedness, and what makes up a type
    if (ParseChar(Iter, TypeIndexOut, *TypeTable))
    {
        Result = true;
    } 
    else if (StringsEqual(Iter->TokenAt->Text, MakeStringLiteral("wchar_t")))
    {
        NextToken(Iter);
        Result = true;
        *TypeIndexOut = GetIndexOfType(MakeStringLiteral("wchar_t"), *TypeTable);
    } 
    else if (ParseBool(Iter, TypeIndexOut, *TypeTable))
    {
        NextToken(Iter);
        Result = true;
    }
    else if (LongLongInt(Iter, TypeIndexOut, *TypeTable))
    {
        Result = true;
    }
    else if (LongInt(Iter, TypeIndexOut, *TypeTable))
    {
        Result = true;
    }
    else if (ShortInt(Iter, TypeIndexOut, *TypeTable))
    {
        Result = true;
    }
    else if (Int(Iter, TypeIndexOut, *TypeTable))
    {
        Result = true;
    }
    else if (ParseFloat(Iter, TypeIndexOut, *TypeTable))
    {
        Result = true;
    } 
    else if (ParseDouble(Iter, TypeIndexOut, *TypeTable))
    {
        Result = true;
    } 
    else if (StringsEqual(Iter->TokenAt->Text, MakeStringLiteral("void")))
    {
        NextToken(Iter);
        Result = true;
        *TypeIndexOut = GetIndexOfType(MakeStringLiteral("void"), *TypeTable);
    }
    else 
    {
        *TypeIndexOut = GetIndexOfType(Iter->TokenAt->Text, *TypeTable);
        if (*TypeIndexOut >= 0)
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
            *TypeIndexOut = -1;
        }
    }
    
    ApplySnapshotIfNotParsedAndPop(Result, Iter);
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
ParseVariableDecl(token_iter* Iter, gs_bucket<token>* TagList, gs_bucket<variable_decl>* VariableList, type_table* TypeTable)
{
    b32 Result = false;
    PushSnapshot(Iter);
    
    if (ParseConstVolatile(Iter))
    {
        // NOTE(Peter): we don't do anything with this atm
        // dont have a reason to just yet
        // :UnusedConstVolatile
    }
    
    s32 TypeIndex = -1;
    if (ParseType(Iter, TypeTable, &TypeIndex))
    {
        // :UndeclaredType
        if (TypeIndex < 0)
        {
            TypeIndex = PushUndeclaredType(Iter->TokenAt->Text, TypeTable);
            NextToken(Iter);
        }
        
        b32 IsPointer = ParsePointer(Iter);
        
        if (ParseConstVolatile(Iter))
        {
            // :UnusedConstVolatile
        }
        
        do {
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
                        
                        if (TokenAtEquals(Iter, "]"))
                        {
                            ArrayParseSuccess = true;
                        }
                    }
                }
                
                if (ArrayParseSuccess)
                {
                    Result = true;
                    
                    variable_decl* Decl = VariableList->TakeElement();
                    *Decl = {};
                    Decl->Identifier = IdentifierToken.Text;
                    Decl->TypeIndex = TypeIndex;
                    Decl->Pointer = IsPointer;
                    Decl->ArrayCount = ArrayCount;
                    CopyMetaTagsAndClear(TagList, &Decl->MetaTags);
                }
            }
        } while (TokenAtEquals(Iter, ","));
    }
    
    ApplySnapshotIfNotParsedAndPop(Result, Iter);
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

internal b32
ParseStruct(token_iter* Iter, s32* StructTypeIndexOut, gs_bucket<token>* TagList, type_table* TypeTable)
{
    b32 Result = false;
    *StructTypeIndexOut = -1;
    
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
            StructDecl.Identifier = IdentifierToken.Text;
            StructDecl.Type = DeclType;
            CopyMetaTagsAndClear(TagList, &StructDecl.MetaTags);
            
            while (!TokenAtEquals(Iter, "}"))
            {
                s32 MemberStructTypeIndex = {};
                variable_decl MemberDecl = {};
                if (ParseMetaTag(Iter, TagList))
                {
                    
                }
                else if (ParseVariableDecl(Iter, TagList, &StructDecl.Struct.MemberDecls, TypeTable))
                {
                    if (!TokenAtEquals(Iter, ";"))
                    {
                        PushFError(Iter->Errors, "No semicolon after struct member variable declaration. %S", StructDecl.Identifier);
                    }
                }
                else if (ParseStruct(Iter, &MemberStructTypeIndex, TagList, TypeTable))
                {
                    // NOTE(Peter): Pretty sure, since we just parsed the struct, that
                    // MemberStructTypeIndex should never be -1 (unknown type). 
                    // Putting this Assert here for now, but remove if there's a valid
                    // reason that you might not be able to find a struct just parsed at
                    // this point.
                    Assert(MemberStructTypeIndex >= 0); 
                    
                    MemberDecl.TypeIndex = MemberStructTypeIndex;
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
                *StructTypeIndexOut = PushTypeDefOnTypeTable(StructDecl, TypeTable);
            }
        }
    }
    
    ApplySnapshotIfNotParsedAndPop(Result, Iter);
    return Result;
}

// ( type *? identifier, ... )
internal b32
ParseFunctionParameterList (token_iter* Iter, type_definition* FunctionPtrDecl, gs_bucket<token>* TagList, type_table* TypeTable)
{
    b32 Result = false;
    PushSnapshot(Iter);
    
    if (TokenAtEquals(Iter, "("))
    {
        Result = true;
        
        while(!StringsEqual(Iter->TokenAt->Text, MakeStringLiteral(")")))
        {
            if (ParseVariableDecl(Iter, TagList, &FunctionPtrDecl->FunctionPtr.Parameters, TypeTable))
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
ParseFunctionDeclaration (token_iter* Iter, token* Identifier, gs_bucket<token>* TagList, type_table* TypeTable)
{
    b32 Result = false;
    PushSnapshot(Iter);
    
    s32 ReturnTypeIndex = -1;
    if (ParseType(Iter, TypeTable, &ReturnTypeIndex))
    {
        if (ReturnTypeIndex < 0) 
        { 
            ReturnTypeIndex = PushUndeclaredType(Iter->TokenAt->Text, TypeTable);
            NextToken(Iter); 
        }
        
        b32 IsPointer = ParsePointer(Iter);
        
        if (TokenAtEquals(Iter, Token_Identifier, Identifier))
        {
            type_definition FunctionPtr = {};
            FunctionPtr.Identifier = Identifier->Text;
            FunctionPtr.Size = sizeof(void*);
            CopyMetaTagsAndClear(TagList, &FunctionPtr.MetaTags);
            FunctionPtr.Type = TypeDef_FunctionPointer;
            FunctionPtr.Pointer = true;
            FunctionPtr.FunctionPtr = {};
            FunctionPtr.FunctionPtr.ReturnTypeIndex = ReturnTypeIndex;
            
            if (ParseFunctionParameterList(Iter, &FunctionPtr, TagList, TypeTable))
            {
                if (TokenAtEquals(Iter, ";"))
                {
                    Result = true;
                    PushTypeDefOnTypeTable(FunctionPtr, TypeTable);
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
ParseTypedef(token_iter* Iter, gs_bucket<token>* TagList, type_table* TypeTable)
{
    b32 Result = false;
    PushSnapshot(Iter);
    
    if (TokenAtEquals(Iter, "typedef"))
    {
        token TypeToken = {0};
        s32 TypeIndex = -1;
        if (TokenAtEquals(Iter, "struct") &&
            ParseStruct(Iter, &TypeIndex, TagList, TypeTable))
        {
            Result = true;
        }
        else if (ParseFunctionDeclaration(Iter, &TypeToken, TagList, TypeTable))
        {
            Result = true;
        }
        else if (ParseType(Iter, TypeTable, &TypeIndex))
        {
            if (TypeIndex < 0)
            {
                TypeIndex = PushUndeclaredType(Iter->TokenAt->Text, TypeTable);
                NextToken(Iter);
            }
            
            b32 IsPointer = ParsePointer(Iter);
            
            type_definition* BasisType = TypeTable->Types.GetElementAtIndex(TypeIndex);
            
            type_definition NewType = {};
            NewType.Size = BasisType->Size;
            CopyMetaTagsAndClear(TagList, &NewType.MetaTags);
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
                PushTypeDefOnTypeTable(NewType, TypeTable);
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
    return Result;
}

internal b32
ParseEnum (token_iter* Iter, gs_bucket<token>* TagList, type_table* TypeTable)
{
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
            CopyMetaTagsAndClear(TagList, &EnumDecl.MetaTags);
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
                    PushTypeDefOnTypeTable(EnumDecl, TypeTable);
                    Result = true;
                }
            }
        }
    }
    
    ApplySnapshotIfNotParsedAndPop(Result, Iter);
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
    type_definition* MemberTypeDef = TypeTable.Types.GetElementAtIndex(Member.TypeIndex);
    if ((MemberTypeDef->Type == TypeDef_Struct || MemberTypeDef->Type == TypeDef_Union)
        && MemberTypeDef->Identifier.Length == 0)
    {
        PrintStructDecl(MemberTypeDef, TypeTable, Indent);
    }
    else
    {
        PrintIndent(Indent);
        if (Member.TypeIndex == -1)
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
    type_definition* ReturnType = TypeTable.Types.GetElementAtIndex(FnPtrDecl->FunctionPtr.ReturnTypeIndex);
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

struct typeinfo_generator
{
    string_builder TypeList;
    string_builder StructMembers;
    string_builder TypeDefinitions;
    
    u32 GeneratedInfoTypesCount;
    u32 TypesMax;
    b8* TypesGeneratedMask;
};

internal typeinfo_generator
InitTypeInfoGenerator(type_table TypeTable)
{
    typeinfo_generator Result = {};
    
    Result.TypesMax = TypeTable.Types.Used;
    Result.TypesGeneratedMask = (b8*)malloc(sizeof(b8) * Result.TypesMax);
    GSZeroMemory((u8*)Result.TypesGeneratedMask, Result.TypesMax);
    
    WriteF(&Result.TypeList, "enum gsm_struct_type\n{\n");
    
    WriteF(&Result.TypeDefinitions, "static gsm_struct_type_info StructTypes[] = {\n");
    return Result;
}

internal void
FinishGeneratingTypes(typeinfo_generator* Generator)
{
    WriteF(&Generator->TypeList, "gsm_StructTypeCount,\n};\n\n");
    
    WriteF(&Generator->StructMembers, "\n");
    
    WriteF(&Generator->TypeDefinitions, "};\n");
    WriteF(&Generator->TypeDefinitions, "gsm_u32 StructTypesCount = %d;\n", Generator->GeneratedInfoTypesCount);
}

internal void
GenerateMetaTagInfo (gs_bucket<meta_tag> Tags, string_builder* Builder)
{
    WriteF(Builder, "{");
    for (u32 t = 0; t < Tags.Used; t++)
    {
        meta_tag* Tag = Tags.GetElementAtIndex(t);
        WriteF(Builder, "{ \"%S\", %d }", Tag->Identifier, Tag->Identifier.Length);
        if ((t + 1) < Tags.Used)
        {
            WriteF(Builder, ", ");
        }
    }
    WriteF(Builder, "}, %d", Tags.Used);
}

internal void
GenerateStructMemberInfo (variable_decl* Member, string StructIdentifier, type_table TypeTable, typeinfo_generator* Gen)
{
    WriteF(&Gen->StructMembers, "{ \"%S\", %d, ", Member->Identifier, Member->Identifier.Length);
    WriteF(&Gen->StructMembers, "(u64)&((%S*)0)->%S ", StructIdentifier, Member->Identifier); 
    WriteF(&Gen->StructMembers, "},\n");
}

internal void
GenerateTypeInfo (type_definition* Type, u32 TypeIndex, type_table TypeTable, typeinfo_generator* Generator)
{
    Generator->TypesGeneratedMask[TypeIndex] = true;
    Generator->GeneratedInfoTypesCount++;
    
    {
        // NOTE(Peter): This block MUST come before generating
        // type info for any member types. If it doesn't, it will screw
        // up array ordering
        
        // Lookup Enum
        WriteF(&Generator->TypeList, "gsm_StructType_%S,\n", Type->Identifier);
        
        // Type Info
        WriteF(&Generator->TypeDefinitions, "{ gsm_StructType_%S, \"%S\", %d, %d, 0, 0, ",
               Type->Identifier,
               Type->Identifier, Type->Identifier.Length,
               Type->Size
               // TODO(Peter): include Meta Tags somehow
               );
        if ((Type->Type == TypeDef_Struct || Type->Type == TypeDef_Union) &&
            Type->Struct.MemberDecls.Used > 0)
        {
            WriteF(&Generator->TypeDefinitions, "StructMembers_%S, %d },\n",
                   Type->Identifier,
                   Type->Struct.MemberDecls.Used);
        }
        else
        {
            WriteF(&Generator->TypeDefinitions, "0, 0 },\n");
        }
    }
    
    if (Type->Type == TypeDef_Struct || 
        Type->Type == TypeDef_Union)
    {
        for (u32 m = 0; m < Type->Struct.MemberDecls.Used; m++)
        {
            variable_decl* Member = Type->Struct.MemberDecls.GetElementAtIndex(m);
            type_definition* MemberType = TypeTable.Types.GetElementAtIndex(Member->TypeIndex);
            
            if (MemberType->Identifier.Length == 0) { continue; } // Don't gen info for anonymous struct and union members
            if (Generator->TypesGeneratedMask[Member->TypeIndex]) { continue; }
            
            GenerateTypeInfo(MemberType, Member->TypeIndex, TypeTable, Generator);
        }
        
        //
        WriteF(&Generator->StructMembers, "static gsm_struct_member_type_info StructMembers_%S[] = {\n", Type->Identifier);
        for (u32 m = 0; m < Type->Struct.MemberDecls.Used; m++)
        {
            variable_decl* Member = Type->Struct.MemberDecls.GetElementAtIndex(m);
            type_definition* MemberType = TypeTable.Types.GetElementAtIndex(Member->TypeIndex);
            
            if (MemberType->Identifier.Length > 0)
            {
                GenerateStructMemberInfo(Member, Type->Identifier, TypeTable, Generator);
            }
            else if (MemberType->Type == TypeDef_Struct ||
                     MemberType->Type == TypeDef_Union)
            {
                // Anonymous Members
                for (u32 a = 0; a < MemberType->Struct.MemberDecls.Used; a++)
                {
                    variable_decl* AnonMember = MemberType->Struct.MemberDecls.GetElementAtIndex(a);
                    GenerateStructMemberInfo(AnonMember, Type->Identifier, TypeTable, Generator);
                }
            }
        }
        WriteF(&Generator->StructMembers, "};\n", Type->Struct.MemberDecls.Used);
    }
}

internal void
GenerateFilteredTypeInfo (string MetaTagFilter, type_table TypeTable, typeinfo_generator* Generator)
{
    for (u32 i = 0; i < TypeTable.Types.Used; i++)
    {
        if (Generator->TypesGeneratedMask[i])
        {
            continue;
        }
        
        type_definition* Type = TypeTable.Types.GetElementAtIndex(i);
        if (HasTag(MetaTagFilter, Type->MetaTags))
        {
            GenerateTypeInfo(Type, i, TypeTable, Generator);
        }
    }
}

int main(int ArgCount, char** ArgV)
{
    s64 TotalStart = GetWallClock();
    
    if (ArgCount <= 1)
    {
        printf("Please supply at least one source directory to analyze.\n");
        return 0;
    }
    
    errors Errors = {0};
    
    gs_bucket<source_code_file> SourceFiles;
    
    string CurrentWorkingDirectory = MakeString((char*)malloc(1024), 0, 1024);
    if (ArgCount > 1)
    {
        string RootFile = MakeString(ArgV[1]);
        AddFileToSource(RootFile, &SourceFiles, &Errors);
        
        s32 LastSlash = ReverseSearchForCharInSet(RootFile, "\\/");
        Assert(LastSlash > 0);
        
        string RootPath = Substring(RootFile, 0, LastSlash + 1);
        CopyStringTo(RootPath, &CurrentWorkingDirectory);
    }
    
    
    // NOTE(Peter): this is a temporary list of GSMetaTags. It gets copied and cleared
    // after use
    gs_bucket<token> Tokens;
    gs_bucket<token> TagList;
    type_table TypeTable = {0};
    PopulateTableWithDefaultCPPTypes(&TypeTable);
    
    s32 NodeProcCount = 0;
    for (u32 SourceFileIdx = 0; SourceFileIdx < SourceFiles.Used; SourceFileIdx++)
    {
        source_code_file* File = SourceFiles.GetElementAtIndex(SourceFileIdx);
        TokenizeFile(File, &Tokens);
        
        token_iter Iter = {};
        Iter.Tokens = &Tokens;
        Iter.FirstToken = File->FirstTokenIndex;
        Iter.LastToken = File->LastTokenIndex;
        Iter.TokenAtIndex = Iter.FirstToken;
        Iter.TokenAt = Tokens.GetElementAtIndex(Iter.TokenAtIndex);
        Iter.Errors = &Errors;
        
        while (Iter.TokenAtIndex < Iter.LastToken)
        {
            b32 ParseSuccess = false;
            
            s32 TypeIndex = -1;
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
                    if (!FileAlreadyInSource(TempFilePath, SourceFiles))
                    {
                        AddFileToSource(TempFilePath, &SourceFiles, &Errors);
                    }
                }
            }
            else if(ParseMetaTag(&Iter, &TagList))
            {
                ParseSuccess = true;
            }
            else if (ParseEnum(&Iter, &TagList, &TypeTable))
            {
                ParseSuccess = true;
            }
            else if (ParseStruct(&Iter, &TypeIndex, &TagList, &TypeTable))
            {
                ParseSuccess = true;
            }
            else if (ParseTypedef(&Iter, &TagList, &TypeTable))
            {
                ParseSuccess = true;
            }
            
            if (!ParseSuccess)
            {
                NextToken(&Iter);
            }
        }
    }
    
    // Type Table Fixup
    for (u32 i = 0; i < TypeTable.Types.Used; i++)
    {
        type_definition* TypeDef = TypeTable.Types.GetElementAtIndex(i);
        if (TypeDef->Type == TypeDef_Struct)
        {
            FixUpStructSize(i, TypeTable, &Errors);
        }
        else if (TypeDef->Type == TypeDef_Union)
        {
            FixUpUnionSize(i, TypeTable, &Errors);
        }
    }
    
    s64 Cycles_Preprocess = GetWallClock();
    
    PrintAllErrors(Errors);
    
    typeinfo_generator TypeGenerator = InitTypeInfoGenerator(TypeTable);
    
    GenerateFilteredTypeInfo(MakeStringLiteral("node_struct"), TypeTable, &TypeGenerator);
    GenerateFilteredTypeInfo(MakeStringLiteral("gen_type_info"), TypeTable, &TypeGenerator);
    
    FinishGeneratingTypes(&TypeGenerator);
    FILE* TypeInfoH = fopen("C:\\projects\\foldhaus\\src\\generated\\gs_meta_generated_typeinfo.h", "w");
    if (TypeInfoH)
    {
        WriteStringBuilderToFile(TypeGenerator.TypeList, TypeInfoH);
        WriteStringBuilderToFile(TypeGenerator.StructMembers, TypeInfoH);
        WriteStringBuilderToFile(TypeGenerator.TypeDefinitions, TypeInfoH);
        fclose(TypeInfoH);
    }
    
    PrintErrorList(GlobalErrorList);
    
    s64 TotalEnd = GetWallClock();
    r32 TotalTime = GetSecondsElapsed(TotalStart, TotalEnd);
    printf("Metaprogram Preproc Time: %.*f sec\n", 6, TotalTime);
    
    //__debugbreak();
    return 0;
}