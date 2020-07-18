//
// File: foldhaus_util_radialumia_file_converter.cpp
// Author: Peter Slattery
// Creation Date: 2020-01-01
//
#ifndef FOLDHAUS_UTIL_RADIALUMIA_FILE_CONVERTER_CPP

#define DEBUG
#define DEBUG_TRACK_SCOPE(name)

#include <stdlib.h>
#include <stdio.h>
#include <gs_string.h>

#include "gs/gs_language.h"
#include "gs/gs_string.h"
#include "../meta/gs_meta_lexer.h"
#include "gs/gs_vector.h"

#define gs_string_BUFFER_SIZE 512
struct gs_string_buffer
{
    char* Memory;
    s32 Size;
    gs_string_buffer* Next;
};

struct gs_string_writer
{
    char* Cursor;
    s32 UsedIngs_string;
    gs_string_buffer* Buffer;
};

internal gs_string_buffer*
Growgs_stringBuffer (gs_string_buffer* Buffer)
{
    gs_string_buffer* Result;
    if (Buffer->Next)
    {
        Result = Growgs_stringBuffer(Buffer->Next);
    }
    else
    {
        Result = (gs_string_buffer*)malloc(sizeof(gs_string_buffer));
        Result->Memory = (char*)malloc(sizeof(char) * gs_string_BUFFER_SIZE);
        memset(Result->Memory, 0, gs_string_BUFFER_SIZE);
        Result->Size = gs_string_BUFFER_SIZE;
        Result->Next = 0;
        
        Buffer->Next = Result;
    }
    return Result;
}

internal void
Writegs_string(gs_string_writer* Writer, char* gs_string, s32 Length)
{
    char* Src = gs_string;
    char* Dst = Writer->Cursor;
    s32 LengthWritten = 0;
    
    while (*Src && Writer->UsedIngs_string < Writer->Buffer->Size &&LengthWritten < Length)
    {
        LengthWritten++;
        *Dst++ = *Src++;
        Writer->UsedIngs_string++;
    }
    
    Writer->Cursor = Dst;
    
    if (*Src && Writer->UsedIngs_string == Writer->Buffer->Size)
    {
        *(Dst - 1) = 0; // Null terminate the buffer
        Writer->Buffer = Growgs_stringBuffer(Writer->Buffer);
        Writer->Cursor = Writer->Buffer->Memory;
        Writer->UsedIngs_string = 0;
        Writegs_string(Writer, (Src - 1), (Length - LengthWritten) + 1);
    }
}

struct control_box_pairs
{
    s32 Start;
    s32 End;
};

struct extra_strips
{
    s32 BoxID;
    v3 Start;
    v3 End;
};

struct control_box
{
    s32 ID;
    s32 Neighbors[6];
    r32 X;
    r32 Y;
    r32 Z;
    char* Address;
};

