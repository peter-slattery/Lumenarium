////////////////////////////////////////////////////////////////
//        String 
////////////////////////////////////////////////////////////////

struct string
{
    char* Memory;
    s32 Length;
    s32 Max;
};

////////////////////////////////////////////////////////////////
//        String Tokenizing
////////////////////////////////////////////////////////////////

struct tokenizer
{
    char* At;
    char* Memory;
    s32 MemoryLength;
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
    
    Token_PoundDefine,
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
    
    Token_Number,
    Token_Char,
    Token_String,
    Token_Identifier,
    
    Token_Comment,
    Token_MultilineComment,
    
    Token_Unknown,
    Token_EndOfStream,
};

char* TokenNames[] = {
    "Token_Error             ",
    "Token_LeftParen         ",
    "Token_RightParen        ",
    "Token_LeftSquareBracket         ",
    "Token_RightSquareBracket        ",
    "Token_LeftCurlyBracket       ",
    "Token_RightCurlyBracket      ",
    "Token_Semicolon         ",
    "Token_Operator          ",
    "Token_Comma             ",
    "Token_Period            ",
    "Token_PointerReference  ",
    "Token_PoundDefine       ",
    "Token_PoundUndef        ",
    "Token_PoundInclude      ",
    "Token_PoundIfDef        ",
    "Token_PoundIfNDef       ",
    "Token_PoundIf           ",
    "Token_PoundElif         ",
    "Token_PoundElse         ",
    "Token_PoundEndif        ",
    "Token_PoundError        ",
    "Token_PoundPragma       ",
    "Token_Number            ",
    "Token_Char              ",
    "Token_String            ",
    "Token_Identifier        ",
    "Token_Comment           ",
    "Token_MultilineComment  ",
    "Token_Unknown           ",
    "Token_EndOfStream       ",
};

struct token
{
    token_type Type;
    char*      Text;
    s32        TextLength;
    
    token* Next;
};

////////////////////////////////////////////////////////////////
//        String Memory
////////////////////////////////////////////////////////////////

struct slot_header
{
    slot_header* Next;
    s32 Size;
};

struct slot_arena
{
    u8* Memory;
    s32 SlotSize;
    s32 SlotCount;
    slot_header* FreeList;
};

struct contiguous_slot_count_result
{
    s32 Count;
    slot_header* LastContiguousSlot;
};


////////////////////////////////////////////////////////////////
//        String Function Declarations
////////////////////////////////////////////////////////////////

// Utility
#if !defined GS_LANGUAGE_H

static void    GSZeroMemory (u8* Memory, s32 Size);
static s32     GSMin (s32 A, s32 B); 
static s32     GSAbs (s32 A); 
static float   GSAbsF (float A);
static float   GSPowF (float N, s32 Power);

#endif

// Setup

#ifdef GS_MEMORY_H
#define PushString(str, arena, size) (str)->Memory = PushArray(arena, char, size); (str)->Length = 0; (str)->Max = size;
#endif

static void   InitializeString (string* String, char* Data, s32 DataSize);
static string InitializeString (char* Data, s32 DataSize);
static void   ClearString (string* String);

// Character Values
static bool     IsSlash (char C);
static bool     IsNewline (char C);
static bool     IsWhitespace (char C);
static bool     IsAlpha (char C);
static bool     IsUpper (char C);
static bool     IsLower (char C);
static bool     IsNumeric (char C);
static bool     IsNumericExtended (char C);
static bool     ToUpper (char C);
static bool     ToLower (char C);
static bool     IsAlphaNumeric (char C);
static bool     IsOperator (char C);

// Tokenizing
static b32      AtValidToken(tokenizer Tokenizer);
static char*    EatToNewLine(char* C);
static void     EatToNewLine(tokenizer* T);
static char*    EatWhitespace(char* C);
static void     EatWhitespace(tokenizer* T);
static char*    EatToWhitespace(char* C);
static void     EatToWhitespace(tokenizer* T);
static char*    EatToCharacter(char* C, char Char);
static void     EatToCharacter(tokenizer* T, char Char);
static char*    EatPastCharacter(char* C, char Char);
static void     EatPastCharacter(tokenizer* T, char Char);
static char*    EatNumber(char* C);
static void     EatNumber(tokenizer* T);

// Char/Char Array
static u32      CharToUInt (char C);
static s32      CharArrayLength (char* CharArray);
static bool     CharArraysEqual (char* A, s32 ALength, char* B, s32 BLength);
static bool     CharArraysEqualUnsafe (char* A, char* B);
static void     ReverseCharArray (char* Array, s32 Length);
#define         FirstIndexOfChar(array, find) IndexOfChar(array, 0, find)
static s32      IndexOfChar (char* Array, s32 Start, char Find);
#define         FastLastIndexOfChar(array, len, find) FastReverseIndexOfChar(array, len, 0, find)
static s32      FastReverseIndexOfChar (char* Array, s32 Length, s32 OffsetFromEnd, char Find);
#define         LastIndexOfChar(array, find) ReverseIndexOfChar(array, 0, find)
static s32      ReverseIndexOfChar (char* Array, s32 OffsetFromEnd, char Find);
static b32      CharArrayContains(char* Array, char* CheckFor);
static b32      CharArrayContainsSafe(char* Array, s32 ArrayLength, char* CheckFor, s32 CheckForLength);

// String
static string  MakeString (char* Array, s32 Length, s32 Max);
static string  MakeString (char* Array, s32 Length);
static string  MakeString (char* Array);
static string  MakeStringLiteral(char* Data);

static bool    StringsEqual (string A, string B);
static bool    StringEqualsCharArray (string String, char* CharArray);
static bool    StringEqualsCharArray (string String, char* CharArray, s32 CharArrayLength);
static s32     FindFirstChar (string String, char C);

static void    SetStringToChar (string* Dest, char C, s32 Count);
static void    SetStringToCharArray (string* Dest, char* Source);

static void    ConcatString (string* Dest, string Source);
static void    ConcatString (string* Dest, string Source, s32 Length);
static void    ConcatCharToString(string* Dest, char C);
static void    ConcatCharArrayToString (string* Dest, char* Source);
static void    ConcatCharArrayToString (string* Dest, char* Source, s32 SourceLength);

static void    CopyStringTo (string Source, string* Dest);
static s32     CopyStringToCharArray (string Source, char* Dest, s32 DestLength);
static void    CopyCharArrayToString (char* Src, string* Dest);
static void    CopyCharArrayToString (char* Src, s32 SrcLength, string* Dest);
static s32     CopyCharArray (char* Source, char* Dest, s32 DestLength);
static s32     CopyCharArrayAt (char* Source, char* Dest, s32 DestLength, s32 Offset);

static void    InsertChar (string* String, char Char, s32 Index);
static void    InsertStringAt (string* Dest, string Source, s32 At);
static void    RemoveCharAt (string* String, s32 Index);

static string  Substring (string* String, s32 Start, s32 End);
static string  Substring (string* String, s32 Start);

static void    NullTerminate (string* String);


