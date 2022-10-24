//
// File: foldhaus_assembly_parser.cpp
// Author: Peter Slattery
// Creation Date: 2020-01-01
//
#ifndef FOLDHAUS_ASSEMBLY_PARSER_CPP

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
    
    AssemblyField_SegmentSequence,
    AssemblyField_SegmentSequenceLength,
    AssemblyField_Segment,
    
    AssemblyField_TagsCount,
    AssemblyField_Tag,
    AssemblyField_Name,
    AssemblyField_Value,
    
    AssemblyField_Count,
};

global gs_const_string AssemblyFieldIdentifiers[] = {
    ConstString("assembly_name"), // AssemblyField_AssemblyName
    ConstString("assembly_scale"), // AssemblyField_AssemblyScale
    ConstString("assembly_center"), // AssemblyField_AssemblyCenter
    ConstString("led_strip_count"), // AssemblyField_LedStripCount
    ConstString("output_mode"), // AssemblyField_OutputMode
    
    ConstString("led_strip"), // AssemblyField_LedStrip
    
    ConstString("output_sacn"), // AssemblyField_OutputSACN
    ConstString("start_universe"), // AssemblyField_SACN_StartUniverse
    ConstString("start_channel"), // AssemblyField_SACN_StartChannel
    
    ConstString("output_uart"), // AssemblyField_OutputUART
    ConstString("channel"), // AssemblyField_UART_Channel
    ConstString("com_port"), // AssemblyField_UART_ComPort
    
    ConstString("point_placement_type"), // AssemblyField_PointPlacementType
    
    ConstString("interpolate_points"), // AssemblyField_InterpolatePoints
    ConstString("start"), // AssemblyField_Start
    ConstString("end"), // AssemblyField_End
    ConstString("led_count"), // AssemblyField_LedCount
    
    ConstString("segment_sequence"), // AssemblyField_SegmentSequence
    ConstString("segment_count"), // AssemblyField_SegmentSequenceLength
    ConstString("segment"), // AssemblyField_Segment
    
    ConstString("tags_count"), // AssemblyField_TagCount
    ConstString("tag"), // AssemblyField_Tag
    ConstString("name"), // AssemblyField_Name
    ConstString("value"), // AssemblyField_Value
};

internal void
StripSetTag(v2_strip* Strip, u32 TagIndex, gs_const_string TagName, gs_const_string TagValue)
{
    Assert(TagIndex < Strip->TagsCount);
    v2_tag* TagAt = &Strip->Tags[TagIndex];
    TagAt->NameHash = HashDJB2ToU32(StringExpand(TagName));
    TagAt->ValueHash = HashDJB2ToU32(StringExpand(TagValue));
}

internal strip_sacn_addr
AssemblyParser_ReadSACNAddr(parser* Parser, assembly Assembly)
{
    strip_sacn_addr Result = {0};
    
    if (Parser_ReadOpenStruct(Parser, AssemblyField_OutputSACN))
    {
        Result.StartUniverse = Parser_ReadU32Value(Parser, AssemblyField_SACN_StartUniverse);
        Result.StartChannel = Parser_ReadU32Value(Parser, AssemblyField_SACN_StartChannel);
        
        if (!Parser_ReadCloseStruct(Parser))
        {
            //TokenizerPushError(&Tokenizer, "Struct doesn't close where expected");
        }
    }
    
    return Result;
}

internal strip_uart_addr
AssemblyParser_ReadUARTAddr(parser* Parser, assembly Assembly)
{
    strip_uart_addr Result = {0};
    
    if (Parser_ReadOpenStruct(Parser, AssemblyField_OutputUART))
    {
        Result.Channel = (u8)Parser_ReadU32Value(Parser, AssemblyField_UART_Channel);
        
        bool HasNetPort = Parser_ReadStringValue(Parser, AssemblyField_UART_ComPort, &Result.ComPort, true);
        if (Assembly.NetPortMode == NetworkPortMode_PortPerStrip && !HasNetPort)
        {
            Parser_PushErrorF(Parser, "NetPortMode for assembly is PortPerStrip, but this strip doesn't have an output port.");
        }
        
        if (!Parser_ReadCloseStruct(Parser))
        {
            Parser_PushErrorF(Parser, "Struct doesn't close where expected");
        }
    }
    
    return Result;
}

