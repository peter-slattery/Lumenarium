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

internal void 
EatToNextLine (tokenizer* Tokenizer)
{
    while (AtValidPosition(*Tokenizer) && !IsNewline(*Tokenizer->At))
    {
        EatChar(Tokenizer); 
    }
}

internal s32
GetTypedefSize (token* Token)
{
    s32 Size = 0;
    
    token* LookingAt = Token->Next;
    
    if (StringsEqual(LookingAt->Text, MakeStringLiteral("unsigned")) ||
        StringsEqual(LookingAt->Text, MakeStringLiteral("signed")))
    {
        LookingAt = LookingAt->Next;
    }
    
    s32 Level = 1;
    if (StringsEqual(LookingAt->Text, MakeStringLiteral("short")))
    {
        Level = -1;
        LookingAt = LookingAt->Next;
    }
    while (StringsEqual(LookingAt->Text, MakeStringLiteral("long")))
    {
        Level= 2.f;
        LookingAt = LookingAt->Next;
    }
    
    if (StringsEqual(LookingAt->Text, MakeStringLiteral("char")))
    {
        Size = 1;
    }
    else if (StringsEqual(LookingAt->Text, MakeStringLiteral("int")))
    {
        switch (Level)
        {
            case -1: { Size = 2; } break;
            case  1: { Size = 4; } break;
            case  2: { Size = 8; } break;
            InvalidDefaultCase;
        }
    }
    else  if (StringsEqual(LookingAt->Text, MakeStringLiteral("float")))
    {
        Size = 4;
    }
    else if (StringsEqual(LookingAt->Text, MakeStringLiteral("double")))
    {
        Size = 8;
    }
    else if (StringsEqual(LookingAt->Text, MakeStringLiteral("bool")))
    {
        Size = 1;
    }
    else if (StringsEqual(LookingAt->Text, MakeStringLiteral("void")))
    {
        LogError(&GlobalErrorList, "void type Not Handled");
    }
    
    return Size;
}

