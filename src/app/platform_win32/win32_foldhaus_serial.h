//
// File: win32_serial.h
// Author: Peter Slattery
// Creation Date: 2020-10-01
//
#ifndef WIN32_SERIAL_H

global u32        Win32SerialHandlesCountMax;
global HANDLE*    Win32SerialHandles;
global gs_string* Win32SerialPortNames;
global s32*       Win32SerialPortFilled;

DCB
Win32SerialPort_GetState(HANDLE ComPortHandle)
{
  DEBUG_TRACK_FUNCTION;
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
  DEBUG_TRACK_FUNCTION;
  DCB ControlSettings = Win32SerialPort_GetState(ComPortHandle);
  
  // TODO(pjs): Validate BaudRate - There's only certain rates that are valid right?
  ControlSettings.BaudRate = BaudRate;
  
  if (Parity == NOPARITY)
  {
    ControlSettings.Parity = Parity;
    ControlSettings.fParity = 0;
  }
  if (Parity == EVENPARITY || Parity == ODDPARITY)
  {
    ControlSettings.Parity = Parity;
    ControlSettings.fParity = 1;
  }
  
  ControlSettings.StopBits = StopBits;
  ControlSettings.ByteSize = ByteSize;
  
  ControlSettings.fBinary = true;
  
  ControlSettings.fOutxCtsFlow = false;
  ControlSettings.fOutxDsrFlow = false;
  ControlSettings.fDtrControl = DTR_CONTROL_DISABLE;
  ControlSettings.fDsrSensitivity = 0;
  ControlSettings.fRtsControl = RTS_CONTROL_DISABLE;
  ControlSettings.fOutX = false;
  ControlSettings.fInX = false;
  
  ControlSettings.fErrorChar = 0;
  ControlSettings.fNull = false;
  ControlSettings.fAbortOnError = false;
  ControlSettings.wReserved = false;
  ControlSettings.XonLim = 2;
  ControlSettings.XoffLim = 4;
  ControlSettings.XonChar = 0x13;
  ControlSettings.XoffChar = 0x19;
  ControlSettings.EvtChar = 0;
  
  bool Success = SetCommState(ComPortHandle, &ControlSettings);
}

gs_const_string_array
Win32SerialPorts_List(gs_memory_arena* Arena, gs_memory_arena* Transient)
{
  gs_const_string_array Result = {};
  
  DWORD SizeNeeded0 = 0;
  DWORD CountReturned0 = 0;
  EnumPorts(NULL, 1, 0, 0, &SizeNeeded0, &CountReturned0);
  Assert(GetLastError() == ERROR_INSUFFICIENT_BUFFER);
  
  DWORD SizeNeeded1 = 0;
  DWORD CountReturned1 = 0;
  PORT_INFO_1* PortsArray = (PORT_INFO_1*)PushSize(Transient, SizeNeeded0).Memory;
  if (EnumPorts(NULL, 
                1, 
                (u8*)PortsArray, 
                SizeNeeded0, 
                &SizeNeeded1, 
                &CountReturned1))
  {
    Result.CountMax = (u64)CountReturned1;
    Result.Strings = PushArray(Arena, gs_const_string, Result.CountMax);
    
    for (; Result.Count < Result.CountMax; Result.Count++)
    {
      u64 Index = Result.Count;
      u64 StrLen = CStringLength(PortsArray[Index].pName);
      gs_string Str = PushString(Arena, StrLen);
      PrintF(&Str, "%.*s", StrLen, PortsArray[Index].pName);
      Result.Strings[Result.Count] = Str.ConstString;
    }
  }
  
  return Result;
}

bool
Win32SerialPort_Exists(char* PortName, gs_memory_arena* Transient)
{
  bool Result = false;
  if (PortName != 0)
  {
    gs_const_string PortIdent = ConstString(PortName);
    u32 IdentBegin = FindLast(PortIdent, '\\') + 1;
    PortIdent = Substring(PortIdent, IdentBegin, PortIdent.Length);
    
    gs_const_string_array PortsAvailable = Win32SerialPorts_List(Transient, Transient);
    
    for (u64 i = 0; i < PortsAvailable.Count; i++)
    {
      gs_const_string AvailablePortName = PortsAvailable.Strings[i];
      if (StringsEqualUpToLength(AvailablePortName, PortIdent, PortIdent.Length))
      {
        Result = true;
        break;
      }
    }
  }
  return Result;
}

