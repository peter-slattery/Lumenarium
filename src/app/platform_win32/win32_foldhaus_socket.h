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

global WSADATA WSAData;
global win32_socket_array Win32Sockets;

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
// Win32 Sockets

internal win32_socket
Win32Socket_Create(s32 AddressFamily, s32 Type, s32 Protocol)
{
    win32_socket Result = {0};
    Result.Socket = socket(AddressFamily, Type, Protocol);
    if (Result.Socket == INVALID_SOCKET)
    {
        s32 Error = WSAGetLastError();
        InvalidCodePath;
    }
    return Result;
}

internal void
Win32Socket_Bind(win32_socket* Socket, s32 AddressFamily, char* Address, s32 Port)
{
    sockaddr_in Service = {0};
    Service.sin_family = AddressFamily;
    Service.sin_addr.s_addr = inet_addr(Address);
    Service.sin_port = htons(Port);
    
    s32 Result = bind(Socket->Socket, (SOCKADDR*)&Service, sizeof(Service));
    if (Result == SOCKET_ERROR)
    {
        s32 Error = WSAGetLastError();
        InvalidCodePath;
    }
}

internal win32_socket
Win32Socket_ConnectToAddress(char* Address, char* DefaultPort)
{
    win32_socket Result = {};
    
    addrinfo Hints = {0};
    Hints.ai_family = AF_UNSPEC;
    Hints.ai_socktype = SOCK_STREAM;
    Hints.ai_protocol = IPPROTO_TCP;
    
    addrinfo* PotentialConnections;
    s32 Error = getaddrinfo(Address, DefaultPort, &Hints, &PotentialConnections);
    if (Error == 0)
    {
        for (addrinfo* InfoAt = PotentialConnections; InfoAt != NULL; InfoAt = InfoAt->ai_next)
        {
            win32_socket Socket = Win32Socket_Create(InfoAt->ai_family, InfoAt->ai_socktype, InfoAt->ai_protocol);
            if (Socket.Socket == INVALID_SOCKET)
            {
                Error = WSAGetLastError();
                InvalidCodePath;
            }
            
            Error = connect(Socket.Socket, InfoAt->ai_addr, (int)InfoAt->ai_addrlen);
            if (Error == SOCKET_ERROR)
            {
                closesocket(Socket.Socket);
                continue;
            }
            else
            {
                Result = Socket;
                break;
            }
        }
    }
    else
    {
        Error = WSAGetLastError();
        InvalidCodePath;
    }
    
    freeaddrinfo(PotentialConnections);
    
    return Result;
}

internal bool
Win32ConnectSocket(platform_socket_manager* Manager, platform_socket* Socket)
{
    bool Result = false;
    
    addrinfo Hints = {0};
    Hints.ai_family = AF_UNSPEC;
    Hints.ai_socktype = SOCK_STREAM;
    Hints.ai_protocol = IPPROTO_TCP;
    
    addrinfo* PotentialConnections;
    s32 Error = getaddrinfo(Socket->Addr, Socket->Port, &Hints, &PotentialConnections);
    if (Error == 0)
    {
        for (addrinfo* InfoAt = PotentialConnections; InfoAt != NULL; InfoAt = InfoAt->ai_next)
        {
            SOCKET SocketHandle = socket(InfoAt->ai_family, InfoAt->ai_socktype, InfoAt->ai_protocol);
            if (SocketHandle == INVALID_SOCKET)
            {
                Error = WSAGetLastError();
                InvalidCodePath;
            }
            
            // If iMode == 0, blocking is enabled
            // if iMode != 0, non-blocking mode is enabled
            u_long iMode = 0;
            Error = ioctlsocket(SocketHandle, FIONBIO, &iMode);
            if (Error != NO_ERROR)
            {
                InvalidCodePath;
            }
            
            Error = connect(SocketHandle, InfoAt->ai_addr, (int)InfoAt->ai_addrlen);
            if (Error == SOCKET_ERROR)
            {
                u32 Status = WSAGetLastError();
                if (Status == WSAEWOULDBLOCK)
                {
                    // Non-blocking sockets
#if 0
                    TIMEVAL Timeout = { 0, 500 };
                    fd_set SocketSet = {};
                    FD_ZERO(&SocketSet);
                    FD_SET(SocketHandle, &SocketSet);
                    Assert(FD_ISSET(SocketHandle, &SocketSet));
                    Status = select(0, &SocketSet, 0, 0, (const TIMEVAL*)&Timeout);
                    if (Status == SOCKET_ERROR)
                    {
                        
                    }
                    else if (Status == 0)
                    {
                    }
                    else
                    {
                        
                    }
#endif
                }
                else
                {
                    closesocket(SocketHandle);
                    continue;
                }
            }
            
            Socket->PlatformHandle = (u8*)Win32Alloc(sizeof(SOCKET), 0);
            *(SOCKET*)Socket->PlatformHandle = SocketHandle;
            Result = true;
            break;
        }
    }
    else
    {
        Error = WSAGetLastError();
        InvalidCodePath;
    }
    
    if (!Result)
    {
        Assert(Socket->PlatformHandle == 0);
    }
    
    freeaddrinfo(PotentialConnections);
    return Result;
}

