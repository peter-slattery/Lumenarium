// TODO
/*
- InsertCharArrayIntoStringAt
- AppendChar
*/
#ifndef GS_STRING_H
#include <stdarg.h>

////////////////////////////////////////////////////////////////
//        String Tokenizing
////////////////////////////////////////////////////////////////

struct tokenizer
{
    char* At;
    char* LineStart;
    
    char* Memory;
    s32 MemoryLength;
    u32 LineNumber;
};

enum token_type
{
    Token_Error,
    
    Token_LeftParen,
    Token_RightParen,
    Token_LeftSquareBracket,
    Token_RightSquareBracket,
    Token_LeftCurlyBracket,
    Token_RightCurlyBracket,
    Token_Semicolon,
    Token_Operator,
    Token_Comma,
    Token_Period,
    Token_PointerReference,
    
    Token_PoundDefine, // Must stay first preproc token type
    Token_PoundUndef,
    Token_PoundInclude,
    Token_PoundIfDef,
    Token_PoundIfNDef,
    Token_PoundIf,
    Token_PoundElif,
    Token_PoundElse,
    Token_PoundEndif,
    Token_PoundError,
    Token_PoundPragma,
    Token_PoundLine,  // Must stay Last Preproc Token Type
    
    Token_Number,
    Token_Char,
    Token_String,
    Token_Identifier,
    
    Token_Comment,
    Token_MultilineComment,
    
    Token_EndOfLine,
    
    Token_Unknown,
    Token_EndOfStream,
};

char* TokenNames[] = {
    "Token_Error",
    "Token_LeftParen",
    "Token_RightParen",
    "Token_LeftSquareBracket",
    "Token_RightSquareBracket",
    "Token_LeftCurlyBracket",
    "Token_RightCurlyBracket",
    "Token_Semicolon",
    "Token_Operator",
    "Token_Comma",
    "Token_Period",
    "Token_PointerReference",
    "Token_PoundDefine",
    "Token_PoundUndef",
    "Token_PoundInclude",
    "Token_PoundIfDef",
    "Token_PoundIfNDef",
    "Token_PoundIf",
    "Token_PoundElif",
    "Token_PoundElse",
    "Token_PoundEndif",
    "Token_PoundError",
    "Token_PoundPragma",
    "Token_PoundLine",
    "Token_Number",
    "Token_Char",
    "Token_String",
    "Token_Identifier",
    "Token_Comment",
    "Token_MultilineComment",
    "Token_EndOfLine",
    "Token_Unknown",
    "Token_EndOfStream",
};

struct token
{
    token_type Type;
    u32 LineNumber;
    gs_string Text;
    token* Next; // TODO(Peter): Get rid of this
};

////////////////////////////////////////////////////////////////
//        String Function Declarations
////////////////////////////////////////////////////////////////

// Tokenizing
static void     EatChar(tokenizer* T);
static b32      AtValidPosition(tokenizer Tokenizer);
static b32      AtValidToken(tokenizer Tokenizer);
static char*    EatToNewLine(char* C);
static s32      EatToNewLine(tokenizer* T);
static char*    EatPastNewLine(char* C);
static s32      EatPastNewLine(tokenizer* T);
static char*    EatWhitespace(char* C);
static s32      EatWhitespace(tokenizer* T);
static char*    EatToNonWhitespaceOrNewline(char* C);
static s32      EatToNonWhitespaceOrNewline(tokenizer* T);
static char*    EatToWhitespace(char* C);
static s32      EatToWhitespace(tokenizer* T);
static char*    EatToCharacter(char* C, char Char);
static s32      EatToCharacter(tokenizer* T, char Char);
static char*    EatPastCharacter(char* C, char Char);
static s32      EatPastCharacter(tokenizer* T, char Char);
static char*    EatNumber(char* C);
static s32      EatNumber(tokenizer* T);

////////////////////////////////////////////////////////////////
//        Tokenizing
////////////////////////////////////////////////////////////////

