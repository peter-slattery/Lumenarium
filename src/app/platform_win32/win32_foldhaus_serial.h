//
// File: win32_serial.h
// Author: Peter Slattery
// Creation Date: 2020-10-01
//
#ifndef WIN32_SERIAL_H

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
    
    if (ComPortHandle == INVALID_HANDLE_VALUE)
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
    }
}

#define WIN32_SERIAL_H
#endif // WIN32_SERIAL_H