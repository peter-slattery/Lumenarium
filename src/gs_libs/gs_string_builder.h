//
// File: gs_string_builder.h
// Author: Peter Slattery
// Creation Date: 2020-01-01
//
#ifndef GS_STRING_BUILDER_H

#ifndef GS_STRING_BUILDER_NO_STDIO
#include <stdio.h>
#endif

#define STRING_BUILDER_BUFFER_CAPACITY 4096
struct string_builder_buffer
{
    u8* BufferMemory;
    string String;
    string_builder_buffer* Next;
};

struct string_builder
{
    string_builder_buffer* Buffers;
    string_builder_buffer* Head;
};

internal void
GrowStringBuilder(string_builder* StringBuilder)
{
    u8* BufferAndHeader = (u8*)malloc(sizeof(string_builder_buffer) + STRING_BUILDER_BUFFER_CAPACITY);
    string_builder_buffer* NewBuffer = (string_builder_buffer*)BufferAndHeader;
    *NewBuffer = {0};
    
    NewBuffer->BufferMemory = (u8*)(NewBuffer + 1);
    NewBuffer->String = MakeString((char*)NewBuffer->BufferMemory, 0, STRING_BUILDER_BUFFER_CAPACITY);
    
    if (!StringBuilder->Buffers)
    {
        StringBuilder->Buffers = NewBuffer;
        StringBuilder->Head = NewBuffer;
    } 
    else
    {
        StringBuilder->Head->Next = NewBuffer;
        StringBuilder->Head = NewBuffer;
    }
}

internal void
Write(string Text, string_builder* StringBuilder)
{
    string TextLeft = Text;
    
    if (StringBuilder->Buffers == 0)
    {
        GrowStringBuilder(StringBuilder);
    }
    
    while (TextLeft.Length > 0)
    {
        // Copy what there is room for
        s32 SpaceAvailable  = StringBuilder->Head->String.Max - StringBuilder->Head->String.Length;
        
        ConcatString(TextLeft, GSMin(SpaceAvailable, TextLeft.Length), &StringBuilder->Head->String);
        TextLeft.Memory += SpaceAvailable;
        TextLeft.Length -= SpaceAvailable;
        
        if (TextLeft.Length > 0)
        {
            GrowStringBuilder(StringBuilder);
        }
    }
}

internal void
WriteF(string_builder* StringBuilder, char* Format, ...)
{
    MakeStringBuffer(Buffer, 256);
    
    va_list Args;
    va_start(Args, Format);
    Buffer.Length = PrintFArgsList(Buffer.Memory, Buffer.Max, Format, Args);
    va_end(Args);
    
    Write(Buffer, StringBuilder);
}

#ifndef GS_STRING_BUILDER_NO_STDIO

internal void
WriteStringBuilderToFile(string_builder StringBuilder, FILE* WriteFile)
{
    string_builder_buffer* BufferAt = StringBuilder.Buffers;
    while (BufferAt)
    {
        string String = BufferAt->String;
        fwrite(String.Memory, 1, String.Length, WriteFile);
        BufferAt = BufferAt->Next;
    }
}

internal void
StdPrintStringBuilder(string_builder StringBuilder)
{
    string_builder_buffer* BufferAt = StringBuilder.Buffers;
    while (BufferAt)
    {
        string String = BufferAt->String;
        printf("%.*s", StringExpand(String));
        BufferAt = BufferAt->Next;
    }
}

#endif // GS_STRING_BUILDER_NO_STDIO

#define GS_STRING_BUILDER_H
#endif // GS_STRING_BUILDER_H