//
// File: foldhaus_serializer.h
// Author: Peter Slattery
// Creation Date: 2020-10-09
//
#ifndef FOLDHAUS_SERIALIZER_H

struct serializer
{
    gs_string String;
    u32 Indent;
    gs_const_string* Identifiers;
    u32 IdentifiersCount;
};

internal gs_const_string
Serializer_GetIdent(serializer* Serializer, u32 Index)
{
    gs_const_string Result = Serializer->Identifiers[Index];
    return Result;
}

// Serializing

internal void
Serializer_WriteIndent(serializer* Serializer)
{
    for (u32 i = 0; i < Serializer->Indent; i++)
    {
        AppendPrintF(&Serializer->String, "    ");
    }
}

internal void
Serializer_WriteF(serializer* Serializer, char* Format, ...)
{
    Serializer_WriteIndent(Serializer);
    
    va_list Args;
    va_start(Args, Format);
    PrintFArgsList(&Serializer->String, Format, Args);
    va_end(Args);
}

internal void
Serializer_OpenStruct(serializer* Serializer, gs_const_string StructName)
{
    Serializer_WriteF(Serializer, "%S:{\n", StructName);
    Serializer->Indent++;
}

internal void
Serializer_OpenStruct(serializer* Serializer, u32 StructIdentifier)
{
    gs_const_string Ident = Serializer_GetIdent(Serializer, StructIdentifier);
    Serializer_WriteF(Serializer, "%S:{\n", Ident);
    Serializer->Indent++;
}

internal void
Serializer_CloseStruct(serializer* Serializer)
{
    Serializer->Indent--;
    Serializer_WriteF(Serializer, "};\n");
}

internal void
Serializer_WriteValue(serializer* Serializer, gs_const_string Ident, gs_const_string Value)
{
    Serializer_WriteF(Serializer, "%S: %S;\n", Ident, Value);
}

internal void
Serializer_WriteValue(serializer* Serializer, u32 IdentIndex, gs_const_string Value)
{
    gs_const_string Ident = Serializer_GetIdent(Serializer, IdentIndex);
    Serializer_WriteValue(Serializer, Ident, Value);
}

internal void
Serializer_WriteValue(serializer* Serializer, gs_const_string Ident, u32 Value)
{
    Serializer_WriteF(Serializer, "%S: %d;\n", Ident, Value);
}

internal void
Serializer_WriteValue(serializer* Serializer, u32 IdentIndex, u32 Value)
{
    gs_const_string Ident = Serializer_GetIdent(Serializer, IdentIndex);
    Serializer_WriteValue(Serializer, Ident, Value);
}

internal void
Serializer_WriteValue(serializer* Serializer, gs_const_string Ident, r32 Value)
{
    Serializer_WriteF(Serializer, "%S: %f;\n", Ident, Value);
}

internal void
Serializer_WriteValue(serializer* Serializer, u32 IdentIndex, r32 Value)
{
    gs_const_string Ident = Serializer_GetIdent(Serializer, IdentIndex);
    Serializer_WriteValue(Serializer, Ident, Value);
}

internal void
Serializer_WriteStringValue(serializer* Serializer, gs_const_string Ident, gs_const_string Value)
{
    Serializer_WriteF(Serializer, "%S: \"%S\";\n", Ident, Value);
}

internal void
Serializer_WriteStringValue(serializer* Serializer, u32 IdentIndex, gs_const_string Value)
{
    gs_const_string Ident = Serializer_GetIdent(Serializer, IdentIndex);
    Serializer_WriteStringValue(Serializer, Ident, Value);
}

internal void
Serializer_WriteV3Value(serializer* Serializer, gs_const_string Ident, v3 Value)
{
    Serializer_WriteF(Serializer, "%S: (%f, %f, %f);\n", Ident, Value.x, Value.y, Value.z);
}

internal void
Serializer_WriteV3Value(serializer* Serializer, u32 IdentIndex, v3 Value)
{
    gs_const_string Ident = Serializer_GetIdent(Serializer, IdentIndex);
    Serializer_WriteV3Value(Serializer, Ident, Value);
}

