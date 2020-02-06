//
// File: assembly_parser.cpp
// Author: Peter Slattery
// Creation Date: 2020-01-01
//
#ifndef ASSEMBLY_PARSER_CPP


internal assembly_token
ParseToken (tokenizer* Tokenizer)
{
    assembly_token Result = {};
    Result.Token = Tokenizer->At;
    Result.Length = 1;
    EatChar(Tokenizer);
    
    if (*Result.Token == ':'){ Result.Type = AssemblyToken_Colon; }
    else if (*Result.Token == ';'){ Result.Type = AssemblyToken_SemiColon; }
    else if (*Result.Token =='{'){ Result.Type = AssemblyToken_LeftCurlyBrace; }
    else if (*Result.Token =='}'){ Result.Type = AssemblyToken_RightCurlyBrace; }
    else if (*Result.Token ==','){ Result.Type = AssemblyToken_Comma; }
    else if (IsNumericExtended(*Result.Token))
    {
        while(*Tokenizer->At && IsNumericExtended(*Tokenizer->At)) { EatChar(Tokenizer); }
        Result.Type = AssemblyToken_Number;
        Result.Length = Tokenizer->At - Result.Token;
    }
    else if (*Result.Token =='\"')
    {
        while(*Tokenizer->At && *Tokenizer->At != '\"') { EatChar(Tokenizer); }
        Result.Token++; // Skip the quote
        Result.Type = AssemblyToken_String;
        Result.Length = (Tokenizer->At - Result.Token) - 1;
    }
    else if (*Result.Token == '(')
    {
        while(*Tokenizer->At && *Tokenizer->At != ')') { EatChar(Tokenizer); }
        Result.Token++; // Skip the paren
        Result.Type = AssemblyToken_Vector;
        Result.Length = (Tokenizer->At - Result.Token) - 1;
    }
    else if (CharArraysEqualUpToLength(Result.Token, LED_STRIP_IDENTIFIER, CharArrayLength(LED_STRIP_IDENTIFIER)))
    {
        Result.Type = AssemblyToken_LEDStrip;
        Result.Length = CharArrayLength(LED_STRIP_IDENTIFIER);
        Tokenizer->At += Result.Length - 1;
    }
    else if (CharArraysEqualUpToLength(Result.Token, END_ASSEMBLY_FILE_IDENTIFIER, CharArrayLength(END_ASSEMBLY_FILE_IDENTIFIER)))
    {
        Result.Type = AssemblyToken_EndOfFile;
        Result.Length = CharArrayLength(END_ASSEMBLY_FILE_IDENTIFIER);
        Tokenizer->At += Result.Length - 1;
    }
    else
    {
        Result.Type = AssemblyToken_Identifier;
        while(*Tokenizer->At && !IsWhitespace(*Tokenizer->At)) { EatChar(Tokenizer); }
    }
    
    return Result;
}

internal v3
ParseAssemblyVector (char* String)
{
    v3 Result = {};
    
    tokenizer Tokenizer = {};
    Tokenizer.At = String;
    
    EatWhitespace(&Tokenizer);
    Result.x = ParseFloatUnsafe(Tokenizer.At).FloatValue;
    EatPastCharacter(&Tokenizer, ',');
    
    EatWhitespace(&Tokenizer);
    Result.y = ParseFloatUnsafe(Tokenizer.At).FloatValue;
    EatPastCharacter(&Tokenizer, ',');
    
    EatWhitespace(&Tokenizer);
    Result.z = ParseFloatUnsafe(Tokenizer.At).FloatValue;
    EatPastCharacter(&Tokenizer, ',');
    
    return Result;
}

internal b32
ParseAssemblyFileHeader (assembly_definition* Assembly, tokenizer* Tokenizer)
{
    b32 HeaderIsValid = false;
    
    if (CharArraysEqualUpToLength(Tokenizer->At, LED_STRIP_COUNT_IDENTIFIER, CharArrayLength(LED_STRIP_COUNT_IDENTIFIER)))
    {
        Tokenizer->At += CharArrayLength(LED_STRIP_COUNT_IDENTIFIER);
        EatWhitespace(Tokenizer);
        assembly_token CountToken = ParseToken(Tokenizer);
        if (CountToken.Type == AssemblyToken_Number)
        {
            Assembly->LEDStripSize = ParseSignedIntUnsafe(CountToken.Token).SignedIntValue;
            HeaderIsValid = true;
        }
        EatWhitespace(Tokenizer);
    }
    return HeaderIsValid;
}

