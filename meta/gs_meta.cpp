#include "..\src\gs_language.h"
#include "..\src\gs_string.h"

#include "gs_meta_error.h"
#include "gs_meta_lexer.h"

#ifndef MAX_PATH
#define MAX_PATH 512
#endif // MAX_PATH

#define META_FILES_ARRAY_INCREMENT_SIZE 32

struct file_contents
{
    u8* Backbuffer;
    string Buffer;
};

#define TOKEN_BUFFER_SIZE 256

struct token_buffer
{
    token* Tokens;
    s32 Used;
    s32 Max;
    token_buffer* Next;
};

struct token_list
{
    token_buffer* Head;
    s32 TotalUsed;
    s32 TotalMax;
};

struct token_list_iterator
{
    token_list* List;
    token_buffer* BufferAt;
    token* At;
    s32 AtTotalIndex;
};

enum basic_type
{
    BasicType_Int8,
    BasicType_Int16,
    BasicType_Int32,
    BasicType_Int64,
    BasicType_UnsignedInt8,
    BasicType_UnsignedInt16,
    BasicType_UnsignedInt32,
    BasicType_UnsignedInt64,
    BasicType_Char,
    BasicType_Float32,
    BasicType_Float64,
    BasicType_Void,
};

struct variable_definition_symbol
{
    string Identifier;
    basic_type Type;
    b32 IsArray;
    s32 Size;
};

struct struct_definition_symbol
{
    string Identifier;
    s32 MemberCount;
    variable_definition_symbol* Members;
};

struct function_definition_symbol
{
    string Identifier;
    
    s32 ArgumentCount;
    variable_definition_symbol* Arguments;
    
    // TODO(Peter): An AST of the actual code that makes up the function?
};

enum symbol_type
{
    SymbolType_Function,
    SymbolType_Struct
};

struct symbol
{
    symbol_type Type;
    union
    {
        struct_definition_symbol StructDef;
        function_definition_symbol FunctionDef;
    };
};

enum parser_symbol_type
{
    ParserSymbol_Invalid,
    ParserSymbol_Unknown,
    
    ParserSymbol_Auto, 
    ParserSymbol_Break,
    ParserSymbol_Case, 
    ParserSymbol_Char, 
    ParserSymbol_Const,
    ParserSymbol_Continue, 
    ParserSymbol_Default, 
    ParserSymbol_Do, 
    ParserSymbol_Double,
    ParserSymbol_Else, 
    ParserSymbol_Enum, 
    ParserSymbol_Extern,
    ParserSymbol_Float, 
    ParserSymbol_For, 
    ParserSymbol_Goto, 
    ParserSymbol_If, 
    ParserSymbol_Inline, 
    ParserSymbol_Int, 
    ParserSymbol_Long,
    ParserSymbol_Register,
    ParserSymbol_Restrict,
    ParserSymbol_Return, 
    ParserSymbol_Short, 
    ParserSymbol_Signed,
    ParserSymbol_Sizeof,
    ParserSymbol_Static,
    ParserSymbol_Struct,
    ParserSymbol_Switch,
    ParserSymbol_Typedef, 
    ParserSymbol_Union, 
    ParserSymbol_Unsigned,
    ParserSymbol_Void, 
    ParserSymbol_Volatile,
    ParserSymbol_While, 
    
    // Assignment Operators
    ParserSymbol_AssignEquals,
    ParserSymbol_PlusEquals, 
    ParserSymbol_MinusEquals,
    ParserSymbol_TimesEquals,
    ParserSymbol_DividedEquals,
    ParserSymbol_ModEquals, 
    ParserSymbol_BitAndEquals,
    ParserSymbol_BitOrEquals,
    ParserSymbol_BitNotEquals,
    ParserSymbol_BitShiftUpEquals,
    ParserSymbol_BitShiftDownEquals,
    
    // Increment/Decrement Operators
    ParserSymbol_Increment, 
    ParserSymbol_Decrement, 
};

struct parser_symbol_definition
{
    parser_symbol_type Type;
    string Text;
};