// Parsing
enum parse_type
{
    ParseType_UnsignedInt,
    ParseType_SignedInt,
    ParseType_Float,
};

struct parse_result
{
    parse_type Type;
    char* OnePastLast;
    union
    {
        u32 UnsignedIntValue;
        s32 SignedIntValue;
        r32 FloatValue;
    };
};

enum format_flags
{
    FormatFlags_LeftJustify = 0x1,
    FormatFlags_ForceSign = 0x2,
    FormatFlags_ForceSpaceInsteadOfSign = 0x4,
    FormatFlags_ForceDecimalOrPrependOx = 0x8,
    FormatFlags_PadWithZeroesInsteadOfSpaces = 0x16,
};

static parse_result ParseUnsignedInt (char* String, s32 Length);
static parse_result ParseSignedInt (char* String, s32 Length);
static parse_result ParseFloat (char* String, s32 Length);

// PrintF
static void PrintFArgList(char* Dest, s32 DestMax, char* Format, va_list Args);
static void  PrintF(string* String, char* Format, ...);

////////////////////////////////////////////////////////////////
//        String Memory Function Declarations
////////////////////////////////////////////////////////////////

static s32                          CalculateSlotCountFromSize (s32 RequestedSize, s32 SlotSize);
static bool                         SlotsAreContiguous (slot_header* First, slot_header* Second);
static contiguous_slot_count_result CountContiguousSlots (slot_header* First);
static slot_header*                 GetSlotAtOffset(slot_header* First, s32 Offset);
static slot_header*                 InsertSlotIntoList (slot_header* NewSlot, slot_header* ListStart);
static void                         AllocStringFromStringArena (string* String, s32 Size, slot_arena* Storage);
static string                       AllocStringFromStringArena (s32 Size, slot_arena* Storage);
static void                         FreeToStringArena (string* String, slot_arena* Storage);
static void                         ReallocFromStringArena (string* String, s32 NewSize, slot_arena* Storage);

////////////////////////////////////////////////////////////////
//        String Utility Functions
////////////////////////////////////////////////////////////////

#if !defined GS_LANGUAGE_H

static void
GSZeroMemory (u8* Memory, s32 Size)
{
    for (int i = 0; i < Size; i++) { Memory[i] = 0; }
}

static s32
GSMin (s32 A, s32 B)
{
    return (A < B ? A : B);
}

static s32
GSAbs (s32 A)
{
    return (A < 0 ? -A : A);
}

static float
GSAbs (float A)
{
    return (A < 0 ? -A : A);
}

static float
GSPow (float N, s32 Power)
{
    float Result = N;
    for(s32 i = 1; i < Power; i++) { Result *= N; }
    return Result;
}

#endif

////////////////////////////////////////////////////////////////
//        Init and Clear
////////////////////////////////////////////////////////////////

static void
InitializeString (string* String, char* Data, s32 DataSize)
{
    String->Memory = Data;
    String->Max = DataSize;
    String->Length = 0;
}

static string
InitializeString (char* Data, s32 DataSize)
{
    string Result = {};
    Result.Memory = Data;
    Result.Max = DataSize;
    Result.Length = 0;
    return Result;
}

static void
ClearString (string* String)
{
    String->Memory = 0;
    String->Max = 0;
    String->Length = 0;
}

////////////////////////////////////////////////////////////////
//        Char Value Types
////////////////////////////////////////////////////////////////

static bool IsSlash (char C) { return ((C == '\\') || (C == '/')); }
static bool IsNewline (char C) { return (C == '\n') || (C == '\r'); }
static bool IsWhitespace (char C) { return (C == ' ') || (C == '\t') || IsNewline(C); }
static bool IsAlpha (char C)
{
    // TODO(Peter): support UTF8 chars
    return ((C >= 'A') && (C <= 'Z')) || ((C >= 'a') && (C <= 'z')) || (C == '_');
}
static bool IsUpper (char C)
{
    return ((C >= 'A') && (C <= 'Z'));
}
static bool IsLower (char C)
{
    return ((C >= 'a') && (C <= 'z'));
}
static bool IsNumeric (char C)
{
    return (C >= '0') && (C <= '9');
}
static bool IsNumericExtended (char C)
{
    return (IsNumeric(C) || (C == 'x') || (C == 'f') || (C == '.'));
}
static bool IsAlphaNumeric (char C)
{
    return IsAlpha(C) || IsNumeric(C);
}
static bool IsOperator (char C)
{
    return ((C == '+') ||
            (C == '-') ||
            (C == '*') ||
            (C == '/') ||
            (C == '=') ||
            (C == '%') ||
            (C == '<') ||
            (C == '>'));
}
////////////////////////////////////////////////////////////////
//        Tokenizing
////////////////////////////////////////////////////////////////

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
    while (*Result && !IsNewline(*Result)) { Result++; }
    if (*Result) { Result++; } // NOTE(Peter): eat past the newline character
    return Result;
}

static void     
EatToNewLine(tokenizer* T)
{
    while (*T->At && !IsNewline(*T->At)) { T->At++; }
    if (*T->At) { T->At++; } // NOTE(Peter): eat past the newline character
}

static char*    
EatWhitespace(char* C)
{
    char* Result = C;
    while (*Result && IsWhitespace(*Result)) { Result++; }
    return Result;
}

static void     
EatWhitespace(tokenizer* T)
{
    while (*T->At && IsWhitespace(*T->At)) { T->At++; }
}

static char*    
EatToWhitespace(char* C)
{
    char* Result = C;
    while (*Result && !IsWhitespace(*Result)) { Result++; }
    return Result;
}

static void     
EatToWhitespace(tokenizer* T)
{
    while (*T->At && !IsWhitespace(*T->At)) { T->At++; }
}

static char*    
EatToCharacter(char* C, char Char)
{
    char* Result = C;
    while (*Result && *Result != Char) { Result++; }
    return Result;
}

static void     
EatToCharacter(tokenizer* T, char Char)
{
    while (*T->At && *T->At != Char) { T->At++; }
}

static char*    
EatPastCharacter(char* C, char Char)
{
    char* Result = EatToCharacter(C, Char);
    if (*Result && *Result == Char) { Result++; }
    return Result;
}

static void     
EatPastCharacter(tokenizer* T, char Char)
{
    EatToCharacter(T, Char);
    if (*T->At && *T->At == Char) { T->At++; }
}

static char*    
EatNumber(char* C)
{
    char* Result = C;
    while (*Result && IsNumericExtended(*Result)) { Result++; }
    return Result;
}

static void     
EatNumber(tokenizer* T)
{
    while (*T->At && IsNumericExtended(*T->At)) { T->At++; }
}

////////////////////////////////////////////////////////////////
//        Basic Char Operations
////////////////////////////////////////////////////////////////

static u32 CharToUInt (char C) { 
    u32 Result = (C - '0');
    return Result; 
}

static s32
CharArrayLength (char* Array)
{
    char* C = Array;
    s32 Result = 0;
    while (*C)
    {
        *C++;
        Result++;
    }
    return Result;
}

