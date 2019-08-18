// This file left empty for SwD Kraftwerks


NODE_STRUCT(swd_color_data)
{
    NODE_IN(v4, Color);
    NODE_IN(v4, ColorB);
    NODE_COLOR_BUFFER_INOUT;
};

NODE_PROC(SwdColorProc, swd_color_data)
{
    u8 R = (u8)GSClamp(0.f, (Data->Color.r * 255), 255.f);
    u8 G = (u8)GSClamp(0.f, (Data->Color.g * 255), 255.f);
    u8 B = (u8)GSClamp(0.f, (Data->Color.b * 255), 255.f);
    
    led* LED = Data->LEDs;
    for (s32 l = 0; l < Data->LEDCount; l++)
    {
        Assert(LED->Index >= 0 && LED->Index < Data->LEDCount);
        
        Data->Colors[LED->Index].R = R;
        Data->Colors[LED->Index].G = R;
        Data->Colors[LED->Index].B = R;
        LED++;
    }
}