// Parsing

struct parser_error
{
    gs_string Message;
    
    gs_string FileName;
    u32 LineNumber;
    
    parser_error* Next;
};

struct parser
{
    gs_string FileName;
    
    gs_string String;
    
    gs_const_string* Identifiers;
    u32 IdentifiersCount;
    
    u32 Line;
    
    char* LineStart;
    char* At;
    
    gs_memory_arena* Arena;
    gs_memory_arena* Transient;
    
    parser_error* ErrorsRoot;
    parser_error* ErrorsHead;
};

internal void
Parser_PushErrorF(parser* Parser, char* Format, ...)
{
    parser_error* Error = PushStruct(Parser->Transient, parser_error);
    Error->FileName = Parser->FileName;
    Error->LineNumber = Parser->Line;
    
    Error->Message = PushString(Parser->Transient, 1024);
    PrintF(&Error->Message, "File: %S Line: %d - ", Error->FileName, Error->LineNumber);
    
    va_list Args;
    va_start(Args, Format);
    PrintFArgsList(&Error->Message, Format, Args);
    va_end(Args);
    
    SLLPushOrInit(Parser->ErrorsRoot, Parser->ErrorsHead, Error);
}

internal gs_const_string
Parser_GetIdent(parser Parser, u32 Index)
{
    gs_const_string Result = Parser.Identifiers[Index];
    return Result;
}

internal bool
Parser_AtValidPosition(parser Parser)
{
    u64 Offset = Parser.At - Parser.String.Str;
    bool Result = (Offset < Parser.String.Length);
    return Result;
}

internal void
Parser_AdvanceChar(parser* P)
{
    if (IsNewline(P->At[0]))
    {
        P->Line += 1;
        P->LineStart = P->At + 1;
    }
    P->At++;
}

internal void
Parser_EatWhitespace(parser* P)
{
    while(Parser_AtValidPosition(*P) && IsNewlineOrWhitespace(P->At[0]))
    {
        Parser_AdvanceChar(P);
    }
}

internal void
Parser_EatToNewLine(parser* P)
{
    while(Parser_AtValidPosition(*P) && !IsNewline(P->At[0]))
    {
        Parser_AdvanceChar(P);
    }
    Parser_EatWhitespace(P);
}

internal bool
Parser_AdvanceIfTokenEquals(parser* P, gs_const_string Value)
{
    bool Result = true;
    
    char* PAt = P->At;
    char* VAt = Value.Str;
    while (*VAt != 0)
    {
        if (*PAt != *VAt)
        {
            Result = false;
            break;
        }
        PAt += 1;
        VAt += 1;
    }
    
    // TODO(Peter): What if the token is a subset of Value? ie. this would return true for
    //    T->At = hello_world and Value = hello_
    // But the next token we read would fail
    
    if (Result)
    {
        P->At = PAt;
        Parser_EatWhitespace(P);
    }
    return Result;
}

internal bool
Parser_AdvanceIfLineEnd(parser* P)
{
    bool Result = Parser_AdvanceIfTokenEquals(P, ConstString(";"));
    return Result;
}

internal bool
Parser_ReadString(parser* P, gs_const_string String)
{
    // string;
    bool Result = Parser_AdvanceIfTokenEquals(P, String);
    Result &= Parser_AdvanceIfLineEnd(P);
    return Result;
}

internal bool
Parser_ReadString(parser* P, u32 IdentIndex)
{
    gs_const_string Ident = Parser_GetIdent(*P, IdentIndex);
    return Parser_ReadString(P, Ident);
}

