//
// File: foldhaus_uart.cpp
// Author: Peter Slattery
// Creation Date: 2020-10-10
//
#ifndef FOLDHAUS_UART_CPP


internal void
UART_SetChannelBuffer_Create(gs_memory_cursor* WriteCursor, uart_channel ChannelSettings, v2_strip Strip, led_buffer LedBuffer)
{
    // NOTE(pjs): This is just here because the information is duplicated and I want to be sure
    // to catch the error where they are different
    Assert(ChannelSettings.PixelsCount == Strip.LedCount);
    
    uart_header* Header = PushStructOnCursor(WriteCursor, uart_header);
    UART_FillHeader(Header, Strip.UARTAddr.Channel, UART_SET_CHANNEL_WS2812);
    
    uart_channel* Channel = PushStructOnCursor(WriteCursor, uart_channel);
    *Channel = ChannelSettings;
    
    for (u32 i = 0; i < Channel->PixelsCount; i++)
    {
        u32 LedIndex = Strip.LedLUT[i];
        pixel Color = LedBuffer.Colors[LedIndex];
        
        u8* OutputPixel = PushArrayOnCursor(WriteCursor, u8, 3);
        
        // TODO(pjs): Use the Output mask
        OutputPixel[0] = Color.R;
        OutputPixel[1] = Color.G;
        OutputPixel[2] = Color.B;
        
        if (Channel->ElementsCount == 4)
        {
            // TODO(pjs): Calculate white from the RGB components?
            //            Generally we just need a good way to handle the white channel,
            //            both in the renderer and in output
            
            //OutputPixel[Channel->WhiteIndex] = Color.W;
        }
    }
    
    uart_footer* Footer = PushStructOnCursor(WriteCursor, uart_footer);
    UART_FillFooter(Footer, (u8*)Header);
}

internal void
UART_DrawAll_Create(gs_memory_cursor* WriteCursor)
{
    uart_header* Header = PushStructOnCursor(WriteCursor, uart_header);
    UART_FillHeader(Header, 1, UART_DRAW_ALL);
    
    uart_footer* Footer = PushStructOnCursor(WriteCursor, uart_footer);
    UART_FillFooter(Footer, (u8*)Header);
}

internal void
UART_BuildOutputData(addressed_data_buffer_list* Output, assembly_array Assemblies, led_system* LedSystem)
{
    uart_channel ChannelSettings = {0};
    ChannelSettings.ElementsCount = 3;
    ChannelSettings.ColorPackingOrder = 36;
    
    // NOTE(pjs): This is the minimum size of every UART message. SetChannelBuffer messages will
    // be bigger than this, but their size is based on the number of pixels in each channel
    u32 MessageBaseSize = sizeof(uart_header) + sizeof(uart_channel) + sizeof(uart_footer);
    
    for (u32 AssemblyIdx = 0; AssemblyIdx < Assemblies.Count; AssemblyIdx++)
    {
        assembly Assembly = Assemblies.Values[AssemblyIdx];
        led_buffer* LedBuffer = LedSystemGetBuffer(LedSystem, Assembly.LedBufferIndex);
        
        u32 TotalBufferSize = MessageBaseSize * Assembly.StripCount; // SetChannelBuffer messages
        TotalBufferSize += MessageBaseSize; // DrawAll message
        TotalBufferSize += ChannelSettings.ElementsCount * Assembly.LedCountTotal; // pixels * channels per pixel
        
        addressed_data_buffer* Buffer = AddressedDataBufferList_Push(Output, TotalBufferSize);
        AddressedDataBuffer_SetCOMPort(Buffer, Assembly.UARTComPort);
        gs_memory_cursor WriteCursor = CreateMemoryCursor(Buffer->Data);
        
        for (u32 StripIdx = 0; StripIdx < Assembly.StripCount; StripIdx++)
        {
            v2_strip StripAt = Assembly.Strips[StripIdx];
            ChannelSettings.PixelsCount = StripAt.LedCount;
            UART_SetChannelBuffer_Create(&WriteCursor, ChannelSettings, StripAt, *LedBuffer);
        }
        
        UART_DrawAll_Create(&WriteCursor);
    }
}


#define FOLDHAUS_UART_CPP
#endif // FOLDHAUS_UART_CPP