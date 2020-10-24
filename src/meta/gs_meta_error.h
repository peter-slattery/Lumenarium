
struct error_buffer
{
    char* Backbuffer;
    string* Contents;
};

#define ERROR_MAX_LENGTH 256
#define ERROR_BUFFER_SIZE 256
struct errors
{
    error_buffer* Buffers;
    u32 BuffersCount;
    u32 Used;
};

#define ErrorReallocArray(base, type, oldcount, newcount) (type*)ErrorRealloc_((u8*)(base), (u64)(sizeof(type) * oldcount), (u64)(sizeof(type) * newcount))
internal u8*
ErrorRealloc_(u8* Base, u64 OldSize, u64 NewSize)
{
    Assert(NewSize > 0);
    u8* Result = (u8*)malloc(NewSize);
    if (Base != 0 && OldSize > 0)
    {
        GSMemCopy(Base, Result, OldSize);
        free(Base);
    }
    return Result;
}

internal void
PushFError (errors* Errors, char* Format, ...)
{
    if (Errors->Used >= (Errors->BuffersCount * ERROR_BUFFER_SIZE))
    {
#if 0
        Errors->BuffersCount += 1;
        Errors->Buffers = (error_buffer*)realloc(Errors->Buffers, sizeof(error_buffer*) * Errors->BuffersCount);
#else
        Errors->Buffers = ErrorReallocArray(Errors->Buffers, error_buffer, Errors->BuffersCount, Errors->BuffersCount + 1);
        Errors->BuffersCount += 1;
#endif
        
        error_buffer* NewBuffer = Errors->Buffers + (Errors->BuffersCount - 1);
        NewBuffer->Backbuffer = (char*)malloc(sizeof(char) * ERROR_MAX_LENGTH * ERROR_BUFFER_SIZE);
        NewBuffer->Contents = (string*)malloc(sizeof(string) * ERROR_BUFFER_SIZE);
        
        for (u32 i = 0; i < ERROR_BUFFER_SIZE; i++)
        {
            NewBuffer->Contents[i].Str = NewBuffer->Backbuffer + (i * ERROR_MAX_LENGTH);
            NewBuffer->Contents[i].Size = ERROR_MAX_LENGTH;
            NewBuffer->Contents[i].Length = 0;
        }
    }
    
    u32 NewErrorIndex = Errors->Used++;
    u32 BufferIndex = NewErrorIndex / ERROR_BUFFER_SIZE;
    u32 IndexInBuffer = NewErrorIndex % ERROR_BUFFER_SIZE;
    string* NewError = Errors->Buffers[BufferIndex].Contents + IndexInBuffer;
    
    va_list Args;
    va_start(Args, Format);
    NewError->Length = PrintFArgsList(NewError->Memory, NewError->Max, Format, Args);
    va_end(Args);
}

internal string*
TakeError (errors* Errors)
{
    u32 NewErrorIndex = Errors->Used++;
    u32 BufferIndex = NewErrorIndex / ERROR_BUFFER_SIZE;
    u32 IndexInBuffer = NewErrorIndex % ERROR_BUFFER_SIZE;
    string* NewError = Errors->Buffers[BufferIndex].Contents + IndexInBuffer;
    return NewError;
}

internal void
PrintAllErrors (errors Errors)
{
    for (u32 i = 0; i < Errors.Used; i++)
    {
        u32 BufferIndex = i / ERROR_BUFFER_SIZE;
        u32 IndexInBuffer = i % ERROR_BUFFER_SIZE;
        string Error = Errors.Buffers[BufferIndex].Contents[IndexInBuffer];
        printf("%.*s\n", StringExpand(Error));
    }
}
