//
// File: assembly_parser.cpp
// Author: Peter Slattery
// Creation Date: 2020-01-01
//
#ifndef ASSEMBLY_PARSER_CPP

// TODO(pjs): This is good for meta generation
// ie. It would be great to have
//     //             enum ident      enum prefix
//     BEGIN_GEN_ENUM(assembly_field, AssemblyField_)
//         //             value name    gen string of the value name  the paired string identifier
//         ADD_ENUM_VALUE(AssemblyName, DO_GEN_STRING,                "assembly_name")
//         ADD_ENUM_VALUE(AssemblyScale, DO_GEN_STRING, "assembly_scale")
//     END_GEN_ENUM(assembly_field)

enum assembly_field
{
    AssemblyField_AssemblyName,
    AssemblyField_AssemblyScale,
    AssemblyField_AssemblyCenter,
    AssemblyField_LedStripCount,
    AssemblyField_OutputMode,
    
    AssemblyField_LedStrip,
    
    AssemblyField_OutputSACN,
    AssemblyField_SACN_StartUniverse,
    AssemblyField_SACN_StartChannel,
    
    AssemblyField_OutputUART,
    AssemblyField_UART_Channel,
    AssemblyField_UART_ComPort,
    
    AssemblyField_PointPlacementType,
    AssemblyField_InterpolatePoints,
    AssemblyField_Start,
    AssemblyField_End,
    AssemblyField_LedCount,
    AssemblyField_TagsCount,
    AssemblyField_Tag,
    AssemblyField_Name,
    AssemblyField_Value,
    
    AssemblyField_Count,
};

global char* AssemblyFieldIdentifiers[] = {
    "assembly_name", // AssemblyField_AssemblyName
    "assembly_scale", // AssemblyField_AssemblyScale
    "assembly_center", // AssemblyField_AssemblyCenter
    "led_strip_count", // AssemblyField_LedStripCount
    "output_mode", // AssemblyField_OutputMode
    
    "led_strip", // AssemblyField_LedStrip
    
    "output_sacn", // AssemblyField_OutputSACN
    "start_universe", // AssemblyField_SACN_StartUniverse
    "start_channel", // AssemblyField_SACN_StartChannel
    
    "output_uart", // AssemblyField_OutputUART
    "channel", // AssemblyField_UART_Channel
    "com_port", // AssemblyField_UART_ComPort
    
    "point_placement_type", // AssemblyField_PointPlacementType
    "interpolate_points", // AssemblyField_InterpolatePoints
    "start", // AssemblyField_Start
    "end", // AssemblyField_End
    
    "led_count", // AssemblyField_LedCount
    
    "tags_count", // AssemblyField_TagCount
    "tag", // AssemblyField_Tag
    "name", // AssemblyField_Name
    "value", // AssemblyField_Value
};

struct assembly_error_list
{
    gs_string String;
    assembly_error_list* Next;
};

struct assembly_tokenizer
{
    gs_string Text;
    char* At;
    
    gs_const_string FileName;
    u32 LineNumber;
    
    bool ParsingIsValid;
    
    gs_memory_arena* ErrorArena;
    assembly_error_list* ErrorsRoot;
    assembly_error_list* ErrorsTail;
};

internal bool
AtValidPosition(assembly_tokenizer* T)
{
    u64 Offset = T->At - T->Text.Str;
    bool Result = (Offset < T->Text.Length);
    return Result;
}

internal void
AdvanceChar(assembly_tokenizer* T)
{
    if (IsNewline(T->At[0]))
    {
        T->LineNumber += 1;
    }
    T->At++;
}

internal void
EatWhitespace(assembly_tokenizer* T)
{
    while(AtValidPosition(T) && IsNewlineOrWhitespace(T->At[0]))
    {
        AdvanceChar(T);
    }
}

internal void
EatToNewLine(assembly_tokenizer* T)
{
    while(AtValidPosition(T) && !IsNewline(T->At[0]))
    {
        AdvanceChar(T);
    }
    EatWhitespace(T);
}