internal bool
Win32CloseSocket(platform_socket_manager* Manager, platform_socket* Socket)
{
    SOCKET* Win32Sock = (SOCKET*)Socket->PlatformHandle;
    closesocket(*Win32Sock);
    Win32Free((u8*)Socket->PlatformHandle, sizeof(SOCKET));
    *Socket = {};
    return true;
}

internal bool
Win32SocketQueryStatus(platform_socket_manager* Manager, platform_socket* Socket)
{
    SOCKET* Win32Sock = (SOCKET*)Socket->PlatformHandle;
    bool Result = (*Win32Sock != INVALID_SOCKET);
    return Result;
}

internal u32
Win32SocketPeek(platform_socket_manager* Manager, platform_socket* Socket)
{
    u32 Result = 0;
    s32 Flags = MSG_PEEK;
    SOCKET* Win32Sock = (SOCKET*)Socket->PlatformHandle;
    char Temp[4];
    u32 TempSize = 4;
    
    //OutputDebugString("Pre Peek...");
    //s32 BytesQueued = recv(*Win32Sock, Temp, TempSize, Flags);
    u_long BytesQueued = 0;
    ioctlsocket(*Win32Sock, FIONREAD, &BytesQueued);
    //OutputDebugString("Post Peek\n");
    
    if (BytesQueued != SOCKET_ERROR)
    {
        Result = (u32)BytesQueued;
    }
    else
    {
        s32 Error = WSAGetLastError();
        switch (Error)
        {
            case WSAEWOULDBLOCK:
            {
                // NOTE(PS): This case covers non-blocking sockets
                // if we peek and there's nothing there, it returns
                // this error code. MSDN says its a non-fatal error
                // and the operation should be retried later
                Result = 0;
            } break;
            
            case WSAENOTCONN:
            case WSAECONNRESET:
            case WSAECONNABORTED:
            {
                CloseSocket(Manager, Socket);
            }break;
            
            InvalidDefaultCase;
        }
    }
    return (s32)Result;
}

internal gs_data
Win32SocketReceive(platform_socket_manager* Manager, platform_socket* Socket, gs_memory_arena* Storage)
{
    // TODO(pjs): Test this first code path when you have data running - it should
    // get the actual size of the data packet being sent
#if 0
    gs_data Result = {};
    s32 BytesQueued = Win32Socket_PeekGetTotalSize(Socket);
    if (BytesQueued > 0)
    {
        Result = PushSizeToData(Storage, BytesQueued);
        s32 Flags = 0;
        s32 BytesReceived = recv(Socket->Socket, (char*)Result.Memory, Result.Size, Flags);
        Assert(BytesReceived == BytesQueued);
    }
    return Result;
#else
    gs_data Result = PushSizeToData(Storage, 1024);
    s32 Flags = 0;
    SOCKET* Win32Sock = (SOCKET*)Socket->PlatformHandle;
    s32 BytesReceived = recv(*Win32Sock, (char*)Result.Memory, Result.Size, Flags);
    if (BytesReceived == SOCKET_ERROR)
    {
        // TODO(pjs): Error logging
        s32 Error = WSAGetLastError();
        InvalidCodePath;
    }
    Result.Size = BytesReceived;
    return Result;
#endif
}


internal s32
Win32SocketSend(platform_socket_manager* Manager, platform_socket* Socket, u32 Address, u32 Port, gs_data Data, s32 Flags)
{
    SOCKET* Win32Sock = (SOCKET*)Socket->PlatformHandle;
    
    sockaddr_in SockAddress = {};
    SockAddress.sin_family = AF_INET;
    SockAddress.sin_port = HostToNetU16(Port);
    SockAddress.sin_addr.s_addr = HostToNetU32(Address);
    
    s32 LengthSent = sendto(*Win32Sock, (char*)Data.Memory, Data.Size, Flags, (sockaddr*)&SockAddress, sizeof(sockaddr_in));
    
    //OutputDebugString("Attempting To Send Network Data: ");
    if (LengthSent == SOCKET_ERROR)
    {
        s32 Error = WSAGetLastError();
        switch (Error)
        {
            case WSAEWOULDBLOCK:
            {
                // NOTE(PS): This covers non-blocking sockets
                // In this case the message should be tried again
                LengthSent = 0;
                //OutputDebugString("Not sent, buffered\n");
            }break;
            
            case WSAECONNABORTED:
            case WSAENETUNREACH:
            case WSAECONNRESET:
            case WSAENOTCONN:
            {
                if (CloseSocket(Manager, Socket))
                {
                    Error = 0;
                    OutputDebugString("Error\n");
                }
            }break;
            
            InvalidDefaultCase;
        }
    }
    else
    {
        OutputDebugString("Sent\n");
    }
    
    return LengthSent;
}

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


