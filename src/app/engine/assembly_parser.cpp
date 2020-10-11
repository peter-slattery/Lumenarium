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

internal bool
ParseAssemblyFile(assembly* Assembly, gs_const_string FileName, gs_string FileText, gs_memory_arena* Transient)
{
    Assembly->LedCountTotal = 0;
    
    parser Parser = {0};
    Parser.String = FileText;
    Parser.Identifiers = &AssemblyFieldIdentifiers[0];
    Parser.IdentifiersCount = AssemblyField_Count;
    Parser.At = Parser.String.Str;
    Parser.LineStart = Parser.At;
    Parser.Arena = &Assembly->Arena;
    
    Assembly->Name = Parser_ReadStringValue(&Parser, AssemblyField_AssemblyName);
    Assembly->Scale = Parser_ReadR32Value(&Parser, AssemblyField_AssemblyScale);
    Assembly->Center = Parser_ReadV3Value(&Parser, AssemblyField_AssemblyCenter);
    Assembly->StripCount = Parser_ReadU32Value(&Parser, AssemblyField_LedStripCount);
    
    Assembly->Strips = PushArray(&Assembly->Arena, v2_strip, Assembly->StripCount);
    
    gs_string OutputModeString = Parser_ReadStringValue(&Parser, AssemblyField_OutputMode);
    if (StringsEqual(OutputModeString.ConstString, ConstString("UART")))
    {
        Assembly->OutputMode = NetworkProtocol_UART;
        Assembly->UARTComPort = Parser_ReadStringValue(&Parser, AssemblyField_UART_ComPort, true).ConstString;
    }
    else if (StringsEqual(OutputModeString.ConstString, ConstString("SACN")))
    {
        Assembly->OutputMode = NetworkProtocol_SACN;
    }
    else
    {
        //TokenizerPushError(&Tokenizer, "Invalid output mode specified.");
    }
    
    for (u32 i = 0; i < Assembly->StripCount; i++)
    {
        v2_strip* StripAt = Assembly->Strips + i;
        if (Parser_ReadOpenStruct(&Parser, AssemblyField_LedStrip))
        {
            if (Parser_ReadOpenStruct(&Parser, AssemblyField_OutputSACN))
            {
                StripAt->SACNAddr.StartUniverse = Parser_ReadU32Value(&Parser, AssemblyField_SACN_StartUniverse);
                StripAt->SACNAddr.StartChannel = Parser_ReadU32Value(&Parser, AssemblyField_SACN_StartChannel);
                
                if (!Parser_ReadCloseStruct(&Parser))
                {
                    //TokenizerPushError(&Tokenizer, "Struct doesn't close where expected");
                }
            }
            
            if (Parser_ReadOpenStruct(&Parser, AssemblyField_OutputUART))
            {
                StripAt->UARTAddr.Channel = (u8)Parser_ReadU32Value(&Parser, AssemblyField_UART_Channel);
                
                if (!Parser_ReadCloseStruct(&Parser))
                {
                    //TokenizerPushError(&Tokenizer, "Struct doesn't close where expected");
                }
            }
            
            // TODO(Peter): Need to store this
            gs_string PointPlacementType = Parser_ReadStringValue(&Parser, AssemblyField_PointPlacementType);
            // TODO(Peter): Switch on value of PointPlacementType
            if (Parser_ReadOpenStruct(&Parser, AssemblyField_InterpolatePoints))
            {
                StripAt->StartPosition = Parser_ReadV3Value(&Parser, AssemblyField_Start);
                StripAt->EndPosition = Parser_ReadV3Value(&Parser, AssemblyField_End);
                if (!Parser_ReadCloseStruct(&Parser))
                {
                    // TODO(Peter): @ErrorHandling
                    // Have this function prepend the filename and line number.
                    // Create an error display popup window, or an error log window that takes over a panel automatically
                    // TokenizerPushError(&Tokenizer, "Unable to read
                }
            }
            
            StripAt->LedCount = Parser_ReadU32Value(&Parser, AssemblyField_LedCount);
            Assembly->LedCountTotal += StripAt->LedCount;
            
            StripAt->TagsCount = Parser_ReadU32Value(&Parser, AssemblyField_TagsCount);
            // NOTE(pjs): Always add one tag to the input to leave room for the assembly name
            StripAt->TagsCount += 1;
            StripAt->Tags = PushArray(&Assembly->Arena, v2_tag, StripAt->TagsCount);
            StripSetTag(StripAt, 0, ConstString("assembly"), Assembly->Name.ConstString);
            for (u32 Tag = 1; Tag < StripAt->TagsCount; Tag++)
            {
                if (Parser_ReadOpenStruct(&Parser, AssemblyField_Tag))
                {
                    // TODO(Peter): Need to store the gs_string somewhere we can look it up for display in the interface
                    // right now they are stored in temp memory and won't persist
                    gs_string TagName = Parser_ReadStringValue(&Parser, AssemblyField_Name);
                    gs_string TagValue = Parser_ReadStringValue(&Parser, AssemblyField_Value);
                    StripSetTag(StripAt, Tag, TagName.ConstString, TagValue.ConstString);
                    if (!Parser_ReadCloseStruct(&Parser))
                    {
                        //TokenizerPushError(&Tokenizer, "Struct doesn't close where expected");
                    }
                }
                else
                {
                    //TokenizerPushError(&Tokenizer, "Expected a struct opening, but none was found");
                }
            }
            
            
            if (!Parser_ReadCloseStruct(&Parser))
            {
                //TokenizerPushError(&Tokenizer, "Struct doesn't close where expected");
            }
        }
        else
        {
            //TokenizerPushError(&Tokenizer, "Expected a struct opening, but none was found");
        }
    }
    
    return true; //Tokenizer.ParsingIsValid;
}

#define ASSEMBLY_PARSER_CPP
#endif // ASSEMBLY_PARSER_CPP