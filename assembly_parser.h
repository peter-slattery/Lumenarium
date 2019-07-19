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
    s32 ControlBoxID;
    s32 StartUniverse, StartChannel;
    
    strip_interpolation_type InterpolationType;
    // Interpolate Boxes
    s32 StartBoxIndex, EndBoxIndex;
    // Interpolate Positions
    v3 InterpolatePositionStart, InterpolatePositionEnd;
    // Universal Interpolation
    s32 LEDsPerStrip;
};

struct assembly_definition
{
    s32 LEDStripSize;
    s32 LEDStripCount;
    led_strip_definition* LEDStrips;
};