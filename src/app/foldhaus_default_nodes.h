//
// File: foldhaus_default_nodes.h
// Author: Peter Slattery
// Creation Date: 2020-01-01
//
#ifndef FOLDHAUS_DEFAULT_NODES_H

//////////////////////////////////
//
//   Values
//
/////////////////////////////////

NODE_STRUCT(float_value_data)
{
    NODE_IN(r32, Value);
    NODE_OUT(r32, Result);
};

NODE_PROC(FloatValue, float_value_data)
{
    Data->Result = Data->Value;
}

NODE_STRUCT(vector_data)
{
    NODE_IN(r32, X);
    NODE_IN(r32, Y);
    NODE_IN(r32, Z);
    NODE_IN(r32, W);
    NODE_OUT(v4, Result);
};

NODE_PROC(VectorValue, vector_data)
{
    Data->Result = v4{Data->X, Data->Y, Data->Z, Data->W};
}

//////////////////////////////////
//
//   Arithmetic
//
/////////////////////////////////

NODE_STRUCT(multiply_data)
{
    NODE_IN(r32, A);
    NODE_IN(r32, B);
    NODE_OUT(r32, Result);
};

NODE_PROC(MultiplyNodeProc, multiply_data)
{
    Data->Result = Data->A * Data->B;
}

NODE_STRUCT(add_data)
{
    NODE_IN(v4, A);
    NODE_IN(v4, B);
    NODE_OUT(v4, Result);
};

NODE_PROC(AddNodeProc, add_data)
{
    Data->Result = Data->A + Data->B;
}

//////////////////////////////////
//
//   Animators
//
/////////////////////////////////

GSMetaTag(node_struct);
struct sin_wave_data
{
    GSMetaTag(node_input);
    r32 Period;
    
    GSMetaTag(node_input);
    r32 Min;
    
    GSMetaTag(node_input);
    r32 Max;
    
    GSMetaTag(node_input);
    r32 Result;
    
    r32 Accumulator;
};

NODE_PROC(SinWave, sin_wave_data)
{
    Data->Accumulator += DeltaTime;
    if (Data->Period > 0)
    {
        while (Data->Accumulator > Data->Period)
        {
            Data->Accumulator -= Data->Period;
        }
        
        r32 ActualMin = GSMin(Data->Min, Data->Max);
        r32 ActualMax = GSMax(Data->Min, Data->Max);
        r32 SinResult = GSSin((Data->Accumulator / Data->Period) * PI * 2);
        Data->Result = GSRemap(SinResult, -1.f, 1.f, ActualMin, ActualMax);
    }
    else
    {
        Data->Result = 0;
    }
}

//////////////////////////////////
//
//   Pattern Mixing
//
/////////////////////////////////

GSMetaTag(node_struct);
struct multiply_patterns_data
{
    GSMetaTag(node_input);
    color_buffer A;
    
    GSMetaTag(node_input);
    color_buffer B;
    
    GSMetaTag(node_output);
    color_buffer Result;
};

NODE_PROC(MultiplyPatterns, multiply_patterns_data)
{
    for (s32 LedIndex = 0; LedIndex < Data->Result.LEDCount; LedIndex++)
    {
        Assert(LedIndex >= 0 && LedIndex < Data->Result.LEDCount);
        
        s32 AR = Data->A.Colors[LedIndex].R;
        s32 AG = Data->A.Colors[LedIndex].G;
        s32 AB = Data->A.Colors[LedIndex].B;
        
        s32 BR = Data->B.Colors[LedIndex].R;
        s32 BG = Data->B.Colors[LedIndex].G;
        s32 BB = Data->B.Colors[LedIndex].B;
        
        s32 RCombined = (AR * BR) / 255;
        s32 GCombined = (AG * BG) / 255;
        s32 BCombined = (AB * BB) / 255;
        
        Data->Result.Colors[LedIndex].R = (u8)RCombined;
        Data->Result.Colors[LedIndex].G = (u8)GCombined;
        Data->Result.Colors[LedIndex].B = (u8)BCombined;
    }
}


#define FOLDHAUS_DEFAULT_NODES_H
#endif // FOLDHAUS_DEFAULT_NODES_H