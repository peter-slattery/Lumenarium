struct token_selection_spec
{
    b32 MatchText;
    string Text;
};

internal s32
EatPreprocessor (tokenizer* Tokenizer)
{
    char* TStart = Tokenizer->At;
    while (AtValidPosition(*Tokenizer) && !IsNewline(*Tokenizer->At))
    {
        if (Tokenizer->At[0] == '\\')
        {
            EatChar(Tokenizer);
            
            while (IsWhitespace(*Tokenizer->At))
            {
                EatChar(Tokenizer);
            }
            
            if (IsNewline(*Tokenizer->At))
            {
                EatPastNewLine(Tokenizer);
            }
        }
        else if (!IsNewline(*Tokenizer->At))
        {
            EatChar(Tokenizer);
        }
    }
    
    return Tokenizer->At - TStart;
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

internal b32
TokenAtEquals(tokenizer* Tokenizer, char* Needle)
{
    b32 Result = true;
    
    char* TokenizerStart = Tokenizer->At;
    
    char* NeedleAt = Needle;
    while (AtValidPosition(*Tokenizer) && *NeedleAt)
    {
        if (*NeedleAt != *Tokenizer->At)
        {
            Result = false;
            break;
        }
        NeedleAt++;
        EatChar(Tokenizer);
    }
    
    // NOTE(Peter): rewind tokenizer
    if (!Result)
    {
        Tokenizer->At = TokenizerStart;
    }
    
    return Result;
}

internal token
GetNextToken (tokenizer* Tokenizer)
{
    token Result = {};
    
    EatWhitespace(Tokenizer);
    
    // Don't include comments in tokens
    while (Tokenizer->At[0] && Tokenizer->At[0] == '/' && Tokenizer->At[1] &&  Tokenizer->At[1] == '/')
    {
        EatToNewLine(Tokenizer);
        EatWhitespace(Tokenizer);
    }
    
    while(Tokenizer->At[0] && Tokenizer->At[0] == '/' && Tokenizer->At[1] &&  Tokenizer->At[1] == '*')
    {
        Tokenizer->At += 2;
        while (*Tokenizer->At)
        {
            if (Tokenizer->At[0] && Tokenizer->At[0] == '*' && Tokenizer->At[1] &&  Tokenizer->At[1] == '/')
            {
                Tokenizer->At += 2;
                break;
            }
            EatToNewLine(Tokenizer);
            EatWhitespace(Tokenizer);
        }
        EatWhitespace(Tokenizer);
    }
    
    Result.Text = MakeString(Tokenizer->At, 1, 1);
    
    // NOTE(Peter): Adding one because I want the tokenizer to work with clear to zero
    // but line numbers generally start at 1, not 0
    Result.LineNumber = Tokenizer->LineNumber + 1;
    
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
        // NOTE(Peter): Technically correct to do things like "# define"
        EatWhitespace(Tokenizer);
        
        if (TokenAtEquals(Tokenizer, "define"))
        { 
            Result.Type = Token_PoundDefine; 
            EatPreprocessor(Tokenizer);
            Result.Text.Length = Tokenizer->At - Result.Text.Memory;
        }
        else if (TokenAtEquals(Tokenizer, "undef"))
        { 
            Result.Type = Token_PoundUndef; 
            EatToNewLine(Tokenizer);
            Result.Text.Length = Tokenizer->At - Result.Text.Memory;
        }
        else if (TokenAtEquals(Tokenizer, "include"))
        { 
            Result.Type = Token_PoundInclude; 
            Result.Text.Length = Tokenizer->At - Result.Text.Memory;
        }
        else if (TokenAtEquals(Tokenizer, "ifdef"))
        { 
            Result.Type = Token_PoundIfDef; 
            EatToNewLine(Tokenizer);
            Result.Text.Length = Tokenizer->At - Result.Text.Memory;
        }
        else if (TokenAtEquals(Tokenizer, "ifndef"))
        { 
            Result.Type = Token_PoundIfNDef; 
            EatToNewLine(Tokenizer);
            Result.Text.Length = Tokenizer->At - Result.Text.Memory;
        }
        else if (TokenAtEquals(Tokenizer, "if"))
        { 
            Result.Type = Token_PoundIf; 
            EatToNewLine(Tokenizer);
            Result.Text.Length = Tokenizer->At - Result.Text.Memory;
        }
        else if (TokenAtEquals(Tokenizer, "elif"))
        { 
            Result.Type = Token_PoundElif; 
            EatToNewLine(Tokenizer);
            Result.Text.Length = Tokenizer->At - Result.Text.Memory;
        }
        else if (TokenAtEquals(Tokenizer, "else"))
        { 
            Result.Type = Token_PoundElse; 
            EatToNewLine(Tokenizer);
            Result.Text.Length = Tokenizer->At - Result.Text.Memory;
        }
        else if (TokenAtEquals(Tokenizer, "endif"))
        { 
            Result.Type = Token_PoundEndif; 
            EatToNewLine(Tokenizer);
            Result.Text.Length = Tokenizer->At - Result.Text.Memory;
        }
        else if (TokenAtEquals(Tokenizer, "error"))
        { 
            Result.Type = Token_PoundError; 
            EatToNewLine(Tokenizer);
            Result.Text.Length = Tokenizer->At - Result.Text.Memory;
        }
        else if (TokenAtEquals(Tokenizer, "pragma"))
        { 
            Result.Type = Token_PoundPragma; 
            EatToNewLine(Tokenizer);
            Result.Text.Length = Tokenizer->At - Result.Text.Memory;
        }
    }
    else if (IsNumeric(C))
    {
        Result.Type = Token_Number;
        
        // NOTE(Peter): adding 1 to account for the fact that we've already advanced
        // Tokenizer once
        Result.Text.Length = 1 + EatNumber(Tokenizer);
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