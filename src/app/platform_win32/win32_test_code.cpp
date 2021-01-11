//
// File: win32_test_code.cpp
// Author: Peter Slattery
// Creation Date: 2021-01-10
//
#ifndef WIN32_TEST_CODE_CPP

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

internal void
Win32_TestCode_SocketReading(gs_thread_context ThreadContext)
{
    win32_socket TestSocket = Win32Socket_ConnectToAddress("127.0.0.1", "20185");
    test_microphone_packet* Recv = 0;
    while (true)
    {
        gs_data Data = Win32Socket_Receive(&TestSocket, ThreadContext.Transient);
        if (Data.Size > 0)
        {
            OutputDebugStringA("Received\n");
            Recv = (test_microphone_packet*)Data.Memory;
        }
        ClearArena(ThreadContext.Transient);
    }
}

#define WIN32_TEST_CODE_CPP
#endif // WIN32_TEST_CODE_CPP