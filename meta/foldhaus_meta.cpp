#include "..\src\gs_language.h"
#include "..\src\foldhaus_memory.h"
#include "..\src\gs_string.h"

#include "gs_meta_error.h"
#include "gs_meta_lexer.h"

#include <windows.h>
#include <stdio.h>

error_list GlobalErrorList = {};

PLATFORM_ALLOC(StdAlloc)
{
    platform_memory_result Result = {};
    Result.Base = (u8*)malloc(Size);
    Result.Size = Size;
    Result.Error = 0;
    return Result;
}

struct source_code_file
{
    string Path;
    s32 FileSize;
    string Contents;
};

struct code_block_builder
{
    memory_arena Data;
    string String;
};

#define TYPE_TABLE_IDENTIFIER_MAX_LENGTH 128
#define TYPE_TABLE_BUFFER_MAX 64
struct type_table_buffer
{
    u8* IdentifiersBackbuffer;
    string* Identifiers;
    s32* Sizes;
    s32 Used;
    s32 Max;
    type_table_buffer* Next;
};

struct type_table
{
    type_table_buffer* Head;
    s32 TotalUsed;
    s32 TotalMax;
};

internal void
AddTypeToTable (string Identifier, s32 Size, type_table* Table)
{
    if (!Table->Head || Table->TotalUsed >= Table->TotalMax)
    {
        s32 NewHeadSize = sizeof(type_table_buffer) + ((sizeof(string) + TYPE_TABLE_IDENTIFIER_MAX_LENGTH + sizeof(s32)) * TYPE_TABLE_BUFFER_MAX);
        u8* NewHeadBuffer = (u8*)malloc(NewHeadSize);
        static_memory_arena Memory = CreateMemoryArena(NewHeadBuffer, NewHeadSize);
        
        type_table_buffer* NewHead = PushStruct(&Memory, type_table_buffer);
        NewHead->IdentifiersBackbuffer = PushArray(&Memory, u8, TYPE_TABLE_IDENTIFIER_MAX_LENGTH * TYPE_TABLE_BUFFER_MAX);
        NewHead->Identifiers = PushArray(&Memory, string, TYPE_TABLE_BUFFER_MAX);
        NewHead->Sizes = PushArray(&Memory, s32, TYPE_TABLE_BUFFER_MAX);
        NewHead->Used = 0;
        NewHead->Max = TYPE_TABLE_BUFFER_MAX;
        NewHead->Next = 0;
        
        // Init Strings
        for (s32 i = 0; i < NewHead->Max; i++)
        {
            string* String = NewHead->Identifiers + i;
            u8* Backbuffer = NewHead->IdentifiersBackbuffer + (TYPE_TABLE_IDENTIFIER_MAX_LENGTH * i);
            InitializeString(String, (char*)Backbuffer, 0, TYPE_TABLE_IDENTIFIER_MAX_LENGTH);
        }
        
        if (Table->Head) { NewHead->Next = Table->Head; }
        Table->Head = NewHead;
        Table->TotalMax += NewHead->Max;
    }
    
    s32 TypeIndex = Table->Head->Used++;
    string* DestIdentifier = Table->Head->Identifiers + TypeIndex;
    
    CopyStringTo(Identifier, DestIdentifier);
    Table->Head->Sizes[TypeIndex] = Size;
}

internal s32
GetSizeOfType (string Identifier, type_table* TypeTable)
{
    s32 Result = -1;
    
    type_table_buffer* Buffer = TypeTable->Head;
    while (Buffer)
    {
        for (s32 i = 0; i < Buffer->Used; i++)
        {
            string StoredIdentifier = Buffer->Identifiers[i];
            if (StringsEqual(StoredIdentifier, Identifier))
            {
                Result = Buffer->Sizes[i];
                break;
            }
        }
        
        if (Result > 0)
        {
            break;
        }
        else
        {
            Buffer = Buffer->Next;
        }
    }
    
    return Result;
}

internal code_block_builder
InitCodeBlockBuilder()
{
    code_block_builder Result = {};
    InitMemoryArena(&Result.Data, 0, 0, StdAlloc);
    return Result;
}

