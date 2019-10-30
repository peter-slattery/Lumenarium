struct token_selection_spec
{
    b32 MatchText;
    string Text;
};

internal s32
EatPreprocessor (tokenizer* Tokenizer, token_type* Type)
{
    s32 Length = 0;
    
    // TODO(Peter): Make this actually separate out the different arguments?
    while (Tokenizer->At[0] && !IsNewline(Tokenizer->At[0]))
    {
        ++Tokenizer->At;
        Length++;
    }
    
    return Length;
}

internal s32
EatString (tokenizer* Tokenizer)
{
    s32 Length = 0;
    
    while (Tokenizer->At[0] && Tokenizer->At[0] != '"')
    {
        if (Tokenizer->At[0] == '/')
        {
            ++Tokenizer->At;
            Length++;
        }
        ++Tokenizer->At;
        Length++;
    }
    
    ++Tokenizer->At;
    
    return Length;
}

internal s32
EatIdentifier (tokenizer* Tokenizer)
{
    s32 Length = 0;
    
    while (Tokenizer->At[0] && 
           (IsAlpha(Tokenizer->At[0]) || IsNumericExtended(Tokenizer->At[0])))
    {
        ++Tokenizer->At;
        Length++;
    }
    
    return Length;
}

internal token
GetNextToken (tokenizer* Tokenizer)
{
    token Result = {};
    
    EatWhitespace(Tokenizer);
    
    Result.Text = MakeString(Tokenizer->At, 1, 1);
    
    
    char C = Tokenizer->At[0];
    ++Tokenizer->At;
    
    if (C == 0) { Result.Type = Token_EndOfStream; } 
    else if (C == '(') { Result.Type = Token_LeftParen; }
    else if (C == ')') { Result.Type = Token_RightParen; }
    else if (C == '[') { Result.Type = Token_LeftSquareBracket; }
    else if (C == ']') { Result.Type = Token_RightSquareBracket; }
    else if (C == '{') { Result.Type = Token_LeftCurlyBracket; }
    else if (C == '}') { Result.Type = Token_RightCurlyBracket; }
    else if (C == ';') { Result.Type = Token_Semicolon; }
    else if (C == ',') { Result.Type = Token_Comma; }
    else if (C == '.') { Result.Type = Token_Period; }
    else if (C == '-' && Tokenizer->At[0] && Tokenizer->At[0] == '>') 
    { 
        Result.Type = Token_PointerReference; 
        Result.Text.Length = 2;
        ++Tokenizer->At;
    }
    else if (C == '#')
    {
        Result.Text.Length += EatPreprocessor(Tokenizer, &Result.Type);
        
        if (CharArraysEqual(Result.Text.Memory, Result.Text.Length, "#define", 7))
        { Result.Type = Token_PoundDefine; }
        else if (CharArraysEqual(Result.Text.Memory, Result.Text.Length, "#undef", 6))
        { Result.Type = Token_PoundUndef; }
        else if (CharArraysEqual(Result.Text.Memory, Result.Text.Length, "#include", 8))
        { Result.Type = Token_PoundInclude; }
        else if (CharArraysEqual(Result.Text.Memory, Result.Text.Length, "#ifdef", 6))
        { Result.Type = Token_PoundIfDef; }
        else if (CharArraysEqual(Result.Text.Memory, Result.Text.Length, "#ifndef", 7))
        { Result.Type = Token_PoundIfNDef; }
        else if (CharArraysEqual(Result.Text.Memory, Result.Text.Length, "#if", 3))
        { Result.Type = Token_PoundIf; }
        else if (CharArraysEqual(Result.Text.Memory, Result.Text.Length, "#elif", 5))
        { Result.Type = Token_PoundElif; }
        else if (CharArraysEqual(Result.Text.Memory, Result.Text.Length, "#else", 5))
        { Result.Type = Token_PoundElse; }
        else if (CharArraysEqual(Result.Text.Memory, Result.Text.Length, "#endif", 6))
        { Result.Type = Token_PoundEndif; }
        else if (CharArraysEqual(Result.Text.Memory, Result.Text.Length, "#error", 6))
        { Result.Type = Token_PoundError; }
        else if (CharArraysEqual(Result.Text.Memory, Result.Text.Length, "#pragma", 7))
        { Result.Type = Token_PoundPragma; }
    }
    else if (IsNumeric(C))
    {
        Result.Type = Token_Number;
        char* Start = Tokenizer->At;
        EatNumber(Tokenizer);
        Result.Text.Length = Tokenizer->At - Start; 
    }
    else if (C == '\'')
    {
        Result.Type = Token_Char;
        Result.Text.Memory = Tokenizer->At;
        if (Tokenizer->At[0] && Tokenizer->At[0] == '\\')
        {
            ++Tokenizer->At;
        }
        ++Tokenizer->At;
        ++Tokenizer->At;
    }
    else if (C == '"')
    {
        Result.Type = Token_String;
        // replace the length added by the quote
        Result.Text.Memory = Tokenizer->At;
        Result.Text.Length = EatString(Tokenizer);
    }
    else if (C == '/' && Tokenizer->At[0] &&  Tokenizer->At[0] == '/')
    {
        Result.Type = Token_Comment;
        char* Start = Tokenizer->At;
        EatToNewLine(Tokenizer);
        Result.Text.Length += 1 + (Tokenizer->At - Start);
    }
    else if (C == '/' && Tokenizer->At[0] && Tokenizer->At[0] == '*')
    {
        s32 CommentLength = 1;
        while (Tokenizer->At[0] && Tokenizer->At[0] != '*' &&
               Tokenizer->At[1] && Tokenizer->At[1] != '/')
        {
            ++Tokenizer->At;
            CommentLength++;
        }
        
        Result.Text.Length += CommentLength;
    }
    // NOTE(Peter): This is after comment parsing so that the division operator
    // falls through the comment case
    else if (IsOperator(C)) { Result.Type = Token_Operator; }
    else
    {
        Result.Type = Token_Identifier;
        Result.Text.Length += EatIdentifier(Tokenizer);
    }
    
    return Result;
}

internal token*
FindNextMatchingToken (token* Tokens, token_selection_spec Spec)
{
    token* Result = 0;
    
    token* Token = Tokens;
    while (Token)
    {
        if (Token->Text.Memory)
        {
            b32 Matches = false;
            if (Spec.MatchText && StringsEqual(Spec.Text, Token->Text))
            {
                Matches = true;
            }
            
            if (Matches)
            {
                Result = Token;
                break;
            }
        }
        
        Token = Token->Next;
    }
    
    return Result;
}

internal token*
GetNextTokenOfType (token* Tokens, token_type Type)
{
    token* Result = 0;
    
    token* Iter = Tokens->Next;
    while((Iter != 0) && (Iter->Type != Type))
    {
        Iter = Iter->Next;
    }
    
    Result = Iter;
    return Result;
}