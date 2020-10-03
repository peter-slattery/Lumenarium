//
// File: win32_serial.h
// Author: Peter Slattery
// Creation Date: 2020-10-01
//
#ifndef WIN32_SERIAL_H

global u32        Win32SerialHandlesCountMax;
global u32        Win32SerialHandlesCount;
global HANDLE*    Win32SerialHandles;
global gs_string* Win32SerialPortNames;

DCB
Win32SerialPort_GetState(HANDLE ComPortHandle)
{
    DCB ControlSettings = {0};
    ZeroStruct(&ControlSettings);
    ControlSettings.DCBlength = sizeof(ControlSettings);
    
    bool Success = GetCommState(ComPortHandle, &ControlSettings);
    Assert(Success);
    
    return ControlSettings;
}

void
Win32SerialPort_SetState(HANDLE ComPortHandle, u32 BaudRate, u8 ByteSize, u8 Parity, u8 StopBits)
{
    DCB ControlSettings = Win32SerialPort_GetState(ComPortHandle);
    //PrintCommState(ControlSettings);
    
    // TODO(pjs): Validate BaudRate - There's only certain rates that are valid right?
    ControlSettings.BaudRate = BaudRate;
    ControlSettings.ByteSize = ByteSize;
    ControlSettings.Parity = Parity;
    ControlSettings.StopBits = StopBits;
    
    bool Success = SetCommState(ComPortHandle, &ControlSettings);
}

HANDLE
Win32SerialPort_Open(char* PortName)
{
    HANDLE ComPortHandle = CreateFile(PortName,
                                      GENERIC_READ | GENERIC_WRITE,
                                      FILE_SHARE_READ | FILE_SHARE_WRITE,
                                      NULL, // Default Security Attr
                                      OPEN_EXISTING,
                                      0, // Not overlapped I/O
                                      NULL);
    
    if (ComPortHandle != INVALID_HANDLE_VALUE)
    {
        COMMTIMEOUTS Timeouts = { 0 };
        Timeouts.ReadIntervalTimeout         = 50; // in milliseconds
        Timeouts.ReadTotalTimeoutConstant    = 50; // in milliseconds
        Timeouts.ReadTotalTimeoutMultiplier  = 10; // in milliseconds
        Timeouts.WriteTotalTimeoutConstant   = 50; // in milliseconds
        Timeouts.WriteTotalTimeoutMultiplier = 10; // in milliseconds
        
        if (SetCommTimeouts(ComPortHandle, &Timeouts))
        {
            
        }
        else
        {
            s32 Error = GetLastError();
            InvalidCodePath;
        }
    }
    else
    {
        // Error
        s32 Error = GetLastError();
        InvalidCodePath;
    }
    
    return ComPortHandle;
}

void
Win32SerialPort_Close(HANDLE PortHandle)
{
    CloseHandle(PortHandle);
}

void
Win32SerialPort_Write(HANDLE PortHandle, gs_data Buffer)
{
    Assert(PortHandle != INVALID_HANDLE_VALUE);
    
    DWORD BytesWritten = 0;
    if (WriteFile(PortHandle, Buffer.Memory, Buffer.Size, &BytesWritten, NULL))
    {
        if (BytesWritten != Buffer.Size)
        {
            OutputDebugString("Error: Entire buffer not written.\n");
        }
    }
    else
    {
        OutputDebugStringA("Error: Unable to write to port\n");
        s32 Error = GetLastError();
        //InvalidCodePath;
    }
}

/////////////////////////
// Win32SerialArray

void
Win32SerialArray_Create(gs_thread_context Context)
{
    Win32SerialHandlesCountMax = 32;
    Win32SerialHandlesCount = 0;
    Win32SerialHandles = AllocatorAllocArray(Context.Allocator, HANDLE, Win32SerialHandlesCountMax);
    Win32SerialPortNames = AllocatorAllocArray(Context.Allocator, gs_string, Win32SerialHandlesCountMax);
    for (u32 i = 0; i < Win32SerialHandlesCountMax; i++)
    {
        Win32SerialPortNames[i] = AllocatorAllocString(Context.Allocator, 256);
    }
}

void
Win32SerialArray_Push(HANDLE SerialHandle, gs_const_string PortName)
{
    Assert(Win32SerialHandlesCount < Win32SerialHandlesCountMax);
    u32 Index = Win32SerialHandlesCount++;
    Win32SerialHandles[Index] = SerialHandle;
    PrintF(&Win32SerialPortNames[Index], "%S", PortName);
}

HANDLE
Win32SerialArray_Get(gs_const_string PortName)
{
    HANDLE PortHandle = INVALID_HANDLE_VALUE;
    for (u32 i = 0; i < Win32SerialHandlesCount; i++)
    {
        if (StringsEqual(Win32SerialPortNames[i].ConstString, PortName))
        {
            PortHandle = Win32SerialHandles[i];
            break;
        }
    }
    return PortHandle;
}

HANDLE
Win32SerialArray_GetOrOpen(gs_const_string PortName, u32 BaudRate, u8 ByteSize, u8 Parity, u8 StopBits)
{
    HANDLE PortHandle = Win32SerialArray_Get(PortName);
    if (PortHandle == INVALID_HANDLE_VALUE)
    {
        Assert(IsNullTerminated(PortName));
        PortHandle = Win32SerialPort_Open(PortName.Str);
        Win32SerialPort_SetState(PortHandle, BaudRate, ByteSize, Parity, StopBits);
        Win32SerialArray_Push(PortHandle, PortName);
    }
    return PortHandle;
}

#define WIN32_SERIAL_H
#endif // WIN32_SERIAL_H