internal void
AssemblyParser_ReadTag(parser* Parser, v2_strip* StripAt, u32 TagIndex)
{
    if (Parser_ReadOpenStruct(Parser, AssemblyField_Tag))
    {
        // TODO(Peter): Need to store the gs_string somewhere we can look it up for display in the interface
        // right now they are stored in temp memory and won't persist
        gs_string TagName = Parser_ReadStringValue(Parser, AssemblyField_Name);
        gs_string TagValue = Parser_ReadStringValue(Parser, AssemblyField_Value);
        StripSetTag(StripAt, TagIndex, TagName.ConstString, TagValue.ConstString);
        if (!Parser_ReadCloseStruct(Parser))
        {
            Parser_PushErrorF(Parser, "Tag struct doesn't close where expected");
        }
    }
    else
    {
        Parser_PushErrorF(Parser, "Expected a tag struct, but none was found.");
    }
}


internal void
AssemblyParser_ReadTagList(parser* Parser, v2_strip* StripAt, assembly* Assembly)
{
    StripAt->TagsCount = Parser_ReadU32Value(Parser, AssemblyField_TagsCount);
    // NOTE(pjs): Always add one tag to the input to leave room for the assembly name
    StripAt->TagsCount += 1;
    StripAt->Tags = PushArray(&Assembly->Arena, v2_tag, StripAt->TagsCount);
    
    StripSetTag(StripAt, 0, ConstString("assembly"), Assembly->Name.ConstString);
    
    for (u32 Tag = 1; Tag < StripAt->TagsCount; Tag++)
    {
        AssemblyParser_ReadTag(Parser, StripAt, Tag);
    }
}

internal strip_gen_data AssemblyParser_ReadStripGenData(parser* Parser, assembly* Assembly);

internal strip_gen_interpolate_points
AssemblyParser_ReadInterpolatePoints(parser* Parser)
{
    strip_gen_interpolate_points Result = {0};
    if (Parser_ReadOpenStruct(Parser, AssemblyField_InterpolatePoints))
    {
        Result.StartPosition = Parser_ReadV3Value(Parser, AssemblyField_Start);
        Result.EndPosition = Parser_ReadV3Value(Parser, AssemblyField_End);
        Result.LedCount = Parser_ReadU32Value(Parser, AssemblyField_LedCount);
        if (!Parser_ReadCloseStruct(Parser))
        {
            // TODO(pjs):
        }
    }
    else
    {
        // TODO(pjs):
    }
    return Result;
}

internal strip_gen_sequence
AssemblyParser_ReadSequence(parser* Parser, assembly* Assembly)
{
    strip_gen_sequence Result = {0};
    if (Parser_ReadOpenStruct(Parser, AssemblyField_SegmentSequence))
    {
        Result.ElementsCount = Parser_ReadU32Value(Parser, AssemblyField_SegmentSequenceLength);
        Result.Elements = PushArray(&Assembly->Arena, strip_gen_data, Result.ElementsCount);
        for (u32 i = 0; i < Result.ElementsCount; i++)
        {
            Result.Elements[i] = AssemblyParser_ReadStripGenData(Parser, Assembly);
        }
        if (!Parser_ReadCloseStruct(Parser))
        {
            // TODO(pjs):
        }
    }
    else
    {
        // TODO(pjs):
    }
    return Result;
}