global_variable parser_symbol_definition ParserSymbolTable[] = 
{
    // Keywords
    { ParserSymbol_Auto, MakeStringLiteral("auto")},
    { ParserSymbol_Break, MakeStringLiteral("break")},
    { ParserSymbol_Case, MakeStringLiteral("case")},
    { ParserSymbol_Char, MakeStringLiteral("char")},
    { ParserSymbol_Const, MakeStringLiteral("const")},
    { ParserSymbol_Continue, MakeStringLiteral("continue")},
    { ParserSymbol_Default, MakeStringLiteral("default")},
    { ParserSymbol_Do, MakeStringLiteral("do")},
    { ParserSymbol_Double, MakeStringLiteral("double")},
    { ParserSymbol_Else, MakeStringLiteral("else")},
    { ParserSymbol_Enum, MakeStringLiteral("enum")},
    { ParserSymbol_Extern, MakeStringLiteral("extern")},
    { ParserSymbol_Float, MakeStringLiteral("float")},
    { ParserSymbol_For, MakeStringLiteral("for")},
    { ParserSymbol_Goto, MakeStringLiteral("goto")},
    { ParserSymbol_If, MakeStringLiteral("if")},
    { ParserSymbol_Inline, MakeStringLiteral("inline")},
    { ParserSymbol_Int, MakeStringLiteral("int")},
    { ParserSymbol_Long, MakeStringLiteral("long")},
    { ParserSymbol_Register, MakeStringLiteral("register")},
    { ParserSymbol_Restrict, MakeStringLiteral("restrict")},
    { ParserSymbol_Return, MakeStringLiteral("return")},
    { ParserSymbol_Short, MakeStringLiteral("short")},
    { ParserSymbol_Signed, MakeStringLiteral("signed")},
    { ParserSymbol_Sizeof, MakeStringLiteral("sizeof")},
    { ParserSymbol_Static, MakeStringLiteral("static")},
    { ParserSymbol_Struct, MakeStringLiteral("struct")},
    { ParserSymbol_Switch, MakeStringLiteral("switch")},
    { ParserSymbol_Typedef, MakeStringLiteral("typedef")},
    { ParserSymbol_Union, MakeStringLiteral("union")},
    { ParserSymbol_Unsigned, MakeStringLiteral("unsigned")},
    { ParserSymbol_Void, MakeStringLiteral("void")},
    { ParserSymbol_Volatile, MakeStringLiteral("volatile")},
    { ParserSymbol_While, MakeStringLiteral("while")},
    
    // Assignment Operators
    { ParserSymbol_AssignEquals, MakeStringLiteral("=")},
    { ParserSymbol_PlusEquals, MakeStringLiteral("+=")},
    { ParserSymbol_MinusEquals, MakeStringLiteral("-=")},
    { ParserSymbol_TimesEquals, MakeStringLiteral("*=")},
    { ParserSymbol_DividedEquals, MakeStringLiteral("/=")},
    { ParserSymbol_ModEquals, MakeStringLiteral("%=")},
    { ParserSymbol_BitAndEquals, MakeStringLiteral("&=")},
    { ParserSymbol_BitOrEquals, MakeStringLiteral("|=")},
    { ParserSymbol_BitNotEquals, MakeStringLiteral("^=")},
    { ParserSymbol_BitShiftUpEquals, MakeStringLiteral("<<=")},
    { ParserSymbol_BitShiftDownEquals, MakeStringLiteral(">>=")},
    
    // Increment/Decrement Operators
    { ParserSymbol_Increment, MakeStringLiteral("++")},
    { ParserSymbol_Decrement, MakeStringLiteral("--")},
    
    /*
    // Arithmetic Operators
    { ParserSymbol_, MakeStringLiteral("")},
    { ParserSymbol_, MakeStringLiteral("")},
    { ParserSymbol_, MakeStringLiteral("")},
    { ParserSymbol_, MakeStringLiteral("")},
    { ParserSymbol_, MakeStringLiteral("")},
    { ParserSymbol_, MakeStringLiteral("")},
    { ParserSymbol_, MakeStringLiteral("")},
    { ParserSymbol_, MakeStringLiteral("")},
    { ParserSymbol_, MakeStringLiteral("")},
    { ParserSymbol_, MakeStringLiteral("")},
    { ParserSymbol_, MakeStringLiteral("")},
    */
};