internal bool
Parser_ReadStringValue(parser* P, gs_const_string Ident, gs_string* Output, bool ShouldNullTerminate = false)
{
    Assert(Output != 0);
    
    // ident: "value";
    bool Result = false;
    if (Parser_AdvanceIfTokenEquals(P, Ident) &&
        Parser_AdvanceIfTokenEquals(P, ConstString(":")) &&
        Parser_AdvanceIfTokenEquals(P, ConstString("\"")))
    {
        gs_const_string FileString = {0};
        FileString.Str = P->At;
        
        while (Parser_AtValidPosition(*P) && P->At[0] != '"')
        {
            Parser_AdvanceChar(P);
        }
        
        FileString.Length = P->At - FileString.Str;
        if (Parser_AdvanceIfTokenEquals(P, ConstString("\"")) &&
            Parser_AdvanceIfLineEnd(P))
        {
            u32 StringLength = FileString.Length;
            if (ShouldNullTerminate)
            {
                StringLength += 1;
            }
            
            Result = true;
            *Output = PushStringF(P->Arena, StringLength, "%S", FileString);
            if (ShouldNullTerminate)
            {
                NullTerminate(Output);
            }
        }
        else
        {
            Parser_PushErrorF(P, "String doesn't have a closing quote, or line doesn't end with a semicolon");
        }
    }
    
    return Result;
}

internal bool
Parser_ReadStringValue(parser* P, u32 IdentIndex, gs_string* Result, bool ShouldNullTerminate = false)
{
    gs_const_string Ident = Parser_GetIdent(*P, IdentIndex);
    return Parser_ReadStringValue(P, Ident, Result, ShouldNullTerminate);
}

internal gs_string
Parser_ReadStringValue(parser* P, gs_const_string Ident, bool ShouldNullTerminate = false)
{
    gs_string Result = {0};
    Parser_ReadStringValue(P, Ident, &Result, ShouldNullTerminate);
    return Result;
}

internal gs_string
Parser_ReadStringValue(parser* P, u32 IdentIndex, bool ShouldNullTerminate = false)
{
    gs_const_string Ident = Parser_GetIdent(*P, IdentIndex);
    return Parser_ReadStringValue(P, Ident, ShouldNullTerminate);
}

internal bool
Parser_ReadOpenStruct(parser* P, gs_const_string Ident)
{
    // ident: {
    bool Result = Parser_AdvanceIfTokenEquals(P, Ident);
    Result &= Parser_AdvanceIfTokenEquals(P, ConstString(":"));
    Result &= Parser_AdvanceIfTokenEquals(P, ConstString("{"));
    return Result;
}

internal bool
Parser_ReadOpenStruct(parser* P, u32 IdentIndex)
{
    gs_const_string Ident = Parser_GetIdent(*P, IdentIndex);
    return Parser_ReadOpenStruct(P, Ident);
}

internal bool
Parser_ReadCloseStruct(parser* P)
{
    // }
    bool Result = Parser_AdvanceIfTokenEquals(P, ConstString("}"));
    Result &= Parser_AdvanceIfLineEnd(P);
    return Result;
}

internal bool
Parser_ReadNumberString(parser* P, gs_const_string* Output)
{
    Assert(Output != 0);
    
    bool Success = false;
    
    if (IsNumericExtended(P->At[0]))
    {
        char* NumStart = P->At;
        while(Parser_AtValidPosition(*P) && IsNumericExtended(P->At[0]))
        {
            Parser_AdvanceChar(P);
        }
        
        Output->Str = NumStart;
        Output->Length = P->At - NumStart;
        Success = true;
    }
    
    return Success;
}

internal gs_const_string
Parser_ReadNumberString(parser* P)
{
    gs_const_string Result = {0};
    Parser_ReadNumberString(P, &Result);
    return Result;
}

internal bool
Parser_ReadU32Value(parser* P, gs_const_string Ident, u32* Result)
{
    // ident: value;
    bool Success = false;
    
    if (Parser_AdvanceIfTokenEquals(P, Ident) &&
        Parser_AdvanceIfTokenEquals(P, ConstString(":")))
    {
        gs_const_string NumStr = Parser_ReadNumberString(P);
        if (Parser_AdvanceIfLineEnd(P))
        {
            *Result = (u32)ParseInt(NumStr);
            Success = true;
        }
        else
        {
            Parser_PushErrorF(P, "U32 Value doesn't end with semicolon");
        }
    }
    
    return Success;
}