internal led_strip_definition
ParseLEDStrip (tokenizer* Tokenizer)
{
    led_strip_definition Result = {};
    
    // Control Box Index
    while (*Tokenizer->At && !IsNumericExtended(*Tokenizer->At)) { EatChar(Tokenizer); }
    assembly_token BoxIDToken = ParseToken(Tokenizer);
    Assert(BoxIDToken.Type == AssemblyToken_Number);
    Result.ControlBoxID = ParseSignedIntUnsafe(BoxIDToken.Token).SignedIntValue;
    
    // Start Universe
    EatPastCharacter(Tokenizer, ',');
    EatWhitespace(Tokenizer);
    assembly_token StartUniverseToken = ParseToken(Tokenizer);
    Assert(BoxIDToken.Type == AssemblyToken_Number);
    Result.StartUniverse = ParseSignedIntUnsafe(StartUniverseToken.Token).SignedIntValue;
    
    // Start Channel
    EatPastCharacter(Tokenizer, ',');
    EatWhitespace(Tokenizer);
    assembly_token StartChannelToken = ParseToken(Tokenizer);
    Assert(BoxIDToken.Type == AssemblyToken_Number);
    Result.StartChannel = ParseSignedIntUnsafe(StartChannelToken.Token).SignedIntValue;
    
    // Strip Type
    // TODO(Peter): This is unused for now, and would be a branch point for parsing 
    // the rest of the info. Fix this.
    EatPastCharacter(Tokenizer, ',');
    EatWhitespace(Tokenizer);
    if (CharArraysEqualUpToLength(Tokenizer->At, INTERPOLATE_POINTS_IDENTIFIER, CharArrayLength(INTERPOLATE_POINTS_IDENTIFIER)))
    {
        Result.InterpolationType = StripInterpolate_Points;
        
        // Start Position
        EatPastCharacter(Tokenizer, ',');
        EatWhitespace(Tokenizer);
        assembly_token StartPositionToken = ParseToken(Tokenizer);
        Assert(StartPositionToken.Type == AssemblyToken_Vector);
        Result.InterpolatePositionStart = ParseAssemblyVector(StartPositionToken.Token);
        
        // End Position
        EatPastCharacter(Tokenizer, ',');
        EatWhitespace(Tokenizer);
        assembly_token EndPositionToken = ParseToken(Tokenizer);
        Assert(EndPositionToken.Type == AssemblyToken_Vector);
        Result.InterpolatePositionEnd = ParseAssemblyVector(EndPositionToken.Token);
        
        // LEDs Per Strip
        EatPastCharacter(Tokenizer, ',');
        EatWhitespace(Tokenizer);
        assembly_token LEDsPerStripToken = ParseToken(Tokenizer);
        Assert(BoxIDToken.Type == AssemblyToken_Number);
        Result.LEDsPerStrip = ParseSignedIntUnsafe(LEDsPerStripToken.Token).SignedIntValue;
    }
    
    EatPastCharacter(Tokenizer, '}');
    EatWhitespace(Tokenizer);
    
    return Result;
}

internal void
ParseAssemblyFileBody (assembly_definition* Assembly, tokenizer* Tokenizer)
{
    EatWhitespace(Tokenizer);
    
    while(*Tokenizer->At)
    {
        EatWhitespace(Tokenizer);
        assembly_token Token = ParseToken(Tokenizer);
        
        if (Token.Type != AssemblyToken_EndOfFile)
        {
            switch (Token.Type)
            {
                case AssemblyToken_LEDStrip:
                {
                    led_strip_definition* LEDStripDef = Assembly->LEDStrips + Assembly->LEDStripCount;
                    Assert(Assembly->LEDStripCount < Assembly->LEDStripSize);
                    
                    *LEDStripDef = ParseLEDStrip(Tokenizer);
                    Assembly->TotalLEDCount += LEDStripDef->LEDsPerStrip;
                    
                    Assembly->LEDStripCount++;
                } break;
                
                // TODO(Peter): Other cases? What else would need to be in the assembly body?
                
                InvalidDefaultCase;
            }
        }
        else
        {
            break;
        }
    }
    
    // NOTE(Peter): Ensure the validity of the assembly file. We probably don't want an assert here,
    // more likely we want to load a valid assembly anyways, and just raise this as an error to the user
    // so they can fix it.
    Assert(Assembly->LEDStripCount == Assembly->LEDStripSize);
}

inline b32
ParseTokenEquals (tokenizer* T, char* Validate)
{
    b32 Result = true;
    
    char* TAt = T->At;
    char* VAt = Validate;
    while (((TAt - T->Memory) < T->MemoryLength) && *VAt)
    {
        if (*VAt != *TAt)
        {
            Result = false;
            break;
        }
        TAt++;
        *VAt++;
    }
    
    if (Result)
    {
        T->At = TAt;
        EatWhitespace(T);
    }
    
    return Result;
}

internal b32
ParseComma(tokenizer* T)
{
    b32 Result = ParseTokenEquals(T, ",");
    return Result;
}

internal b32
ParseOpenCurlyBrace(tokenizer* T)
{
    b32 Result = ParseTokenEquals(T, "{");
    return Result;
}

internal b32
ParseCloseCurlyBrace(tokenizer* T)
{
    b32 Result = ParseTokenEquals(T, "}");
    return Result;
}

internal b32
ParseOpenParen(tokenizer* T)
{
    b32 Result = ParseTokenEquals(T, "(");
    return Result;
}

internal b32
ParseCloseParen(tokenizer* T)
{
    b32 Result = ParseTokenEquals(T, ")");
    return Result;
}