static s32      
NullTerminatedCharArrayLength (char* CharArray)
{
    char* Iter = CharArray;
    while (*Iter)
    {
        *Iter++;
    }
    return (Iter - CharArray);
}

static bool
CharArraysEqual (char* A, s32 ALength, char* B, s32 BLength)
{
    bool Result = false;
    if (ALength == BLength)
    {
        Result = true;
        char* AIter = A;
        char* BIter = B;
        for (s32 i = 0; i < ALength; i++)
        {
            if(*AIter++ != *BIter++)
            {
                Result = false;
                break;
            }
        }
    }
    return Result;
}

static bool
CharArraysEqualUnsafe (char* A, char* B)
{
    bool Result = true;
    
    char* AIter = A;
    char* BIter = B;
    while(*AIter && *BIter)
    {
        if(*AIter++ != *BIter++)
        {
            Result = false;
            break;
        }
    }
    
    if((*AIter && !*BIter) || (!*AIter && *BIter))
    {
        Result = false;
    }
    
    return Result;
}

static bool
CharArraysEqualUpToLength (char* A, char* B, s32 Length)
{
    bool Result = true;
    
    char* AIter = A;
    char* BIter = B;
    for (s32 i = 0; i < Length; i++)
    {
        if(*AIter++ != *BIter++)
        {
            Result = false;
            break;
        }
    }
    
    return Result;
}

static void
ReverseCharArray (char* Array, s32 Length)
{
    char* ForwardIter = Array;
    char* BackwardIter = Array + Length - 1;
    for (s32 i = 0; i < (Length / 2); i++)
    {
        char F = *ForwardIter;
        char B = *BackwardIter;
        *ForwardIter++ = B;
        *BackwardIter-- = F;
    }
}

static s32 
IndexOfChar (char* Array, s32 After, char Find)
{
    s32 Result = -1;
    
    s32 Counter = After;
    char* Iter = Array + After;
    while (*Iter)
    {
        if (*Iter == Find)
        {
            Result = Counter;
            break;
        }
        Counter++;
        *Iter++;
    }
    
    return Result;
}

static s32
FastReverseIndexOfChar (char* Array, s32 Length, s32 OffsetFromEnd, char Find)
{
    s32 Result = -1;
    
    s32 Counter = Length - OffsetFromEnd;
    char* Iter = Array + Length - OffsetFromEnd;
    for (int i = 0; i < (Length - OffsetFromEnd); i++)
    {
        if (*Iter == Find)
        {
            Result = Counter;
            break;
        }
        
        *Iter--;
        Counter--;
    }
    
    return Result;
}

static s32
ReverseIndexOfChar (char* Array, s32 OffsetFromEnd, char Find)
{
    s32 StringLength = NullTerminatedCharArrayLength(Array);
    return FastReverseIndexOfChar(Array, StringLength, OffsetFromEnd, Find);
}

static b32      
CharArrayContains(char* Array, char* CheckFor)
{
    b32 Result = false;
    
    char* Src = Array;
    while (*Src)
    {
        if (*Src == *CheckFor)
        {
            char* A = CheckFor;
            char* B = Src;
            while (*B && *A && *A == *B)
            {
                *B++; *A++;
            }
            
            if (*A == 0)
            {
                Result = true;
                break;
            }
        }
        
        Src++;
    }
    
    return Result;
}

static b32      
CharArrayContainsSafe(char* Array, s32 ArrayLength, char* CheckFor, s32 CheckForLength)
{
    b32 Result = false;
    
    if (ArrayLength >= CheckForLength) 
    {
        char* Src = Array;
        for (s32 s = 0; s < ArrayLength; s++)
        {
            if (*Src == *CheckFor && (s + CheckForLength <= ArrayLength))
            {
                char* A = CheckFor;
                char* B = Src;
                for (s32 d = 0; d < CheckForLength; d++)
                {
                    if (*B != *A) { break; }
                    *B++; *A++;
                }
                
                if (*A == 0)
                {
                    Result = true;
                    break;
                }
            }
            
            Src++;
        }
    }
    
    return Result;
}

////////////////////////////////////////////////////////////////
//        Basic String Operations
////////////////////////////////////////////////////////////////

static bool
StringsEqual (string A, string B)
{
    bool Result = false;
    
    if (A.Length == B.Length)
    {
        Result = true;
        char* AIter = A.Memory;
        char* BIter = B.Memory;
        for (s32 i = 0; i < A.Length; i++)
        {
            if (*AIter++ != *BIter++)
            {
                Result = false;
                break;
            }
        }
    }
    
    return Result;
}

static string
MakeString (char* Array, s32 Length, s32 Max)
{
    string Result = {};
    Result.Memory = Array;
    Result.Length = Length;
    Result.Max = Max;
    return Result;
}

static string
MakeString (char* Array, s32 Length)
{
    string Result = {};
    Result.Memory = Array;
    Result.Length = Length;
    Result.Max = Length;
    return Result;
}

static string
MakeString (char* Array)
{
    s32 Length = CharArrayLength (Array);
    return MakeString(Array, Length);
}

static string
MakeStringLiteral (char* String)
{
    string Result = {};
    Result.Memory = String;
    Result.Max = CharArrayLength(String);
    Result.Length = Result.Max;
    return Result;
}

static bool
StringEqualsCharArray (string String, char* CharArray)
{
    bool Result = true;
    
    char* S = String.Memory;
    char* C = CharArray;
    
    s32 Count = 0;
    while (*C && Count < String.Length)
    {
        if (*C++ != *S++)
        {
            Result = false;
            break;
        }
        Count++;
    }
    
    return Result;
}

static bool    
StringEqualsCharArray (string String, char* CharArray, s32 CharArrayLength)
{
    bool Result = false;
    
    if (CharArrayLength == String.Length)
    {
        Result = true;
        
        char* S = String.Memory;
        char* C = CharArray;
        for (s32 i = 0; i < String.Length; i++)
        {
            if (*C++ != *S++)
            {
                Result = false;
                break;
            }
        }
    }
    
    return Result;
}

static s32
FindFirstChar (string String, char C)
{
    s32 Result = -1;
    
    char* Iter = String.Memory;
    for (int i = 0; i < String.Length; i++)
    {
        if (*Iter++ == C)
        {
            Result = i;
            break;
        }
    }
    
    return Result;
}

static void
SetStringToChar (string* Dest, char C, s32 Count)
{
    Assert(Count <= Dest->Max);
    
    char* Iter = Dest->Memory;
    for (int i = 0; i < Count; i++)
    {
        *Iter++ = C;
    }
    Dest->Length = Count;
}

static void
SetStringToCharArray (string* Dest, char* Source)
{
    Dest->Length = 0;
    
    char* Src = Source;
    char* Dst = Dest->Memory;
    while (*Src && Dest->Length < Dest->Max)
    {
        *Dst++ = *Src++;
        Dest->Length++;
    }
}

static void
ConcatString (string* Dest, string Source)
{
    Assert((Dest->Length + Source.Length) <= Dest->Max);
    
    char* Dst = Dest->Memory + Dest->Length;
    char* Src = Source.Memory;
    for (s32 i = 0; i < Source.Length; i++)
    {
        *Dst++ = *Src++;
        Dest->Length++;
    }
}

