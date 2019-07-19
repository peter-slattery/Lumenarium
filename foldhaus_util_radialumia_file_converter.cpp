#define DEBUG
#define DEBUG_TRACK_SCOPE(name)

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "gs/gs_language.h"
#include "gs/gs_string.h"
#include "../meta/gs_meta_lexer.h"
#include "gs/gs_vector.h"

#define STRING_BUFFER_SIZE 512
struct string_buffer
{
    char* Memory;
    s32 Size;
    string_buffer* Next;
};

struct string_writer
{
    char* Cursor;
    s32 UsedInString;
    string_buffer* Buffer;
};

internal string_buffer*
GrowStringBuffer (string_buffer* Buffer)
{
    string_buffer* Result;
    if (Buffer->Next)
    {
        Result = GrowStringBuffer(Buffer->Next);
    }
    else
    {
        Result = (string_buffer*)malloc(sizeof(string_buffer));
        Result->Memory = (char*)malloc(sizeof(char) * STRING_BUFFER_SIZE);
        memset(Result->Memory, 0, STRING_BUFFER_SIZE);
        Result->Size = STRING_BUFFER_SIZE;
        Result->Next = 0;
        
        Buffer->Next = Result;
    }
    return Result;
}

internal void
WriteString(string_writer* Writer, char* String, s32 Length)
{
    char* Src = String;
    char* Dst = Writer->Cursor;
    s32 LengthWritten = 0;
    
    while (*Src && Writer->UsedInString < Writer->Buffer->Size &&LengthWritten < Length)
    {
        LengthWritten++;
        *Dst++ = *Src++;
        Writer->UsedInString++;
    }
    
    Writer->Cursor = Dst;
    
    if (*Src && Writer->UsedInString == Writer->Buffer->Size)
    {
        *(Dst - 1) = 0; // Null terminate the buffer
        Writer->Buffer = GrowStringBuffer(Writer->Buffer);
        Writer->Cursor = Writer->Buffer->Memory;
        Writer->UsedInString = 0;
        WriteString(Writer, (Src - 1), (Length - LengthWritten) + 1);
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
        
        if (StringsEqual(Tokenizer.At, "EOF"))
        {
            break;
        }
        
        EatToCharacterInclusive(&Tokenizer, '{');
        EatWhitespace(&Tokenizer);
        Assert(StringsEqual(Tokenizer.At, "neighbors: ["));
        Tokenizer.At += StringLength("neighbors: [");
        
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
        Assert(StringsEqual(Tokenizer.At, "ip: "));
        Tokenizer.At += StringLength("ip: ");
        NextControlBox->Address = (char*)malloc(sizeof(char) * 13);
        memcpy(NextControlBox->Address, Tokenizer.At, 13);
        Tokenizer.At += 13;
        Tokenizer.At++; // Eat past ";"
        
        // Parse X
        EatWhitespace(&Tokenizer);
        Assert(StringsEqual(Tokenizer.At, "x: "));
        Tokenizer.At += StringLength("x: ");
        NextControlBox->X = ParseFloat(Tokenizer.At);
        EatToCharacterInclusive(&Tokenizer, ';');
        // Parse Y
        EatWhitespace(&Tokenizer);
        Assert(StringsEqual(Tokenizer.At, "y: "));
        Tokenizer.At += StringLength("y: ");
        NextControlBox->Y = ParseFloat(Tokenizer.At);
        EatToCharacterInclusive(&Tokenizer, ';');
        // Parse Z
        EatWhitespace(&Tokenizer);
        Assert(StringsEqual(Tokenizer.At, "z: "));
        Tokenizer.At += StringLength("z: ");
        NextControlBox->Z = ParseFloat(Tokenizer.At);
        EatToCharacterInclusive(&Tokenizer, ';');
        
        // Parse ID
        EatWhitespace(&Tokenizer);
        Assert(StringsEqual(Tokenizer.At, "id: "));
        Tokenizer.At += StringLength("id: ");
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
    
    
    string_buffer OutputFileBuffer = {};
    OutputFileBuffer.Memory = (char*)malloc(sizeof(char) * STRING_BUFFER_SIZE);
    OutputFileBuffer.Size = STRING_BUFFER_SIZE;
    OutputFileBuffer.Next = 0;
    
    string_writer RefWriter = {};
    RefWriter.Cursor = OutputFileBuffer.Memory;
    RefWriter.UsedInString = 0;
    RefWriter.Buffer = &OutputFileBuffer;
    string_writer* Writer = &RefWriter;
    
    char StringBuffer[512];
    s32 Len = 0;
    
    Len = sprintf_s(StringBuffer, 512, "control_box_count %d\n", ControlBoxesUsed);
    WriteString(Writer, StringBuffer, Len);
    Len = sprintf_s(StringBuffer, 512, "led_strip_count %d\n\n", ControlBoxPairsUsed);
    WriteString(Writer, StringBuffer, Len);
    
    for (s32 c = 0; c < ControlBoxesUsed; c++)
    {
        control_box* Box = ControlBoxes + c;
        Len = sprintf_s(StringBuffer, 512, 
                        "control_box { %d, \"%s\", (%f, %f, %f) }\n",
                        Box->ID, Box->Address,
                        Box->X, Box->Y, Box->Z);
        WriteString(Writer, StringBuffer, Len);
    }
    
    WriteString(Writer, "\n", 1);
    
#define UNIVERSES_PER_BOX 25
    s32 UniversesPerBox[64];
    for (s32 u = 0; u < 64; u++)
    {
        UniversesPerBox[u] = UNIVERSES_PER_BOX * u;
    }
    
    char LEDStripFormatString[] = "led_strip { %d, %d, %d, INTERPOLATE_POINTS, (%f, %f, %f), (%f, %f, %f), 144 } \n";
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
        
        Len = sprintf_s(StringBuffer, 512,
                        LEDStripFormatString, 
                        Pair->Start, Universe, 0,
                        StartX, StartY, StartZ,
                        EndX, EndY, EndZ);
        WriteString(Writer, StringBuffer, Len);
    }
    
    WriteString(Writer, "\n", 1);
    
    for (s32 sp = 0; sp < ExtraStripsUsed; sp++)
    {
        extra_strips* Strip = ExtraStrips + sp;
        
        s32 Universe = UniversesPerBox[Strip->BoxID];
        UniversesPerBox[Strip->BoxID]++;
        
        Len = sprintf_s(StringBuffer, 512,
                        LEDStripFormatString,
                        Strip->BoxID, Universe, 0,
                        Strip->Start.x, Strip->Start.y, Strip->Start.z,
                        Strip->End.x, Strip->End.y, Strip->End.z);
        WriteString(Writer, StringBuffer, Len);
    }
    
    WriteString(Writer, "END_OF_ASSEMBLY_FILE", StringLength("END_OF_ASSEMBLY_FILE"));
    
    *Writer->Cursor = 0;
    
    FILE* OutputFile = fopen("F:/data/radialumia.fold", "w");
    string_buffer* BufferCursor = &OutputFileBuffer;
    while(BufferCursor)
    {
        fprintf(OutputFile, BufferCursor->Memory);
        BufferCursor = BufferCursor->Next;
    }
    fclose(OutputFile);
    
    return 0;
}