HANDLE
Win32SerialPort_Open(char* PortName, gs_memory_arena* Transient)
{
  DEBUG_TRACK_FUNCTION;
  HANDLE ComPortHandle = INVALID_HANDLE_VALUE;;
  
  if (Win32SerialPort_Exists(PortName, Transient))
  {
    
    ComPortHandle = CreateFile(PortName,
                               GENERIC_READ | GENERIC_WRITE,
                               FILE_SHARE_READ | FILE_SHARE_WRITE,
                               NULL, // Default Security Attr
                               OPEN_EXISTING,
                               0, // Not overlapped I/O
                               NULL);
    
    bool HasError = false;
    
    if (ComPortHandle != INVALID_HANDLE_VALUE)
    {
      COMMTIMEOUTS Timeouts = { 0 };
      Timeouts.ReadIntervalTimeout         = 0; // in milliseconds
      Timeouts.ReadTotalTimeoutConstant    = 0; // in milliseconds
      Timeouts.ReadTotalTimeoutMultiplier  = 0; // in milliseconds
      Timeouts.WriteTotalTimeoutConstant   = 0; // in milliseconds
      Timeouts.WriteTotalTimeoutMultiplier = 0; // in milliseconds
      
      HasError = !SetCommTimeouts(ComPortHandle, &Timeouts);
    }
    else
    {
      HasError = true;
    }
    
    if (HasError) 
    {
      // Error
      s32 Error = GetLastError();
      switch (Error)
      {
        case ERROR_INVALID_FUNCTION:
        case ERROR_NO_SUCH_DEVICE:
        case ERROR_FILE_NOT_FOUND:
        {
          // NOTE(PS): The outer scope should handle these cases
          ComPortHandle = INVALID_HANDLE_VALUE;
        }break;
        
        InvalidDefaultCase;
      }
    }
  }
  
  return ComPortHandle;
}

void
Win32SerialPort_Close(HANDLE PortHandle)
{
  CloseHandle(PortHandle);
}

bool
Win32SerialPort_Write(HANDLE PortHandle, gs_data Buffer)
{
  DEBUG_TRACK_FUNCTION;
  Assert(PortHandle != INVALID_HANDLE_VALUE);
  bool Success = false;
  
  DWORD BytesWritten = 0;
  if (WriteFile(PortHandle, Buffer.Memory, Buffer.Size, &BytesWritten, NULL))
  {
    Success = (BytesWritten == Buffer.Size);
    if (!Success)
    {
      Log_Error(GlobalLogBuffer, "Error: Entire buffer not written.\n");
    }
  }
  else
  {
    Log_Error(GlobalLogBuffer, "Error: Unable to write to port\n");
    s32 Error = GetLastError();
    switch (Error)
    {
      case ERROR_OPERATION_ABORTED:
      case ERROR_GEN_FAILURE:
      {
        // NOTE(pjs): Probably means that the serial port became invalid
        // ie. the usb stick was removed
      }break;
      
      case ERROR_ACCESS_DENIED:
      {
        // ??
      }break;
      
      case ERROR_NO_SUCH_DEVICE:
      {
      }break;
      
      case ERROR_INVALID_HANDLE:
      InvalidDefaultCase;
    }
  }
  
  return Success;
}

bool
Win32SerialPort_SetRead(HANDLE PortHandle)
{
  bool Status = SetCommMask(PortHandle, EV_RXCHAR);
  return Status;
}

u32
Win32SerialPort_ReadMessageWhenReady(HANDLE PortHandle, gs_data Data)
{
  u32 ReadSize = 0;
  
  DWORD EventMask = 0;
  bool Status = WaitCommEvent(PortHandle, &EventMask, NULL);
  if (Status)
  {
    DWORD NoBytesRead = 0;
    do
    {
      u8 Byte = 0;
      Status = ReadFile(PortHandle, &Byte, sizeof(char), &NoBytesRead, NULL);
      Data.Memory[ReadSize] = Byte;
      ReadSize++;
    }
    while (NoBytesRead > 0 && ReadSize < Data.Size);
  }
  //Read data and store in a buffer
  
  return ReadSize;
}