static void
ConcatString (string* Dest, string Source, s32 Length)
{
    Assert(Length < Source.Length);
    Assert((Dest->Length + Length) <= Dest->Max);
    
    char* Dst = Dest->Memory + Dest->Length;
    char* Src = Source.Memory;
    for (s32 i = 0; i < Length; i++)
    {
        *Dst++ = *Src++;
        Dest->Length++;
    }
}

static void
ConcatCharToString (string* Dest, char C)
{
    Assert(Dest->Length + 1 <= Dest->Max);
    
    char* Dst = Dest->Memory + Dest->Length;
    *Dst = C;
    Dest->Length++;
}

static void    
ConcatCharArrayToString (string* Dest, char* Source)
{
    Assert(CharArrayLength(Source) + Dest->Length <= Dest->Max);
    
    char* Dst = Dest->Memory + Dest->Length;
    char* Src = Source;
    while (Dest->Length < Dest->Max &&
           *Src)
    {
        *Dst++ = *Src++;
        Dest->Length++;
    }
}

static void    
ConcatCharArrayToString (string* Dest, char* Source, s32 SourceLength)
{
    Assert(SourceLength + Dest->Length <= Dest->Max);
    
    char* Dst = Dest->Memory + Dest->Length;
    char* Src = Source;
    for (int i = 0; i < SourceLength && Dest->Length < Dest->Max; i++)
    {
        *Dst++ = *Src++;
        Dest->Length++;
    }
}

static void
CopyStringTo (string Source, string* Dest)
{
    char* Src = Source.Memory;
    char* Dst = Dest->Memory;
    s32 CopyLength = GSMin(Source.Length, Dest->Max);
    for (int i = 0; i < CopyLength; i++)
    {
        *Dst++ = *Src++;
    }
    Dest->Length = Source.Length;
}

static s32    
CopyStringToCharArray (string Source, char* Dest, s32 DestLength)
{
    char* Src = Source.Memory;
    char* Dst = Dest;
    s32 CopyLength = GSMin(Source.Length, DestLength);
    for (int i = 0; i < CopyLength; i++)
    {
        *Dst++ = *Src++;
    }
    return CopyLength;
}

static void    
CopyCharArrayToString (char* Source, string* Dest)
{
    char* Src = Source;
    char* Dst = Dest->Memory;
    s32 Copied = 0;
    while (*Src && Copied < Dest->Max)
    {
        *Dst++ = *Src++;
        Copied++;
    }
    Dest->Length = Copied;
}

static void    
CopyCharArrayToString (char* Source, s32 SourceLength, string* Dest)
{
    Assert(SourceLength <= Dest->Max);
    
    char* Src = Source;
    char* Dst = Dest->Memory;
    for (s32 i = 0; i < SourceLength; i++)
    {
        *Dst++ = *Src++;
    }
    Dest->Length = SourceLength;
}

static s32
CopyCharArray (char* Source, char* Dest, s32 DestLength)
{
    char* Src = Source;
    char* Dst = Dest;
    s32 i = 0; 
    while (*Src && i < DestLength)
    {
        *Dst++ = *Src++;
        i++;
    }
    return i;
}

static s32    
CopyCharArrayAt (char* Source, char* Dest, s32 DestLength, s32 Offset)
{
    Assert(Offset < DestLength);
    
    char* Src = Source;
    char* Dst = Dest + Offset;
    s32 i = Offset;
    while (*Src && i < DestLength)
    {
        *Dst++ = *Src++;
        i++;
    }
    return i - Offset;
}

static void
InsertChar (string* String, char Char, s32 Index)
{
    Assert(Index >= 0 && Index < String->Max);
    Assert(String->Length < String->Max);
    
    char* Src = String->Memory + String->Length;
    char* Dst = Src + 1;
    for (int i = String->Length - 1; i >= Index; i--)
    {
        *Dst-- = *Src--;
    }
    
    *(String->Memory + Index) = Char;
    String->Length++;
}

static void
RemoveCharAt (string* String, s32 Index)
{
    Assert(Index >= 0 && Index < String->Max);
    
    char* Dst = String->Memory + Index;
    char* Src = Dst + 1;
    for (int i = Index; i < String->Length; i++)
    {
        *Dst++ = *Src++;
    }
    *Dst = 0;
    String->Length--;
}

static string
Substring (string String, s32 Start, s32 End)
{
    Assert(Start >= 0 && End > Start && End <= String.Length);
    
    string Result = {};
    Result.Memory = String.Memory + Start;
    Result.Length = End - Start;
    return Result;
}

static string
Substring (string String, s32 Start)
{
    Assert(Start >= 0 && Start < String.Length);
    
    string Result = {};
    Result.Memory = String.Memory + Start;
    Result.Length = String.Length - Start;
    return Result;
}

static void
NullTerminate (string* String)
{
    Assert(String->Length + 1 <= String->Max);
    *(String->Memory + String->Length) = 0;
    String->Length++;
}

static void    
InsertStringAt (string* Dest, string Source, s32 At)
{
    Assert(At + Source.Length < Dest->Max);
    Assert(At < Dest->Length);
    
    char* Src = Dest->Memory + Dest->Length;
    char* Dst = Dest->Memory + Source.Length + Dest->Length;
    for (s32 i = Dest->Length - 1; i >= At; i--)
    {
        *--Dst = *--Src;
    }
    
    Src = Source.Memory;
    Dst = Dest->Memory + At;
    for (s32 j = 0; j < Source.Length; j++)
    {
        *Dst++ = *Src++;
    }
    
    Dest->Length += Source.Length;
}

////////////////////////////////////////////////////////////////
//        String Parsing
////////////////////////////////////////////////////////////////

static parse_result
ParseUnsignedInt (char* String, s32 Length)
{
    Assert(IsNumeric(*String));
    parse_result Result = {};
    Result.Type = ParseType_UnsignedInt;
    
    char* Iter = String;
    u32 ResultValue = 0;
    for (s32 i = 0; i < Length; i++)
    {
        ResultValue = CharToUInt(*Iter++) + (ResultValue * 10);
    }
    
    Result.UnsignedIntValue = ResultValue;
    Result.OnePastLast = Iter;
    
    return Result;
}

static parse_result
ParseUnsignedIntUnsafe (char* String)
{
    char* Start = String;
    char* End = EatNumber(String + 1);
    return ParseUnsignedInt(String, End - Start);
}

static parse_result
ParseSignedInt (char* String, s32 Length)
{
    Assert(Length > 0);
    parse_result Result = {};
    Result.Type = ParseType_SignedInt;
    
    s32 Negative = 1;
    s32 LengthRemaining = Length;
    s32 ResultValue = 0;
    char* Iter = String;
    
    if (*Iter == '-') { 
        LengthRemaining--;
        *Iter++; 
        Negative = -1; 
    }
    
    for (s32 i = 0; i < LengthRemaining; i++)
    {
        ResultValue = CharToUInt(*Iter++) + (ResultValue * 10);
    }
    
    ResultValue *= Negative;
    
    Result.SignedIntValue = ResultValue;
    Result.OnePastLast = Iter;
    
    return Result;
}