struct gs_meta_processor
{
    error_list ErrorList;
    
    u8* FileStringMemory;
    s32 FileStringMemorySize;
    
    string* Files;
    file_contents* Contents;
    token_list* FileTokens;
    s32 FilesUsed;
    s32 FilesMax;
};

internal gs_meta_processor
CreateMetaProcessor ()
{
    gs_meta_processor Result = {};
    return Result;
}

internal void
SetMetaFilesStringMemory (gs_meta_processor* Meta, s32 StartIndex, s32 EndIndex)
{
    char* StringMemoryAt = (char*)(Meta->FileStringMemory + (MAX_PATH * StartIndex));
    for (s32 i = StartIndex; i < EndIndex; i++)
    {
        string* String = Meta->Files + i;
        InitializeString(String, StringMemoryAt, MAX_PATH);
        StringMemoryAt += MAX_PATH;
    }
}

internal b32
IsValidFileType(char* Filename)
{
    string FileString = MakeString(Filename);
    s32 IndexOfDot = LastIndexOfChar(FileString, '.');
    string ExtensionString = Substring(FileString, IndexOfDot);
    
    b32 Result = false;
    if (StringsEqual(ExtensionString, MakeStringLiteral("cpp")) ||
        StringsEqual(ExtensionString, MakeStringLiteral("h")))
    {
        Result = true;
    }
    
    return Result;
}

internal void
AddFile(gs_meta_processor* Meta, char* Filename)
{
    if (!IsValidFileType(Filename))
    {
        LogError(&Meta->ErrorList, "AddFile: %s has an extension that cannot be processed.", Filename);
    }
    
    if(!Meta->Files)
    {
        Meta->FilesMax = META_FILES_ARRAY_INCREMENT_SIZE;
        Meta->FilesUsed = 0;
        Meta->Files = (string*)malloc(sizeof(string) * Meta->FilesMax);
        
        Meta->FileStringMemorySize = Meta->FilesMax * MAX_PATH;
        Meta->FileStringMemory = (u8*)malloc(Meta->FileStringMemorySize);
        
        SetMetaFilesStringMemory(Meta, 0, Meta->FilesMax);
    }
    
    if (Meta->Files && Meta->FilesUsed >= Meta->FilesMax)
    {
        Meta->FilesMax += META_FILES_ARRAY_INCREMENT_SIZE;
        Meta->Files = (string*)realloc(Meta->Files, sizeof(string) * Meta->FilesMax);
        
        Meta->FileStringMemorySize = Meta->FilesMax * MAX_PATH;
        Meta->FileStringMemory = (u8*)realloc(Meta->FileStringMemory, Meta->FileStringMemorySize);
        
        SetMetaFilesStringMemory(Meta, Meta->FilesUsed, Meta->FilesMax);
    }
    
    string* File = Meta->Files + Meta->FilesUsed++;
    File->Length = CharArrayLength(Filename);
    GSMemCopy(Filename, File->Memory, File->Length);
    NullTerminate(File);
}

internal void
LoadFileContents (file_contents* Contents, string* FileName, error_list* Errors)
{
    FILE* ReadFile = fopen(FileName->Memory, "r");
    if (ReadFile)
    {
        fseek(ReadFile, 0, SEEK_END);
        size_t FileSize = ftell(ReadFile);
        fseek(ReadFile, 0, SEEK_SET);
        if (FileSize > 0)
        {
            Contents->Backbuffer = (u8*)malloc(FileSize + 1);
            size_t ReadSize = fread(Contents->Backbuffer, 1, FileSize, ReadFile);
            Contents->Backbuffer[FileSize] = 0;
            
            InitializeString(&Contents->Buffer, (char*)Contents->Backbuffer, (s32)FileSize + 1);
            Contents->Buffer.Length = (s32)FileSize + 1;
        }
        else
        {
            LogError(Errors, "LoadFileContents: File Size Was Zero for %s", FileName->Memory);
        }
        fclose(ReadFile);
    }
    else
    {
        LogError(Errors, "LoadFileContents: Could Not Read File %s", FileName->Memory);
    }
    
}

