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

struct parser
{
    gs_string String;
    
    gs_const_string* Identifiers;
    u32 IdentifiersCount;
    
    u32 Line;
    
    char* LineStart;
    char* At;
    
    gs_memory_arena* Arena;
};

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

internal gs_string
Parser_ReadStringValue(parser* P, gs_const_string Ident)
{
    // ident: "value";
    gs_string Result = {};
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
            Result = PushStringF(P->Arena, FileString.Length, "%S", FileString);
        }
        else
        {
            // TODO(pjs): Error
        }
    }
    
    return Result;
}

internal gs_string
Parser_ReadStringValue(parser* P, u32 IdentIndex)
{
    gs_const_string Ident = Parser_GetIdent(*P, IdentIndex);
    return Parser_ReadStringValue(P, Ident);
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

internal gs_const_string
Parser_ReadNumberString(parser* P)
{
    gs_const_string Result = {};
    Result.Str = P->At;
    while(Parser_AtValidPosition(*P) && IsNumericExtended(P->At[0]))
    {
        Parser_AdvanceChar(P);
    }
    Result.Length = P->At - Result.Str;
    return Result;
}

internal u32
Parser_ReadU32Value(parser* P, gs_const_string Ident)
{
    // ident: value;
    u32 Result = 0;
    if (Parser_AdvanceIfTokenEquals(P, Ident) &&
        Parser_AdvanceIfTokenEquals(P, ConstString(":")))
    {
        gs_const_string NumStr = Parser_ReadNumberString(P);
        if (Parser_AdvanceIfLineEnd(P))
        {
            Result = (u32)ParseInt(NumStr);
        }
        else
        {
            // TODO(pjs): Error
        }
    }
    
    return Result;
}

internal u32
Parser_ReadU32Value(parser* P, u32 IdentIndex)
{
    gs_const_string Ident = Parser_GetIdent(*P, IdentIndex);
    return Parser_ReadU32Value(P, Ident);
}

internal r32
Parser_ReadR32Value(parser* P, gs_const_string Ident)
{
    // ident: value;
    r32 Result = 0;
    if (Parser_AdvanceIfTokenEquals(P, Ident) &&
        Parser_AdvanceIfTokenEquals(P, ConstString(":")))
    {
        gs_const_string NumStr = Parser_ReadNumberString(P);
        if (Parser_AdvanceIfLineEnd(P))
        {
            Result = (r32)ParseFloat(NumStr);
        }
        else
        {
            // TODO(pjs): Error
        }
    }
    return Result;
}

internal u32
Parser_ReadR32Value(parser* P, u32 IdentIndex)
{
    gs_const_string Ident = Parser_GetIdent(*P, IdentIndex);
    return Parser_ReadR32Value(P, Ident);
}


#define FOLDHAUS_SERIALIZER_H
#endif // FOLDHAUS_SERIALIZER_H