static parse_result
ParseSignedIntUnsafe (char* String)
{
    char* Start = String;
    char* End = EatNumber(String + 1);
    return ParseSignedInt(String, End - Start);
}

static parse_result
ParseFloat (char* String, s32 Length)
{
    parse_result Result = {};
    Result.Type = ParseType_Float;
    
    s32 Negative = 1;
    s32 LengthRemaining = Length;
    float ResultValue = 0;
    char* Iter = String;
    
    if (*Iter == '-') { 
        LengthRemaining--;
        *Iter++; 
        Negative = -1; 
    }
    
    for (s32 i = 0; i < LengthRemaining; i++)
    {
        if (IsNumeric(*Iter))
        {
            ResultValue = (float)CharToUInt(*Iter++) + (ResultValue * 10);
        }
        else if (*Iter == '.' || *Iter == 0) 
        { 
            LengthRemaining -= i;
            break; 
        }
    }
    
    if (*Iter == '.')
    {
        *Iter++;
        float AfterPoint = 0;
        s32 PlacesAfterPoint = 0;
        
        for (s32 i = 0; i < LengthRemaining; i++)
        {
            if (IsNumeric(*Iter))
            {
                AfterPoint = (float)CharToUInt(*Iter++) + (AfterPoint * 10);
                PlacesAfterPoint++;
            }
            else
            {
                break;
            }
        }
        
        AfterPoint = AfterPoint / GSPow(10, PlacesAfterPoint);
        ResultValue += AfterPoint;
    }
    
    ResultValue *= Negative;
    
    Result.FloatValue = ResultValue;
    Result.OnePastLast = Iter;
    
    return Result;
}

static parse_result
ParseFloatUnsafe (char* String)
{
    char* Start = String;
    char* End = EatNumber(String + 1);
    return ParseFloat(String, End - Start);
}

static s32
UIntToString (u32 Int, char* String, s32 MaxLength, b32 FormatFlags = 0, s32 MinimumLength = 0)
{
    s32 Remaining = Int;
    char* Iter = String;
    while (Remaining > 0 && (Iter - String) < MaxLength)
    {
        *Iter++ = '0' + (Remaining % 10);
        Remaining /= 10;
    }
    s32 CharsCopied = Iter - String;
    ReverseCharArray(String, CharsCopied);
    return CharsCopied;
}

static s32
IntToString (s32 Int, char* String, s32 MaxLength, b32 FormatFlags = 0, s32 MinimumLength = 0)
{
    s32 Remaining = Int;
    s32 CharsCopied = 0;
    
    char* Iter = String;
    
    bool Negative = Remaining < 0;
    Remaining = GSAbs(Remaining);
    
    if (Remaining > 0)
    {
        while (Remaining > 0 && CharsCopied < MaxLength)
        {
            *Iter++ = '0' + (Remaining % 10);
            Remaining /= 10;
            CharsCopied++;
        }
    }
    else if (Remaining == 0)
    {
        *Iter++ = '0';
    }
    
    if (Negative)
    {
        *Iter++ = '-';
        CharsCopied++;
    }
    
    ReverseCharArray(String, CharsCopied);
    return CharsCopied;
}

static s32 
IntToString (s32 Int, char* String, s32 MaxLength, s32 Offset, b32 FormatFlags = 0, s32 MinimumWidth = 0)
{
    char* StringStart = String + Offset;
    s32 LengthWritten = IntToString(Int, StringStart, MaxLength - Offset);
    return LengthWritten;
}

static s32
FloatToString(float Float, char *String, s32 MaxLength, s32 AfterPoint = 0, b32 FormatFlags = 0, s32 MinimumWidth = 0)
{
    s32 IPart = (s32)Float;
    float FPart = GSAbs(Float - (float)IPart);
    
    s32 i = IntToString(IPart, String, MaxLength);
    
    if (AfterPoint > 1)
    {
        String[i++] = '.';
        
        s32 FPartInt = FPart * GSPow(10, AfterPoint);
        i += IntToString(FPartInt, String, MaxLength, i, 0, 0);
    }
    
    return i;
}

////////////////////////////////////////////////////////////////
//        PrintF
////////////////////////////////////////////////////////////////

static void
OutChar (string* String, char C)
{
    if (String->Length < String->Max)
    {
        String->Memory[String->Length] = C;
        String->Length++;
    }
}

char OctalDigits[] = "01234567";
char DecimalDigits[] = "0123456789";
char HexDigits[] = "0123456789ABCDEF";

static void
U64ToASCII (string* String, u64 Value, s32 Base, char* Digits)
{
    u64 ValueRemaining = Value;
    char* Start = String->Memory + String->Length;
    do {
        s32 DigitsIndex = ValueRemaining % Base;
        char Digit = Digits[DigitsIndex];
        OutChar(String, Digit);
        ValueRemaining /= Base;
    }while (ValueRemaining);
    char* End = String->Memory + String->Length;
    
    while (Start < End)
    {
        End--;
        char Temp = *End;
        *End = *Start;
        *Start = Temp;
        *Start++;
    }
}

static void
F64ToASCII (string* String, r64 Value, s32 Precision)
{
    if (Value < 0)
    {
        OutChar(String, '-');
        Value = -Value;
    }
    
    u64 IntegerPart = (u64)Value;
    Value -= IntegerPart;
    
    U64ToASCII(String, IntegerPart, 10, DecimalDigits);
    
    OutChar(String, '.');
    
    for (s32 i = 0; i < Precision; i++)
    {
        Value *= 10.f;
        u32 DecimalPlace = Value;
        Value -= DecimalPlace;
        OutChar(String, DecimalDigits[DecimalPlace]);
    }
}

internal s64
ReadVarArgsSignedInteger (s32 Width, va_list* Args)
{
    s64 Result = 0;
    switch (Width)
    {
        case 1: { Result = (s64)va_arg(*Args, s8); } break;
        case 2: { Result = (s64)va_arg(*Args, s16); } break;
        case 4: { Result = (s64)va_arg(*Args, s32); } break;
        case 8: { Result = (s64)va_arg(*Args, s64); } break;
        InvalidDefaultCase;
    }
    return Result;
}

internal r64
ReadVarArgsUnsignedInteger (s32 Width, va_list* Args)
{
    u64 Result = 0;
    switch (Width)
    {
        case 1: { Result = (u64)va_arg(*Args, u8); } break;
        case 2: { Result = (u64)va_arg(*Args, u16); } break;
        case 4: { Result = (u64)va_arg(*Args, u32); } break;
        case 8: { Result = (u64)va_arg(*Args, u64); } break;
        InvalidDefaultCase;
    }
    return Result;
}

internal r64
ReadVarArgsFloat (s32 Width, va_list* Args)
{
    r64 Result = 0;
    switch (Width)
    {
        case 4: { Result = (r64)va_arg(*Args, r64); } break;
        case 8: { Result = (r64)va_arg(*Args, r64); } break;
        InvalidDefaultCase;
    }
    return Result;
}