internal b32
ParseUnsignedInteger(tokenizer* T, u32* Value)
{
    parse_result Result = ParseUnsignedIntUnsafe(T->At);
    *Value = Result.UnsignedIntValue;
    T->At = Result.OnePastLast;
    
    // TODO(Peter): Parse functions in gs_string don't actually check for errors or
    // whether or not they actually parsed an int.
    // :GSStringParseErrors
    return true;
}

internal b32
ParseFloat(tokenizer* T, r32* Value)
{
    parse_result ParseResult = ParseFloatUnsafe(T->At);
    *Value = ParseResult.FloatValue;
    T->At = ParseResult.OnePastLast;
    
    // TODO(Peter):
    // :GSStringParseErrors
    return true;
}

internal b32
ParseVector(tokenizer* T, v3* Value)
{
    b32 Result = true;
    
    if (ParseOpenParen(T))
    {
        for (u32 i = 0; i < 3; i++)
        {
            b32 ValueSuccess = ParseFloat(T, &(Value->E[i]));
            if (!ValueSuccess)
            {
                Result = false;
                break;
            }
            
            b32 CommaSuccess = ParseComma(T);
            if (!CommaSuccess)
            {
                break;
            }
        }
        
        if (!ParseCloseParen(T))
        {
            Result = false;
        }
    }
    else
    {
        Result = false;
    }
    
    return Result;
}

// TODO(Peter): :ErrorLogging
#define ParseLEDStripToken(tokenizer, parse_expr, error_msg) \
(parse_expr) && (ParseComma(tokenizer))

internal b32
ParseLEDStripCount (tokenizer* T, u32* Value)
{
    b32 Result = false;
    
    if (ParseTokenEquals(T, LED_STRIP_COUNT_IDENTIFIER))
    {
        EatWhitespace(T);
        if (ParseUnsignedInteger(T, Value))
        {
            Result = true;
        }
    }
    
    return Result;
}

internal b32
ParseLEDStrip (led_strip_definition* Strip, tokenizer* T, memory_arena* Arena)
{
    b32 Result = false;
    
    if (ParseTokenEquals(T, LED_STRIP_IDENTIFIER) &&
        ParseOpenCurlyBrace(T))
    {
        Result = true;
        
        u32 ControlBoxIndex, StartUniverse, StartChannel;
        ParseLEDStripToken(T, ParseUnsignedInteger(T, &Strip->ControlBoxID), "Control Box Error");
        ParseLEDStripToken(T, ParseUnsignedInteger(T, &Strip->StartUniverse), "Start Universe Error");
        ParseLEDStripToken(T, ParseUnsignedInteger(T, &Strip->StartChannel), "Start Channel Error");
        
        if (ParseTokenEquals(T, INTERPOLATE_POINTS_IDENTIFIER) &&
            ParseComma(T))
        {
            Strip->InterpolationType = StripInterpolate_Points;
            
            ParseLEDStripToken(T, ParseVector(T, &Strip->InterpolatePositionStart), "Position Start Error");
            ParseLEDStripToken(T, ParseVector(T, &Strip->InterpolatePositionEnd), "Position End Error");
        }
        
        ParseLEDStripToken(T, ParseUnsignedInteger(T, &Strip->LEDsPerStrip), "LEDs Per Strip Error");
        
        EatWhitespace(T);
        if (!ParseCloseCurlyBrace(T))
        {
            Result = false;
        }
    }
    
    return Result;
}

internal assembly_definition
ParseAssemblyFile (u8* FileBase, s32 FileSize, memory_arena* Arena, event_log* EventLog)
{
    assembly_definition Assembly = {};
    
    tokenizer Tokenizer = {};
    Tokenizer.At = (char*)FileBase;
    Tokenizer.Memory = (char*)FileBase;
    Tokenizer.MemoryLength = FileSize;
    
    if (ParseLEDStripCount(&Tokenizer, &Assembly.LEDStripSize))
    {
        Assembly.LEDStrips = PushArray(Arena, led_strip_definition, Assembly.LEDStripSize);
        
        while (AtValidPosition(Tokenizer))
        {
            EatWhitespace(&Tokenizer);
            
            if (Assembly.LEDStripCount < Assembly.LEDStripSize)
            {
                led_strip_definition* LEDStrip = Assembly.LEDStrips + Assembly.LEDStripCount++;
                
                if (ParseLEDStrip(LEDStrip, &Tokenizer, Arena))
                {
                    Assembly.TotalLEDCount += LEDStrip->LEDsPerStrip;
                }
                else
                {
                    LogError(EventLog, "Unable to parse LED strip in assembly file");
                    break;
                }
            }
            else
            {
                if (ParseTokenEquals(&Tokenizer, END_ASSEMBLY_FILE_IDENTIFIER))
                {
                    break;
                }
                else
                {
                    LogError(EventLog, "Did not find ent of file identifier in assembly file");
                    break;
                }
            }
        }
    }
    else
    {
        // TODO(Peter): :ErrorLoggong
        InvalidCodePath;
    }
    
    return Assembly;
}

#define ASSEMBLY_PARSER_CPP
#endif // ASSEMBLY_PARSER_CPP