internal bool
AdvanceIfTokenEquals(assembly_tokenizer* T, char* Value)
{
    bool Result = true;
    
    char* TAt = T->At;
    char* VAt = Value;
    while (*VAt != 0)
    {
        if (*TAt != *VAt)
        {
            Result = false;
            break;
        }
        TAt += 1;
        VAt += 1;
    }
    
    // TODO(Peter): What if the token is a subset of Value? ie. this would return true for
    //    T->At = hello_world and Value = hello_
    // But the next token we read would fail
    
    if (Result)
    {
        T->At = TAt;
        EatWhitespace(T);
    }
    return Result;
}

internal void
TokenizerPushError(assembly_tokenizer* T, char* ErrorString)
{
    // NOTE(Peter): We can make this more expressive if we need to
    assembly_error_list* Error = PushStruct(T->ErrorArena, assembly_error_list);
    Error->String = PushString(T->ErrorArena, 512);
    PrintF(&Error->String, "%S(%d): %s", T->FileName, T->LineNumber, ErrorString);
    SLLPushOrInit(T->ErrorsRoot, T->ErrorsTail, Error);
    T->ParsingIsValid = false;
    
    // NOTE(Peter): I'm not sure this is the best idea, but at least this way,
    // if there's multiple errors, you'll get a number of them, rather than
    // a bunch of erroneous errors happening on the same line
    EatToNewLine(T);
}

#define PARSER_FIELD_REQUIRED true
#define PARSER_FIELD_OPTIONAL false

internal bool
ReadFieldIdentifier(assembly_field Field, assembly_tokenizer* T, bool Required = true)
{
    bool Result = false;
    if (AdvanceIfTokenEquals(T, AssemblyFieldIdentifiers[Field]))
    {
        if (AdvanceIfTokenEquals(T, ":"))
        {
            Result = true;
        }
        else
        {
            // We always throw an error if we get this far because we know you were trying to
            // open the identifier
            TokenizerPushError(T, "Field identifier is missing a colon");
        }
    }
    else if (Required)
    {
        TokenizerPushError(T, "Field Identifier Invalid");
    }
    return Result;
}

internal bool
ReadFieldEnd(assembly_tokenizer* T)
{
    bool Result = AdvanceIfTokenEquals(T, ";");
    if (Result)
    {
        EatWhitespace(T);
    }
    else
    {
        TokenizerPushError(T, "Missing a semicolon");
    }
    return Result;
}

internal gs_string
ReadString(assembly_tokenizer* T)
{
    gs_string Result = {};
    if (AdvanceIfTokenEquals(T, "\""))
    {
        char* StringStart = T->At;
        while(AtValidPosition(T) && T->At[0] != '\"')
        {
            T->At++;
        }
        Result.Str = StringStart;
        Result.Size = T->At - StringStart;
        Result.Length = Result.Size;
        if (AdvanceIfTokenEquals(T, "\""))
        {
            // Success
        }
        else
        {
            TokenizerPushError(T, "String not closed with a \"");
        }
    }
    else
    {
        TokenizerPushError(T, "Expecting a string, but none was found");
    }
    return Result;
}

internal gs_string
GetNumberString(assembly_tokenizer* T)
{
    gs_string Result = {};
    Result.Str = T->At;
    while(AtValidPosition(T) && IsNumericExtended(T->At[0]))
    {
        AdvanceChar(T);
    }
    Result.Length = T->At - Result.Str;
    Result.Size = Result.Length;
    return Result;
}

internal r32
ReadFloat(assembly_tokenizer* T)
{
    r32 Result = 0;
    gs_string NumberString = GetNumberString(T);
    Result = (r32)ParseFloat(NumberString.ConstString);
    return Result;
}

internal s32
ReadInt(assembly_tokenizer* T)
{
    s32 Result = 0;
    gs_string NumberString = GetNumberString(T);
    Result = (r32)ParseInt(NumberString.ConstString);
    return Result;
}

internal gs_string
ReadStringField(assembly_field Field, assembly_tokenizer* T, gs_memory_arena* Arena)
{
    gs_string Result = {};
    if (ReadFieldIdentifier(Field, T))
    {
        gs_string ExistingString = ReadString(T);
        if (ReadFieldEnd(T))
        {
            // Success
            Result = PushString(Arena, ExistingString.Length);
            PrintF(&Result, "%S", ExistingString);
        }
    }
    return Result;
}

