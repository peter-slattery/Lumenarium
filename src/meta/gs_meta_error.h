
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

internal void 
PushFError (errors* Errors, char* Format, ...)
{
    if (Errors->Used >= (Errors->BuffersCount * ERROR_BUFFER_SIZE))
    {
        Errors->BuffersCount += 1;
        Errors->Buffers = (error_buffer*)realloc(Errors->Buffers, sizeof(error_buffer*) * Errors->BuffersCount);
        
        error_buffer* NewBuffer = Errors->Buffers + (Errors->BuffersCount - 1);
        NewBuffer->Backbuffer = (char*)malloc(sizeof(char) * ERROR_MAX_LENGTH * ERROR_BUFFER_SIZE);
        NewBuffer->Contents = (string*)malloc(sizeof(string) * ERROR_BUFFER_SIZE);
        
        for (u32 i = 0; i < ERROR_BUFFER_SIZE; i++)
        {
            NewBuffer->Contents[i].Memory = NewBuffer->Backbuffer + (i * ERROR_MAX_LENGTH);
            NewBuffer->Contents[i].Max = ERROR_MAX_LENGTH;
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
