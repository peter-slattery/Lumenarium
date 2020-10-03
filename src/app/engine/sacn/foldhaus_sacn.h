//
// File: sacn.h
// Author: Peter Slattery
// Creation Date: 2020-01-01
//
#ifndef SACN_H

#define NETWORKINTID_INVALID -1

#define DEFAULT_STREAMING_ACN_PORT 5568

#define IP_ADDRESS_BYTES 16
#define STARTCODE_DMX 0

/*
 * a description of the address space being used
 */
#define PREAMBLE_SIZE_ADDR 0
#define POSTAMBLE_SIZE_ADDR 2
#define ACN_IDENTIFIER_ADDR 4
#define ROOT_FLAGS_AND_LENGTH_ADDR 16
#define ROOT_VECTOR_ADDR 18
#define CID_ADDR 22
#define FRAMING_FLAGS_AND_LENGTH_ADDR 38
#define FRAMING_VECTOR_ADDR 40
#define SOURCE_NAME_ADDR 44
#define PRIORITY_ADDR 108
#define RESERVED_ADDR 109
#define SEQ_NUM_ADDR 111
#define OPTIONS_ADDR 112
#define UNIVERSE_ADDR 113
#define DMP_FLAGS_AND_LENGTH_ADDR 115
#define DMP_VECTOR_ADDR 117
#define DMP_ADDRESS_AND_DATA_ADDR 118
#define FIRST_PROPERTY_ADDRESS_ADDR 119
#define ADDRESS_INC_ADDR 121
#define PROP_COUNT_ADDR 123
#define START_CODE_ADDR 125
#define PROP_VALUES_ADDR (START_CODE_ADDR + 1)

/*
 * common sizes
 */
#define STREAM_HEADER_SIZE 126
#define STREAM_BODY_SIZE 512

#define SOURCE_NAME_SIZE 64
#define RLP_PREAMBLE_SIZE 16
#define RLP_POSTAMBLE_SIZE 0
#define ACN_IDENTIFIER_SIZE 12

/*
 * data definitions
 */
#define ACN_IDENTIFIER "ASC-E1.17\0\0\0"
#define ROOT_VECTOR 4
#define FRAMING_VECTOR 2
#define DMP_VECTOR 2
#define ADDRESS_AND_DATA_FORMAT 0xa1
#define ADDRESS_INC 1
#define DMP_FIRST_PROPERTY_ADDRESS_FORCE 0
#define RESERVED_VALUE 0

//for support of the early draft
#define DRAFT_STREAM_HEADER_SIZE 90
#define DRAFT_SOURCE_NAME_SIZE 32

//for support of the early draft
#define DRAFT_ROOT_VECTOR 3

const u32 VHD_MAXFLAGBYTES = 7;  //The maximum amount of bytes used to pack the flags, len, and vector
const u32 VHD_MAXLEN = 0x0fffff;  //The maximum packet length is 20 bytes long
const u32 VHD_MAXMINLENGTH = 4095;  //The highest length that will fit in the "smallest" length pack

//Defines for the VHD flags
const u8 VHD_L_FLAG = 0x80;
const u8 VHD_V_FLAG = 0x40;
const u8 VHD_H_FLAG = 0x20;
const u8 VHD_D_FLAG = 0x10;

#define CID_Bytes 16
struct cid
{
    u8 Bytes[CID_Bytes];
};

struct streaming_acn
{
    platform_socket_handle SendSocket;
    cid CID;
    s32 SequenceIterator;
};

///////////////////////////////////////////////
//
//         SACN Data Header Functions
//
///////////////////////////////////////////////

internal void
SetStreamHeaderSequence_ (u8* Buffer, u8 Sequence, b32 Draft)
{
    DEBUG_TRACK_FUNCTION;
    PackB1(Buffer + SEQ_NUM_ADDR, Sequence);
}

internal void
VHD_PackFlags_(u8* Buffer, b32 InheritVec, b32 InheritHead, b32 InheritData)
{
    u8* Cursor = Buffer;
    u8 NewByte = UpackB1(Cursor) & 0x8f;
    
    if (!InheritVec) { NewByte |= VHD_V_FLAG; }
    if (!InheritHead) { NewByte |= VHD_H_FLAG; }
    if (!InheritData) { NewByte |= VHD_D_FLAG; }
    
    PackB1(Cursor, NewByte);
}