internal void
CodeBlockPrint (code_block_builder* Block, string Source)
{
    char* NewCode = PushArray(&Block->Data, char, Source.Length);
    GSMemCopy(Source.Memory, NewCode, Source.Length);
    if (Block->String.Memory == 0)
    {
        Block->String.Memory = NewCode;
    }
    Block->String.Max += Source.Length;
    Block->String.Length += Source.Length;
}

internal void
WriteMemoryRegionToFile (memory_region* Region, FILE* WriteFile)
{
    if (Region->PreviousRegion)
    {
        WriteMemoryRegionToFile(Region->PreviousRegion, WriteFile);
    }
    fwrite(Region->Base, 1, Region->Used, WriteFile);
}

internal void
WriteCodeBlockToFile(code_block_builder CodeBlock, FILE* WriteFile)
{
    memory_region* Region = CodeBlock.Data.CurrentRegion;
    WriteMemoryRegionToFile (Region, WriteFile);
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
ReadEntireFileAndNullTerminate (char* Filename, char* Memory, s32 MemorySize)
{
    s32 LengthRead = 0;
    
    FILE* ReadFile = fopen(Filename, "r");
    if (ReadFile)
    {
        fseek(ReadFile, 0, SEEK_END);
        size_t FileSize = ftell(ReadFile);
        fseek(ReadFile, 0, SEEK_SET);
        if (FileSize <= MemorySize)
        {
            size_t ReadSize = fread(Memory, 1, FileSize, ReadFile);
            Memory[FileSize] = 0;
            LengthRead = (s32)ReadSize + 1;
        }
        fclose(ReadFile);
    }
    else
    {
        LogError(&GlobalErrorList, "Could Not Read File: %s", Filename);
    }
    
    return LengthRead;
}

internal void 
EatToNextLine (tokenizer* Tokenizer)
{
    while (AtValidPosition(*Tokenizer) && !IsNewline(*Tokenizer->At))
    {
        Tokenizer->At++;
    }
}

struct seen_node_struct
{
    string Name;
    s32 MembersSize;
    s32 MembersCount;
    seen_node_struct* Next;
};

internal seen_node_struct*
FindSeenStructInList (seen_node_struct* SeenStructList, string Name)
{
    seen_node_struct* Result = 0;
    
    seen_node_struct* Iter = SeenStructList;
    while(Iter)
    {
        if (StringsEqual(Name, Iter->Name))
        {
            Result = Iter;
            break;
        }
        Iter = Iter->Next;
    }
    
    return Result;
}

internal void
ParseNodeStruct (token* NodeStruct, code_block_builder* NodeMembersBlock, seen_node_struct* SeenStruct, type_table* TypeTable)
{
    token* OpenParen = NodeStruct->Next;
    token* StructName = OpenParen->Next;
    
    SeenStruct->Name = StructName->Text;
    
    MakeStringBuffer(Buffer, 256);
    
    PrintF(&Buffer, "node_struct_member MemberList_%.*s[] = {\n", 
           StructName->Text.Length, StructName->Text.Memory);
    CodeBlockPrint(NodeMembersBlock, Buffer);
    
    token* Token = StructName->Next;
    while (Token->Type != Token_RightCurlyBracket)
    {
        if (Token->Type != Token_Identifier)
        {
            Token = Token->Next;
        }
        else
        {
            b32 IsInput = false;
            b32 IsOutput = false;
            
            if (StringsEqual(MakeStringLiteral("NODE_IN"), Token->Text) ||
                StringsEqual(MakeStringLiteral("NODE_COLOR_BUFFER_IN"), Token->Text))
            {
                IsInput = true;
            }
            else if (StringsEqual(MakeStringLiteral("NODE_OUT"),  Token->Text) ||
                     StringsEqual(MakeStringLiteral("NODE_COLOR_BUFFER_OUT"), Token->Text))
            {
                IsOutput = true;
            }
            else if (StringsEqual(MakeStringLiteral("NODE_COLOR_BUFFER_INOUT"), Token->Text))
            {
                IsInput = true;
                IsOutput = true;
            }
            else
            {
                SeenStruct->MembersSize += GetSizeOfType(Token->Text, TypeTable);
                Token = GetNextTokenOfType(Token, Token_Semicolon)->Next;
                continue;
            }
            
            token* TypeToken = GetNextTokenOfType(Token, Token_Identifier);
            token* NameToken = GetNextTokenOfType(TypeToken, Token_Identifier);
            
            MakeStringBuffer(TypeBuffer, 64);
            MakeStringBuffer(NameBuffer, 64);
            
            CopyStringTo(TypeToken->Text, &TypeBuffer);
            CopyStringTo(NameToken->Text, &NameBuffer);
            
            if (StringsEqual(MakeStringLiteral("s32"), TypeToken->Text))
            {
                SeenStruct->MembersSize += sizeof(s32);
            }
            else if (StringsEqual(MakeStringLiteral("r32"), TypeToken->Text))
            {
                SeenStruct->MembersSize += sizeof(r32);
            }
            else if (StringsEqual(MakeStringLiteral("v4"), TypeToken->Text))
            {
                SeenStruct->MembersSize += sizeof(r32) * 4;
            }
            else if (StringsEqual(MakeStringLiteral("NODE_COLOR_BUFFER_INOUT"), Token->Text))
            {
                SeenStruct->MembersSize += sizeof(u32*) + sizeof(u32*) + sizeof(s32);
                
                CopyStringTo(MakeStringLiteral("NODE_COLOR_BUFFER"), &TypeBuffer);
                CopyStringTo(MakeStringLiteral("LEDs"), &NameBuffer);
            }
            else if (StringsEqual(MakeStringLiteral("NODE_COLOR_BUFFER_IN"), Token->Text) ||
                     StringsEqual(MakeStringLiteral("NODE_COLOR_BUFFER_OUT"), Token->Text))
            {
                SeenStruct->MembersSize += sizeof(u32*) + sizeof(u32*) + sizeof(s32);
                
                CopyStringTo(MakeStringLiteral("NODE_COLOR_BUFFER"), &TypeBuffer);
                
                CopyStringTo(TypeToken->Text, &NameBuffer);
                ConcatString(MakeStringLiteral("LEDs"), &NameBuffer);
            }
            else
            {
                PrintF(&Buffer, "Invalid Type Specified for %.*s %.*s\n", 
                       TypeToken->Text.Length, TypeToken->Text.Memory,
                       NameToken->Text.Length, NameToken->Text.Memory);
                NullTerminate(&Buffer);
                printf(Buffer.Memory);
                return;
            }
            
            PrintF(&Buffer, "{ MemberType_%.*s, \"%.*s\", (u64)&((%.*s*)0)->%.*s, %s %s %s},\n", 
                   TypeBuffer.Length, TypeBuffer.Memory, 
                   NameBuffer.Length, NameBuffer.Memory, 
                   StructName->Text.Length, StructName->Text.Memory,
                   NameBuffer.Length, NameBuffer.Memory,
                   (IsInput ? "IsInputMember" : ""),
                   (IsInput && IsOutput ? "|" : ""),
                   (IsOutput ? "IsOutputMember" : ""));
            CodeBlockPrint(NodeMembersBlock, Buffer);
            
            SeenStruct->MembersCount++;
            Token = GetNextTokenOfType(Token, Token_Semicolon)->Next;
        }
    }
    
    CodeBlockPrint(NodeMembersBlock, MakeStringLiteral("};\n\n"));
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

internal void
ParseTypedefs (token* Tokens, type_table* TypeTable)
{
    string TypedefIdentifier = MakeStringLiteral("typedef");
    string StructIdentifier = MakeStringLiteral("struct");
    
    token* Token = Tokens;
    while (Token)
    {
        if (StringsEqual(Token->Text, TypedefIdentifier))
        {
            if (!StringsEqual(Token->Next->Text, StructIdentifier))
            {
                s32 Size = GetTypedefSize(Token);
                string Identifier = GetTypedefIdentifier(Token);
                if (Size > 0) // NOTE(Peter): This is just to skip over typedefs of structs and function pointers
                {
                    AddTypeToTable(Identifier, Size, TypeTable);
                }
            }
        }
        Token = Token->Next;
    }
}

internal void
ParseNodeProc (token* NodeProc, 
               code_block_builder* NodeTypeBlock,
               code_block_builder* NodeSpecificationsBlock,
               code_block_builder* CallNodeProcBlock,
               seen_node_struct* SeenStructs,
               b32 IsPatternProc)
{
    token* ProcName = GetNextTokenOfType(NodeProc, Token_Identifier);
    token* ProcArg = GetNextTokenOfType(ProcName, Token_Identifier);
    
    MakeStringBuffer(Buffer, 256);
    
    // Types Enum
    PrintF(&Buffer, "NodeType_%.*s,\n", ProcName->Text.Length, ProcName->Text.Memory);
    CodeBlockPrint(NodeTypeBlock, Buffer);
    
    // Node Specification
    string ArgName = ProcArg->Text;
    seen_node_struct* ArgStruct = FindSeenStructInList(SeenStructs, ArgName);
    
    PrintF(&Buffer, "{ NodeType_%.*s, \"%.*s\", %d, MemberList_%.*s, %d, %d, %.*s},\n",
           ProcName->Text.Length, ProcName->Text.Memory,
           ProcName->Text.Length, ProcName->Text.Memory, 
           ProcName->Text.Length,
           ProcArg->Text.Length, ProcArg->Text.Memory,
           ArgStruct->MembersSize,
           ArgStruct->MembersCount,
           (IsPatternProc ? 4 : 5), (IsPatternProc ? "true" : "false"));
    CodeBlockPrint(NodeSpecificationsBlock, Buffer);
    
    // Call Node Proc
    if (IsPatternProc)
    {
        LogError(&GlobalErrorList, "Node Proc is no longer supported");
    }
    else
    {
        PrintF(&Buffer, "case NodeType_%.*s: { %.*s((%.*s*)Data, DeltaTime); } break; \n",
               ProcName->Text.Length, ProcName->Text.Memory,
               ProcName->Text.Length, ProcName->Text.Memory,
               ProcArg->Text.Length, ProcArg->Text.Memory);
        CodeBlockPrint(CallNodeProcBlock, Buffer);
    }
}

internal s32
PreprocessStructsAndProcs (string StructSearchString, string ProcSearchString, b32 FindingPatterns,
                           token* Tokens, 
                           code_block_builder* NodeMembersBlock,
                           code_block_builder* NodeTypeBlock,
                           code_block_builder* NodeSpecificationsBlock,
                           code_block_builder* CallNodeProcBlock,
                           type_table* TypeTable)
{
    // Node Structs
    seen_node_struct* Structs = 0;
    
    token_selection_spec NodeStructSpec = {};
    NodeStructSpec.MatchText = true;
    NodeStructSpec.Text = StructSearchString;
    
    token* NodeStructToken = FindNextMatchingToken(Tokens, NodeStructSpec);
    while (NodeStructToken)
    {
        seen_node_struct* SeenStruct = (seen_node_struct*)malloc(sizeof(seen_node_struct));
        *SeenStruct = {};
        
        if (Structs != 0)
        {
            SeenStruct->Next = Structs;
        }
        Structs = SeenStruct;
        
        ParseNodeStruct(NodeStructToken, NodeMembersBlock, SeenStruct, TypeTable);
        NodeStructToken = FindNextMatchingToken(NodeStructToken->Next, NodeStructSpec);
    }
    
    // Node Procs
    token_selection_spec NodeProcSpec = {};
    NodeProcSpec.MatchText = true;
    NodeProcSpec.Text = ProcSearchString;
    
    token* NodeProcToken = FindNextMatchingToken(Tokens, NodeProcSpec);
    s32 NodeProcCount = 0;
    while (NodeProcToken)
    {
        NodeProcCount++;
        ParseNodeProc(NodeProcToken, NodeTypeBlock, NodeSpecificationsBlock, CallNodeProcBlock, Structs, FindingPatterns);
        NodeProcToken = FindNextMatchingToken(NodeProcToken->Next, NodeProcSpec);
    }
    return NodeProcCount;
}

// Step 1: Get All Tokens, for every file
// Step 2: Identify all preprocessor directives
// Step 3: Apply Preprocessor Directives && Generate Code
// Step 4: Write out new files
// Step 5: Compile
int main(int ArgCount, char** ArgV)
{
    if (ArgCount <= 1)
    {
        printf("Please supply at least one source directory to analyze.\n");
        return 0;
    }
    
    type_table TypeTable = {};
    
    memory_arena SourceFileArena = {};
    InitMemoryArena(&SourceFileArena, 0, 0, StdAlloc);
    
    code_block_builder NodeTypeBlock = InitCodeBlockBuilder();
    CodeBlockPrint(&NodeTypeBlock, MakeStringLiteral("enum node_type\n{\nNodeType_OutputNode,\n"));
    
    code_block_builder NodeMembersBlock = InitCodeBlockBuilder();
    
    code_block_builder NodeSpecificationsBlock = InitCodeBlockBuilder();
    CodeBlockPrint(&NodeSpecificationsBlock, MakeStringLiteral("node_specification NodeSpecifications[] = {\n"));
    
    code_block_builder CallNodeProcBlock = InitCodeBlockBuilder();
    CodeBlockPrint(&CallNodeProcBlock, MakeStringLiteral("internal void CallNodeProc(node_header* Node, u8* Data, led* LEDs, s32 LEDsCount, r32 DeltaTime)\n{\n"));
    CodeBlockPrint(&CallNodeProcBlock, MakeStringLiteral("switch (Node->Type)\n{\n"));
    
    // Build Search Paths Array
    s32 SearchPathsCount = 1; //ArgCount - 1;
    string* SearchPaths = PushArray(&SourceFileArena, string, SearchPathsCount);
    for (s32 InputPath = 0; InputPath < SearchPathsCount; InputPath++)
    {
        string* SearchPathString = SearchPaths + InputPath;
        InitializeEmptyString(SearchPathString, PushArray(&SourceFileArena, char, MAX_PATH), MAX_PATH);
        
        // NOTE(Peter): Adding one to skip the default argument which is the name of the application currently running
        CopyCharArrayToString(ArgV[InputPath + 1], SearchPathString);
        ConcatCharArrayToString("*", SearchPathString);
        NullTerminate(SearchPathString);
    }
    
    // Total Source Files Count
    s32 SourceFileCount = 0;
    for (s32 SearchPath = 0; SearchPath < SearchPathsCount; SearchPath++)
    {
        string* SearchPathString = SearchPaths + SearchPath;
        
        WIN32_FIND_DATA FindFileData;
        HANDLE CurrentFile = FindFirstFile(SearchPathString->Memory, &FindFileData);
        
        if (CurrentFile == INVALID_HANDLE_VALUE)
        {
            printf("Invalid File Handle\n");
            return 0;
        }
        
        do {
            if (FindFileData.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY) 
            { 
                continue; // TODO(Peter): Recurse?
            }
            SourceFileCount++;
        } while (FindNextFile(CurrentFile, &FindFileData));
    }
    
    // Allocate Source File Array
    source_code_file* SourceFiles = PushArray(&SourceFileArena, source_code_file, SourceFileCount);
    
    // Populate Source File Array
    s32 SourceFilesUsed = 0;
    for (s32 SearchPath = 0; SearchPath < SearchPathsCount; SearchPath++)
    {
        string* SearchPathString = SearchPaths + SearchPath;
        
        WIN32_FIND_DATA FindFileData;
        HANDLE CurrentFile = FindFirstFile(SearchPathString->Memory, &FindFileData);
        
        do {
            if (FindFileData.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY) 
            { 
                continue; // TODO(Peter): Recurse?
            }
            
            string FileName = MakeString(FindFileData.cFileName);
            string FileExtension = Substring(FileName, LastIndexOfChar(FileName, '.'));
            
            if (StringsEqual(FileExtension, MakeStringLiteral("cpp")) ||
                StringsEqual(FileExtension, MakeStringLiteral("h")))
            {
                source_code_file* File = SourceFiles + SourceFilesUsed++;
                
                PushString(&File->Path, &SourceFileArena, SearchPathString->Length + CharArrayLength(FindFileData.cFileName));
                CopyStringTo(Substring(*SearchPathString, 0, SearchPathString->Length - 2), &File->Path);
                ConcatCharArrayToString(FindFileData.cFileName, &File->Path);
                NullTerminate(&File->Path);
                
                File->FileSize = FindFileData.nFileSizeLow;
                ErrorAssert(FindFileData.nFileSizeHigh == 0, &GlobalErrorList, "File Too Big. Peter needs to handle this. File: %.*s", FileName.Length, FileName.Memory); 
                
                PushString(&File->Contents, &SourceFileArena, File->FileSize + 1);
                File->Contents.Length = ReadEntireFileAndNullTerminate(File->Path.Memory, File->Contents.Memory, File->Contents.Max);
            }
            
        } while (FindNextFile(CurrentFile, &FindFileData));
    }
    
    // Tokenize All The Files
    s32 TokensCount = 0;
    token* Tokens = 0;
    token** FileStartTokens = PushArray(&SourceFileArena, token*, SourceFilesUsed);
    for (s32 SourceFileIdx = 0; SourceFileIdx < SourceFilesUsed; SourceFileIdx++)
    {
        source_code_file* File = SourceFiles + SourceFileIdx;
        
        tokenizer Tokenizer = {};
        Tokenizer.At = File->Contents.Memory;
        Tokenizer.Memory = File->Contents.Memory;
        Tokenizer.MemoryLength = File->Contents.Max;
        
        token* LastToken = 0;
        while(AtValidPosition(Tokenizer))
        {
            token* Token = PushStruct(&SourceFileArena, token);
            if (Tokens == 0)
            {
                Tokens = Token;
            }
            
            TokensCount++;
            *Token = GetNextToken(&Tokenizer);
            
            Token->Next = 0;
            if (LastToken) { 
                LastToken->Next = Token; 
            }
            else
            {
                FileStartTokens[SourceFileIdx] = Token;
            }
            LastToken = Token;
        }
    }
    
    for (s32 SourceFile = 0; SourceFile < SourceFilesUsed; SourceFile++)
    {
        ParseTypedefs (FileStartTokens[SourceFile], &TypeTable);
    }
    
    s32 NodeProcCount = 0;
    
    
    for (s32 SourceFileIdx = 0; SourceFileIdx < SourceFilesUsed; SourceFileIdx++)
    {
        token* FileStartToken = FileStartTokens[SourceFileIdx];
        NodeProcCount += PreprocessStructsAndProcs (MakeStringLiteral("NODE_STRUCT"), 
                                                    MakeStringLiteral("NODE_PROC"), 
                                                    false,
                                                    FileStartToken, &NodeMembersBlock, &NodeTypeBlock, &NodeSpecificationsBlock, &CallNodeProcBlock, 
                                                    &TypeTable);
        NodeProcCount += PreprocessStructsAndProcs (MakeStringLiteral("NODE_PATTERN_STRUCT"), 
                                                    MakeStringLiteral("NODE_PATTERN_PROC"), 
                                                    true,
                                                    FileStartToken, &NodeMembersBlock, &NodeTypeBlock, &NodeSpecificationsBlock, &CallNodeProcBlock,
                                                    &TypeTable);
    }
    
    MakeStringBuffer(Buffer, 256);
    
    // Close Types Block - overwrite the last comma and '\' newline character with newlines.
    CodeBlockPrint(&NodeTypeBlock, MakeStringLiteral("NodeType_Count,\n};\n\n"));
    
    // Close Specifications Block
    CodeBlockPrint(&NodeSpecificationsBlock, MakeStringLiteral("};\n"));
    PrintF(&Buffer, "s32 NodeSpecificationsCount = %d;\n\n", NodeProcCount);
    CodeBlockPrint(&NodeSpecificationsBlock, Buffer);
    
    // Close Call Node Proc Block
    CodeBlockPrint(&CallNodeProcBlock, MakeStringLiteral("}\n}\n"));
    
    FILE* NodeGeneratedCPP = fopen("C:\\projects\\foldhaus\\src\\generated\\foldhaus_nodes_generated.cpp", "w");
    if (NodeGeneratedCPP)
    {
        WriteCodeBlockToFile(NodeTypeBlock, NodeGeneratedCPP);
        WriteCodeBlockToFile(NodeMembersBlock, NodeGeneratedCPP);
        WriteCodeBlockToFile(NodeSpecificationsBlock, NodeGeneratedCPP);
        WriteCodeBlockToFile(CallNodeProcBlock, NodeGeneratedCPP);
        fclose(NodeGeneratedCPP);
    }
    
    PrintErrorList(GlobalErrorList);
    return 0;
}