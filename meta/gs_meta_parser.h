enum ast_node_type
{
    ASTNode_Invalid,
    
    ASTNode_Program,
    ASTNode_StructDeclaration,
    ASTNode_FunctionDeclaration,
    
    ASTNode_Count
};

struct ast_field_declaration
{
    s32 NameLength;
    char* Name;
    
    s32 TypeLength;
    char* Type;
    b32 TypePointer;
    b32 TypeArray;
    
    ast_field_declaration* Next;
};

struct ast_struct_declaration
{
    s32 NameLength;
    char* Name;
    
    s32 MembersCount;
    ast_field_declaration* Members;
};

struct ast_function_declaration
{
    s32 ReturnTypeLength;
    char* ReturnType;
    
    s32 NameLength;
    char* Name;
    
    ast_field_declaration* Arguments;
};

struct ast_node
{
    ast_node_type Type;
    ast_node* Children;
    
    union
    {
        ast_struct_declaration StructDeclaration;
        ast_function_declaration FunctionDeclaration;
    };
    
    ast_node* Next;
};

struct parse_field_decl_result
{
    ast_field_declaration* Member;
    token* NextToken;
};

internal parse_field_decl_result
ParseFieldDeclaration (//ast_field_declaration** Container, ast_field_declaration* PreviousMember, 
                       token* StartToken, token_type TerminatorToken, token_type EnclosingToken)
{
    parse_field_decl_result Result = {};
    
    ast_field_declaration* Member = 0;
    
    token* CurrentToken = StartToken;
    if (StartToken->Type == Token_Identifier)
    {
        Member = (ast_field_declaration*)malloc(sizeof(ast_field_declaration));
        Member->Next = 0;
        
        Member->Type = CurrentToken->Text;
        Member->TypeLength = CurrentToken->TextLength;
        Member->TypePointer = false;
        Member->TypeArray = false;
        
        CurrentToken = CurrentToken->Next;
        if (*CurrentToken->Text == '*')
        {
            Member->TypePointer = true;
            CurrentToken = CurrentToken->Next;
        }
        
        Member->Name = CurrentToken->Text;
        Member->NameLength = CurrentToken->TextLength;
        
        CurrentToken = CurrentToken->Next;
        if (CurrentToken->Type == Token_LeftSquareBracket)
        {
            CurrentToken = CurrentToken->Next;
            if (CurrentToken->Type != Token_RightSquareBracket)
            {
                GS_DEBUG_PRINTF("ALERT: Skipping Array parameter that has a size");
                while (CurrentToken->Type != Token_RightSquareBracket)
                {
                    CurrentToken = CurrentToken->Next;
                }
            }
            CurrentToken = CurrentToken->Next;
            Member->TypeArray = true;
        }
        else if (CurrentToken->Type != TerminatorToken &&
                 CurrentToken->Type != EnclosingToken)
        {
            GS_DEBUG_PRINTF("Error: struct member %s %.*s not followed by its terminator type: %s or %s\n",
                            TokenNames[CurrentToken->Type], CurrentToken->TextLength, CurrentToken->Text,
                            TokenNames[(int)TerminatorToken], TokenNames[(int)EnclosingToken]);
        }
    }
    else if (StartToken->Type == Token_Comment)
    {
        CurrentToken = StartToken->Next;
    }
    
    // NOTE(Peter): this is here because if the current token is the enclosing type,
    // ie. the ')' in a parameter list, it isn't the responsibility of this function
    // to deal with that.
    if (CurrentToken->Type == TerminatorToken)
    {
        CurrentToken = CurrentToken->Next;
    }
    
    Result.Member = Member;
    Result.NextToken = CurrentToken;
    
    return Result;
}

struct parse_result
{
    b32 Parsed;
    ast_node* Node;
    token* NextToken;
};