internal u8*
VHD_PackLength_(u8* Buffer, u32 Length, b32 IncludeLength)
{
    u8* Cursor = Buffer;
    u32 AdjustedLength = Length;
    if (IncludeLength)
    {
        if (Length + 1 > VHD_MAXMINLENGTH)
        {
            AdjustedLength += 2;
        }
        else
        {
            AdjustedLength += 1;
        }
    }
    
    // Mask out the length bits to keep flags intact
    u8 NewByte = UpackB1(Cursor) & 0x70;
    if (AdjustedLength > VHD_MAXMINLENGTH)
    {
        NewByte |= VHD_L_FLAG;
    }
    
    u8 PackBuffer[4];
    PackB4(PackBuffer, AdjustedLength);
    if (AdjustedLength <= VHD_MAXMINLENGTH)
    {
        NewByte |= (PackBuffer[2] & 0x0f);
        Cursor = PackB1(Cursor, NewByte);
        Cursor = PackB1(Cursor, PackBuffer[3]);
    }
    else
    {
        NewByte |= (PackBuffer[1] & 0x0f);
        Cursor = PackB1(Cursor, PackBuffer[2]);
        Cursor = PackB1(Cursor, PackBuffer[3]);
    }
    
    return Cursor;
}

internal cid
gs_stringToCID_ (const char* gs_string)
{
    cid Result = {};
    
    const char* Src = gs_string;
    u8* Dest = &Result.Bytes[0];
    b32 FirstNibble = true;
    
    while(*Src && (Dest - &Result.Bytes[0] < CID_Bytes))
    {
        u8 Offset = 0;
        if ((*Src >= 0x30) && (*Src <= 0x39)){ Offset = 0x30; }
        else if ((*Src >= 0x41) && (*Src <= 0x46)) { Offset = 0x37; }
        else if ((*Src >= 0x61) && (*Src <= 0x66)) { Offset = 0x66; }
        
        if (Offset != 0)
        {
            if (FirstNibble)
            {
                *Dest = (u8)(*Src - Offset);
                *Dest <<= 4;
                FirstNibble = false;
            }
            else
            {
                *Dest |= (*Src - Offset);
                Dest++;
                FirstNibble = true;
            }
        }
        Src++;
    }
    
    return Result;
}

internal void
InitStreamHeader (u8* Buffer, s32 BufferSize,
                  u16 SlotCount,
                  u8 StartCode,
                  u16 Universe,
                  u8 Priority,
                  u16 Reserved,
                  u8 Options,
                  const char* SourceName,
                  cid CID
                  )
{
    // TODO(pjs): Replace packing with gs_memory_cursor
    
    u8* Cursor = Buffer;
    
    // Preamble Size
    Cursor = PackB2(Cursor, RLP_PREAMBLE_SIZE);
    Cursor = PackB2(Cursor, RLP_POSTAMBLE_SIZE);
    
    CopyMemoryTo(ACN_IDENTIFIER, Cursor, ACN_IDENTIFIER_SIZE);
    Cursor += ACN_IDENTIFIER_SIZE;
    
    // TODO(Peter): If you never use this anywhere else, go back and remove the parameters
    VHD_PackFlags_(Cursor, false, false, false);
    Cursor = VHD_PackLength_(Cursor,
                             STREAM_HEADER_SIZE - RLP_PREAMBLE_SIZE + SlotCount,
                             false);
    
    // root vector
    Cursor = PackB4(Cursor, ROOT_VECTOR);
    
    // CID Pack
    for (s32 i = 0; i < CID_Bytes; i++)
    {
        *Cursor++ = CID.Bytes[i];
    }
    
    VHD_PackFlags_(Cursor, false, false, false);
    Cursor = VHD_PackLength_(Cursor,
                             STREAM_HEADER_SIZE - FRAMING_FLAGS_AND_LENGTH_ADDR + SlotCount,
                             false);
    
    // framing vector
    Cursor = PackB4(Cursor, FRAMING_VECTOR);
    
    // framing source name
    // :Check
    CopyMemoryTo(SourceName, (char*)Cursor, SOURCE_NAME_SIZE);
    Cursor[SOURCE_NAME_SIZE - 1] = '\0';
    Cursor += SOURCE_NAME_SIZE;
    
    // priority
    Cursor = PackB1(Cursor, Priority);
    
    // reserved
    Cursor = PackB2(Cursor, Reserved);
    
    // Sequence # is always set to 0/NONE at the beginning, but it is incremented when sending data
    Cursor = PackB1(Cursor, 0);
    
    // Options
    Cursor = PackB1(Cursor, Options);
    
    // Universe
    Cursor = PackB2(Cursor, Universe);
    
    VHD_PackFlags_(Cursor, false, false, false);
    Cursor = VHD_PackLength_(Cursor,
                             STREAM_HEADER_SIZE - DMP_FLAGS_AND_LENGTH_ADDR + SlotCount,
                             false);
    
    // DMP Vector
    Cursor = PackB1(Cursor, DMP_VECTOR);
    
    // DMP Address and data type
    Cursor = PackB1(Cursor, ADDRESS_AND_DATA_FORMAT);
    
    // DMP first property address
    Cursor = PackB2(Cursor, 0);
    
    // DMP Address Increment
    Cursor = PackB2(Cursor, ADDRESS_INC);
    
    // Property Value Count -- Includes one byte for start code
    Cursor = PackB2(Cursor, SlotCount + 1);
    
    Cursor = PackB1(Cursor, StartCode);
    
    Assert(Cursor - Buffer == STREAM_HEADER_SIZE);
    
}