internal s32
PrintFArgsList (char* Dest, s32 DestMax, char* Format, va_list Args)
{
    char* DestAt = Dest;
    
    char* FormatAt = Format;
    while (*FormatAt)
    {
        if (FormatAt[0] != '%')
        {
            *DestAt++ = *FormatAt++;
        }
        else if (FormatAt[0] == '%' && FormatAt[1] == '%')  // Print the % symbol
        { 
            *DestAt++ = *FormatAt++; 
        }
        else
        {
            FormatAt++;
            
            // Flags
            if (FormatAt[0] == '-')
            {
                FormatAt++;
            }
            else if (FormatAt[0] == '+')
            {
                FormatAt++;
            }
            else if (FormatAt[0] == ' ')
            {
                FormatAt++;
            }
            else if (FormatAt[0] == '#')
            {
                FormatAt++;
            }
            else if (FormatAt[0] == '0')
            {
                FormatAt++;
            }
            
            // Width
            b32 WidthSpecified = false;
            s32 Width = 0;
            
            if (IsNumeric(FormatAt[0]))
            {
                WidthSpecified = true;
                parse_result Parse = ParseSignedIntUnsafe(FormatAt);
                FormatAt = Parse.OnePastLast;
                Width = Parse.SignedIntValue;
            }
            else if (FormatAt[0] == '*')
            {
                WidthSpecified = true;
                Width = va_arg(Args, s32);
                Assert(Width >= 0);
                FormatAt++;
            }
            
            // Precision
            b32 PrecisionSpecified = false;
            s32 Precision = 0;
            
            if (FormatAt[0] == '.')
            {
                FormatAt++;
                if (IsNumeric(FormatAt[0]))
                {
                    PrecisionSpecified = true;
                    parse_result Parse = ParseSignedIntUnsafe(FormatAt);
                    FormatAt = Parse.OnePastLast;
                    Precision = Parse.SignedIntValue;
                }
                else if (FormatAt[0] == '*')
                {
                    PrecisionSpecified = true;
                    Precision = va_arg(Args, s32);
                    Assert(Precision >= 0);
                    FormatAt++;
                }
            }
            
            // Length
            b32 LengthSpecified = false;
            s32 Length = 4;
            
            if (FormatAt[0] == 'h' && FormatAt[1] == 'h')
            {
                LengthSpecified = true;
                LengthSpecified = 1;
                FormatAt += 2;
            }
            else if (FormatAt[0] == 'h')
            {
                LengthSpecified = true;
                LengthSpecified = 2;
                FormatAt++;
            }
            else if (FormatAt[0] == 'l' && FormatAt[1] == 'l')
            {
                LengthSpecified = true;
                LengthSpecified = 8;
                FormatAt += 2;
            }
            else if (FormatAt[0] == 'l')
            {
                LengthSpecified = true;
                LengthSpecified = 4;
                FormatAt++;
            }
            else if (FormatAt[0] == 'j')
            {
                LengthSpecified = true;
                LengthSpecified = 8;
                FormatAt++;
            }
            else if (FormatAt[0] == 'z')
            {
                FormatAt++;
            }
            else if (FormatAt[0] == 't')
            {
                FormatAt++;
            }
            else if (FormatAt[0] == 'L')
            {
                FormatAt++;
            }
            
            // Format Specifier
            s32 DestLengthRemaining = DestMax - (DestAt - Dest);
            
            char Temp[64];
            string TempDest = MakeString(Temp, 0, 64);
            
            if (FormatAt[0] == 'd' || FormatAt[0] == 'i')
            {
                s64 SignedInt = ReadVarArgsSignedInteger(Length, &Args);
                if (SignedInt < 0)
                {
                    OutChar(&TempDest, '-');
                }
                U64ToASCII(&TempDest, (u64)SignedInt, 10, DecimalDigits);
            }
            else if (FormatAt[0] == 'u')
            {
                u32 UnsignedInt = ReadVarArgsUnsignedInteger(Length, &Args);
                U64ToASCII(&TempDest, UnsignedInt, 10, DecimalDigits);
            } 
            else if (FormatAt[0] == 'o')
            {
                u32 UnsignedInt = ReadVarArgsUnsignedInteger(Length, &Args);
                U64ToASCII(&TempDest, UnsignedInt, 8, OctalDigits);
            } 
            else if (FormatAt[0] == 'x' || FormatAt[0] == 'X')
            {
                u32 UnsignedInt = ReadVarArgsUnsignedInteger(Length, &Args);
                U64ToASCII(&TempDest, UnsignedInt, 16, HexDigits);
            } 
            else if (FormatAt[0] == 'f' || FormatAt[0] == 'F')
            {
                r64 Float = ReadVarArgsFloat(Length, &Args);
                s32 AfterPoint = 6;
                if (PrecisionSpecified)
                {
                    AfterPoint = Precision;
                }
                F64ToASCII(&TempDest, Float, AfterPoint);
            } 
            else if (FormatAt[0] == 'c')
            {
                char InsertChar = va_arg(Args, char);
                OutChar(&TempDest, InsertChar);
            } 
            else if (FormatAt[0] == 's')
            {
                char* InsertString = va_arg(Args, char*);
                
                s32 InsertStringLength = CharArrayLength(InsertString);
                if (PrecisionSpecified)
                {
                    InsertStringLength = GSMin(InsertStringLength, Precision);
                }
                InsertStringLength = GSMin(DestLengthRemaining, InsertStringLength);
                
                for (s32 c = 0; c < InsertStringLength; c++)
                {
                    OutChar(&TempDest, *InsertString++);
                }
            } 
            else if (FormatAt[0] == 'p')
            {
                // TODO(Peter): Pointer Address
            }
            else
            {
                // NOTE(Peter): Non-specifier character found
                InvalidCodePath;
            }
            
            for (s32 i = 0; i < TempDest.Length; i++)
            {
                *DestAt++ = TempDest.Memory[i];
            }
            
            *FormatAt++;
        }
    }
    
    s32 FormattedLength = DestAt - Dest;
    return FormattedLength;
}

internal void
PrintF (string* String, char* Format, ...)
{
    va_list Args;
    va_start(Args, Format);
    String->Length = PrintFArgsList(String->Memory, String->Max, Format, Args);
    va_end(Args);
}


////////////////////////////////////////////////////////////////
//        String Memory Function Definitions
////////////////////////////////////////////////////////////////

static s32
CalculateSlotCountFromSize (s32 RequestedSize, s32 SlotSize)
{
    s32 SlotCount = RequestedSize / SlotSize;
    if (SlotCount * SlotSize < RequestedSize)
    {
        SlotCount += 1;
    }
    return SlotCount;
}

static bool
SlotsAreContiguous (slot_header* First, slot_header* Second)
{
    bool Result = false;
    u8* FirstSlotNextAddress = (u8*)First + First->Size;
    u8* SecondAddress = (u8*)Second;
    Result = FirstSlotNextAddress == SecondAddress;
    return Result;
}

