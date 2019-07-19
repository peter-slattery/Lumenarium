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

struct sacn_universe
{
    s16 Universe;
    
    u8* StartPositionInSendBuffer;
    s32 SizeInSendBuffer;
    s32 OffsetInSendBuffer;
    
    s32 BeginPixelCopyFromOffset;
    
    platform_network_address_handle SendAddress;
};

struct sacn_send_buffer
{
    u8* Memory;
    s32 Size;
    sacn_send_buffer* Next;
};

struct sacn_universe_buffer
{
    sacn_universe* Universes;
    s32 Used;
    s32 Max;
    sacn_universe_buffer* Next;
};

struct streaming_acn
{
    memory_arena Memory;
    
    // These get created and freed together
    sacn_universe_buffer* UniverseBuffer;
    sacn_send_buffer* SendBuffer;
    
    platform_socket_handle SendSocket;
    cid CID;
    
    s32 SequenceIterator;
};


// SACN Data Header Functions
internal void InitStreamHeader (u8* Buffer, s32 BufferSize, u16 SlotCount, u8 StartCode, u16 Universe, u8 Priority, u16 Reserved, u8 Options, const char* SourceName, cid CID);
internal void SetStreamHeaderSequence_ (u8* Buffer, u8 Sequence, b32 Draft);
internal void VHD_PackFlags_(u8* Buffer, b32 InheritVec, b32 InheritHead, b32 InheritData);
internal u8*  VHD_PackLength_(u8* Buffer, u32 Length, b32 IncludeLength);
internal cid  StringToCID_ (const char* String);

#define CalculateSendBufferSize(UniverseCount) ((UniverseCount * (STREAM_HEADER_SIZE + STREAM_BODY_SIZE)) + sizeof(sacn_send_buffer))
#define CalculateUniverseBufferSize(UniverseCount) ((UniverseCount * sizeof(sacn_universe)) + sizeof(sacn_universe_buffer))

// Utility

struct sacn_pixel
{
    u8 R;
    u8 G;
    u8 B;
};

//

internal sacn_universe*
SACNGetUniverse (s32 UniverseNumber, streaming_acn* SACN)
{
    sacn_universe* Result = 0;
    
    sacn_universe_buffer* Header = SACN->UniverseBuffer;
    while (Header)
    {
        sacn_universe* Cursor = Header->Universes;
        for (s32 i = 0; i < Header->Used; i++)
        {
            if (Cursor->Universe == UniverseNumber)
            {
                Result = Cursor;
                break;
            }
            Cursor++;
        }
        Header = Header->Next;
    }
    
    return Result;
}

internal void
SACNPushSendBufferOnList (sacn_send_buffer* ListHead, sacn_send_buffer* NewBuffer)
{
    if (ListHead->Next)
    {
        SACNPushSendBufferOnList(ListHead->Next, NewBuffer);
    }
    else
    {
        ListHead->Next = NewBuffer;
    }
}

internal sacn_send_buffer* 
SACNRemoveSendBufferFromList (sacn_send_buffer* List, sacn_send_buffer* Entry)
{
    sacn_send_buffer* ListHead = 0;
    if (List != Entry && List->Next)
    {
        ListHead = SACNRemoveSendBufferFromList(List->Next, Entry);
    }
    else if (List == Entry)
    {
        ListHead = Entry->Next;
    }
    else
    {
        // NOTE(Peter): Trying to remove an entry from a list that doesn't contain it
        InvalidCodePath;
    }
    return ListHead;
}

internal void
SACNPushUniverseBufferOnList (sacn_universe_buffer* ListHead, sacn_universe_buffer* NewBuffer)
{
    if (ListHead->Next)
    {
        SACNPushUniverseBufferOnList(ListHead->Next, NewBuffer);
    }
    else
    {
        ListHead->Next = NewBuffer;
    }
}

internal sacn_universe_buffer* 
SACNRemoveUniverseBufferFromList (sacn_universe_buffer* List, sacn_universe_buffer* Entry)
{
    sacn_universe_buffer* ListHead = 0;
    if (List != Entry && List->Next)
    {
        ListHead = SACNRemoveUniverseBufferFromList(List->Next, Entry);
    }
    else if (List == Entry)
    {
        ListHead = Entry->Next;
    }
    else
    {
        // NOTE(Peter): Trying to remove an entry from a list that doesn't contain it
        InvalidCodePath;
    }
    return ListHead;
}

