//
// File: gen_blumen_lumen.cpp
// Author: Peter Slattery
// Creation Date: 2021-01-06
//
#ifndef GEN_BLUMEN_LUMEN_CPP

#include <stdio.h>
#include <windows.h>

#include "../gs_libs/gs_types.h"
#include "../gs_libs/gs_types.cpp"
#include "../app/platform_win32/win32_foldhaus_utils.h"
#include "../app/platform_win32/win32_foldhaus_memory.h"
#include "../app/platform_win32/win32_foldhaus_fileio.h"
#include "../app/platform_win32/win32_foldhaus_work_queue.h"

#include "sculpture_gen.h"

typedef struct
{
    v3 CenterStart;
    v3 CenterEnd;
    r32 Radius;
    u32 SegmentsCount;
    u32 SubsegmentsCount;
    u32 SubsegmentLeds;
    
    // Only one of these two values is needed.
    // If ChannelsArray != 0, then it will be used, and assumed to
    // have SegmentsCount values
    // Otherwise, each segment will increment from ChannelStart
    u32 ChannelStart;
    u32* ChannelsArray;
    
    char* ComPort;
    char* SectionTagValue;
    char* FlowerTagValue;
} loop_desc;

internal void
BuildLoop(gs_string* OutputBuffer, loop_desc Desc)
{
    r32 SegmentsArc = TauR32 / Desc.SegmentsCount;
    r32 SubsegmentsArc = SegmentsArc / Desc.SubsegmentsCount;
    
    for (u32 i = 0; i < Desc.SegmentsCount; i++)
    {
        r32 ArcBase = SegmentsArc * i;
        
        u32 Channel = 0;
        if (Desc.ChannelsArray != 0)
        {
            Channel = Desc.ChannelsArray[i];
        }
        else
        {
            Channel = Desc.ChannelStart + i;
        }
        
        WriteLedStripOpen(OutputBuffer, Channel, Desc.ComPort);
        WriteSegmentSequenceOpen(OutputBuffer, Desc.SubsegmentsCount);
        
        for (u32 j = 0; j < Desc.SubsegmentsCount; j++)
        {
            r32 Arc = ArcBase + (SubsegmentsArc * j);
            v3 Offset = v3{ SinR32(Arc), 0, CosR32(Arc) } * Desc.Radius;
            v3 P0 = Desc.CenterStart + Offset;
            v3 P1 = Desc.CenterEnd + Offset;
            
            // Swap directions on the middle strip
            if (j%2 != 0)
            {
                v3 Temp = P0;
                P0 = P1;
                P1 = Temp;
            }
            
            WriteSegmentSequenceSegment(OutputBuffer, P0, P1, Desc.SubsegmentLeds);
        }
        
        WriteSegmentSequenceClose(OutputBuffer);
        WriteSegmentTagsOpen(OutputBuffer, 2);
        WriteSegmentTag(OutputBuffer, "section", Desc.SectionTagValue);
        WriteSegmentTag(OutputBuffer, "flower", Desc.FlowerTagValue);
        WriteSegmentTagsClose(OutputBuffer);
        WriteLedStripClose(OutputBuffer);
    }
    
}

typedef struct
{
    v3 Pos;
    char* ComPort;
    char* FlowerTagValue;
    u32* StemChannels;
    u32* BloomOuterChannels;
    u32* BloomInnerChannels;
} flower_desc;

