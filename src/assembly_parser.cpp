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
    Tokenizer->At++;
    
    if (*Result.Token == ':'){ Result.Type = AssemblyToken_Colon; }
    else if (*Result.Token == ';'){ Result.Type = AssemblyToken_SemiColon; }
    else if (*Result.Token =='{'){ Result.Type = AssemblyToken_LeftCurlyBrace; }
    else if (*Result.Token =='}'){ Result.Type = AssemblyToken_RightCurlyBrace; }
    else if (*Result.Token ==','){ Result.Type = AssemblyToken_Comma; }
    else if (IsNumericExtended(*Result.Token))
    {
        while(*Tokenizer->At && IsNumericExtended(*Tokenizer->At)) { Tokenizer->At++; }
        Result.Type = AssemblyToken_Number;
        Result.Length = Tokenizer->At - Result.Token;
    }
    else if (*Result.Token =='\"')
    {
        while(*Tokenizer->At && *Tokenizer->At != '\"') { Tokenizer->At++; }
        Result.Token++; // Skip the quote
        Result.Type = AssemblyToken_String;
        Result.Length = (Tokenizer->At - Result.Token) - 1;
    }
    else if (*Result.Token == '(')
    {
        while(*Tokenizer->At && *Tokenizer->At != ')') { Tokenizer->At++; }
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
        while(*Tokenizer->At && !IsWhitespace(*Tokenizer->At)) { Tokenizer->At++; }
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

internal void
ParseAssemblyFileHeader (assembly_definition* Assembly, tokenizer* Tokenizer)
{
    if (CharArraysEqualUpToLength(Tokenizer->At, LED_STRIP_COUNT_IDENTIFIER, CharArrayLength(LED_STRIP_COUNT_IDENTIFIER)))
    {
        Tokenizer->At += CharArrayLength(LED_STRIP_COUNT_IDENTIFIER);
        EatWhitespace(Tokenizer);
        assembly_token CountToken = ParseToken(Tokenizer);
        if (CountToken.Type == AssemblyToken_Number)
        {
            Assembly->LEDStripSize = ParseSignedIntUnsafe(CountToken.Token).SignedIntValue;
        }
        else
        {
            InvalidCodePath;
        }
        EatWhitespace(Tokenizer);
    }
    else
    {
        // TODO(Peter): Handle corrupted files, try to recover?
        InvalidCodePath;
    }
    
}

internal led_strip_definition
ParseLEDStrip (tokenizer* Tokenizer)
{
    led_strip_definition Result = {};
    
    // Control Box Index
    while (*Tokenizer->At && !IsNumericExtended(*Tokenizer->At)) { Tokenizer->At++; }
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

internal assembly_definition
ParseAssemblyFile (u8* FileBase, s32 FileSize, memory_arena* Arena)
{
    assembly_definition AssemblyDefinition = {};
    
    tokenizer AssemblyFileTokenizer = {};
    AssemblyFileTokenizer.At = (char*)FileBase;
    AssemblyFileTokenizer.Memory = (char*)FileBase;
    AssemblyFileTokenizer.MemoryLength = FileSize;
    
    ParseAssemblyFileHeader(&AssemblyDefinition, &AssemblyFileTokenizer);
    AssemblyDefinition.LEDStrips = PushArray(Arena, led_strip_definition, AssemblyDefinition.LEDStripSize);
    ParseAssemblyFileBody(&AssemblyDefinition, &AssemblyFileTokenizer);
    
    return AssemblyDefinition;
}

#define ASSEMBLY_PARSER_CPP
#endif // ASSEMBLY_PARSER_CPP