//
// New SACN
//

internal streaming_acn
SACN_Initialize (context Context)
{
    streaming_acn SACN = {};
    
    s32 Multicast_TimeToLive = 20;
    SACN.SendSocket = Context.PlatformGetSocketHandle(Multicast_TimeToLive);
    SACN.CID = gs_stringToCID_ ("{67F9D986-544E-4abb-8986-D5F79382586C}");
    
    return SACN;
}

internal void
SACN_Cleanup(streaming_acn* SACN, context Context)
{
}

internal void
SACN_UpdateSequence (streaming_acn* SACN)
{
    // Never use 0 after the first one
    if (++SACN->SequenceIterator == 0)
    {
        ++SACN->SequenceIterator;
    }
}

internal void
SACN_PrepareBufferHeader (s32 Universe, u8* Buffer, s32 BufferSize, s32 SizeReservedForHeader, streaming_acn SACN)
{
    Assert(SizeReservedForHeader == STREAM_HEADER_SIZE);
    Assert(Buffer && BufferSize > 0);
    
    s32 Priority = 0;
    InitStreamHeader(Buffer, BufferSize, STREAM_BODY_SIZE, STARTCODE_DMX, Universe, Priority, 0, 0, "Lumenarium", SACN.CID);
    SetStreamHeaderSequence_(Buffer, SACN.SequenceIterator, false);
}

internal u32
SACN_GetUniverseSendAddress(s32 Universe)
{
    u8 MulticastAddressBuffer[4] = {};
    MulticastAddressBuffer[0] = 239;
    MulticastAddressBuffer[1] = 255;
    MulticastAddressBuffer[2] = (u8)((Universe & 0xff00) >> 8); // high bit
    MulticastAddressBuffer[3] = (u8)((Universe & 0x00ff)); // low bit
    
    u32 V4Address = (u32)UpackB4(MulticastAddressBuffer);
    return V4Address;
}

internal void
SACN_FillBufferWithLeds(u8* BufferStart, u32 BufferSize, v2_strip Strip, led_buffer LedBuffer)
{
    u8* DestChannel = BufferStart;
    for (u32 i = 0; i < Strip.LedCount; i++)
    {
        u32 LedIndex = Strip.LedLUT[i];
        pixel Color = LedBuffer.Colors[LedIndex];
        
        DestChannel[0] = Color.R;
        DestChannel[1] = Color.G;
        DestChannel[2] = Color.B;
        DestChannel += 3;
    }
}

internal void
SACN_BuildOutputData(streaming_acn* SACN, addressed_data_buffer_list* Output, assembly_array Assemblies, led_system* LedSystem)
{
    SACN_UpdateSequence(SACN);
    
    // TODO(pjs): 512 is a magic number - make it a constant?
    s32 BufferHeaderSize = STREAM_HEADER_SIZE;
    s32 BufferBodySize = 512;
    s32 BufferSize = BufferHeaderSize + BufferBodySize;
    
    for (u32 AssemblyIdx = 0; AssemblyIdx < Assemblies.Count; AssemblyIdx++)
    {
        assembly Assembly = Assemblies.Values[AssemblyIdx];
        led_buffer* LedBuffer = LedSystemGetBuffer(LedSystem, Assembly.LedBufferIndex);
        
        for (u32 StripIdx = 0; StripIdx < Assembly.StripCount; StripIdx++)
        {
            v2_strip StripAt = Assembly.Strips[StripIdx];
            
            u32 V4SendAddress = SACN_GetUniverseSendAddress(StripAt.SACNAddr.StartUniverse);
            u32 SendPort = DEFAULT_STREAMING_ACN_PORT;
            
            addressed_data_buffer* Data = AddressedDataBufferList_Push(Output, BufferSize);
            AddressedDataBuffer_SetNetworkAddress(Data, SACN->SendSocket, V4SendAddress, SendPort);
            
            SACN_PrepareBufferHeader(StripAt.SACNAddr.StartUniverse, Data->Memory, Data->MemorySize, BufferHeaderSize, *SACN);
            SACN_FillBufferWithLeds(Data->Memory + BufferHeaderSize, BufferBodySize, StripAt, *LedBuffer);
        }
    }
}

#define SACN_H
#endif // SACN_H