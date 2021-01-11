//
// File: foldhaus_animation_serializer.cpp
// Author: Peter Slattery
// Creation Date: 2020-10-04
//
#ifndef FOLDHAUS_ANIMATION_SERIALIZER_CPP

internal gs_string
AnimSerializer_Serialize(animation Anim, animation_pattern_array Patterns, gs_memory_arena* Arena)
{
    serializer Serializer = {0};
    Serializer.String = PushString(Arena, 4096);
    Serializer.Identifiers = AnimationFieldStrings;
    Serializer.IdentifiersCount = AnimField_Count;
    
    Serializer_WriteF(&Serializer, "%S;\n", AnimationFieldStrings[AnimField_FileIdent]);
    Serializer_WriteStringValue(&Serializer, AnimField_AnimName, Anim.Name.ConstString);
    Serializer_WriteValue(&Serializer, AnimField_LayersCount, Anim.Layers.Count);
    Serializer_WriteValue(&Serializer, AnimField_BlocksCount, Anim.Blocks_.Count);
    
    Serializer_OpenStruct(&Serializer, AnimField_PlayableRange);
    {
        Serializer_WriteValue(&Serializer, AnimField_PlayableRangeMin, (u32)Anim.PlayableRange.Min);
        Serializer_WriteValue(&Serializer, AnimField_PlayableRangeMax, (u32)Anim.PlayableRange.Max);
    }
    Serializer_CloseStruct(&Serializer);
    
    Serializer_OpenStruct(&Serializer, AnimField_LayersArray);
    for (u32 i = 0; i < Anim.Layers.Count; i++)
    {
        anim_layer LayerAt = Anim.Layers.Values[i];
        Serializer_OpenStruct(&Serializer, AnimField_Layer);
        {
            Serializer_WriteStringValue(&Serializer, AnimField_LayerName, LayerAt.Name.ConstString);
            Serializer_WriteStringValue(&Serializer, AnimField_LayerBlendMode, BlendModeStrings[LayerAt.BlendMode]);
        }
        Serializer_CloseStruct(&Serializer);
    }
    Serializer_CloseStruct(&Serializer);
    
    
    Serializer_OpenStruct(&Serializer, AnimField_BlocksArray);
    for (u32 i = 0; i < Anim.Blocks_.Count; i++)
    {
        // TODO(pjs): Handle free'd animation blocks
        animation_block AnimationBlockAt = Anim.Blocks_.Values[i];
        
        animation_pattern Animation = Patterns_GetPattern(Patterns, AnimationBlockAt.AnimationProcHandle);
        
        Serializer_OpenStruct(&Serializer, AnimField_Block);
        {
            Serializer_OpenStruct(&Serializer, AnimField_BlockFrameRange);
            {
                Serializer_WriteValue(&Serializer, AnimField_BlockFrameRangeMin, (u32)AnimationBlockAt.Range.Min);
                Serializer_WriteValue(&Serializer, AnimField_BlockFrameRangeMax, (u32)AnimationBlockAt.Range.Max);
            }
            Serializer_CloseStruct(&Serializer);
            
            Serializer_WriteValue(&Serializer, AnimField_BlockLayerIndex, AnimationBlockAt.Layer);
            Serializer_WriteStringValue(&Serializer, AnimField_BlockAnimName, ConstString(Animation.Name));
        }
        Serializer_CloseStruct(&Serializer);
    }
    Serializer_CloseStruct(&Serializer);
    
    return Serializer.String;
}

