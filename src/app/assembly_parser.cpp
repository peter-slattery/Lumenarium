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

global_variable char* AssemblyFieldIdentifiers[] = {
    "assembly_name", // AssemblyField_AssemblyName
    "assembly_scale", // AssemblyField_AssemblyScale
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

struct assembly_tokenizer
{
    string Text;
    char* At;
    
    u32 LineNumber;
    
    bool ParsingIsValid;
};

internal bool
AtValidPosition(assembly_tokenizer* T)
{
    bool Result = ((T->At - T->Text.Memory) < T->Text.Length);
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
            T->ParsingIsValid = false;
        }
    }
    else
    {
        T->ParsingIsValid = false;
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
        T->ParsingIsValid = false;
    }
    return Result;
}

internal string
ReadString(assembly_tokenizer* T)
{
    string Result = {};
    if (AdvanceIfTokenEquals(T, "\""))
    {
        char* StringStart = T->At;
        while(AtValidPosition(T) && T->At[0] != '\"')
        {
            T->At++;
        }
        Result.Memory = StringStart;
        Result.Max = T->At - StringStart;
        Result.Length = Result.Max;
        if (AdvanceIfTokenEquals(T, "\""))
        {
            // Success
        }
        else
        {
            // TODO(Peter): Error
        }
    }
    return Result;
}

internal string
GetNumberString(assembly_tokenizer* T)
{
    string Result = {};
    Result.Memory = T->At;
    while(AtValidPosition(T) && IsNumericExtended(T->At[0]))
    {
        AdvanceChar(T);
    }
    Result.Length = T->At - Result.Memory;
    Result.Max = Result.Length;
    return Result;
}

internal r32
ReadFloat(assembly_tokenizer* T)
{
    r32 Result = 0;
    string NumberString = GetNumberString(T);
    parse_result ParsedFloat = ParseFloat(StringExpand(NumberString));
    Result = ParsedFloat.FloatValue;
    return Result;
}

internal s32
ReadInt(assembly_tokenizer* T)
{
    s32 Result = 0;
    string NumberString = GetNumberString(T);
    parse_result ParsedInt = ParseSignedInt(StringExpand(NumberString));
    Result = ParsedInt.SignedIntValue;
    return Result;
}

internal string
ReadStringField(assembly_field Field, assembly_tokenizer* T, memory_arena* Arena)
{
    string Result = {};
    if (ReadFieldIdentifier(Field, T))
    {
        string ExistingString = ReadString(T);
        if (ReadFieldEnd(T))
        {
            // Success
            Result = PushString(Arena, ExistingString.Length);
            CopyStringTo(ExistingString, &Result);
        }
        else
        {
            T->ParsingIsValid = false;
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
                        T->ParsingIsValid = false;
                    }
                }
                else
                {
                    T->ParsingIsValid = false;
                }
            }
            else
            {
                T->ParsingIsValid = false;
            }
        }
        else
        {
            T->ParsingIsValid = false;
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
ParseAssemblyFile(assembly* Assembly, string FileText, memory_arena* Transient)
{
    Assembly->LedCountTotal = 0;
    
    assembly_tokenizer Tokenizer = {};
    Tokenizer.Text = FileText;
    Tokenizer.At = Tokenizer.Text.Memory;
    Tokenizer.ParsingIsValid = true;
    
    Assembly->Name = ReadStringField(AssemblyField_AssemblyName, &Tokenizer, &Assembly->Arena);
    Assembly->Scale = ReadFloatField(AssemblyField_AssemblyScale, &Tokenizer);
    
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
            string PointPlacementType = ReadStringField(AssemblyField_PointPlacementType, &Tokenizer, &Assembly->Arena);
            // TODO(Peter): Switch on value of PointPlacementType
            if (ReadStructOpening(AssemblyField_InterpolatePoints, &Tokenizer))
            {
                StripAt->StartPosition = ReadV3Field(AssemblyField_Start, &Tokenizer);
                StripAt->EndPosition = ReadV3Field(AssemblyField_End, &Tokenizer);
                if (!ReadStructClosing(&Tokenizer))
                {
                    Tokenizer.ParsingIsValid = false;
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
                    // TODO(Peter): Need to store the string somewhere we can look it up for display in the interface
                    // right now they are stored in temp memory and won't persist
                    string TagName = ReadStringField(AssemblyField_Name, &Tokenizer, Transient);
                    string TagValue = ReadStringField(AssemblyField_Value, &Tokenizer, Transient);
                    TagAt->NameHash = HashString(TagName);
                    TagAt->ValueHash = HashString(TagValue);
                    if (!ReadStructClosing(&Tokenizer))
                    {
                        Tokenizer.ParsingIsValid = false;
                    }
                }
                else
                {
                    Tokenizer.ParsingIsValid = false;
                }
            }
            
            if (!ReadStructClosing(&Tokenizer))
            {
                Tokenizer.ParsingIsValid = false;
            }
        }
        else
        {
            Tokenizer.ParsingIsValid = false;
        }
    }
    
    return Tokenizer.ParsingIsValid;
}

#define ASSEMBLY_PARSER_CPP
#endif // ASSEMBLY_PARSER_CPP