internal parse_result
ParseStructDeclaration (ast_node* PreviousNode, token* StartToken)
{
    parse_result Result = {};
    
    ast_node* Struct = 0;
    token* CurrentToken = StartToken;
    
    if (PreviousNode == 0)
    {
        Struct = (ast_node*)malloc(sizeof(ast_node));;
    }
    else
    {
        PreviousNode->Next = (ast_node*)malloc(sizeof(ast_node));
        Struct = PreviousNode->Next;
    }
    Struct->Next = 0;
    Struct->StructDeclaration.Members = 0;
    
    Struct->Type = ASTNode_StructDeclaration;
    
    CurrentToken = CurrentToken->Next;
    // Name Before Declaration
    if (CurrentToken->Type == Token_Identifier)
    {
        Struct->StructDeclaration.NameLength = CurrentToken->TextLength;
        Struct->StructDeclaration.Name = CurrentToken->Text;
        CurrentToken = CurrentToken->Next;
    }
    
    if (CurrentToken->Type == Token_LeftCurlyBracket)
    {
        CurrentToken = CurrentToken->Next;
        
        ast_field_declaration* Member = 0;
        while (CurrentToken->Type != Token_RightCurlyBracket)
        {
            parse_field_decl_result MemberResult = ParseFieldDeclaration(CurrentToken, Token_Semicolon, Token_RightCurlyBracket);
            
            if (!Member)
            {
                Member = MemberResult.Member;
                Member->Next = 0;
                Struct->StructDeclaration.Members = Member;
            }
            else if (MemberResult.Member)
            {
                Member->Next = MemberResult.Member;
                Member = Member->Next;
            }
            CurrentToken = MemberResult.NextToken;
        }
        // Advance Past the Right Bracket
        CurrentToken = CurrentToken->Next;
    }
    else
    {
        GS_DEBUG_PRINTF("Error: struct <name> not followed by {");
    }
    
    // Name After Declaration
    if (CurrentToken->Type == Token_Identifier)
    {
        Struct->StructDeclaration.NameLength = CurrentToken->TextLength;
        Struct->StructDeclaration.Name = CurrentToken->Text;
        CurrentToken = CurrentToken->Next;
    }
    
    if (CurrentToken->Type == Token_Semicolon)
    {
        CurrentToken = CurrentToken->Next;
    }
    else
    {
        GS_DEBUG_PRINTF("Error: struct declaration did not finish with a semicolon");
    }
    
    Result.Node = Struct;
    Result.NextToken = CurrentToken;
    Result.Parsed = true;
    
    return Result;
}

internal b32
IsFunction (token* StartToken)
{
    b32 Result = true;
    
    token* Current = StartToken;
    // TODO(Peter): check a type table to see if we can do this now. 
    // TODO(Peter): is there a way to defer this to later?
    if (Current->Type != Token_Identifier) // Return Type
    {
        Result = false;
        return Result;
    }
    
    Current = Current->Next;
    if (Current->Type != Token_Identifier) // Function Name
    {
        Result = false;
        return Result;
    }
    
    Current = Current->Next;
    if (Current->Type != Token_LeftParen)
    {
        Result = false;
        return Result;
    }
    
    Current = Current->Next;
    while (Current && Current->Type != Token_RightParen)
    {
        Current = Current->Next;
    }
    
    if (Current->Type != Token_RightParen)
    {
        Result = false;
        return Result;
    }
    
    return Result;
}

internal parse_result
ParseFunctionDeclaration (ast_node* PreviousNode, token* StartToken)
{
    parse_result Result;
    
    ast_node* Function = 0;
    token* CurrentToken = StartToken;
    
    if (PreviousNode == 0)
    {
        Function = (ast_node*)malloc(sizeof(ast_node));
    }
    else
    {
        PreviousNode->Next = (ast_node*)malloc(sizeof(ast_node));
        Function = PreviousNode->Next;
    }
    Function->Next = 0;
    
    Function->Type = ASTNode_FunctionDeclaration;
    Function->FunctionDeclaration.Arguments = 0;
    
    Function->FunctionDeclaration.ReturnTypeLength = CurrentToken->TextLength;
    Function->FunctionDeclaration.ReturnType = CurrentToken->Text;
    CurrentToken = CurrentToken->Next;
    
    Function->FunctionDeclaration.NameLength = CurrentToken->TextLength;
    Function->FunctionDeclaration.Name = CurrentToken->Text;
    CurrentToken = CurrentToken->Next;
    
    if (CurrentToken->Type == Token_LeftParen)
    {
        CurrentToken = CurrentToken->Next;
        
        ast_field_declaration* Param = 0;
        while (CurrentToken->Type != Token_RightParen)
        {
            parse_field_decl_result ParamResult = ParseFieldDeclaration(CurrentToken, Token_Comma, Token_RightParen);
            if (!Param)
            {
                Param = ParamResult.Member;
                Param->Next = 0;
            }
            else if (ParamResult.Member)
            {
                Param->Next = ParamResult.Member;
                Param = Param->Next;
            }
            CurrentToken = ParamResult.NextToken;
            
        }
        
        CurrentToken = CurrentToken->Next;
    }
    else
    {
        GS_DEBUG_PRINTF("Error: Function declaration is not followed by left parenthesis");
        InvalidCodePath;
    }
    
    
    Result.Node = Function;
    Result.NextToken = CurrentToken;
    Result.Parsed = true;
    
    return Result;
}