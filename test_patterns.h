NODE_STRUCT(float_value_data)
{
    NODE_IN(r32, Value);
    NODE_OUT(r32, Result);
};

NODE_PROC(FloatValueProc, float_value_data)
{
    Data->Result = Data->Value;
}

NODE_STRUCT(solid_color_data)
{
    NODE_IN(v4, Color);
    NODE_COLOR_BUFFER_INOUT;
};

NODE_PROC(SolidColorProc, solid_color_data)
{
    u8 R = (u8)(Data->Color.r * 255);
    u8 G = (u8)(Data->Color.g * 255);
    u8 B = (u8)(Data->Color.b * 255);
    
    led* LED = Data->LEDs;
    for (s32 l = 0; l < Data->LEDCount; l++)
    {
        Assert(LED->Index >= 0 && LED->Index < Data->LEDCount);
        
        Data->Colors[LED->Index].R = R;
        Data->Colors[LED->Index].G = G;
        Data->Colors[LED->Index].B = B;
        LED++;
    }
}

NODE_STRUCT(multiply_patterns_data)
{
    NODE_COLOR_BUFFER_IN(A);
    NODE_COLOR_BUFFER_IN(B);
    NODE_COLOR_BUFFER_OUT(Result);
};

NODE_PROC(MultiplyPatterns, multiply_patterns_data)
{
    led* LED = Data->ResultLEDs;
    for (s32 l = 0; l < Data->ResultLEDCount; l++)
    {
        Assert(LED->Index >= 0 && LED->Index < Data->ResultLEDCount);
        
        Data->ResultColors[LED->Index].R = (Data->AColors[LED->Index].R + Data->BColors[LED->Index].R) / 2;
        Data->ResultColors[LED->Index].G = (Data->AColors[LED->Index].G + Data->BColors[LED->Index].G) / 2;
        Data->ResultColors[LED->Index].B = (Data->AColors[LED->Index].B + Data->BColors[LED->Index].B) / 2;
        LED++;
    }
}


NODE_PATTERN_STRUCT(vertical_color_fade_data)
{
    NODE_IN(v4, Color);
    NODE_IN(r32, Min);
    NODE_IN(r32, Max);
};

NODE_PATTERN_PROC(VerticalColorFadeProc, vertical_color_fade_data)
{
    r32 R = (Data->Color.r * 255);
    r32 G = (Data->Color.g * 255);
    r32 B = (Data->Color.b * 255);
    
    r32 Range = Data->Max - Data->Min;
    
    led* LED = LEDs;
    for (s32 l = 0; l < LEDCount; l++)
    {
        r32 Amount = (LED->Position.y - Data->Min) / Range;
        Amount = GSClamp01(1.0f - Amount);
        
        Colors[LED->Index].R = (u8)(R * Amount);
        Colors[LED->Index].G = (u8)(G * Amount);
        Colors[LED->Index].B = (u8)(B * Amount);
        LED++;
    }
}

// ^^^ New ^^^
// vvv Old vvv

PATTERN_INIT_PROC(SolidPatternInitProc)
{
    Pattern->Memory = (void*)PushArray(Storage, u8, 3);
    
    u8* Color = (u8*)Pattern->Memory;
    Color[0] = 0;
    Color[1] = 0;
    Color[2] = 128; 
}

PATTERN_UPDATE_PROC(SolidPatternUpdateProc)
{
    u8* Color = (u8*)Memory;
    u8 R = Color[0];
    u8 G = Color[1];
    u8 B = Color[2];
    
    led* LED = LEDs;
    for (s32 l = 0; l < LEDCount; l++)
    {
        PushColor(LED++, Colors, R, G, B);
    }
}

struct rainbow_pattern_memory
{
    r32 TimeAccumulator;
    r32 Period;
};

PATTERN_INIT_PROC(InitRainbowPatternProc)
{
    Pattern->Memory = (void*)PushStruct(Storage, rainbow_pattern_memory);
    rainbow_pattern_memory* Mem = (rainbow_pattern_memory*)Pattern->Memory;
    Mem->TimeAccumulator = 0;
    Mem->Period = 6.0f;
}

PATTERN_UPDATE_PROC(RainbowPatternProc)
{
    DEBUG_TRACK_SCOPE(RainbowPatternProc);
    
    rainbow_pattern_memory* Mem = (rainbow_pattern_memory*)Memory;
    Mem->TimeAccumulator += DeltaTime;
    if (Mem->TimeAccumulator >= Mem->Period)
    {
        Mem->TimeAccumulator -= Mem->Period;
    }
    
    r32 Percent = Mem->TimeAccumulator / Mem->Period;
    r32 HueAdd = Percent * 360.0f;
    
    r32 HueScale = 360.0f / 100;
    
    led* LED = LEDs;
    for (s32 l = 0; l < LEDCount; l++)
    {
        r32 Hue = (LED->Position.y * HueScale) + HueAdd;
        v4 Color = HSVToRGB(v4{Hue, 1, 1, 1}) * .75f;
        
        PushColor(LED++, Colors, (u8)(Color.r * 255), (u8)(Color.g * 255), (u8)(Color.b * 255));
    }
}

PATTERN_INIT_PROC(InitRadialProc)
{
    Pattern->Memory = (void*)PushStruct(Storage, rainbow_pattern_memory);
    rainbow_pattern_memory* Mem = (rainbow_pattern_memory*)Pattern->Memory;
    Mem->TimeAccumulator = 0;
    Mem->Period = 10.0;
}

PATTERN_UPDATE_PROC(UpdateRadialProc)
{
    
    rainbow_pattern_memory* Mem = (rainbow_pattern_memory*)Memory;
    Mem->TimeAccumulator += DeltaTime;
    if (Mem->TimeAccumulator >= Mem->Period)
    {
        Mem->TimeAccumulator -= Mem->Period;
    }
    
    r32 Percent = Mem->TimeAccumulator / Mem->Period;
    r32 AngleAdd = Percent * PI * 2;
    r32 HueAdd = Percent * 360;
    
    v2 DirectionVector = v2{GSSin(AngleAdd), GSCos(AngleAdd)};
    
    led* LED = LEDs;
    for (s32 l = 0; l < LEDCount; l++)
    {
        v4 Color = {0, 0, 0, 1};
        
        if (LED->Position.y >= 70)
        {
            v2 TwoDPos = v2{LED->Position.x, LED->Position.z};
            r32 Angle = Dot(Normalize(TwoDPos), DirectionVector) * .25f;
            r32 Hue = Angle * 360 + HueAdd;
            Color = HSVToRGB(v4{Hue, 1, 1, 1}) * .9f;
        }
        else
        {
            Color = HSVToRGB(v4{HueAdd, 1, 1, 1}) * .9f;
        }
        
        PushColor(LED++, Colors, (u8)(Color.r * 255), (u8)(Color.g * 255), (u8)(Color.b * 255));
    }
}