internal strip_gen_data
AssemblyParser_ReadStripGenData(parser* Parser, assembly* Assembly)
{
    strip_gen_data Result = {};
    
    if (Parser_ReadOpenStruct(Parser, AssemblyField_Segment))
    {
        gs_string PointPlacementType = Parser_ReadStringValue(Parser, AssemblyField_PointPlacementType);
        
        // TODO(pjs): We want to store enum strings in some unified way
        // :EnumStringsGen
        if (StringsEqual(PointPlacementType.ConstString, ConstString("InterpolatePoints")))
        {
            Result.Method = StripGeneration_InterpolatePoints;
            Result.InterpolatePoints = AssemblyParser_ReadInterpolatePoints(Parser);
        }
        else if (StringsEqual(PointPlacementType.ConstString,
                              ConstString("SegmentSequence")))
        {
            Result.Method = StripGeneration_Sequence;
            Result.Sequence = AssemblyParser_ReadSequence(Parser, Assembly);
        }
        else
        {
            Parser_PushErrorF(Parser, "Incorrect Point Placement Type found for segment");
        }
        
        if (!Parser_ReadCloseStruct(Parser))
        {
            Parser_PushErrorF(Parser, "Strip Gen Data did not close the struct where expected");
        }
    }
    
    return Result;
}

internal parser
ParseAssemblyFile(assembly* Assembly, gs_const_string FileName, gs_string FileText, gs_memory_arena* Transient)
{
    Assembly->LedCountTotal = 0;
    
    parser Parser = {0};
    Parser.FileName = FileName;
    Parser.String = FileText;
    Parser.Identifiers = &AssemblyFieldIdentifiers[0];
    Parser.IdentifiersCount = AssemblyField_Count;
    Parser.At = Parser.String.Str;
    Parser.LineStart = Parser.At;
    Parser.Arena = &Assembly->Arena;
    Parser.Transient = Transient;
    Parser.Success = true;
    
    Assembly->Name = Parser_ReadStringValue(&Parser, AssemblyField_AssemblyName);
    Assembly->Scale = Parser_ReadR32Value(&Parser, AssemblyField_AssemblyScale);
    Assembly->Center = Parser_ReadV3Value(&Parser, AssemblyField_AssemblyCenter) * Assembly->Scale;
    Assembly->StripCount = Parser_ReadU32Value(&Parser, AssemblyField_LedStripCount);
    Assembly->Strips = PushArray(&Assembly->Arena, v2_strip, Assembly->StripCount);
    
    gs_string OutputModeString = Parser_ReadStringValue(&Parser, AssemblyField_OutputMode);
    if (StringsEqual(OutputModeString.ConstString, ConstString("UART")))
    {
        Assembly->OutputMode = NetworkProtocol_UART;
        if (Parser_ReadStringValue(&Parser, AssemblyField_UART_ComPort, &Assembly->UARTComPort, true))
        {
            Assembly->NetPortMode = NetworkPortMode_GlobalPort;
        }
        else
        {
            Assembly->NetPortMode = NetworkPortMode_PortPerStrip;
        }
    }
    else if (StringsEqual(OutputModeString.ConstString, ConstString("SACN")))
    {
        Assembly->OutputMode = NetworkProtocol_SACN;
    }
    else
    {
        Parser_PushErrorF(&Parser, "Invalid output mode specified for assembly.");
        Parser.Success = false;
    }
    
    for (u32 i = 0; i < Assembly->StripCount; i++)
    {
        v2_strip* StripAt = Assembly->Strips + i;
        if (Parser_ReadOpenStruct(&Parser, AssemblyField_LedStrip))
        {
            StripAt->SACNAddr = AssemblyParser_ReadSACNAddr(&Parser, *Assembly);
            StripAt->UARTAddr = AssemblyParser_ReadUARTAddr(&Parser, *Assembly);
            StripAt->GenerationData = AssemblyParser_ReadStripGenData(&Parser, Assembly);
            StripAt->LedCount = StripGenData_CountLeds(StripAt->GenerationData);
            AssemblyParser_ReadTagList(&Parser, StripAt, Assembly);
            
            Assembly->LedCountTotal += StripAt->LedCount;
            
            if (!Parser_ReadCloseStruct(&Parser))
            {
                Parser_PushErrorF(&Parser, "Strip struct doesn't close where expected");
                Parser.Success = false;
            }
        }
        else
        {
            Parser_PushErrorF(&Parser, "Expected a strip struct but none was found");
            Parser.Success = false;
        }
    }
    
    return Parser;
}

#define FOLDHAUS_ASSEMBLY_PARSER_CPP
#endif // FOLDHAUS_ASSEMBLY_PARSER_CPP