internal r32
ReadFloatField(assembly_field Field, assembly_tokenizer* T)
{
    r32 Result = 0.0f;
    if (ReadFieldIdentifier(Field, T))
    {
        Result = ReadFloat(T);
        if (!ReadFieldEnd(T))
        {
            T->ParsingIsValid = false;
        }
    }
    return Result;
}

internal s32
ReadIntField(assembly_field Field, assembly_tokenizer* T)
{
    r32 Result = 0.0f;
    if (ReadFieldIdentifier(Field, T))
    {
        Result = ReadInt(T);
        if (!ReadFieldEnd(T))
        {
            T->ParsingIsValid = false;
        }
    }
    return Result;
}

internal v3
ReadV3Field(assembly_field Field, assembly_tokenizer* T)
{
    v3 Result = {};
    if (ReadFieldIdentifier(Field, T))
    {
        if (AdvanceIfTokenEquals(T, "("))
        {
            Result.x = ReadFloat(T);
            if (AdvanceIfTokenEquals(T, ","))
            {
                Result.y = ReadFloat(T);
                if (AdvanceIfTokenEquals(T, ","))
                {
                    Result.z = ReadFloat(T);
                    if (AdvanceIfTokenEquals(T, ")"))
                    {
                        if (!ReadFieldEnd(T))
                        {
                            T->ParsingIsValid = false;
                        }
                    }
                    else
                    {
                        TokenizerPushError(T, "Vector 3 doesn't end with a ')'");
                    }
                }
                else
                {
                    TokenizerPushError(T, "Vector 3: unable to read a field");
                }
            }
            else
            {
                TokenizerPushError(T, "Vector 3: unable to read a field");
            }
        }
        else
        {
            TokenizerPushError(T, "Vector 3: unable to read a field");
        }
    }
    return Result;
}

internal bool
ReadStructOpening(assembly_field Field, assembly_tokenizer* T, bool Required = true)
{
    bool Result = false;
    if (ReadFieldIdentifier(Field, T, Required))
    {
        if (AdvanceIfTokenEquals(T, "{"))
        {
            Result = true;
        }
    }
    return Result;
}

internal bool
ReadStructClosing(assembly_tokenizer* T)
{
    bool Result = AdvanceIfTokenEquals(T, "};");
    return Result;
}

internal void
StripSetTag(v2_strip* Strip, u32 TagIndex, gs_const_string TagName, gs_const_string TagValue)
{
    Assert(TagIndex < Strip->TagsCount);
    v2_tag* TagAt = &Strip->Tags[TagIndex];
    TagAt->NameHash = HashDJB2ToU32(StringExpand(TagName));
    TagAt->ValueHash = HashDJB2ToU32(StringExpand(TagValue));
}

