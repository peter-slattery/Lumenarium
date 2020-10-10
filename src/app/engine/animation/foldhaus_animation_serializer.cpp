//
// File: foldhaus_animation_serializer.cpp
// Author: Peter Slattery
// Creation Date: 2020-10-04
//
#ifndef FOLDHAUS_ANIMATION_SERIALIZER_CPP

internal gs_string
AnimSerializer_Serialize(animation Anim, animation_clip* GlobalClips, gs_memory_arena* Arena)
{
    serializer Serializer = {0};
    Serializer.String = PushString(Arena, 4096);
    Serializer.Identifiers = AnimationFieldStrings;
    Serializer.IdentifiersCount = AnimField_Count;
    
    Serializer_WriteF(&Serializer, "%S;\n", AnimationFieldStrings[AnimField_FileIdent]);
    Serializer_WriteStringValue(&Serializer, AnimField_AnimName, Anim.Name.ConstString);
    Serializer_WriteValue(&Serializer, AnimField_LayersCount, Anim.Layers.Count);
    Serializer_WriteValue(&Serializer, AnimField_BlocksCount, Anim.Blocks.Used);
    
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
    for (u32 i = 0; i < Anim.Blocks.Used; i++)
    {
        gs_list_entry<animation_block>* AnimationBlockEntry = Anim.Blocks.GetEntryAtIndex(i);
        if (EntryIsFree(AnimationBlockEntry)) { continue; }
        gs_list_handle CurrentBlockHandle = AnimationBlockEntry->Handle;
        animation_block AnimationBlockAt = AnimationBlockEntry->Value;
        
        // TODO(pjs): Systematize the AnimationProcHandle
        // :AnimProcHandle
        u32 AnimationProcIndex = AnimationBlockAt.AnimationProcHandle - 1;
        animation_clip Animation = GlobalClips[AnimationProcIndex];
        
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
AnimParser_Parse(gs_string File, animation_clip* GlobalClips, gs_memory_arena* Arena, u32 AnimClipsCount, animation_clip* AnimClips)
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
        
        // TODO(pjs): We're not using this now because Blocks are built on gs_list or something,
        // but I want to replace that eventually, so this is here to preallocate the blocks we need
        u32 BlocksCount = Parser_ReadU32Value(&Parser, AnimField_BlocksCount);
        
        if (Parser_ReadOpenStruct(&Parser, AnimField_PlayableRange))
        {
            Result.PlayableRange.Min = Parser_ReadU32Value(&Parser, AnimField_PlayableRangeMin);
            Result.PlayableRange.Max = Parser_ReadU32Value(&Parser, AnimField_PlayableRangeMax);
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
                    Block.AnimationProcHandle = 0;
                    for (u32 i = 0; i < AnimClipsCount; i++)
                    {
                        if (StringEqualsCharArray(AnimName.ConstString, AnimClips[i].Name, CStringLength(AnimClips[i].Name)))
                        {
                            Block.AnimationProcHandle = i + 1;
                            break;
                        }
                    }
                    
                    if (Parser_ReadCloseStruct(&Parser))
                    {
                        Result.Blocks.PushElementOnList(Block);
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