internal void
LoadAllFiles (gs_meta_processor* Meta)
{
    Meta->Contents = (file_contents*)malloc(sizeof(file_contents) * Meta->FilesUsed);
    
    for (s32 FileIdx = 0; FileIdx < Meta->FilesUsed; FileIdx++)
    {
        file_contents* Contents = Meta->Contents + FileIdx;
        string* FileName = Meta->Files + FileIdx;
        LoadFileContents(Contents, FileName, &Meta->ErrorList);
    }
}

internal token*
PushTokenOnList(token_list* List)
{
    token* Result = 0;
    
    if (!List->Head)
    {
        s32 NewBufferSize = sizeof(token_buffer) + (sizeof(token) * TOKEN_BUFFER_SIZE);
        u8* NewBufferContents = (u8*)malloc(NewBufferSize);
        List->Head = (token_buffer*)NewBufferContents;
        List->Head->Used = 0;
        List->Head->Max = TOKEN_BUFFER_SIZE;
        List->Head->Tokens = (token*)(NewBufferContents + sizeof(token_buffer));
        List->TotalUsed = List->Head->Used;
        List->TotalMax = List->Head->Max;
    }
    else if (List->TotalUsed >= List->TotalMax)
    {
        s32 NewBufferSize = sizeof(token_buffer) + (sizeof(token) * TOKEN_BUFFER_SIZE);
        u8* NewBufferContents = (u8*)malloc(NewBufferSize);
        
        token_buffer* NewBuffer = (token_buffer*)NewBufferContents;
        NewBuffer->Tokens = (token*)(NewBufferContents + sizeof(token_buffer));
        
        NewBuffer->Next = NewBuffer;
        List->Head = NewBuffer;
        List->TotalUsed += List->Head->Used;
        List->TotalMax += List->Head->Max;
    }
    
    Result = List->Head->Tokens + List->Head->Used++;
    List->TotalUsed++;
    
    return Result;
}

internal void
LexFile (file_contents* Contents, token_list* TokensList)
{
    tokenizer FileTokenizer = {};
    FileTokenizer.Memory = Contents->Buffer.Memory;
    FileTokenizer.MemoryLength = Contents->Buffer.Length;
    FileTokenizer.At = FileTokenizer.Memory;
    
    while (AtValidPosition(FileTokenizer))
    {
        token* Token = PushTokenOnList(TokensList);
        *Token = GetNextToken(&FileTokenizer);
    }
}

internal void
LexAllFiles (gs_meta_processor* Meta)
{
    LoadAllFiles(Meta);
    
    Meta->FileTokens = (token_list*)malloc(sizeof(token_list) * Meta->FilesUsed);
    
    for (s32 SourceFile = 0; SourceFile < Meta->FilesUsed; SourceFile++)
    {
        Meta->FileTokens[SourceFile] = {};
        
        token_list* FileTokenList = Meta->FileTokens + SourceFile;
        file_contents* FileContents = Meta->Contents + SourceFile;
        LexFile (FileContents, FileTokenList);
    }
}

internal token_list_iterator
GetTokenIterator(token_list* List)
{
    token_list_iterator Result = {};
    Result.List = List;
    Result.BufferAt = List->Head;
    Result.At = List->Head->Tokens;
    Result.AtTotalIndex = 0;
    return Result;
}

internal b32
IsValid (token_list_iterator Iterator)
{
    b32 Result = false;
    if (Iterator.At && Iterator.AtTotalIndex < Iterator.List->TotalUsed)
    {
        Result = true;
    }
    return Result;
}