int main(int ArgCount, char* Args[])
{
    FILE* OldFilePtr = fopen("F:/data/radia_old.fold", "r");
    if (!OldFilePtr)
    {
        InvalidCodePath;
    }
    
    fseek(OldFilePtr, 0, SEEK_END);
    s32 OldFileSize = ftell(OldFilePtr);
    fseek(OldFilePtr, 0, SEEK_SET);
    
    char* OldFile = (char*)malloc(sizeof(char) * OldFileSize);
    fread(OldFile, 1, OldFileSize, OldFilePtr);
    
    fclose(OldFilePtr);
    
    s32 ControlBoxPairsUsed = 0;
    control_box_pairs* ControlBoxPairs = (control_box_pairs*)malloc(sizeof(control_box_pairs) * 512);
    s32 ControlBoxesUsed = 0;
    control_box* ControlBoxes = (control_box*)malloc(sizeof(control_box) * 64);
    s32 ExtraStripsUsed = 0;
    extra_strips* ExtraStrips = (extra_strips*)malloc(sizeof(extra_strips) * (42 * 4));
    
    control_box_pairs* NextControlBoxPair = &ControlBoxPairs[0];
    control_box* NextControlBox = &ControlBoxes[0];
    extra_strips* NextStrip = &ExtraStrips[0];
    
    tokenizer Tokenizer = {};
    Tokenizer.At = OldFile;
    while(*Tokenizer.At)
    {
        // Parse a Control Box
        memset(NextControlBox->Neighbors, -1, 6);
        s32 NeighborsAdded = 0;
        
        control_box_pairs* StartPair = NextControlBoxPair;
        s32 PairsCount = 0;
        
        if (gs_stringsEqual(Tokenizer.At, "EOF"))
        {
            break;
        }
        
        EatToCharacterInclusive(&Tokenizer, '{');
        EatWhitespace(&Tokenizer);
        Assert(gs_stringsEqual(Tokenizer.At, "neighbors: ["));
        Tokenizer.At += gs_stringLength("neighbors: [");
        
        // Parse Neighbors
        while(*Tokenizer.At && *Tokenizer.At != ']')
        {
            s32 NeighborIndex = ParseSignedInt(Tokenizer.At);
            NextControlBox->Neighbors[NeighborsAdded++] = NeighborIndex;
            NextControlBoxPair->End = NeighborIndex;
            NextControlBoxPair++;
            PairsCount++;
            ControlBoxPairsUsed++;
            
            EatNumber(&Tokenizer);
            if (*Tokenizer.At == ']')
            {
                Tokenizer.At += 2; // Eat past "];"
                break;
            }
            else
            {
                EatToCharacterInclusive(&Tokenizer, ',');
                EatWhitespace(&Tokenizer);
            }
        }
        
        EatWhitespace(&Tokenizer);
        
        //Parse IP
        Assert(gs_stringsEqual(Tokenizer.At, "ip: "));
        Tokenizer.At += gs_stringLength("ip: ");
        NextControlBox->Address = (char*)malloc(sizeof(char) * 13);
        memcpy(NextControlBox->Address, Tokenizer.At, 13);
        Tokenizer.At += 13;
        Tokenizer.At++; // Eat past ";"
        
        // Parse X
        EatWhitespace(&Tokenizer);
        Assert(gs_stringsEqual(Tokenizer.At, "x: "));
        Tokenizer.At += gs_stringLength("x: ");
        NextControlBox->X = ParseFloat(Tokenizer.At);
        EatToCharacterInclusive(&Tokenizer, ';');
        // Parse Y
        EatWhitespace(&Tokenizer);
        Assert(gs_stringsEqual(Tokenizer.At, "y: "));
        Tokenizer.At += gs_stringLength("y: ");
        NextControlBox->Y = ParseFloat(Tokenizer.At);
        EatToCharacterInclusive(&Tokenizer, ';');
        // Parse Z
        EatWhitespace(&Tokenizer);
        Assert(gs_stringsEqual(Tokenizer.At, "z: "));
        Tokenizer.At += gs_stringLength("z: ");
        NextControlBox->Z = ParseFloat(Tokenizer.At);
        EatToCharacterInclusive(&Tokenizer, ';');
        
        // Parse ID
        EatWhitespace(&Tokenizer);
        Assert(gs_stringsEqual(Tokenizer.At, "id: "));
        Tokenizer.At += gs_stringLength("id: ");
        NextControlBox->ID = ParseSignedInt(Tokenizer.At);
        EatToCharacterInclusive(&Tokenizer, ';');
        
        control_box_pairs* PairCursor = StartPair;
        for(s32 i = 0; i < PairsCount; i++)
        {
            PairCursor->Start = NextControlBox->ID;
            PairCursor++;
        }
        
        NextControlBox++;
        ControlBoxesUsed++;
        
        EatToCharacterInclusive(&Tokenizer, ';');
        EatWhitespace(&Tokenizer);
    }
    
    // Add Spikes
    
    
#define SPIKE_LEDS 346
    for (s32 sp = 0; sp < ControlBoxesUsed; sp++)
    {
        control_box* Box = &ControlBoxes[sp];
        
        control_box* NeighborA = &ControlBoxes[Box->Neighbors[0]];
        control_box* NeighborB = &ControlBoxes[Box->Neighbors[1]];
        
        v3 SpikeCenter = v3{Box->X, Box->Y, Box->Z};
        v3 StripPitch = Normalize(SpikeCenter) * ((2.f/8.f) / SPIKE_LEDS);
        v3 ToNA = Normalize(v3{NeighborA->X, NeighborA->Y, NeighborA->Z} - SpikeCenter);
        v3 ToNB = Normalize(v3{NeighborB->X, NeighborB->Y, NeighborB->Z} - SpikeCenter);
        
        v3 StripAOutStart = SpikeCenter + (ToNA * .01f);
        v3 StripAOutEnd = StripAOutStart + (StripPitch * SPIKE_LEDS);
        
        v3 StripBOutStart = SpikeCenter + (ToNB * .01f);
        v3 StripBOutEnd = StripBOutStart + (StripPitch * SPIKE_LEDS);
        
        v3 StripAInStart = StripAOutEnd - (ToNA * .02f);
        v3 StripAInEnd = StripAOutStart - (ToNA * .02f);
        
        v3 StripBInStart = StripBOutEnd - (ToNA * .02f);
        v3 StripBInEnd = StripBOutStart - (ToNA * .02f);
        
        NextStrip->BoxID = Box->ID;
        NextStrip->Start = StripAOutStart;
        NextStrip->End = StripAOutEnd;
        NextStrip++;
        ExtraStripsUsed++;
        
        NextStrip->BoxID = Box->ID;
        NextStrip->Start = StripAInStart;
        NextStrip->End = StripAInEnd;
        NextStrip++;
        ExtraStripsUsed++;
        
        NextStrip->BoxID = Box->ID;
        NextStrip->Start = StripBOutStart;
        NextStrip->End = StripBOutEnd;
        NextStrip++;
        ExtraStripsUsed++;
        
        NextStrip->BoxID = Box->ID;
        NextStrip->Start = StripBInStart;
        NextStrip->End = StripBInEnd;
        NextStrip++;
        ExtraStripsUsed++;
    }
    
    
    gs_string_buffer OutputFileBuffer = {};
    OutputFileBuffer.Memory = (char*)malloc(sizeof(char) * gs_string_BUFFER_SIZE);
    OutputFileBuffer.Size = gs_string_BUFFER_SIZE;
    OutputFileBuffer.Next = 0;
    
    gs_string_writer RefWriter = {};
    RefWriter.Cursor = OutputFileBuffer.Memory;
    RefWriter.UsedIngs_string = 0;
    RefWriter.Buffer = &OutputFileBuffer;
    gs_string_writer* Writer = &RefWriter;
    
    char gs_stringBuffer[512];
    s32 Len = 0;
    
    Len = sprintf_s(gs_stringBuffer, 512, "control_box_count %d\n", ControlBoxesUsed);
    Writegs_string(Writer, gs_stringBuffer, Len);
    Len = sprintf_s(gs_stringBuffer, 512, "led_strip_count %d\n\n", ControlBoxPairsUsed);
    Writegs_string(Writer, gs_stringBuffer, Len);
    
    for (s32 c = 0; c < ControlBoxesUsed; c++)
    {
        control_box* Box = ControlBoxes + c;
        Len = sprintf_s(gs_stringBuffer, 512,
                        "control_box { %d, \"%s\", (%f, %f, %f) }\n",
                        Box->ID, Box->Address,
                        Box->X, Box->Y, Box->Z);
        Writegs_string(Writer, gs_stringBuffer, Len);
    }
    
    Writegs_string(Writer, "\n", 1);
    
#define UNIVERSES_PER_BOX 25
    s32 UniversesPerBox[64];
    for (s32 u = 0; u < 64; u++)
    {
        UniversesPerBox[u] = UNIVERSES_PER_BOX * u;
    }
    
    char LEDStripFormatgs_string[] = "led_strip { %d, %d, %d, INTERPOLATE_POINTS, (%f, %f, %f), (%f, %f, %f), 144 } \n";
    for (s32 s = 0; s < ControlBoxPairsUsed; s++)
    {
        control_box_pairs* Pair = ControlBoxPairs + s;
        
        s32 Universe = UniversesPerBox[Pair->Start];
        UniversesPerBox[Pair->Start]++;
        
        r32 StartX = ControlBoxes[Pair->Start].X;
        r32 StartY = ControlBoxes[Pair->Start].Y;
        r32 StartZ = ControlBoxes[Pair->Start].Z;
        
        r32 EndX = ControlBoxes[Pair->End].X;
        r32 EndY = ControlBoxes[Pair->End].Y;
        r32 EndZ = ControlBoxes[Pair->End].Z;
        
        Len = sprintf_s(gs_stringBuffer, 512,
                        LEDStripFormatgs_string,
                        Pair->Start, Universe, 0,
                        StartX, StartY, StartZ,
                        EndX, EndY, EndZ);
        Writegs_string(Writer, gs_stringBuffer, Len);
    }
    
    Writegs_string(Writer, "\n", 1);
    
    for (s32 sp = 0; sp < ExtraStripsUsed; sp++)
    {
        extra_strips* Strip = ExtraStrips + sp;
        
        s32 Universe = UniversesPerBox[Strip->BoxID];
        UniversesPerBox[Strip->BoxID]++;
        
        Len = sprintf_s(gs_stringBuffer, 512,
                        LEDStripFormatgs_string,
                        Strip->BoxID, Universe, 0,
                        Strip->Start.x, Strip->Start.y, Strip->Start.z,
                        Strip->End.x, Strip->End.y, Strip->End.z);
        Writegs_string(Writer, gs_stringBuffer, Len);
    }
    
    Writegs_string(Writer, "END_OF_ASSEMBLY_FILE", gs_stringLength("END_OF_ASSEMBLY_FILE"));
    
    *Writer->Cursor = 0;
    
    FILE* OutputFile = fopen("F:/data/radialumia.fold", "w");
    gs_string_buffer* BufferCursor = &OutputFileBuffer;
    while(BufferCursor)
    {
        fprintf(OutputFile, BufferCursor->Memory);
        BufferCursor = BufferCursor->Next;
    }
    fclose(OutputFile);
    
    return 0;
}

#define FOLDHAUS_UTIL_RADIALUMIA_FILE_CONVERTER_CPP
#endif // FOLDHAUS_UTIL_RADIALUMIA_FILE_CONVERTER_CPP