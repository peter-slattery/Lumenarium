//
// File: first.cpp
// Author: Peter Slattery
// Creation Date: 2020-10-10
//
#ifndef FIRST_CPP


#include <windows.h>
#include <stdio.h>

#include "../gs_libs/gs_types.h"
#include "../gs_libs/gs_types.cpp"
#include "../app/engine/foldhaus_log.h"
global log_buffer* GlobalLogBuffer;

#define DEBUG_TRACK_FUNCTION


//#include "../app/foldhaus_platform.h"
//#include "../gs_libs/gs_win32.cpp"
#include "../app/platform_win32/win32_foldhaus_utils.h"
#include "../app/platform_win32/win32_foldhaus_memory.h"
#include "../app/platform_win32/win32_foldhaus_fileio.h"
#include "../app/platform_win32/win32_foldhaus_serial.h"
#include "../app/platform_win32/win32_foldhaus_work_queue.h"

#include "../app/engine/uart/foldhaus_uart.h"

u8*
FindNextHeader(gs_data Data, u8* StartAt)
{
    u8* At = StartAt;
    while (!(At[0] == 'U' &&
             At[1] == 'P' &&
             At[2] == 'X' &&
             At[3] == 'L') &&
           (u32)(At - Data.Memory) < Data.Size)
    {
        At++;
    }
    return At;
}

void
CreateMessage(gs_data* Data, u8 Count)
{
    gs_memory_cursor WriteCursor = CreateMemoryCursor(*Data);
    
    u32 Channels[] = {
        0, 1, 2, 3, 4, 5, 6, 7,
        16, 17, 18, 19, 20, 21, 22, 23,
        //40, 41, 42, 43, 44, 45, 46, 47,
    };
    
    u8* FirstHeaderAddr = 0;
    
    for (u32 j = 0; j < sizeof(Channels) / sizeof(u32); j++)
    {
        u32 ChannelIndex = Channels[j];
        uart_header* Header = PushStructOnCursor(&WriteCursor, uart_header);
        UART_FillHeader(Header, ChannelIndex, UART_SET_CHANNEL_WS2812);
        
        if (FirstHeaderAddr == 0)
        {
            FirstHeaderAddr = (u8*)Header;
        }
        
        uart_channel* Channel = PushStructOnCursor(&WriteCursor, uart_channel);
        Channel->ElementsCount = 3;
        Channel->ColorPackingOrder = 36; // 10010000
        Channel->PixelsCount = 300;
        
        for (u32 i = 0; i < Channel->PixelsCount; i++)
        {
            u8* Pixel = PushArrayOnCursor(&WriteCursor, u8, 3);
            Pixel[0] = Count;
            Pixel[1] = 0;
            Pixel[2] = 255 - Count;
        }
        
        uart_footer* Footer = PushStructOnCursor(&WriteCursor, uart_footer);
        Footer->CRC = UART_CalculateCRC((u8*)Header, (u8*)(Footer));
    }
    
    uart_header* DrawAllHeader = PushStructOnCursor(&WriteCursor, uart_header);
    UART_FillHeader(DrawAllHeader, 255, UART_DRAW_ALL);
    uart_footer* DrawAllFooter =
        PushStructOnCursor(&WriteCursor, uart_footer);
    DrawAllFooter->CRC = UART_CalculateCRC((u8*)DrawAllHeader, (u8*)(DrawAllFooter));
    
    Data->Size = ((u8*)DrawAllFooter - (u8*)FirstHeaderAddr) + sizeof(uart_footer);
}