internal s32
Win32Socket_SendTo(platform_socket_handle SocketHandle, u32 Address, u32 Port, const char* Buffer, s32 BufferLength, s32 Flags)
{
    win32_socket* Socket = Win32SocketArray_Get(Win32Sockets, (s32)SocketHandle);
    
    sockaddr_in SockAddress = {};
    SockAddress.sin_family = AF_INET;
    SockAddress.sin_port = HostToNetU16(Port);
    SockAddress.sin_addr.s_addr = HostToNetU32(Address);
    
    s32 LengthSent = sendto(Socket->Socket, Buffer, BufferLength, Flags, (sockaddr*)&SockAddress, sizeof(sockaddr_in));
    
    OutputDebugString("Attempting To Send Network Data: ");
    if (LengthSent == SOCKET_ERROR)
    {
        s32 Error = WSAGetLastError();
        OutputDebugString("Error\n");
        if (Error == 10051)
        {
        }
        else
        {
            // TODO(Peter): :ErrorLogging
            InvalidCodePath;
        }
    }
    else
    {
        OutputDebugString("Sent\n");
    }
    
    return LengthSent;
}

internal void
Win32Socket_SetListening(win32_socket* Socket)
{
    if (listen(Socket->Socket, SOMAXCONN) == SOCKET_ERROR)
    {
        // TODO(pjs): Error logging
        s32 Error = WSAGetLastError();
        InvalidCodePath;
    }
}

internal s32
Win32Socket_PeekGetTotalSize(win32_socket* Socket)
{
    s32 Flags = MSG_PEEK;
    char Temp[4];
    s32 BytesQueued = recv(Socket->Socket, Temp, 4, Flags);
    if (BytesQueued == SOCKET_ERROR)
    {
        // TODO(pjs): Error logging
        s32 Error = WSAGetLastError();
        BytesQueued = 0;
    }
    return BytesQueued;
}

internal gs_data
Win32Socket_Receive(win32_socket* Socket, gs_memory_arena* Storage)
{
#if 0
    gs_data Result = {};
    s32 BytesQueued = Win32Socket_PeekGetTotalSize(Socket);
    if (BytesQueued > 0)
    {
        Result = PushSizeToData(Storage, BytesQueued);
        s32 Flags = 0;
        s32 BytesReceived = recv(Socket->Socket, (char*)Result.Memory, Result.Size, Flags);
        Assert(BytesReceived == BytesQueued);
    }
    return Result;
#else
    gs_data Result = PushSizeToData(Storage, 1024);
    s32 Flags = 0;
    s32 BytesReceived = recv(Socket->Socket, (char*)Result.Memory, Result.Size, Flags);
    if (BytesReceived == SOCKET_ERROR)
    {
        // TODO(pjs): Error logging
        s32 Error = WSAGetLastError();
        switch (Error)
        {
            case WSAECONNABORTED:
            case WSANOTINITIALISED:
            break;
            
            case WSAENOTCONN:
            {
                
            }break;
            InvalidDefaultCase;
        }
    }
    Result.Size = BytesReceived;
    return Result;
#endif
}

internal void
Win32Socket_Close(win32_socket* Socket)
{
    closesocket(Socket->Socket);
    Socket->Socket = INVALID_SOCKET;
}

internal void
Win32Socket_CloseArray(win32_socket_array Array)
{
    for (s32 i = 0; i < Array.Count; i++)
    {
        win32_socket* Socket = Array.Values + i;
        Win32Socket_Close(Socket);
    }
}

//////////////////////
//
// Win32 Socket System

internal void
Win32SocketSystem_Init(gs_memory_arena* Arena)
{
    WSAStartup(MAKEWORD(2, 2), &WSAData);
    Win32Sockets = Win32SocketArray_Create(16, Arena);
}

internal void
Win32SocketSystem_Cleanup()
{
    Win32Socket_CloseArray(Win32Sockets);
    
    s32 CleanupResult = 0;
    do {
        CleanupResult = WSACleanup();
    }while(CleanupResult == SOCKET_ERROR);
}

PLATFORM_GET_SOCKET_HANDLE(Win32GetSocketHandle)
{
    s32 Result = Win32SocketArray_Take(&Win32Sockets);
    s32 Error = 0;
    win32_socket* Socket = Win32SocketArray_Get(Win32Sockets, Result);
    *Socket = Win32Socket_Create(AF_INET, SOCK_DGRAM, 0);
    Error = Win32Socket_SetOption(Socket, IPPROTO_IP, IP_MULTICAST_TTL,
                                  (const char*)(&Multicast_TimeToLive), sizeof(Multicast_TimeToLive));
    return (platform_socket_handle)Result;
}

#define WIN32_FOLDHAUS_SOCKET_H
#endif // WIN32_FOLDHAUS_SOCKET_H