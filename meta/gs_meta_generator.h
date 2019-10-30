#define CODE_SIZE 256
struct generated_code
{
    s32 Used;
    char* Code;
    generated_code* Next;
};

internal void
InitGeneratedCode (generated_code* Code)
{
    Code->Used = 0;
    Code->Code = (char*)malloc(sizeof(char) * CODE_SIZE);
    *(Code->Code + CODE_SIZE - 1) = 0;
    Code->Next = 0;
}

// NOTE(Peter): This ONLY supports printing strings into the buffer at the moment
static void
PrintF_ (
generated_code* Dest,
char* Format, 
va_list Args
)
{
    if (!Dest->Code) { 
        InitGeneratedCode(Dest);
    }
    
    char* Src = Format;
    char* Dst = Dest->Code + Dest->Used;
    
    while (*Src && (Dst - Dest->Code) < CODE_SIZE)
    {
        if (*Src == '\\' && *(Src + 1) && *(Src + 1) == '%')
        {
            *Src++;
            *Dst++ = *Src++;
        }
        else if (*Src == '%')
        {
            Src++;
            if (*Src == 's')
            {
                Src++;
                s32 StringLength = va_arg(Args, s32);
                char* String = va_arg(Args, char*);
                char* C = String;
                while(*C && StringLength > 0 && (Dst - Dest->Code) < CODE_SIZE)
                {
                    StringLength--;
                    *Dst++ = *C++;
                }
            }
            else
            {
                InvalidCodePath;
            }
        }
        else
        {
            *Dst++ = *Src++;
        }
    }
    
    if (!*Dst && *Src)
    {
        Dest->Next = (generated_code*)malloc(sizeof(generated_code));
        InitGeneratedCode(Dest->Next);
        PrintF_(Dest->Next, Src, Args);
    }
    
    if (*Dst && !*Src) { *Dst = 0; }
    
    Dest->Used = (s32)(Dst - Dest->Code);
}

static void
PrintF (
generated_code* Dest,
char* Format, 
...)
{
    va_list Args;
    va_start(Args, Format);
    
    PrintF_(Dest, Format, Args); 
    
    va_end(Args);
}

static void
PrintCode (generated_code* Code)
{
    GS_PRINTF(Code->Code);
    if (Code->Next) { PrintCode(Code->Next); }
}

static void
GenerateFieldCode (ast_field_declaration* Field, string_buffer* CodeBuffer, string_partition* Partition,
                   s32 IndentLevel, char* Terminator)
{
    for (s32 i = 0; i < IndentLevel; i++)
    {
        PrintStringBufferFormat(CodeBuffer, Partition, "    ");
    }
    
    PrintStringBufferFormat(CodeBuffer, Partition, "%s%s %s%s",
                            Field->TypeLength, Field->Type,
                            (Field->TypePointer ? 1 : 0), (Field->TypePointer ? "*" : ""),
                            Field->NameLength, Field->Name,
                            (Field->TypeArray ? 2 : 0), (Field->TypeArray ? "[]" : ""));
    
    PrintStringBufferFormat(CodeBuffer, Partition, Terminator);
}

static string_buffer*
GenerateStructCode (ast_node* Struct, string_partition* StringPartition)
{
    Assert(Struct->Type == ASTNode_StructDeclaration);
    
    string_buffer* StructCodeBuffer = GetStringBuffer(StringPartition);
    
    PrintStringBufferFormat(StructCodeBuffer, StringPartition, "struct %s\n{\n",
                            Struct->StructDeclaration.NameLength, Struct->StructDeclaration.Name);
    
    ast_field_declaration* Member = Struct->StructDeclaration.Members;
    while (Member)
    {
        GenerateFieldCode(Member, StructCodeBuffer, StringPartition, 1, ";\n");
        Member = Member->Next;
    }
    
    PrintStringBufferFormat(StructCodeBuffer, StringPartition, "}\n\n");
    return StructCodeBuffer;
}

static string_buffer*
GenerateFunctionDeclaration (ast_node* Function, string_partition* Partition)
{
    Assert(Function->Type == ASTNode_FunctionDeclaration);
    
    ast_function_declaration* FuncDecl = &Function->FunctionDeclaration;
    
    string_buffer* FunctionDeclBuffer = GetStringBuffer(Partition);
    
    PrintStringBufferFormat(FunctionDeclBuffer, Partition, "%s %s (",
                            FuncDecl->ReturnTypeLength, FuncDecl->ReturnType,
                            FuncDecl->NameLength, FuncDecl->Name);
    
    ast_field_declaration* Param = FuncDecl->Arguments;
    while (Param)
    {
        GenerateFieldCode(Param, FunctionDeclBuffer, Partition, 0, "");
        if (Param->Next)
        {
            PrintStringBufferFormat(FunctionDeclBuffer, Partition, ", ");
        }
        Param = Param->Next;
    }
    
    PrintStringBufferFormat(FunctionDeclBuffer, Partition, ")\n");
    return FunctionDeclBuffer;
}