/////////////////////////
// Win32SerialArray

void
Win32SerialArray_Create(gs_memory_arena* A)
{
  DEBUG_TRACK_FUNCTION;
  
  Win32SerialHandlesCountMax = 32;
  
  Win32SerialHandles = PushArray(A, HANDLE, Win32SerialHandlesCountMax);
  Win32SerialPortNames = PushArray(A, gs_string, Win32SerialHandlesCountMax);
  Win32SerialPortFilled = PushArray(A, s32, Win32SerialHandlesCountMax);
  
  u64 PortNameSize = 256;
  u64 PortNameBufferSize = PortNameSize * Win32SerialHandlesCountMax;
  char* PortNameBuffer = PushArray(A, char, PortNameBufferSize);
  for (u32 i = 0; i < Win32SerialHandlesCountMax; i++)
  {
    char* NameBase = PortNameBuffer + (PortNameSize * i);
    Win32SerialPortNames[i] = MakeString(NameBase, 0, PortNameSize);
    Win32SerialPortFilled[i] = 0;
  }
}

void
Win32SerialArray_Push(HANDLE SerialHandle, gs_const_string PortName)
{
  DEBUG_TRACK_FUNCTION;
  
  bool Found = false;
  for (u32 i = 0; i < Win32SerialHandlesCountMax; i++)
  {
    bool WasFilled = InterlockedCompareExchange((LONG volatile*)Win32SerialPortFilled + i, 1, 0);
    if (!WasFilled)
    {
      Win32SerialHandles[i] = SerialHandle;
      PrintF(&Win32SerialPortNames[i], "%S", PortName);
      Found = true;
      break;
    }
  }
  Assert(Found);
}

void
Win32SerialArray_Pop(u32 Index)
{
  bool WasFilled = InterlockedCompareExchange((LONG volatile*)Win32SerialPortFilled + Index, 0, 1);
  Assert(WasFilled);
  Win32SerialPortFilled[Index] = false;
  Win32SerialHandles[Index] = INVALID_HANDLE_VALUE;
}

HANDLE
Win32SerialArray_Get(gs_const_string PortName)
{
  DEBUG_TRACK_FUNCTION;
  
  HANDLE PortHandle = INVALID_HANDLE_VALUE;
  for (u32 i = 0; i < Win32SerialHandlesCountMax; i++)
  {
    if (Win32SerialPortFilled[i] &&
        StringsEqual(Win32SerialPortNames[i].ConstString, PortName))
    {
      PortHandle = Win32SerialHandles[i];
      break;
    }
  }
  return PortHandle;
}

HANDLE
Win32SerialArray_GetOrOpen(gs_const_string PortName, u32 BaudRate, u8 ByteSize, u8 Parity, u8 StopBits, gs_memory_arena* Transient)
{
  DEBUG_TRACK_FUNCTION;
  
  HANDLE PortHandle = Win32SerialArray_Get(PortName);
  if (PortHandle == INVALID_HANDLE_VALUE)
  {
    Assert(IsNullTerminated(PortName));
    PortHandle = Win32SerialPort_Open(PortName.Str, Transient);
    if (PortHandle != INVALID_HANDLE_VALUE)
    {
      Win32SerialPort_SetState(PortHandle, BaudRate, ByteSize, Parity, StopBits);
      Win32SerialArray_Push(PortHandle, PortName);
    }
  }
  return PortHandle;
}

void
Win32SerialArray_Close(gs_const_string PortName)
{
  for (u32 i = 0; i < Win32SerialHandlesCountMax; i++)
  {
    if (Win32SerialPortFilled[i] && StringsEqual(Win32SerialPortNames[i].ConstString, PortName))
    {
      Win32SerialPort_Close(Win32SerialHandles[i]);
      Win32SerialArray_Pop(i);
      break;
    }
  }
}

#define WIN32_SERIAL_H
#endif // WIN32_SERIAL_H