static void
EatChar (tokenizer* T)
{
    if (AtValidPosition(*T))
    {
        if (IsNewline(*T->At))
        {
            T->LineNumber++;
            T->At++;
            T->LineStart = T->At;
        }
        else
        {
            T->At++;
        }
    }
}

static b32
AtValidPosition (tokenizer Tokenizer)
{
    b32 Result = (Tokenizer.At - Tokenizer.Memory) <= Tokenizer.MemoryLength;
    return Result;
}

static b32
AtValidToken(tokenizer Tokenizer)
{
    b32 Result = *Tokenizer.At && Tokenizer.At < (Tokenizer.Memory + Tokenizer.MemoryLength);
    return Result;
}

static char*
EatToNewLine(char* C)
{
    char* Result = C;
    while (*Result && !IsNewline(*Result))
    {
        Result++;
    }
    return Result;
}

static s32
EatToNewLine(tokenizer* T)
{
    char* TStart = T->At;
    while (AtValidPosition(*T) && !IsNewline(*T->At))
    {
        EatChar(T);
    }
    return T->At - TStart;
}

static char*
EatPastNewLine(char* C)
{
    char* Result = EatToNewLine(C);
    while(*Result && IsNewline(*Result))
    {
        Result++;
    }
    return Result;
}

static s32
EatPastNewLine(tokenizer* T)
{
    char* TStart = T->At;
    
    EatToNewLine(T);
    while(AtValidPosition(*T) && IsNewline(*T->At))
    {
        EatChar(T);
    }
    
    return T->At - TStart;
}

static char*
EatWhitespace(char* C)
{
    char* Result = C;
    while (*Result && IsNewlineOrWhitespace(*Result)) { Result++; }
    return Result;
}

static s32
EatWhitespace(tokenizer* T)
{
    char* TStart = T->At;
    while (AtValidPosition(*T) && IsNewlineOrWhitespace(*T->At)) { EatChar(T); }
    return T->At - TStart;
}

static char*
EatToNonWhitespaceOrNewline(char* C)
{
    char* Result = C;
    while (*Result && IsWhitespace(*Result)) { Result++; }
    return Result;
}

static s32
EatToNonWhitespaceOrNewline(tokenizer* T)
{
    char* TStart = T->At;
    while (AtValidPosition(*T) && IsWhitespace(*T->At)) { EatChar(T); }
    return T->At - TStart;
}

static char*
EatToWhitespace(char* C)
{
    char* Result = C;
    while (*Result && !IsWhitespace(*Result)) { Result++; }
    return Result;
}

static s32
EatToWhitespace(tokenizer* T)
{
    char* TStart = T->At;
    while (AtValidPosition(*T) && !IsWhitespace(*T->At)) { EatChar(T); }
    return T->At - TStart;
}

static char*
EatToCharacter(char* C, char Char)
{
    char* Result = C;
    while (*Result && *Result != Char) { Result++; }
    return Result;
}

static s32
EatToCharacter(tokenizer* T, char Char)
{
    char* TStart = T->At;
    while (AtValidPosition(*T) && *T->At != Char) { EatChar(T); }
    return T->At - TStart;
}

static char*
EatPastCharacter(char* C, char Char)
{
    char* Result = EatToCharacter(C, Char);
    if (*Result && *Result == Char) { Result++; }
    return Result;
}

static s32
EatPastCharacter(tokenizer* T, char Char)
{
    char* TStart = T->At;
    EatToCharacter(T, Char);
    if (AtValidPosition(*T) && *T->At == Char) { EatChar(T); }
    return T->At - TStart;
}

static char*
EatNumber(char* C)
{
    char* Result = C;
    while (*Result && IsNumericExtended(*Result)) { Result++; }
    return Result;
}

static s32
EatNumber(tokenizer* T)
{
    char* TStart = T->At;
    while (AtValidPosition(*T) && IsNumericExtended(*T->At)) { EatChar(T); }
    return T->At - TStart;
}

#define GS_STRING_H
#endif // GS_STRING_H


