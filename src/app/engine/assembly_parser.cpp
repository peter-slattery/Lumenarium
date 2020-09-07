//
// File: assembly_parser.cpp
// Author: Peter Slattery
// Creation Date: 2020-01-01
//
#ifndef ASSEMBLY_PARSER_CPP

enum assembly_field
{
    AssemblyField_AssemblyName,
    AssemblyField_AssemblyScale,
    AssemblyField_AssemblyCenter,
    AssemblyField_LedStripCount,
    
    AssemblyField_LedStrip,
    AssemblyField_ControlBoxId,
    AssemblyField_StartUniverse,
    AssemblyField_StartChannel,
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
    
    "led_strip", // AssemblyField_LedStrip
    
    "control_box_id", // AssemblyField_ControlBoxId
    "start_universe", // AssemblyField_StartUniverse
    "start_channel", // AssemblyField_StartChannel
    
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

internal bool
ReadFieldIdentifier(assembly_field Field, assembly_tokenizer* T)
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
            TokenizerPushError(T, "Field identifier is missing a colon");
        }
    }
    else
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
ReadStructOpening(assembly_field Field, assembly_tokenizer* T)
{
    bool Result = false;
    if (ReadFieldIdentifier(Field, T))
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
    
    for (u32 i = 0; i < Assembly->StripCount; i++)
    {
        v2_strip* StripAt = Assembly->Strips + i;
        if (ReadStructOpening(AssemblyField_LedStrip, &Tokenizer))
        {
            StripAt->ControlBoxID = ReadIntField(AssemblyField_ControlBoxId, &Tokenizer);
            StripAt->StartUniverse = ReadIntField(AssemblyField_StartUniverse, &Tokenizer);
            StripAt->StartChannel = ReadIntField(AssemblyField_StartChannel, &Tokenizer);
            
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
            StripAt->Tags = PushArray(&Assembly->Arena, v2_tag, StripAt->TagsCount);
            for (u32 Tag = 0; Tag < StripAt->TagsCount; Tag++)
            {
                v2_tag* TagAt = StripAt->Tags + Tag;
                if (ReadStructOpening(AssemblyField_Tag, &Tokenizer))
                {
                    // TODO(Peter): Need to store the gs_string somewhere we can look it up for display in the interface
                    // right now they are stored in temp memory and won't persist
                    gs_string TagName = ReadStringField(AssemblyField_Name, &Tokenizer, Transient);
                    gs_string TagValue = ReadStringField(AssemblyField_Value, &Tokenizer, Transient);
                    TagAt->NameHash = HashDJB2ToU32(StringExpand(TagName));
                    TagAt->ValueHash = HashDJB2ToU32(StringExpand(TagValue));
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