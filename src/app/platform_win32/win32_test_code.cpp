//
// File: win32_test_code.cpp
// Author: Peter Slattery
// Creation Date: 2021-01-10
//
#ifndef WIN32_TEST_CODE_CPP

#if 0
internal void
Win32_TestCode_UART(gs_thread_context ThreadContext)
{
    u32 LedCount = 48;
    u32 MessageBaseSize = sizeof(uart_header) + sizeof(uart_channel) + sizeof(uart_footer);
    MessageBaseSize += sizeof(u8) * 3 * LedCount;
    gs_data MessageBuffer = PushSizeToData(ThreadContext.Transient);
    
    gs_memory_cursor WriteCursor = CreateMemoryCursor(MessageBuffer);
    
    uart_header* Header = PushStructOnCursor(WriteCursor, uart_header);
    UART_FillHeader(Header, Strip.UARTAddr.Channel, UART_SET_CHANNEL_WS2812);
    uart_channel* Channel = PushStructOnCursor(WriteCursor, uart_channel);
    *Channel = ChannelSettings;
    
    for (u32 i = 0; i < LedCount; i++)
    {
        u8* OutputPixel = PushArrayOnCursor(WriteCursor, u8, 3);
        OutputPixel[Channel->RedIndex] = (u8)(i);
        OutputPixel[Channel->GreenIndex] = 0;
        OutputPixel[Channel->BlueIndex] = 0;
    }
    
    uart_footer* Footer = PushStructOnCursor(WriteCursor, uart_footer);
    UART_FillFooter(Footer, (u8*)Header);
}
#endif

win32_socket ListenSocket;

DWORD WINAPI
Win32_TestCode_ListenThreadProc(LPVOID ThreadData)
{
    gs_thread_context Ctx = Win32CreateThreadContext();
    
    temp_job_req* Req = (temp_job_req*)ThreadData;
    
    while (true)
    {
        Req->Proc(&Ctx, Req->Memory);
    }
}

internal void
Win32_TestCode_SocketReading(gs_thread_context ThreadContext, temp_job_req* Req)
{
    ListenSocket = Win32Socket_ConnectToAddress("127.0.0.1", "20185");
    u8* Arg = (u8*)Req;
    HANDLE Handle = CreateThread(0, 0, &Win32_TestCode_ListenThreadProc, Arg, 0, 0);
}

internal void
BlumenLumen_MicListenJob(gs_thread_context* Ctx, u8* UserData)
{
    packet_ringbuffer* MicPacketBuffer = (packet_ringbuffer*)UserData;
    
    gs_data Data = Win32Socket_Receive(&ListenSocket, Ctx->Transient);
    if (Data.Size > 0)
    {
        OutputDebugStringA("Listened");
        MicPacketBuffer->Values[MicPacketBuffer->WriteHead++] = Data;
        if (MicPacketBuffer->WriteHead >= PACKETS_MAX)
        {
            MicPacketBuffer->WriteHead = 0;
        }
    }
}

#define WIN32_TEST_CODE_CPP
#endif // WIN32_TEST_CODE_CPP