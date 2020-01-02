//
// File: test_patterns.h
// Author: Peter Slattery
// Creation Date: 2020-01-01
//
#ifndef TEST_PATTERNS_H


NODE_STRUCT(solid_color_data)
{
    NODE_IN(v4, Color);
    NODE_COLOR_BUFFER_OUT(Result);
};

NODE_PROC(SolidColorProc, solid_color_data)
{
    u8 R = (u8)GSClamp(0.f, (Data->Color.r * 255), 255.f);
    u8 G = (u8)GSClamp(0.f, (Data->Color.g * 255), 255.f);
    u8 B = (u8)GSClamp(0.f, (Data->Color.b * 255), 255.f);
    
    led* LED = Data->ResultLEDs;
    for (s32 l = 0; l < Data->ResultLEDCount; l++)
    {
        Assert(LED->Index >= 0 && LED->Index < Data->ResultLEDCount);
        
        Data->ResultColors[LED->Index].R = R;
        Data->ResultColors[LED->Index].G = G;
        Data->ResultColors[LED->Index].B = B;
        LED++;
    }
}

NODE_STRUCT(vertical_color_fade_data)
{
    NODE_IN(v4, Color);
    NODE_IN(r32, Min);
    NODE_IN(r32, Max);
    NODE_COLOR_BUFFER_OUT(Result);
};

NODE_PROC(VerticalColorFadeProc, vertical_color_fade_data)
{
    r32 R = (Data->Color.r * 255);
    r32 G = (Data->Color.g * 255);
    r32 B = (Data->Color.b * 255);
    
    r32 Range = Data->Max - Data->Min;
    
    led* LED = Data->ResultLEDs;
    for (s32 l = 0; l < Data->ResultLEDCount; l++)
    {
        Assert(LED->Index >= 0 && LED->Index < Data->ResultLEDCount);
        
        r32 Amount = (LED->Position.y - Data->Min) / Range;
        Amount = GSClamp01(1.0f - Amount);
        
        Data->ResultColors[LED->Index].R = (u8)(R * Amount);
        Data->ResultColors[LED->Index].G = (u8)(G * Amount);
        Data->ResultColors[LED->Index].B = (u8)(B * Amount);
        LED++;
    }
}

// Original -> DiscPatterns.pde : Revolving Discs
NODE_STRUCT(revolving_discs_data)
{
    NODE_IN(r32, Rotation);
    NODE_IN(r32, ThetaZ);
    NODE_IN(r32, ThetaY);
    NODE_IN(r32, DiscWidth);
    NODE_IN(r32, InnerRadius);
    NODE_IN(r32, OuterRadius);
    NODE_IN(v4, Color);
    NODE_COLOR_BUFFER_OUT(Result);
};

NODE_PROC(RevolvingDiscs, revolving_discs_data)
{
    DEBUG_TRACK_FUNCTION;
    
    pixel Color = {
        (u8)(GSClamp01(Data->Color.r) * 255),
        (u8)(GSClamp01(Data->Color.g) * 255),
        (u8)(GSClamp01(Data->Color.b) * 255),
    };
    
    v4 Center = v4{0, 0, 0, 1};
    v4 Normal = v4{GSCos(Data->ThetaZ), 0, GSSin(Data->ThetaZ), 0}; // NOTE(Peter): dont' need to normalize. Should always be 1
    v4 Right = Cross(Normal, v4{0, 1, 0, 0});
    
    v4 FrontCenter = Center + (Normal * Data->DiscWidth);
    v4 BackCenter = Center - (Normal * Data->DiscWidth);
    
    r32 OuterRadiusSquared = Data->OuterRadius * Data->OuterRadius;
    r32 InnerRadiusSquared = Data->InnerRadius * Data->InnerRadius;
    
    led* LED = Data->ResultLEDs;
    for (s32 l = 0; l < Data->ResultLEDCount; l++)
    {
        v4 Position = LED->Position;
        
        v4 ToFront = Position + FrontCenter;
        v4 ToBack = Position + BackCenter;
        
        r32 ToFrontDotNormal = Dot(ToFront, Normal);
        r32 ToBackDotNormal = Dot(ToBack, Normal);
        
        ToFrontDotNormal = GSClamp01(ToFrontDotNormal * 1000);
        ToBackDotNormal = GSClamp01(ToBackDotNormal * 1000);
        
        r32 SqDistToCenter = MagSqr(Position);
        if (SqDistToCenter < OuterRadiusSquared && SqDistToCenter > InnerRadiusSquared)
        {
            if (XOR(ToFrontDotNormal > 0, ToBackDotNormal > 0))
            {
                Data->ResultColors[LED->Index] = Color;
            }
        }
        LED++;
    }
}

#define TEST_PATTERNS_H
#endif // TEST_PATTERNS_H