static contiguous_slot_count_result
CountContiguousSlots (slot_header* First)
{
    Assert(First != 0);
    
    contiguous_slot_count_result Result = {};
    Result.Count = 1;
    
    slot_header* IterPrev = First;
    slot_header* Iter = First->Next;
    while (Iter && SlotsAreContiguous(IterPrev, Iter))
    {
        Result.Count++;
        IterPrev = Iter;
        Iter = Iter->Next;
    }
    
    Result.LastContiguousSlot = IterPrev;
    return Result;
}

static slot_header*
GetSlotAtOffset(slot_header* First, s32 Offset)
{
    slot_header* Iter = First;
    s32 Count = 0;
    while (Count < Offset && Iter)
    {
        Iter = Iter->Next;
        Count++;
    }
    return Iter;
}

static slot_header*
InsertSlotIntoList (slot_header* NewSlot, slot_header* ListStart)
{
    slot_header* List = ListStart;
    if (NewSlot < List)
    {
        NewSlot->Next = List;
        List = NewSlot;
    }
    else
    {
        slot_header* PrevIter = List;
        slot_header* Iter = List->Next;
        while (Iter && NewSlot > Iter)
        {
            PrevIter = Iter;
            Iter = Iter->Next;
        }
        
        Assert(PrevIter);
        if (PrevIter) 
        { 
            PrevIter->Next = NewSlot;
        }
        
        if (Iter)
        {
            NewSlot->Next = Iter;
        }
    }
    return List;
}

static void
AllocStringFromStringArena (string* String, s32 Size, slot_arena* Storage)
{
    s32 SlotCount = CalculateSlotCountFromSize(Size, Storage->SlotSize);
    slot_header* Slot = Storage->FreeList;
    slot_header* PrevSlot = 0;
    while (Slot)
    {
        contiguous_slot_count_result ContiguousSlots = CountContiguousSlots(Slot);
        if (ContiguousSlots.Count >= SlotCount)
        {
            slot_header* NextStartSlot = GetSlotAtOffset(Slot, SlotCount);
            if (PrevSlot)
            {
                PrevSlot->Next = NextStartSlot;
            }
            else
            {
                Storage->FreeList = NextStartSlot;
            }
            break;
        }
        else
        {
            PrevSlot = Slot;
            Slot = Slot->Next; 
        }
    }
    
    if (Slot)
    {
        String->Memory = (char*)Slot;
        GSZeroMemory((u8*)String->Memory, SlotCount * Storage->SlotSize);
        String->Max = SlotCount * Storage->SlotSize;
        String->Length = 0;
    }
}

static string
AllocStringFromStringArena (s32 Size, slot_arena* Storage)
{
    string Result = {0};
    AllocStringFromStringArena(&Result, Size, Storage);
    return Result;
}

static void
FreeToStringArena (string* String, slot_arena* Storage)
{
    u8* Base = (u8*)(String->Memory);
    u8* End = Base + String->Max - 1;
    u8* MemoryEnd = Storage->Memory + (Storage->SlotSize * Storage->SlotCount);
    Assert((Base >= Storage->Memory) && (End < MemoryEnd));
    Assert((String->Max % Storage->SlotSize) == 0);
    
    s32 SizeReclaimed = 0;
    slot_header* Slot = (slot_header*)Base;
    while (SizeReclaimed < String->Max)
    {
        Slot->Size = Storage->SlotSize;
        Storage->FreeList = InsertSlotIntoList(Slot, Storage->FreeList);
        SizeReclaimed += Storage->SlotSize;
        Slot = (slot_header*)(Base + SizeReclaimed);
    }
    
    String->Memory = 0;
    String->Length = 0;
    String->Max = 0;
}

static void
ReallocFromStringArena (string* String, s32 NewSize, slot_arena* Storage)
{
    string NewString = AllocStringFromStringArena(NewSize, Storage);
    CopyStringTo(*String, &NewString);
    FreeToStringArena(String, Storage);
    *String = NewString;
}

#if defined(DEBUG)

void DEBUGPrintChars (string* String, s32 Count)
{
    char* Iter = String->Memory;
    for (int i = 0; i < Count; i++)
    {
        *Iter++ = (char)('A' + i);
    }
    String->Length = Count;
}

#ifdef DEBUG_GS_STRING

#include <stdlib.h>