internal void
Advance (token_list_iterator* Iterator)
{
    if ((Iterator->At - Iterator->BufferAt->Tokens) >= Iterator->BufferAt->Used)
    {
        Iterator->BufferAt = Iterator->BufferAt->Next;
        if (Iterator->BufferAt)
        {
            Iterator->At = Iterator->BufferAt->Tokens;
            Iterator->AtTotalIndex++;
        }
        else
        {
            Iterator->At = 0;
        }
    }
    else
    {
        Iterator->At++;
        Iterator->AtTotalIndex++;
    }
}

internal parser_symbol_type
GetSymbolType (string Text, parser_symbol_type Fallback)
{
    parser_symbol_type Result = Fallback;
    
    for (s32 i = 0; i < (sizeof(ParserSymbolTable)/sizeof(ParserSymbolTable[0])); i++)
    {
        if (StringsEqual(Text, ParserSymbolTable[i].Text))
        {
            Result = ParserSymbolTable[i].Type;
            break;
        }
    }
    
    return Result;
}

internal void
ParseFile (token_list* TokenList, gs_meta_processor* Meta)
{
    for (token_list_iterator TokenIter = GetTokenIterator(TokenList);
         IsValid(TokenIter);
         Advance(&TokenIter))
    {
        token* At = TokenIter.At;
        parser_symbol_type SymbolType = GetSymbolType(At->Text, ParserSymbol_Unknown);
        
        switch (SymbolType)
        {
            case SymbolType_Struct:
            {
                printf("Found Struct: %.*s\n", (At + 1)->Text.Length, (At + 1)->Text.Memory);
            }break;
        }
    }
}

internal void
ParseAllFiles (gs_meta_processor* Meta)
{
    for (s32 SourceFile = 0; SourceFile < Meta->FilesUsed; SourceFile++)
    {
        token_list* FileTokenList = Meta->FileTokens + SourceFile;
        ParseFile(FileTokenList, Meta);
    }
}

#if 0
#include <windows.h>
#define GS_PRINTF(v) OutputDebugStringA(v)
#include "gs_string.h"

static char DEBUGCharArray[256];
#define GS_DEBUG_PRINTF(format, ...) sprintf(DEBUGCharArray, format, __VA_ARGS__); OutputDebugStringA(DEBUGCharArray); 

#include "gs_meta_lexer.h"
#include "gs_meta_parser.h"
#include "gs_meta_generator.h"

internal char*
ReadEntireFileAndNullTerminate (char* Filename)
{
    char* Result = 0;
    
    FILE* ReadFile = fopen(Filename, "r");
    if (ReadFile)
    {
        fseek(ReadFile, 0, SEEK_END);
        size_t FileSize = ftell(ReadFile);
        fseek(ReadFile, 0, SEEK_SET);
        
        Result = (char*)malloc(FileSize + 1);
        size_t ReadSize = fread(Result, 1, FileSize, ReadFile);
        Result[FileSize] = 0;
        
        fclose(ReadFile);
    }
    else
    {
        GS_DEBUG_PRINTF("Failed to fopen file");
    }
    
    return Result;
}