struct sacn_add_universes_result
{
    sacn_send_buffer* NewSendBuffer;
    sacn_universe_buffer* NewUniverseBuffer;
};
internal sacn_add_universes_result
SACNAddUniverses(s32* Universes, s32 UniversesLength, streaming_acn* SACN, context Context)
{
    sacn_add_universes_result Result = {};
    
    // Determine which universes are already registered and not to be readded. 
    // NOTE(Peter): This might create funky behaviour if two sculptures start sending data to the same universe
    // but I'm not sure its incorrect behavior. I think, eventually, we will want to spit out a report from
    // this function that says what universes were duplicated. We might want to display this information to the user
    // in a way that they don't have to exit out of every single time they load the software. Not sure
    s32 UniversesToAdd = 0;
    for (s32 i = 0; i < UniversesLength; i++)
    {
        sacn_universe* UniverseExists = SACNGetUniverse(Universes[i], SACN);
        if (UniverseExists)
        {
            Universes[i] = -1;
        }
        else
        {
            UniversesToAdd++;
        }
    }
    
    // Push On New Send and Universe Buffers
    s32 SendBufferSize = CalculateSendBufferSize(UniversesToAdd);
    u8* SendBufferMemory = PushArray(&SACN->Memory, u8, SendBufferSize);
    sacn_send_buffer* SendBufferHeader = (sacn_send_buffer*)SendBufferMemory;
    SendBufferHeader->Memory = (u8*)(SendBufferHeader + 1);
    SendBufferHeader->Size = SendBufferSize - sizeof(sacn_send_buffer);
    SendBufferHeader->Next = 0;
    if (SACN->SendBuffer)
    {
        SACNPushSendBufferOnList(SACN->SendBuffer, SendBufferHeader);
    }
    else
    {
        SACN->SendBuffer = SendBufferHeader;
    }
    
    s32 UniverseBufferSize = CalculateUniverseBufferSize(UniversesToAdd);
    u8* UniverseBufferMemory = PushArray(&SACN->Memory, u8, UniverseBufferSize);
    sacn_universe_buffer* UniverseBufferHeader = (sacn_universe_buffer*)UniverseBufferMemory;
    UniverseBufferHeader->Universes = (sacn_universe*)(UniverseBufferHeader + 1);
    UniverseBufferHeader->Used = 0;
    UniverseBufferHeader->Max = UniversesToAdd;
    if (SACN->UniverseBuffer)
    {
        SACNPushUniverseBufferOnList(SACN->UniverseBuffer, UniverseBufferHeader);
    }
    else
    {
        SACN->UniverseBuffer = UniverseBufferHeader;
    }
    
    // Add each of the valid universes
    for (s32 j = 0; j < UniversesLength; j++)
    {
        if (Universes[j] >= 0)
        {
            Assert(UniverseBufferHeader->Used < UniverseBufferHeader->Max);
            s32 Index = UniverseBufferHeader->Used++;
            s32 UniverseID = Universes[j];
            
            UniverseBufferHeader->Universes[Index].Universe         = UniverseID;
            UniverseBufferHeader->Universes[Index].SizeInSendBuffer = STREAM_HEADER_SIZE + STREAM_BODY_SIZE;
            UniverseBufferHeader->Universes[Index].BeginPixelCopyFromOffset = -1;
            
            // Configure how the universe looks into the pixel color buffer
            s32 SendBufferOffset        = (Index * (STREAM_HEADER_SIZE + STREAM_BODY_SIZE));
            u8* SendBufferStartPosition = SendBufferHeader->Memory + SendBufferOffset;
            UniverseBufferHeader->Universes[Index].OffsetInSendBuffer        = SendBufferOffset;
            UniverseBufferHeader->Universes[Index].StartPositionInSendBuffer = SendBufferStartPosition;
            
            // Set up the Send Address
            u8 MulticastAddressBuffer[IP_ADDRESS_BYTES];
            GSMemSet(MulticastAddressBuffer, 0, IP_ADDRESS_BYTES);
            MulticastAddressBuffer[12] = 239;
            MulticastAddressBuffer[13] = 255;
            PackB2(MulticastAddressBuffer + 14, UniverseID);
            u_long V4Address = (u_long)UpackB4(MulticastAddressBuffer + IP_ADDRESS_BYTES - sizeof(u32));
            
            GSMemSet(&UniverseBufferHeader->Universes[Index].SendAddress, 0, sizeof(sockaddr_in));
            UniverseBufferHeader->Universes[Index].SendAddress = Context.PlatformGetSendAddress(
                AF_INET,
                HostToNetU16(DEFAULT_STREAMING_ACN_PORT), 
                HostToNetU32(V4Address));
            
#if 0 // Old Net Code
            UniverseBufferHeader->Universes[Index].SendAddress.sin_family = AF_INET;
            UniverseBufferHeader->Universes[Index].SendAddress.sin_port = HostToNetU16(DEFAULT_STREAMING_ACN_PORT);
            UniverseBufferHeader->Universes[Index].SendAddress.sin_addr.s_addr = HostToNetU32(V4Address);
#endif
            
            s32 SlotCount = 512;
            InitStreamHeader(UniverseBufferHeader->Universes[Index].StartPositionInSendBuffer, 
                             UniverseBufferHeader->Universes[Index].SizeInSendBuffer, 
                             SlotCount,
                             STARTCODE_DMX,
                             UniverseID,
                             0,
                             0, // Reserved
                             0, // Options
                             "Source 1",
                             SACN->CID
                             );
        }
    }
    
    Result.NewUniverseBuffer = UniverseBufferHeader;
    Result.NewSendBuffer= SendBufferHeader;
    return Result;
}

