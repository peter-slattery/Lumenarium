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
    AddressType_Invalid,
};

struct addressed_data_buffer
{
    u8* Memory;
    u32 MemorySize;
    
    data_buffer_address_type AddressType;
    
    // IP Address
    u32 V4SendAddress;
    u32 SendPort;
    
    addressed_data_buffer* Next;
};

struct addressed_data_buffer_list
{
    addressed_data_buffer* Root;
    addressed_data_buffer* Head;
};

internal addressed_data_buffer*
AddressedDataBufferList_Push(addressed_data_buffer_list* List, u32 BufferSize, gs_memory_arena* Storage)
{
    addressed_data_buffer* Result = PushStruct(Storage, addressed_data_buffer);
    *Result = {0};
    Result->MemorySize = BufferSize;
    Result->Memory = PushArray(Storage, u8, Result->MemorySize);
    
    SLLPushOrInit(List->Root, List->Head, Result);
    
    return Result;
}

internal void
AddressedDataBuffer_SetNetworkAddress(addressed_data_buffer* Buffer, u32 V4SendAddress, u32 SendPort)
{
    Buffer->AddressType = AddressType_NetworkIP;
    Buffer->V4SendAddress = V4SendAddress;
    Buffer->SendPort = SendPort;
}

internal void
AddressedDataBuffer_Send(addressed_data_buffer Buffer, platform_socket_handle SendSocket, context Context)
{
    u32 V4SendAddress = Buffer.V4SendAddress;
    Context.PlatformSendTo(SendSocket, Buffer.V4SendAddress, Buffer.SendPort, (const char*)Buffer.Memory, Buffer.MemorySize, 0);
}

internal void
AddressedDataBufferList_SendAll(addressed_data_buffer_list OutputData, platform_socket_handle SendSocket, context Context)
{
    for (addressed_data_buffer* BufferAt = OutputData.Root;
         BufferAt != 0;
         BufferAt = BufferAt->Next)
    {
        AddressedDataBuffer_Send(*BufferAt, SendSocket, Context);
    }
}

#define FOLDHAUS_ADDRESSED_DATA_H
#endif // FOLDHAUS_ADDRESSED_DATA_H