int main(int ArgCount, char** Args)
{
    gs_thread_context Ctx = Win32CreateThreadContext();
    GlobalLogBuffer = AllocatorAllocStruct(Ctx.Allocator, log_buffer);
    *GlobalLogBuffer = Log_Init(Ctx.Allocator, 32);
    
    HANDLE SerialHandle = Win32SerialPort_Open("\\\\.\\COM9", Ctx.Transient);
    Win32SerialPort_SetState(SerialHandle, 2000000, 8, 0, 1);
    
    gs_const_string OutFileName = ConstString("./serial_dump.data");
    
    
    if (false)
    {
        Win32SerialPort_SetRead(SerialHandle);
        
        gs_data Data = PushSizeToData(Ctx.Transient, KB(32));
        
        Win32SerialPort_SetRead(SerialHandle);
        u32 ReadSize = Win32SerialPort_ReadMessageWhenReady(SerialHandle, Data);
        
        u8* SetChannelHeaderAddr = 0;
        uart_header* SetChannelHeader = 0;
        uart_header* DrawAllHeader = 0;
        u8* ScanAt = Data.Memory;
        do
        {
            ScanAt = FindNextHeader(Data, ScanAt);
            uart_header* Header = (uart_header*)ScanAt;
            
            if (Header->RecordType == UART_SET_CHANNEL_WS2812)
            {
                printf("Set Channel:\n");
                printf("    Channel: %d\n", Header->Channel);
                printf("    Pixels: %d\n", ((uart_channel*)(Header + 1))->PixelsCount);
                if (!SetChannelHeader)
                {
                    SetChannelHeaderAddr = (u8*)Header;
                    SetChannelHeader = Header;
                }
            }
            
            if (Header->RecordType == UART_DRAW_ALL)
            {
                printf("Draw All:\n");
                printf("    Channel: %d\n", Header->Channel);
                if (!DrawAllHeader)
                {
                    DrawAllHeader= Header;
                }
            }
            
            ScanAt += sizeof(uart_header);
        }while(((u32)(ScanAt - Data.Memory + sizeof(uart_header)) < Data.Size));
        
        uart_channel* Channel = (uart_channel*)(SetChannelHeader + 1);
        
        u8* DataStart = (u8*)(Channel + 1);
        
        uart_footer* Footer = (uart_footer*)(DataStart + (Channel->ElementsCount * Channel->PixelsCount));
        
        u32 TestCRC = UART_CalculateCRC((u8*)SetChannelHeader, (u8*)(Footer));
        
        uart_footer* DrawAllFooter = (uart_footer*)(DrawAllHeader + 1);
        u32 DrawwAllCRC = UART_CalculateCRC((u8*)DrawAllHeader, (u8*)(DrawAllFooter));
        
        HANDLE FileHandle = CreateFileA(OutFileName.Str, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (FileHandle  != INVALID_HANDLE_VALUE)
        {
            DWORD BytesWritten = 0;
            if (!WriteFile(FileHandle, Data.Memory, Data.Size, &BytesWritten, NULL))
            {
                InvalidCodePath;
            }
        }
        CloseHandle(FileHandle);
        Win32SerialPort_Close(SerialHandle);
        
    }
    else if (true)
    {
        gs_data Data = PushSizeToData(Ctx.Transient, KB(32));
        
        u8 Count = 0;
        while(true)
        {
            CreateMessage(&Data, ++Count);
            Win32SerialPort_Write(SerialHandle, Data);
            Sleep(100);
        }
    }
    else if (false)
    {
        gs_data Data = PushSizeToData(Ctx.Transient, KB(32));
        gs_file File = Win32ReadEntireFile(Ctx.FileHandler, OutFileName, Data);
        
        gs_data Messages = {0};
        u8* ScanAt = Data.Memory;
        ScanAt = FindNextHeader(Data, ScanAt);
        uart_header* FirstHeader = (uart_header*)ScanAt;
        ScanAt += sizeof(uart_header);
        
        uart_header* LastHeader = 0;
        do
        {
            ScanAt = FindNextHeader(Data, ScanAt);
            uart_header* Header = (uart_header*)ScanAt;
            if (Header->RecordType == UART_DRAW_ALL)
            {
                LastHeader = Header;
            }
            ScanAt += sizeof(uart_header);
        }while((u32)(ScanAt - Data.Memory) < Data.Size);
        
        u8* OnePastLastByte = ((u8*)(LastHeader + 1)) + sizeof(uart_footer);
        
        Messages.Memory = (u8*)FirstHeader;
        Messages.Size = OnePastLastByte - Messages.Memory;
        
        while (true)
        {
            Win32SerialPort_Write(SerialHandle, Messages);
            Sleep(100);
        }
    }
    
    return 0;
}


#define FIRST_CPP
#endif // FIRST_CPP