internal string
GetTypedefIdentifier (token* Token)
{
    string Identifier = {};
    
    token* PreviousToken = Token;
    token* LookingAt = Token->Next;
    while (LookingAt->Type != Token_Semicolon)
    {
        PreviousToken = LookingAt;
        LookingAt = LookingAt->Next;
    }
    
    if (PreviousToken->Type == Token_Identifier)
    {
        Identifier = PreviousToken->Text;
    }
    
    return Identifier;
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
AddFileToSource(string RelativePath, gs_bucket<source_code_file>* SourceFiles)
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
        printf("Error: Could not load file %.*s\n", StringExpand(RelativePath));
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
ParseMetaTag(token_iter* Iter, gs_bucket<token*>* TagList)
{
    b32 Result = false;
    PushSnapshot(Iter);
    
    if (TokenAtEquals(Iter, "GSMetaTag") &&
        TokenAtEquals(Iter, "("))
    {
        if (Iter->TokenAt->Type == Token_Identifier)
        {
            TagList->PushElementOnBucket(Iter->TokenAt);
            NextToken(Iter);
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
        Result = true;
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
    else if (ShortInt(Iter, TypeIndexOut, *TypeTable))
    {
        Result = true;
    }
    else if (Int(Iter, TypeIndexOut, *TypeTable))
    {
        Result = true;
    }
    else if (LongInt(Iter, TypeIndexOut, *TypeTable))
    {
        Result = true;
    }
    else if (LongLongInt(Iter, TypeIndexOut, *TypeTable))
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
ParseVariableDecl(token_iter* Iter, gs_bucket<token*>* TagList, variable_decl* VarDecl, type_table* TypeTable)
{
    b32 Result = false;
    *VarDecl = {0};
    PushSnapshot(Iter);
    
    s32 TypeIndex = -1;
    if (ParseType(Iter, TypeTable, &TypeIndex))
    {
        // :UndeclaredType
        if (TypeIndex < 0)
        {
            TypeIndex = PushUndeclaredType(Iter->TokenAt->Text, TypeTable);
            NextToken(Iter);
        }
        
        VarDecl->Pointer = ParsePointer(Iter);
        
        if (Iter->TokenAt->Type == Token_Identifier)
        {
            VarDecl->TypeIndex = TypeIndex;
            VarDecl->Identifier = Iter->TokenAt->Text;
            CopyMetaTagsAndClear(TagList, &VarDecl->MetaTags);
            
            NextToken(Iter);
            
            // Array Notationg ie r32 x[2];
            // NOTE(Peter): True initially because if there is no array notation, we 
            // are still ok to proceed
            b32 ArrayParseSuccess = true;
            if (TokenAtEquals(Iter, "["))
            {
                // NOTE(Peter): Once we get to this point, we have to complete the entire
                // array notation before we have successfully parsed, hence setting 
                // ArrayParseSucces to false here.
                ArrayParseSuccess = false;
                if (Iter->TokenAt->Type == Token_Number)
                {
                    parse_result ArrayCount = ParseUnsignedInt(StringExpand(Iter->TokenAt->Text));
                    VarDecl->ArrayCount = ArrayCount.UnsignedIntValue;
                    NextToken(Iter);
                    
                    if (TokenAtEquals(Iter, "]"))
                    {
                        ArrayParseSuccess = true;
                    }
                }
            }
            
            // TODO(Peter): Handle comma separated members
            // ie. r32 x, y, z;
            
            if (ArrayParseSuccess)
            {
                Result = true;
            }
        }
        
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
ParseStruct(token_iter* Iter, s32* StructTypeIndexOut, gs_bucket<token*>* TagList, type_table* TypeTable)
{
    b32 Result = false;
    *StructTypeIndexOut = -1;
    
    PushSnapshot(Iter);
    
    type_definition_type DeclType;
    if (StructOrUnion(Iter, &DeclType))
    {
        string StructIdentifier = {};
        if (Iter->TokenAt->Type == Token_Identifier)
        {
            StructIdentifier = Iter->TokenAt->Text;
            NextToken(Iter);
        }
        
        if (TokenAtEquals(Iter, "{"))
        {
            type_definition StructDecl = {};
            StructDecl.Identifier = StructIdentifier;
            StructDecl.Type = DeclType;
            CopyMetaTagsAndClear(TagList, &StructDecl.MetaTags);
            
            while (!TokenAtEquals(Iter, "}"))
            {
                s32 MemberStructTypeIndex = {};
                variable_decl MemberDecl = {};
                if (ParseMetaTag(Iter, TagList))
                {
                    
                }
                else if (ParseVariableDecl(Iter, TagList, &MemberDecl, TypeTable))
                {
                    if (TokenAtEquals(Iter, ";"))
                    {
                        StructDecl.Struct.MemberDecls.PushElementOnBucket(MemberDecl);
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
                    // TODO(Peter): Handle unions
                    NextToken(Iter);
                }
            }
            
            if (TokenAtEquals(Iter, ";"))
            {
                Result = true;
                *StructTypeIndexOut = TypeTable->Types.PushElementOnBucket(StructDecl);
            }
        }
    }
    
    ApplySnapshotIfNotParsedAndPop(Result, Iter);
    return Result;
}

// ( type *? identifier, ... )
internal b32
ParseFunctionParameterList (token_iter* Iter, type_definition* FunctionPtrDecl, gs_bucket<token*>* TagList, type_table* TypeTable)
{
    b32 Result = false;
    PushSnapshot(Iter);
    
    if (TokenAtEquals(Iter, "("))
    {
        Result = true;
        
        while(!StringsEqual(Iter->TokenAt->Text, MakeStringLiteral(")")))
        {
            variable_decl ParameterDecl = {};
            if (ParseVariableDecl(Iter, TagList, &ParameterDecl, TypeTable))
            {
                FunctionPtrDecl->FunctionPtr.Parameters.PushElementOnBucket(ParameterDecl);
                if (Iter->TokenAt->Type == Token_Comma)
                {
                    NextToken(Iter);
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
ParseFunctionDeclaration (token_iter* Iter, token* Identifier, gs_bucket<token*>* TagList, type_table* TypeTable)
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
        
        if (Iter->TokenAt->Type == Token_Identifier)
        {
            type_definition FunctionPtr = {};
            FunctionPtr.Identifier = Iter->TokenAt->Text;
            FunctionPtr.Size = sizeof(void*);
            CopyMetaTagsAndClear(TagList, &FunctionPtr.MetaTags);
            FunctionPtr.Type = TypeDef_FunctionPointer;
            FunctionPtr.Pointer = true;
            FunctionPtr.FunctionPtr.ReturnTypeIndex = ReturnTypeIndex;
            
            *Identifier = *Iter->TokenAt;
            NextToken(Iter);
            if (ParseFunctionParameterList(Iter, &FunctionPtr, TagList, TypeTable))
            {
                if (TokenAtEquals(Iter, ";"))
                {
                    Result = true;
                    TypeTable->Types.PushElementOnBucket(FunctionPtr);
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
ParseTypedef(token_iter* Iter, gs_bucket<token*>* TagList, type_table* TypeTable)
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
            
            if (Iter->TokenAt->Type == Token_Identifier)
            {
                NewType.Identifier = Iter->TokenAt->Text;
                NextToken(Iter);
                
                Result = true;
                
                s32 ExistingUndeclaredTypeIndex = GetIndexOfType(NewType.Identifier, *TypeTable);
                if (ExistingUndeclaredTypeIndex < 0)
                {
                    TypeTable->Types.PushElementOnBucket(NewType);
                }
                else
                {
                    type_definition* ExistingTypeDef = TypeTable->Types.GetElementAtIndex(ExistingUndeclaredTypeIndex);
                    *ExistingTypeDef = NewType;
                }
            }
        }
        else
        {
            printf("unhandled typedef ");
            while (!TokenAtEquals(Iter, ";"))
            {
                printf("%.*s ", StringExpand(Iter->TokenAt->Text));
                NextToken(Iter);
            }
            printf("\n");
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

// Step 1: Get All Tokens, for every file
// Step 2: Identify all preprocessor directives
// Step 3: Apply Preprocessor Directives && Generate Code
// Step 4: Write out new files
// Step 5: Compile
int main(int ArgCount, char** ArgV)
{
    s64 TotalStart = GetWallClock();
    
    if (ArgCount <= 1)
    {
        printf("Please supply at least one source directory to analyze.\n");
        return 0;
    }
    
    memory_arena SourceFileArena = {};
    gs_bucket<source_code_file> SourceFiles;
    gs_bucket<token> Tokens;
    
    string_builder NodeTypeBlock = {};
    Write(MakeStringLiteral("enum node_type\n{\n"), &NodeTypeBlock);
    
    string_builder NodeMembersBlock = {};
    
    string_builder NodeSpecificationsBlock = {};
    Write(MakeStringLiteral("node_specification NodeSpecifications[] = {\n"), &NodeSpecificationsBlock);
    
    string_builder CallNodeProcBlock = {};
    Write(MakeStringLiteral("internal void CallNodeProc(u32 SpecificationIndex, u8* Data, led* LEDs, s32 LEDsCount, r32 DeltaTime)\n{\n"), &CallNodeProcBlock);
    Write(MakeStringLiteral("node_specification Spec = NodeSpecifications[SpecificationIndex];\n"), &CallNodeProcBlock);
    Write(MakeStringLiteral("switch (Spec.Type)\n{\n"), &CallNodeProcBlock);
    
    string CurrentWorkingDirectory = MakeString((char*)malloc(1024), 0, 1024);
    
    if (ArgCount > 1)
    {
        string RootFile = MakeString(ArgV[1]);
        AddFileToSource(RootFile, &SourceFiles);
        
        s32 LastSlash = ReverseSearchForCharInSet(RootFile, "\\/");
        Assert(LastSlash > 0);
        
        string RootPath = Substring(RootFile, 0, LastSlash + 1);
        CopyStringTo(RootPath, &CurrentWorkingDirectory);
    }
    
    
    // NOTE(Peter): this is a temporary list of GSMetaTags. It gets copied and cleared
    // after use
    gs_bucket<token*> TagList;
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
                        AddFileToSource(TempFilePath, &SourceFiles);
                    }
                }
            }
            else if(ParseMetaTag(&Iter, &TagList))
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
            FixUpStructSize(TypeDef, TypeTable);
        }
        else if (TypeDef->Type == TypeDef_Union)
        {
            FixUpUnionSize(TypeDef, TypeTable);
        }
    }
    
    // Print All Structs
    for (u32 i = 0; i < TypeTable.Types.Used; i++)
    {
        type_definition* TypeDef = TypeTable.Types.GetElementAtIndex(i);
#if 0
        if ((TypeDef->Type == TypeDef_Struct || TypeDef->Type == TypeDef_Union) && TypeDef->Identifier.Length > 0)
        {
            PrintStructDecl(TypeDef, TypeTable);
            printf("\n\n");
        }
#endif
        
        if (TypeDef->Type == TypeDef_FunctionPointer)
        {
            PrintFunctionPtrDecl(TypeDef, TypeTable);
            printf("\n\n");
        }
    }
    
    
    s64 Cycles_Preprocess = GetWallClock();
    
    MakeStringBuffer(Buffer, 256);
    
    // Close Types Block - overwrite the last comma and '\' newline character with newlines.
    Write(MakeStringLiteral("NodeType_Count,\n};\n\n"), &NodeTypeBlock);
    
    // Close Specifications Block
    Write(MakeStringLiteral("};\n"), &NodeSpecificationsBlock);
    PrintF(&Buffer, "s32 NodeSpecificationsCount = %d;\n\n", NodeProcCount);
    Write(Buffer, &NodeSpecificationsBlock);
    
    // Close Call Node Proc Block
    Write(MakeStringLiteral("}\n}\n"), &CallNodeProcBlock);
    
    FILE* NodeGeneratedCPP = fopen("C:\\projects\\foldhaus\\src\\generated\\foldhaus_nodes_generated.cpp", "w");
    if (NodeGeneratedCPP)
    {
        WriteStringBuilderToFile(NodeTypeBlock, NodeGeneratedCPP);
        WriteStringBuilderToFile(NodeMembersBlock, NodeGeneratedCPP);
        WriteStringBuilderToFile(NodeSpecificationsBlock, NodeGeneratedCPP);
        WriteStringBuilderToFile(CallNodeProcBlock, NodeGeneratedCPP);
        fclose(NodeGeneratedCPP);
    }
    
    PrintErrorList(GlobalErrorList);
    
    s64 TotalEnd = GetWallClock();
    
    r32 TotalTime = GetSecondsElapsed(TotalStart, TotalEnd);
    
    printf("Metaprogram Preproc Time: %.*f sec\n", 6, TotalTime);
    
#if 0
    for (u32 i = 0; i < Structs.Used; i++)
    {
        seen_node_struct* Struct = Structs.GetElementAtIndex(i);
        
#ifdef PRINT_ALL_INFO
        printf("\n");
        for (u32 j = 0; j < Struct->MetaTags.Used; j++)
        {
            token* MetaTag = Struct->MetaTags.GetElementAtIndex(j);
            printf("GSMetaTag(%.*s)\n", StringExpand(MetaTag->Text));
        }
#endif
        
        printf("struct %.*s\n", StringExpand(Struct->Name));
        
#ifdef PRINT_ALL_INFO
        for (u32 j = 0; j < Struct->MemberDecls.Used; j++)
        {
            struct_member_decl* Member = Struct->MemberDecls.GetElementAtIndex(j);
            
            for (u32 k = 0; k < Member->MetaTags.Used; k++)
            {
                token* MetaTag = Member->MetaTags.GetElementAtIndex(k);
                printf("    GSMetaTag(%.*s)\n", StringExpand(MetaTag->Text));
            }
            
            printf("    %.*s %.*s\n", StringExpand(Member->Type), StringExpand(Member->Identifier));
        }
#endif
    }
#endif
    
    __debugbreak();
    return 0;
}