static void
TestStrings()
{
    
    slot_arena StringArena = {};
    
    s32 TestCount = 0;
    s32 SuccessCount = 0;
    
    DebugPrint("\n\n-------------------------------------------------\n  Begin Testing Strings\n\n\n");
    
    ////////////////////////////////////////////////////////////////
    //    Char Functions
    
    char ForwardArray[] = "Hello, Sailor";
    char BackwardArray[] = "roliaS ,olleH";
    TestClean(CharArraysEqual(ForwardArray, 13, ForwardArray, 13), "String Equality");
    TestClean(!CharArraysEqual(ForwardArray, 13, BackwardArray, 13), "String Equality");
    
    TestClean(IndexOfChar(ForwardArray, 0, ',') == 5, "Index Of Char");
    TestClean(IndexOfChar(ForwardArray, 5, 'l') == 10, "Index of Char (skipping first 5)");
    TestClean(FastReverseIndexOfChar(ForwardArray, 13, 0, 'o') == 11, "Fast Reverse Index Of Char");
    TestClean(ReverseIndexOfChar(ForwardArray, 0, 'o') == 11, "Reverse Index of Char");
    TestClean(ReverseIndexOfChar(ForwardArray, 3, 'o') == 4, "Reverse Index of Char (skipping last 3)");
    TestClean(LastIndexOfChar(ForwardArray, 'o') == 11, "Last Index of Char");
    
    ReverseCharArray(ForwardArray, 13);
    TestClean(CharArraysEqual(ForwardArray, 13, BackwardArray, 13), "Reversing Char Array");
    
    char UIntString[] = "1234";
    u32 UIntValue = 1234;
    u32 ParsedUInt = ParseUnsignedInt(UIntString, 4);
    TestClean((ParsedUInt == UIntValue), "String To U32");
    char StringifiedUInt[4] = {};
    UIntToString(UIntValue, StringifiedUInt, 4);
    TestClean(CharArraysEqual(UIntString, 4, StringifiedUInt, 4), "U32 To String");
    
    char IntString[] = "-1234";
    s32 IntValue = -1234;
    s32 ParsedInt = ParseSignedInt(IntString, 5);
    TestClean((ParsedInt == IntValue), "String To S32");
    char StringifiedInt[5] = {};
    IntToString(IntValue, StringifiedInt, 5);
    TestClean(CharArraysEqual(IntString, 5, StringifiedInt, 5), "S32 to String");
    
    char FloatString[] = "-1234.125";
    float FloatValue = -1234.125f;
    float ParsedFloat = ParseFloat(FloatString, 8);
    TestClean((ParsedFloat == FloatValue), "String To Float");
    char StringifiedFloat[10] = {};
    FloatToString(FloatValue, StringifiedFloat, 10, 3);
    TestClean(CharArraysEqual(FloatString, 8, StringifiedFloat, 8), "Float To String");
    
    
    ////////////////////////////////////////////////////////////////
    //
    
    StringArena.SlotSize = 256;
    StringArena.SlotCount = 32;
    StringArena.Memory = malloc(StringArena.SlotSize * StringArena.SlotCount);
    slot_header* PrevSlotHeader = 0;
    for (int i = StringArena.SlotCount - 1; i >= 0; i--)
    {
        u8* SlotBase = StringArena.Memory + (i * StringArena.SlotSize);
        slot_header* SlotHeader = (slot_header*)SlotBase;
        SlotHeader->Size = StringArena.SlotSize;
        SlotHeader->Next = PrevSlotHeader;
        
        // TEST(peter): Should be true always, except on the first iteration, when there is no next slot
        bool Contiguity = SlotsAreContiguous(SlotHeader, PrevSlotHeader);
        TestClean((Contiguity || SlotHeader->Next == 0), "Contiguous Arenas");
        
        PrevSlotHeader = SlotHeader;
    }
    StringArena.FreeList = PrevSlotHeader;
    
    // TEST(peter): Count Should equal StringArena.SlotCount
    s32 ContiguousSlotsCountBefore = CountContiguousSlots(StringArena.FreeList).Count;
    TestClean((ContiguousSlotsCountBefore == StringArena.SlotCount), "Contiguous Arenas");
    
    // TEST(peter): Should be false
    bool Contiguity = SlotsAreContiguous(StringArena.FreeList, StringArena.FreeList->Next->Next);
    Contiguity = SlotsAreContiguous(StringArena.FreeList->Next->Next, StringArena.FreeList);
    TestClean(!Contiguity, "Non Contiguous Arenas");
    
    s32 Slots = CalculateSlotCountFromSize(10, 256);
    TestClean(Slots == 1, "Slot Sizing");
    Slots = CalculateSlotCountFromSize(256, 256);
    TestClean(Slots == 1, "Slot Sizing");
    Slots = CalculateSlotCountFromSize(345, 256);
    TestClean(Slots == 2, "Slot Sizing");
    Slots = CalculateSlotCountFromSize(1024, 256);
    TestClean(Slots == 4, "Slot Sizing");
    
    slot_header* HeaderTen = GetSlotAtOffset(StringArena.FreeList, 10);
    slot_header* HeaderThree = GetSlotAtOffset(StringArena.FreeList, 3);
    slot_header* HeaderFive = GetSlotAtOffset(StringArena.FreeList, 5);
    
    string StringA = AllocStringFromStringArena(10, &StringArena);
    string StringB = AllocStringFromStringArena(345, &StringArena);
    
#if 0
    // TEST(peter): Should TestClean
    u8* RandomMemory = (u8*)malloc(256);
    string RandomMemString = {};
    RandomMemString.Memory = (char*)RandomMemory;
    RandomMemString.Max = 256;
    FreeToStringArena(&RandomMemString, &StringArena); 
#endif
    FreeToStringArena(&StringA, &StringArena);
    FreeToStringArena(&StringB, &StringArena);
    // TEST(peter): After freeing both allocations, ContiguousSlotCountBefore and ContiguousSlotCountAfter should be equal
    s32 ContiguousSlotCountAfter = CountContiguousSlots(StringArena.FreeList).Count;
    TestClean(ContiguousSlotCountAfter == ContiguousSlotsCountBefore, "Add and REmove Slots from Arena");
    
    // TEST(peter): Set up a free list where the first element is too small, so it has to traverse to find an appropriately
    // sized block
    // The slots will look list [256][used][256][256][256] etc..
    StringA = AllocStringFromStringArena(256, &StringArena);
    StringB = AllocStringFromStringArena(256, &StringArena);
    FreeToStringArena(&StringA, &StringArena);
    u32 Contiguous = CountContiguousSlots(StringArena.FreeList).Count; // Should = 1;
    string StringC = AllocStringFromStringArena(512, &StringArena);
    slot_header* HeaderC = (slot_header*)(StringC.Memory);
    
    string ReallocTestString = AllocStringFromStringArena(256, &StringArena);
    DEBUGPrintChars(&ReallocTestString, 24);
    ReallocFromStringArena(&ReallocTestString, 512, &StringArena);
    
    
    string TestString = AllocStringFromStringArena(10, &StringArena);
    DEBUGPrintChars(&TestString, TestString.Max);
    ReallocFromStringArena(&TestString, 20, &StringArena);
    DEBUGPrintChars(&TestString, TestString.Max);
    ReallocFromStringArena(&TestString, 10, &StringArena);
    FreeToStringArena(&TestString, &StringArena);
    
    string EqualityStringA = AllocStringFromStringArena(345, &StringArena);
    string EqualityStringB = AllocStringFromStringArena(415, &StringArena);
    // Equality should succeed despite length differences
    string EqualityStringC = AllocStringFromStringArena(256, &StringArena); 
    string EqualityStringD = AllocStringFromStringArena(256, &StringArena); // Equality should fail
    string EqualityStringEmpty = {};
    
    DEBUGPrintChars(&EqualityStringA, 24);
    DEBUGPrintChars(&EqualityStringB, 24);
    DEBUGPrintChars(&EqualityStringC, 24);
    DEBUGPrintChars(&EqualityStringD, 12);
    
    bool ABEquality = StringsEqual(EqualityStringA, EqualityStringB); // Should Succeed
    bool ACEquality = StringsEqual(EqualityStringA, EqualityStringC); // Should Succeed
    bool ADEquality = StringsEqual(EqualityStringA, EqualityStringD); // Should Fail
    bool AEEquality = StringsEqual(EqualityStringA, EqualityStringEmpty); // Should Fail
    
    TestClean(ABEquality, "String Equality");
    TestClean(ACEquality, "String Equality");
    TestClean(!ADEquality, "String Equality");
    TestClean(!AEEquality, "String Equality");
    
    string CatStringA = AllocStringFromStringArena(256, &StringArena);
    SetStringToCharArray(&CatStringA, "Hello ");
    string CatStringB = AllocStringFromStringArena(512, &StringArena);
    SetStringToCharArray(&CatStringB, "Sailor!");
    string CatStringResult = AllocStringFromStringArena(512, &StringArena);
    SetStringToCharArray(&CatStringResult, "Hello Sailor!");
    ConcatString(&CatStringA, CatStringB);
    TestClean(StringsEqual(CatStringA, CatStringResult), "Cat Strings");
    
    s32 FirstSpaceIndex = FindFirstChar(CatStringA, ' ');
    TestClean(FirstSpaceIndex == 5, "First Index");
    
    SetStringToChar(&CatStringB, 'B', 5);
    TestClean(StringEqualsCharArray(CatStringB, "BBBBB"), "SetStringToChar");
    
    
    DebugPrint("Results: Passed %d / %d\n\n\n", SuccessCount, TestCount);
}
#endif // DEBUG_GS_STRING

#endif // DEBUG