internal void
BuildFlower(gs_string* OutputBuffer, flower_desc Desc)
{
    
#if 1
    // the bloom stem inner
    loop_desc BloomStemInner = {};
    BloomStemInner.CenterStart = v3{0, 1.4f, 0} + Desc.Pos;
    BloomStemInner.CenterEnd = v3{0, .9f, 0} + Desc.Pos;
    BloomStemInner.Radius = .05f;
    BloomStemInner.SegmentsCount = 6;
    BloomStemInner.SubsegmentsCount = 3;
    BloomStemInner.SubsegmentLeds = 35;
    BloomStemInner.ChannelsArray = Desc.BloomInnerChannels;
    BloomStemInner.ComPort = Desc.ComPort;
    BloomStemInner.SectionTagValue = "inner_bloom";
    BloomStemInner.FlowerTagValue = Desc.FlowerTagValue;
    BuildLoop(OutputBuffer, BloomStemInner);
    
    // the bloom stem outer
    loop_desc BloomStemOuter = {};
    BloomStemOuter.CenterStart = v3{0, .5f, 0} + Desc.Pos;
    BloomStemOuter.CenterEnd = v3{0, .9f, 0} + Desc.Pos;
    BloomStemOuter.Radius = .07f;
    BloomStemOuter.SegmentsCount = 9;
    BloomStemOuter.SubsegmentsCount = 3;
    BloomStemOuter.SubsegmentLeds = 41;
    BloomStemOuter.ChannelsArray = Desc.BloomOuterChannels;
    BloomStemOuter.ComPort = Desc.ComPort;
    BloomStemOuter.SectionTagValue = "outer_bloom";
    BloomStemOuter.FlowerTagValue = Desc.FlowerTagValue;
    BuildLoop(OutputBuffer, BloomStemOuter);
#endif
    
#if 1
    // the flower stem
    loop_desc FlowerStem = {};
    FlowerStem.CenterStart = v3{0, -1.5f, 0} + Desc.Pos;
    FlowerStem.CenterEnd = v3{0, .5f, 0} + Desc.Pos;
    FlowerStem.Radius = .05f;
    FlowerStem.SegmentsCount = 6;
    FlowerStem.SubsegmentsCount = 1;
    FlowerStem.SubsegmentLeds = 300;
    FlowerStem.ChannelsArray = Desc.StemChannels;
    FlowerStem.ComPort = Desc.ComPort;
    FlowerStem.SectionTagValue = "stem";
    FlowerStem.FlowerTagValue = Desc.FlowerTagValue;
    BuildLoop(OutputBuffer, FlowerStem);
#endif
    
}

// Just for brevity, no real function provided
#define FSC(f,c) FlowerStripToChannel((f), (c))
internal u8
FlowerStripToChannel(u8 Flower, u8 Channel)
{
    Assert(Flower < 3);
    Assert(Channel < 8);
    
    u8 Result = 0;
    Result |= (Flower & 0x03) << 3;
    Result |= (Channel & 0x07);
    
    return Result;
}

int main(int ArgCount, char** Args)
{
    gs_thread_context Ctx = Win32CreateThreadContext();
    
    gs_string OutputBuffer = PushString(Ctx.Transient, MB(4));
    
    char* ComPort = "\\\\.\\COM3";
    WriteAssemblyUARTOpen(&OutputBuffer,
                          "Blumen Lumen - Silver Spring",
                          100,
                          v3{0, 0, 0},
                          69,
                          "");
    
    u32 StemChannels[] = { FSC(2, 1), FSC(2, 2), FSC(2, 3), FSC(2, 4), FSC(2, 5), FSC(2, 6) };
    u32 BloomOuterChannels[] = { FSC(1, 0), FSC(1, 1), FSC(1, 2), FSC(1, 3), FSC(1, 4), FSC(1, 5), FSC(1, 6), FSC(1, 7), FSC(2, 7) };
    u32 BloomInnerChannels[] = { FSC(0, 0), FSC(0, 1), FSC(0, 2), FSC(0, 3), FSC(0, 4), FSC(0, 5) };
    flower_desc F0 = {};
    F0.Pos = v3{-1, 0, 0};
    F0.ComPort = ComPort;
    F0.FlowerTagValue = "left";
    F0.StemChannels = StemChannels;
    F0.BloomOuterChannels = BloomOuterChannels;
    F0.BloomInnerChannels = BloomInnerChannels;
    BuildFlower(&OutputBuffer, F0);
    
#if 1
    flower_desc F1 = {};
    F1.Pos = v3{0, 0, 0};
    F1.ComPort = "\\\\.\\COM6";
    F1.FlowerTagValue = "center";
    F1.StemChannels = StemChannels;
    F1.BloomInnerChannels = BloomInnerChannels;
    F1.BloomOuterChannels = BloomOuterChannels;
    BuildFlower(&OutputBuffer, F1);
#endif
    
    /*
        flower_desc F2 = {};
        F2.Pos = v3{1, 0, 0};
        F2.FlowerTagValue = "right";
        F2.StemChannels = StemChannels;
        F2.BloomInnerChannels = BloomInnerChannels;
        F2.BloomOuterChannels = BloomOuterChannels;
        BuildFlower(&OutputBuffer, F2);
        */
    
    printf("%.*s\n", (u32)OutputBuffer.Length, OutputBuffer.Str);
    
    return 0;
}


#define GEN_BLUMEN_LUMEN_CPP
#endif // GEN_BLUMEN_LUMEN_CPP