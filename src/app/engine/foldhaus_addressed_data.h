//
// File: foldhaus_addressed_data.h
// Author: Peter Slattery
// Creation Date: 2020-10-01
//
// addressed_data_buffer is a generic buffer of data that also contains information
// regarding how it should be sent to a sculpture.
// This decouples the encoding step from the sending step.
//
#ifndef FOLDHAUS_ADDRESSED_DATA_H

enum data_buffer_address_type
{
    AddressType_NetworkIP,
    AddressType_ComPort,
    AddressType_Invalid,
};

struct addressed_data_buffer
{
    union
    {
        struct
        {
            u8* Memory;
            u32 MemorySize;
        };
        gs_data Data;
    };
    
    data_buffer_address_type AddressType;
    
    // IP Address
    platform_socket_handle SendSocket;
    u32 V4SendAddress;
    u32 SendPort;
    
    // COM
    gs_const_string ComPort;
    
    addressed_data_buffer* Next;
};

struct addressed_data_buffer_list
{
    gs_memory_arena* Arena;
    addressed_data_buffer* Root;
    addressed_data_buffer* Head;
};

internal void
AddressedDataBufferList_Clear(addressed_data_buffer_list* List)
{
    List->Root = 0;
    List->Head = 0;
    ClearArena(List->Arena);
}

internal addressed_data_buffer*
AddressedDataBufferList_PushEmpty(addressed_data_buffer_list* List)
{
    addressed_data_buffer* Result = PushStruct(List->Arena, addressed_data_buffer);
    *Result = {0};
    Result->Next = 0;
    Result->MemorySize = 0;
    Result->Memory = 0;
    
    SLLPushOrInit(List->Root, List->Head, Result);
    
    return Result;
}

internal addressed_data_buffer*
AddressedDataBufferList_Push(addressed_data_buffer_list* List, u32 BufferSize)
{
    addressed_data_buffer* Result = AddressedDataBufferList_PushEmpty(List);
    Result->MemorySize = BufferSize;
    Result->Memory = PushArray(List->Arena, u8, Result->MemorySize);
    return Result;
}

internal void
AddressedDataBuffer_SetNetworkAddress(addressed_data_buffer* Buffer, platform_socket_handle SendSocket, u32 V4SendAddress, u32 SendPort)
{
    Buffer->AddressType = AddressType_NetworkIP;
    Buffer->SendSocket = SendSocket;
    Buffer->V4SendAddress = V4SendAddress;
    Buffer->SendPort = SendPort;
}

internal void
AddressedDataBuffer_SetCOMPort(addressed_data_buffer* Buffer, gs_const_string ComPort)
{
    Buffer->AddressType = AddressType_ComPort;
    Buffer->ComPort = ComPort;
}

internal addressed_data_buffer_list
AddressedDataBufferList_Create(gs_thread_context TC)
{
    addressed_data_buffer_list Result = {};
    Result.Arena = AllocatorAllocStruct(TC.Allocator, gs_memory_arena);
    *Result.Arena = CreateMemoryArena(TC.Allocator, "Addressed Data Buffer List Arena");
    return Result;
}

#define FOLDHAUS_ADDRESSED_DATA_H
#endif // FOLDHAUS_ADDRESSED_DATA_H