internal void
SACNRemoveUniverseAndSendBuffer(streaming_acn* SACN, sacn_universe_buffer* Universes, sacn_send_buffer* SendBuffer)
{
    SACN->UniverseBuffer = SACNRemoveUniverseBufferFromList(SACN->UniverseBuffer, Universes);
    SACN->SendBuffer = SACNRemoveSendBufferFromList(SACN->SendBuffer, SendBuffer);
}

internal streaming_acn
InitializeSACN (platform_alloc* PlatformAlloc, context Context)
{
    streaming_acn SACN = {};
    
    InitMemoryArena(&SACN.Memory, 0, 0, PlatformAlloc);
    
    SACN.SendSocket = Context.PlatformGetSocketHandle(AF_INET, SOCK_DGRAM, 0);
    int Multicast_TimeToLive = 20;
    int Error = Context.PlatformSetSocketOption(SACN.SendSocket, IPPROTO_IP, IP_MULTICAST_TTL, 
                                                (const char*)(&Multicast_TimeToLive), sizeof(Multicast_TimeToLive));
    SACN.CID = StringToCID_ ("{67F9D986-544E-4abb-8986-D5F79382586C}");
    
    SACN.UniverseBuffer = 0;
    SACN.SendBuffer = 0;
    
    return SACN;
}

internal void
SACNSendDataToUniverse (streaming_acn* SACN, sacn_universe* Universe, platform_send_to* PlatformSendTo)
{
    //DEBUG_TRACK_FUNCTION;
    
    u8* StartPositionInSendBuffer = (u8*)Universe->StartPositionInSendBuffer;
    SetStreamHeaderSequence_(StartPositionInSendBuffer, SACN->SequenceIterator, false);
    
    PlatformSendTo(SACN->SendSocket, Universe->SendAddress, (const char*)StartPositionInSendBuffer, Universe->SizeInSendBuffer, 0);
#if 0 // Old Network Code
    // TODO(Peter): HUGE NOTE!!!!!!!!
    // This needs to be put on a separate thread. The sendto call is really slowing us down.
    s32 LengthSent = sendto(SACN->SendSocket, (const char*)StartPositionInSendBuffer, Universe->SizeInSendBuffer, 
                            0, (sockaddr*)(&Universe->SendAddress), sizeof(sockaddr_in));
    
    if (LengthSent == SOCKET_ERROR)
    {
        s32 LastSocketError = WSAGetLastError();
        InvalidCodePath;
    }
#endif
}

internal void
SACNCleanup(streaming_acn* SACN, context Context)
{
    Context.PlatformCloseSocket(SACN->SendSocket);
}


///////////////////////////////////////////////
//
//         SACN Data Header Functions
//
///////////////////////////////////////////////

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
    
    u8* Cursor = Buffer;
    
    // Preamble Size
    Cursor = PackB2(Cursor, RLP_PREAMBLE_SIZE);
    Cursor = PackB2(Cursor, RLP_POSTAMBLE_SIZE);
    
    memcpy(Cursor, ACN_IDENTIFIER, ACN_IDENTIFIER_SIZE);
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
    strncpy((char*)Cursor, SourceName, SOURCE_NAME_SIZE);
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
    
    s32 DiffSize = Cursor - Buffer;
    if (Cursor - Buffer != STREAM_HEADER_SIZE)
    {
        InvalidCodePath;
    }
}

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
StringToCID_ (const char* String)
{
    cid Result = {};
    
    const char* Src = String;
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
