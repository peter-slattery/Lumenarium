//
// File: DMX_H
// Author: Peter Slattery
// Creation Date: 2020-01-01
//
#ifndef DMX_H

struct dmx_buffer
{
    s32 Universe;
    u8* Base;
    s32 TotalSize;
    s32 HeaderSize;
};

struct dmx_buffer_list
{
    dmx_buffer Buffer;
    dmx_buffer_list* Next;
};

internal dmx_buffer_list*
DMXBufferListGetTail (dmx_buffer_list* List)
{
    dmx_buffer_list* Result = 0;
    if (List->Next == 0) 
    { 
        Result = List;
    }
    else
    {
        Result = DMXBufferListGetTail(List->Next);
    }
    return Result;
}

internal dmx_buffer_list*
DMXBufferListAppend (dmx_buffer_list* AppendTo, dmx_buffer_list* Append)
{
    dmx_buffer_list* Result = 0;
    
    if (AppendTo)
    {
        dmx_buffer_list* Tail = DMXBufferListGetTail(AppendTo);
        Tail->Next = Append;
        Result = AppendTo;
    }
    else
    {
        Result = Append;
    }
    
    return Result;
}

#define DMX_H
#endif // DMX_H