internal bool
ParseAssemblyFile(assembly* Assembly, gs_const_string FileName, gs_string FileText, gs_memory_arena* Transient)
{
    Assembly->LedCountTotal = 0;
    
    r32 Value = ParseFloat(ConstString("-2.355"));
    
    assembly_tokenizer Tokenizer = {};
    Tokenizer.Text = FileText;
    Tokenizer.At = Tokenizer.Text.Str;
    Tokenizer.ParsingIsValid = true;
    Tokenizer.ErrorArena = Transient;
    Tokenizer.FileName = FileName;
    
    Assembly->Name = ReadStringField(AssemblyField_AssemblyName, &Tokenizer, &Assembly->Arena);
    Assembly->Scale = ReadFloatField(AssemblyField_AssemblyScale, &Tokenizer);
    Assembly->Center = ReadV3Field(AssemblyField_AssemblyCenter, &Tokenizer);
    
    Assembly->StripCount = ReadIntField(AssemblyField_LedStripCount, &Tokenizer);
    Assembly->Strips = PushArray(&Assembly->Arena, v2_strip, Assembly->StripCount);
    
    gs_string OutputModeString = ReadStringField(AssemblyField_OutputMode, &Tokenizer, Transient);
    if (StringsEqual(OutputModeString.ConstString, ConstString("UART")))
    {
        Assembly->OutputMode = NetworkProtocol_UART;
    }
    else if (StringsEqual(OutputModeString.ConstString, ConstString("SACN")))
    {
        Assembly->OutputMode = NetworkProtocol_SACN;
    }
    else
    {
        TokenizerPushError(&Tokenizer, "Invalid output mode specified.");
    }
    
    for (u32 i = 0; i < Assembly->StripCount; i++)
    {
        v2_strip* StripAt = Assembly->Strips + i;
        if (ReadStructOpening(AssemblyField_LedStrip, &Tokenizer))
        {
            if (ReadStructOpening(AssemblyField_OutputSACN, &Tokenizer, PARSER_FIELD_OPTIONAL))
            {
                StripAt->SACNAddr.StartUniverse = ReadIntField(AssemblyField_SACN_StartUniverse, &Tokenizer);
                StripAt->SACNAddr.StartChannel = ReadIntField(AssemblyField_SACN_StartChannel, &Tokenizer);
                
                if (!ReadStructClosing(&Tokenizer))
                {
                    TokenizerPushError(&Tokenizer, "Struct doesn't close where expected");
                }
            }
            
            if (ReadStructOpening(AssemblyField_OutputUART, &Tokenizer, PARSER_FIELD_OPTIONAL))
            {
                StripAt->UARTAddr.Channel = (u8)ReadIntField(AssemblyField_UART_Channel, &Tokenizer);
                StripAt->UARTAddr.ComPort = ReadStringField(AssemblyField_UART_ComPort, &Tokenizer, &Assembly->Arena).ConstString;
                
                if (!ReadStructClosing(&Tokenizer))
                {
                    TokenizerPushError(&Tokenizer, "Struct doesn't close where expected");
                }
            }
            
            // TODO(Peter): Need to store this
            gs_string PointPlacementType = ReadStringField(AssemblyField_PointPlacementType, &Tokenizer, &Assembly->Arena);
            // TODO(Peter): Switch on value of PointPlacementType
            if (ReadStructOpening(AssemblyField_InterpolatePoints, &Tokenizer))
            {
                StripAt->StartPosition = ReadV3Field(AssemblyField_Start, &Tokenizer);
                StripAt->EndPosition = ReadV3Field(AssemblyField_End, &Tokenizer);
                if (!ReadStructClosing(&Tokenizer))
                {
                    Tokenizer.ParsingIsValid = false;
                    // TODO(Peter): @ErrorHandling
                    // Have this function prepend the filename and line number.
                    // Create an error display popup window, or an error log window that takes over a panel automatically
                    // TokenizerPushError(&Tokenizer, "Unable to read
                }
            }
            
            StripAt->LedCount = ReadIntField(AssemblyField_LedCount, &Tokenizer);
            Assembly->LedCountTotal += StripAt->LedCount;
            
            
            StripAt->TagsCount = ReadIntField(AssemblyField_TagsCount, &Tokenizer);
            // NOTE(pjs): Always add one tag to the input to leave room for the assembly name
            StripAt->TagsCount += 1;
            StripAt->Tags = PushArray(&Assembly->Arena, v2_tag, StripAt->TagsCount);
            StripSetTag(StripAt, 0, ConstString("assembly"), Assembly->Name.ConstString);
            for (u32 Tag = 1; Tag < StripAt->TagsCount; Tag++)
            {
                if (ReadStructOpening(AssemblyField_Tag, &Tokenizer))
                {
                    // TODO(Peter): Need to store the gs_string somewhere we can look it up for display in the interface
                    // right now they are stored in temp memory and won't persist
                    gs_string TagName = ReadStringField(AssemblyField_Name, &Tokenizer, Transient);
                    gs_string TagValue = ReadStringField(AssemblyField_Value, &Tokenizer, Transient);
                    StripSetTag(StripAt, Tag, TagName.ConstString, TagValue.ConstString);
                    if (!ReadStructClosing(&Tokenizer))
                    {
                        TokenizerPushError(&Tokenizer, "Struct doesn't close where expected");
                    }
                }
                else
                {
                    TokenizerPushError(&Tokenizer, "Expected a struct opening, but none was found");
                }
            }
            
            
            if (!ReadStructClosing(&Tokenizer))
            {
                TokenizerPushError(&Tokenizer, "Struct doesn't close where expected");
            }
        }
        else
        {
            TokenizerPushError(&Tokenizer, "Expected a struct opening, but none was found");
        }
    }
    
    return Tokenizer.ParsingIsValid;
}

#define ASSEMBLY_PARSER_CPP
#endif // ASSEMBLY_PARSER_CPP