//
// File: test_patterns.h
// Author: Peter Slattery
// Creation Date: 2020-01-01
//
#ifndef TEST_PATTERNS_H

GSMetaTag(node_struct);
struct solid_color_data
{
    GSMetaTag(node_input);
    v4 Color;
    
    GSMetaTag(node_output);
    color_buffer Result;
};

GSMetaTag(node_proc); // :TagParamsForNodeParamStructs
void SolidColorProc(solid_color_data* Data)
{
    u8 R = (u8)Clamp(0.f, (Data->Color.r * 255), 255.f);
    u8 G = (u8)Clamp(0.f, (Data->Color.g * 255), 255.f);
    u8 B = (u8)Clamp(0.f, (Data->Color.b * 255), 255.f);
    
    for (s32 LedIndex = 0; LedIndex < Data->Result.LEDCount; LedIndex++)
    {
        Assert(LedIndex >= 0 && LedIndex < Data->Result.LEDCount);
        
        Data->Result.Colors[LedIndex].R = R;
        Data->Result.Colors[LedIndex].G = G;
        Data->Result.Colors[LedIndex].B = B;
    }
}

GSMetaTag(node_struct);
struct vertical_color_fade_data
{
    GSMetaTag(node_input);
    v4 Color;
    
    GSMetaTag(node_input);
    r32 Min;
    
    GSMetaTag(node_input);
    r32 Max;
    
    GSMetaTag(node_output);
    color_buffer Result;
};

GSMetaTag(node_proc); // :TagParamsForNodeParamStructs
void VerticalColorFadeProc(vertical_color_fade_data* Data)
{
    r32 R = (Data->Color.r * 255);
    r32 G = (Data->Color.g * 255);
    r32 B = (Data->Color.b * 255);
    
    r32 Range = Data->Max - Data->Min;
    
    
    for (s32 LedIndex = 0; LedIndex < Data->Result.LEDCount; LedIndex++)
    {
        Assert(LedIndex >= 0 && LedIndex < Data->Result.LEDCount);
        v4 LedPosition = Data->Result.LedPositions[LedIndex];
        
        r32 Amount = (LedPosition.y - Data->Min) / Range;
        Amount = Clamp01(1.0f - Amount);
        
        Data->Result.Colors[LedIndex].R = (u8)(R * Amount);
        Data->Result.Colors[LedIndex].G = (u8)(G * Amount);
        Data->Result.Colors[LedIndex].B = (u8)(B * Amount);
    }
}

// Original -> DiscPatterns.pde : Revolving Discs
GSMetaTag(node_struct);
struct revolving_discs_data
{
    GSMetaTag(node_input);
    r32 Rotation;
    
    GSMetaTag(node_input);
    r32 ThetaZ;
    
    GSMetaTag(node_input);
    r32 ThetaY;
    
    GSMetaTag(node_input);
    r32 DiscWidth;
    
    GSMetaTag(node_input);
    r32 InnerRadius;
    
    GSMetaTag(node_input);
    r32 OuterRadius;
    
    GSMetaTag(node_input);
    v4 Color;
    
    GSMetaTag(node_output);
    color_buffer Result;
};

GSMetaTag(node_proc); // :TagParamsForNodeParamStructs
void RevolvingDiscs(revolving_discs_data* Data)
{
    DEBUG_TRACK_FUNCTION;
    
    pixel Color = {
        (u8)(Clamp01(Data->Color.r) * 255),
        (u8)(Clamp01(Data->Color.g) * 255),
        (u8)(Clamp01(Data->Color.b) * 255),
    };
    
    v4 Center = v4{0, 0, 0, 1};
    v4 Normal = v4{CosR32(Data->ThetaZ), 0, SinR32(Data->ThetaZ), 0}; // NOTE(Peter): dont' need to normalize. Should always be 1
    v4 Right = V4Cross(Normal, v4{0, 1, 0, 0});
    
    v4 FrontCenter = Center + (Normal * Data->DiscWidth);
    v4 BackCenter = Center - (Normal * Data->DiscWidth);
    
    r32 OuterRadiusSquared = Data->OuterRadius * Data->OuterRadius;
    r32 InnerRadiusSquared = Data->InnerRadius * Data->InnerRadius;
    
    for (s32 LedIndex = 0; LedIndex < Data->Result.LEDCount; LedIndex++)
    {
        v4 Position = Data->Result.LedPositions[LedIndex];
        
        v4 ToFront = Position + FrontCenter;
        v4 ToBack = Position + BackCenter;
        
        r32 ToFrontDotNormal = V4Dot(ToFront, Normal);
        r32 ToBackDotNormal = V4Dot(ToBack, Normal);
        
        ToFrontDotNormal = Clamp01(ToFrontDotNormal * 1000);
        ToBackDotNormal = Clamp01(ToBackDotNormal * 1000);
        
        r32 SqDistToCenter = V4MagSquared(Position);
        if (SqDistToCenter < OuterRadiusSquared && SqDistToCenter > InnerRadiusSquared)
        {
            if (XOR(ToFrontDotNormal > 0, ToBackDotNormal > 0))
            {
                Data->Result.Colors[LedIndex] = Color;
            }
        }
    }
}

#define TEST_PATTERNS_H
#endif // TEST_PATTERNS_H