internal u32
Parser_ReadU32Value(parser* P, gs_const_string Ident)
{
    u32 Result = 0;
    Parser_ReadU32Value(P, Ident, &Result);
    return Result;
}

internal u32
Parser_ReadU32Value(parser* P, u32 IdentIndex)
{
    gs_const_string Ident = Parser_GetIdent(*P, IdentIndex);
    return Parser_ReadU32Value(P, Ident);
}

internal bool
Parser_ReadR32(parser* P, r32* Result)
{
    bool Success = false;
    gs_const_string NumStr = {0};
    if (Parser_ReadNumberString(P, &NumStr))
    {
        *Result = (r32)ParseFloat(NumStr);
        Success = true;
    }
    return Success;
}

internal r32
Parser_ReadR32(parser* P)
{
    r32 Result = 0;
    Parser_ReadR32(P, &Result);
    return Result;
}

internal bool
Parser_ReadR32Value(parser* P, gs_const_string Ident, r32* Result)
{
    // ident: value;
    bool Success = false;
    if (Parser_AdvanceIfTokenEquals(P, Ident) &&
        Parser_AdvanceIfTokenEquals(P, ConstString(":")))
    {
        r32 Value = Parser_ReadR32(P);
        if (Parser_AdvanceIfLineEnd(P))
        {
            *Result = Value;
            Success = true;
        }
        else
        {
            Parser_PushErrorF(P, "R32 Value doesn't end with semicolon");
        }
    }
    
    return Success;
}

internal r32
Parser_ReadR32Value(parser* P, gs_const_string Ident)
{
    r32 Result = 0;
    Parser_ReadR32Value(P, Ident, &Result);
    return Result;
}

internal r32
Parser_ReadR32Value(parser* P, u32 IdentIndex)
{
    r32 Result = 0;
    gs_const_string Ident = Parser_GetIdent(*P, IdentIndex);
    Parser_ReadR32Value(P, Ident, &Result);
    return Result;
}

internal bool
Parser_ReadV3Value(parser* P, gs_const_string Ident, v3* Result)
{
    Assert(Result != 0);
    bool Success = false;
    if (Parser_AdvanceIfTokenEquals(P, Ident) &&
        Parser_AdvanceIfTokenEquals(P, ConstString(":")) &&
        Parser_AdvanceIfTokenEquals(P, ConstString("(")))
    {
        r32 X = Parser_ReadR32(P);
        if (!Parser_AdvanceIfTokenEquals(P, ConstString(",")))
        {
            Parser_PushErrorF(P, "V3 Value doesn't have comma separated values");
        }
        
        r32 Y = Parser_ReadR32(P);
        if (!Parser_AdvanceIfTokenEquals(P, ConstString(",")))
        {
            Parser_PushErrorF(P, "V3 Value doesn't have comma separated values");
        }
        
        r32 Z = Parser_ReadR32(P);
        if (Parser_AdvanceIfTokenEquals(P, ConstString(")")) &&
            Parser_AdvanceIfLineEnd(P))
        {
            Result->x = X;
            Result->y = Y;
            Result->z = Z;
            Success = true;
        }
        else
        {
            Parser_PushErrorF(P, "V3 Value doesn't end correctly");
        }
    }
    
    return Success;
}

internal v3
Parser_ReadV3Value(parser* P, gs_const_string Ident)
{
    v3 Result = {0};
    Parser_ReadV3Value(P, Ident, &Result);
    return Result;
}

internal v3
Parser_ReadV3Value(parser* P, u32 IdentIndex)
{
    v3 Result = {0};
    gs_const_string Ident = Parser_GetIdent(*P, IdentIndex);
    Parser_ReadV3Value(P, Ident, &Result);
    return Result;
}


#define FOLDHAUS_SERIALIZER_H
#endif // FOLDHAUS_SERIALIZER_H