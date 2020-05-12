//
// File: assembly_parser.h
// Author: Peter Slattery
// Creation Date: 2020-01-01
//
#ifndef ASSEMBLY_PARSER_H

#define LED_STRIP_COUNT_IDENTIFIER "led_strip_count"

#define LED_STRIP_IDENTIFIER "led_strip"

#define INTERPOLATE_POINTS_IDENTIFIER "INTERPOLATE_POINTS"

#define END_ASSEMBLY_FILE_IDENTIFIER "END_OF_ASSEMBLY_FILE"

enum assembly_token_type
{
    AssemblyToken_Colon,
    AssemblyToken_SemiColon,
    AssemblyToken_LeftCurlyBrace,
    AssemblyToken_RightCurlyBrace,
    AssemblyToken_Comma,
    
    AssemblyToken_Number,
    AssemblyToken_String,
    AssemblyToken_Vector,
    
    AssemblyToken_LEDStrip,
    
    AssemblyToken_Identifier,
    
    AssemblyToken_EndOfFile
};

struct assembly_token
{
    char* Token;
    s32 Length;
    assembly_token_type Type;
};

enum strip_interpolation_type
{
    StripInterpolate_Boxes,
    StripInterpolate_Points,
};

struct led_strip_definition
{
    u32 ControlBoxID;
    u32 StartUniverse;
    u32 StartChannel;
    
    strip_interpolation_type InterpolationType;
    // Interpolate Boxes
    u32 StartBoxIndex;
    u32 EndBoxIndex;
    
    // Interpolate Positions
    v3 InterpolatePositionStart;
    v3 InterpolatePositionEnd;
    
    // Universal Interpolation
    u32 LEDsPerStrip;
};

struct assembly_definition
{
    u32 LEDStripSize;
    u32 LEDStripCount;
    u32 TotalLEDCount;
    led_strip_definition* LEDStrips;
};

#define ASSEMBLY_PARSER_H
#endif // ASSEMBLY_PARSER_H