int PrecompileFile (char* FileName)
{
    char* SourceFileContents = ReadEntireFileAndNullTerminate(FileName);
    if (!SourceFileContents)
    {
        GS_DEBUG_PRINTF("Failed to get source file contents");
        return 0;
    }
    
    tokenizer Tokenizer = {};
    Tokenizer.At = SourceFileContents;
    
    s32 TokensGrowSize = 256;
    s32 TokensMax = TokensGrowSize;
    s32 TokensCount = 0;
    token* Tokens = (token*)malloc(TokensGrowSize * sizeof(token));
    
    token* Current = Tokens;
    while (Tokenizer.At[0])
    {
        *Current = GetNextToken(&Tokenizer);
        Current->Next = 0;
        
        TokensCount++;
        
        if (TokensCount >= TokensMax)
        {
            token* Addition = (token*)malloc(TokensGrowSize * sizeof(token));
            TokensMax += TokensGrowSize;
            
            token* Old = Current;
            Current = Addition;
            Old->Next = Current;
        }
        else if(Tokenizer.At[0])
        {
            token* Old = Current;
            Current++;
            Old->Next = Current;
        }
    }
    
    
    
    Current = Tokens;
#if 0 // Print Tokens
    if (Current)
    {
        do
        {
            GS_DEBUG_PRINTF("%s %.*s\n", TokenNames[(int)Current->Type], Current->TextLength, Current->Text);
            Current = Current->Next;
        }while(Current);
    }
#endif
    
    ast_node Ast = {};
    Ast.Type = ASTNode_Program;
    ast_node* CurrentNode = Ast.Children;
    
    // Preprocessor Defines
    // TODO(Peter): Actually preprocess the file
    token* CurrentToken = Tokens;
    /*while (CurrentToken)
    {
    CurrentToken = CurrentToken->Next;
    }
    */
    
    // Analyze File
    CurrentToken = Tokens;
    while (CurrentToken)
    {
        /*GS_DEBUG_PRINTF("%s %.*s\n", TokenNames[(int)CurrentToken->Type], 
        CurrentToken->TextLength, CurrentToken->Text);
        */
        
        parse_result ParseResult = {};
        ParseResult.Parsed = false;
        
        if (CurrentToken->Type == Token_Identifier)
        {
            if (StringsEqual(CurrentToken->Text, "struct", 6))
            {
                ParseResult = ParseStructDeclaration(CurrentNode, CurrentToken);
                CurrentToken = ParseResult.NextToken;
            }
            else if (IsFunction(CurrentToken))
            {
                ParseResult = ParseFunctionDeclaration(CurrentNode, CurrentToken);
                CurrentToken = ParseResult.NextToken;
            }
            else
            {
                CurrentToken = CurrentToken->Next;
            }
            
            if (ParseResult.Parsed)
            {
                if (CurrentNode == 0)
                {
                    Ast.Children = ParseResult.Node;
                    CurrentNode = Ast.Children;
                }
                else
                {
                    CurrentNode->Next = ParseResult.Node;
                    CurrentNode = CurrentNode->Next;
                }
            }
        }
        else
        {
            CurrentToken = CurrentToken->Next;
        }
    }
    
    string_partition StringPart = {};
    {
        memory_arena StringMemory = {};
        InitMemoryArena(&StringMemory, (u8*)malloc(Megabytes(4)), Megabytes(4));
        InitStringPartition(&StringPart, &StringMemory);
    }
    
    // Print Structs
    ast_node* Node = Ast.Children;
    while (Node)
    {
        switch (Node->Type)
        {
            case ASTNode_StructDeclaration:
            {
                string_buffer* CodeBuffer = GenerateStructCode(Node, &StringPart);
                PrintStringBuffer(CodeBuffer);
            }break;
            
            case ASTNode_FunctionDeclaration:
            {
                string_buffer* CodeBuffer = GenerateFunctionDeclaration(Node, &StringPart);
                PrintStringBuffer(CodeBuffer);
            }break;
        }
        
        Node = Node->Next;
    }
    
    return 0;
}

int main(int ArgCount, char** ArgV)
{
    if (ArgCount <= 1)
    {
        printf("Please supply at least one source directory to analyze.\n");
        return 0;
    }
    
    TCHAR Win32FileNameBuffer[MAX_PATH];
    char FullFilePath[MAX_PATH];
    
    for (int i = 1; i < ArgCount; i++)
    {
        WIN32_FIND_DATA FindFileData;
        
        StringCchCopy(Win32FileNameBuffer, MAX_PATH, ArgV[i]);
        StringCchCat(Win32FileNameBuffer, MAX_PATH, TEXT("\\*"));
        
        HANDLE CurrentFile = FindFirstFile(Win32FileNameBuffer, &FindFileData);
        
        int FileIdx = 0;
        do {
            if (FindFileData.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY) { 
                // TODO(Peter): Recurse?
                continue; 
            }
            
            printf("File %d: %s\n", FileIdx++, FindFileData.cFileName);
            
            
        } while (FindNextFile(CurrentFile, &FindFileData));
    }
}
#endif