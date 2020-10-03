//
// File: win32_foldhaus_socket.h
// Author: Peter Slattery
// Creation Date: 2020-10-03
//
#ifndef WIN32_FOLDHAUS_SOCKET_H

struct win32_socket
{
    SOCKET Socket;
};

struct win32_socket_array
{
    win32_socket* Values;
    s32 CountMax;
    s32 Count;
};

//////////////////////
//
// Win32 Socket Array

internal win32_socket_array
Win32SocketArray_Create(u32 CountMax, gs_memory_arena* Storage)
{
    win32_socket_array Result = {};
    Result.CountMax = CountMax;
    Result.Values = PushArray(Storage, win32_socket, CountMax);
    return Result;
}

internal s32
Win32SocketArray_Take(win32_socket_array* Array)
{
    Assert(Array->Count < Array->CountMax);
    s32 Result = Array->Count++;
    win32_socket* Socket = Array->Values + Result;
    *Socket = {0};
    return Result;
}

internal win32_socket*
Win32SocketArray_Get(win32_socket_array Array, s32 Index)
{
    Assert(Index < Array.Count);
    win32_socket* Result = Array.Values + Index;
    return Result;
}

//////////////////////
//
// Win32 Socket System

global win32_socket_array Win32Sockets;

internal s32
Win32Socket_SetOption(win32_socket* Socket, s32 Level, s32 Option, const char* OptionValue, s32 OptionLength)
{
    int Error = setsockopt(Socket->Socket, Level, Option, OptionValue, OptionLength);
    if (Error == SOCKET_ERROR)
    {
        Error = WSAGetLastError();
        // TODO(Peter): :ErrorLogging
    }
    
    return Error;
}

internal s32
Win32Socket_SetOption(platform_socket_handle SocketHandle, s32 Level, s32 Option, const char* OptionValue, s32 OptionLength)
{
    win32_socket* Socket = Win32SocketArray_Get(Win32Sockets, (s32)SocketHandle);
    return Win32Socket_SetOption(Socket, Level, Option, OptionValue, OptionLength);
}

PLATFORM_GET_SOCKET_HANDLE(Win32GetSocketHandle)
{
    // NOTE(Peter): These used to be passed in as paramters, but we only use this function
    // with AF_INET, SOCK_DGRAM, and Protocol = 0. These are also platform specific values
    // so I was having to include windows.h in the platform agnostic code to accomodate that
    // function signature.
    s32 AddressFamily = AF_INET;
    s32 Type = SOCK_DGRAM;
    s32 Protocol = 0;
    
    s32 Result = Win32SocketArray_Take(&Win32Sockets);
    win32_socket* Socket = Win32SocketArray_Get(Win32Sockets, Result);
    Socket->Socket = socket(AddressFamily, Type, Protocol);
    if (Socket->Socket != INVALID_SOCKET)
    {
        int Error = Win32Socket_SetOption(Socket, IPPROTO_IP, IP_MULTICAST_TTL,
                                          (const char*)(&Multicast_TimeToLive), sizeof(Multicast_TimeToLive));
    }
    else
    {
        s32 Error = WSAGetLastError();
        InvalidCodePath;
    }
    
    return (platform_socket_handle)Result;
}

internal s32
Win32Socket_SendTo(platform_socket_handle SocketHandle, u32 Address, u32 Port, const char* Buffer, s32 BufferLength, s32 Flags)
{
    win32_socket* Socket = Win32SocketArray_Get(Win32Sockets, (s32)SocketHandle);
    
    sockaddr_in SockAddress = {};
    SockAddress.sin_family = AF_INET;
    SockAddress.sin_port = HostToNetU16(Port);
    SockAddress.sin_addr.s_addr = HostToNetU32(Address);
    
    s32 LengthSent = sendto(Socket->Socket, Buffer, BufferLength, Flags, (sockaddr*)&SockAddress, sizeof(sockaddr_in));
    
    if (LengthSent == SOCKET_ERROR)
    {
        s32 Error = WSAGetLastError();
        if (Error == 10051)
        {
        }
        else
        {
            // TODO(Peter): :ErrorLogging
            InvalidCodePath;
        }
    }
    
    return LengthSent;
}

internal void
Win32Socket_Close(win32_socket* Socket)
{
    closesocket(Socket->Socket);
    Socket->Socket = INVALID_SOCKET;
}

#define WIN32_FOLDHAUS_SOCKET_H
#endif // WIN32_FOLDHAUS_SOCKET_H