internal animation
AnimParser_Parse(gs_string File, gs_memory_arena* Arena, animation_pattern_array AnimPatterns)
{
    animation Result = {0};
    
    parser Parser = {0};
    Parser.String = File;
    Parser.At = Parser.String.Str;
    Parser.Identifiers = AnimationFieldStrings;
    Parser.IdentifiersCount = AnimField_Count;
    Parser.Arena = Arena;
    
    if (Parser_ReadString(&Parser, AnimationFieldStrings[AnimField_FileIdent]))
    {
        Result.Name = Parser_ReadStringValue(&Parser, AnimField_AnimName);
        
        Result.Layers.CountMax = Parser_ReadU32Value(&Parser, AnimField_LayersCount);
        Result.Layers.Values = PushArray(Arena, anim_layer, Result.Layers.CountMax);
        
        Result.Blocks_.CountMax = Parser_ReadU32Value(&Parser, AnimField_BlocksCount);
        Result.Blocks_.Generations = PushArray(Arena, u32, Result.Blocks_.CountMax);
        Result.Blocks_.Values = PushArray(Arena, animation_block, Result.Blocks_.CountMax);
        
        if (Parser_ReadOpenStruct(&Parser, AnimField_PlayableRange))
        {
            Result.PlayableRange.Min = Parser_ReadU32Value(&Parser, AnimField_PlayableRangeMin);
            Result.PlayableRange.Max = Parser_ReadU32Value(&Parser, AnimField_PlayableRangeMax);
            
            if (Parser_ReadCloseStruct(&Parser))
            {
                // TODO(pjs): Error
            }
        }
        else
        {
            // TODO(pjs): Error
        }
        
        if (Parser_ReadOpenStruct(&Parser, AnimField_LayersArray))
        {
            while (!Parser_ReadCloseStruct(&Parser))
            {
                anim_layer Layer = {0};
                if (Parser_ReadOpenStruct(&Parser, AnimField_Layer))
                {
                    Layer.Name = Parser_ReadStringValue(&Parser, AnimField_LayerName);
                    
                    gs_string BlendModeName = Parser_ReadStringValue(&Parser, AnimField_LayerBlendMode);
                    for (u32 i = 0; i < BlendMode_Count; i++)
                    {
                        if (StringsEqual(BlendModeName.ConstString, BlendModeStrings[i]))
                        {
                            Layer.BlendMode = (blend_mode)i;
                            break;
                        }
                    }
                    
                    if (Parser_ReadCloseStruct(&Parser))
                    {
                        Animation_AddLayer(&Result, Layer);
                    }
                    else
                    {
                        // TODO(pjs): Error
                    }
                }
            }
        }
        
        if (Parser_ReadOpenStruct(&Parser, AnimField_BlocksArray))
        {
            while(!Parser_ReadCloseStruct(&Parser))
            {
                animation_block Block = {0};
                
                if (Parser_ReadOpenStruct(&Parser, AnimField_Block))
                {
                    if (Parser_ReadOpenStruct(&Parser, AnimField_BlockFrameRange))
                    {
                        Block.Range.Min = Parser_ReadU32Value(&Parser, AnimField_BlockFrameRangeMin);
                        Block.Range.Max = Parser_ReadU32Value(&Parser, AnimField_BlockFrameRangeMax);
                        
                        Parser_ReadCloseStruct(&Parser);
                    }
                    else
                    {
                        // TODO(pjs): Error
                    }
                    
                    Block.Layer = Parser_ReadU32Value(&Parser, AnimField_BlockLayerIndex);
                    
                    // TODO(pjs): AnimName -> Animation Proc Handle
                    gs_string AnimName = Parser_ReadStringValue(&Parser, AnimField_BlockAnimName);
                    Block.AnimationProcHandle = {0};
                    for (u32 i = 0; i < AnimPatterns.Count; i++)
                    {
                        animation_pattern Pattern = AnimPatterns.Values[i];
                        if (StringEqualsCharArray(AnimName.ConstString, Pattern.Name, Pattern.NameLength))
                        {
                            Block.AnimationProcHandle = Patterns_IndexToHandle(i);
                            break;
                        }
                    }
                    
                    if (Parser_ReadCloseStruct(&Parser))
                    {
                        AnimBlockArray_Push(&Result.Blocks_, Block);
                    }
                    else
                    {
                        // TODO(pjs): Error
                    }
                }
            }
        }
    }
    
    return Result;
}

#define FOLDHAUS_ANIMATION_SERIALIZER_CPP
#endif // FOLDHAUS_ANIMATION_SERIALIZER_CPP