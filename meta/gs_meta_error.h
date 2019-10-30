#define MAX_ERROR_LENGTH 256

struct error
{
    char Backbuffer[MAX_ERROR_LENGTH];
    string Buffer;
};

struct error_list_buffer
{
    error* List;
    s32 Used;
    s32 Max;
    error_list_buffer* Next;
};

struct error_list
{
    error_list_buffer* Head;
    s32 TotalUsed;
    s32 TotalMax;
};

#define ERROR_LIST_GROW_SIZE 32

internal void
GrowErrorList (error_list* Errors)
{
    Assert(Errors);
    
    s32 ExtensionSize = sizeof(error_list_buffer) + (sizeof(error) * ERROR_LIST_GROW_SIZE);
    u8* ExtensionMemory = (u8*)malloc(ExtensionSize);
    
    error_list_buffer* Extension = (error_list_buffer*)ExtensionMemory;
    Extension->Used = 0;
    Extension->Max = ERROR_LIST_GROW_SIZE;
    Extension->List = (error*)(ExtensionMemory + sizeof(error_list_buffer));
    
    Extension->Next = Errors->Head;
    Errors->Head = Extension;
    
    Errors->TotalMax += ERROR_LIST_GROW_SIZE;
}

#define ErrorAssert(condition, list, format, ...) \
if (!(condition)) { LogError_((list), __FILE__, __LINE__, (format), __VA_ARGS__); }

#define LogError(list, format, ...) LogError_((list), __FILE__, __LINE__, (format), __VA_ARGS__)

internal void
LogError_ (error_list* Errors, char* File, s32 Line, char* Format, ...)
{
    if (Errors->TotalUsed >= Errors->TotalMax)
    {
        GrowErrorList(Errors);
    }
    
    error* Error = (Errors->Head->List + Errors->Head->Used++);
    Errors->TotalUsed++;
    
    InitializeEmptyString(&Error->Buffer, Error->Backbuffer, MAX_ERROR_LENGTH);
    
    va_list Args;
    va_start(Args, Format);
    
    PrintF(&Error->Buffer, "%s(%d): ", File, Line);
    Error->Buffer.Length += PrintFArgsList(Error->Buffer.Memory + Error->Buffer.Length,
                                           Error->Buffer.Max - Error->Buffer.Length,
                                           Format, Args);
    
    va_end(Args);
}

internal void
PrintErrorListBuffer (error_list_buffer* Buffer)
{
    if (Buffer->Next)
    {
        PrintErrorListBuffer(Buffer->Next);
    }
    
    for (s32 i = 0; i < Buffer->Used; i++)
    {
        error* Error = Buffer->List + i;
        NullTerminate(&Error->Buffer);
        printf(Error->Backbuffer);
        printf("\n");
    }
}

internal void
PrintErrorList (error_list Errors)
{
    if (Errors.Head)
    {
        PrintErrorListBuffer(Errors.Head);
    }
}