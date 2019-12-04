#define STRING_BUILDER_ARRAY_BUFFER_SIZE 32

struct string_array
{
    string** Buckets;
    s32 BucketSize;
    s32 BucketCount;
    s32 Used;
    free_list FreeList;
};

internal string*
GetEntryAtIndex (s32 Index, string_array Buffer)
{
    string* Result = 0;
    if (Buffer.Buckets)
    {
        bucket_index BucketIndex = GetBucketIndexForIndex(Index, Buffer.BucketSize);
        Result = Buffer.Buckets[BucketIndex.Bucket] + BucketIndex.IndexInBucket;
    }
    return Result;
}

internal s32
PushElement (string Data, string_array* Buffer)
{
    s32 Result = -1;
    
    if (Buffer->Used >= Buffer->BucketSize * Buffer->BucketCount)
    {
        Buffer->Buckets = (string**)GrowBuffer(Buffer->BucketSize, sizeof(string), &Buffer->BucketCount, (void**)Buffer->Buckets);
    }
    
    s32 Index = Buffer->Used++;
    s32 BucketIndex = Index / Buffer->BucketSize;
    s32 IndexInBucket = Index % Buffer->BucketSize;
    
    Buffer->Buckets[BucketIndex][IndexInBucket] = Data;
    Result = Index;
    return Result;
}

struct string_builder
{
    string_array Buffer;
    s32 BufferElementSize;
};

internal string_builder
InitStringBuilder(s32 BufferSize)
{
    string_builder Result = {};
    Result.BufferElementSize = BufferSize;
    Result.Buffer.BucketSize = STRING_BUILDER_ARRAY_BUFFER_SIZE;
    Result.Buffer.FreeList.Next = &Result.Buffer.FreeList;
    return Result;
}

internal void
GrowStringBuilder (string_builder* StringBuilder)
{
    string NewSegment = {};
    NewSegment.Memory = (char*)malloc(StringBuilder->BufferElementSize * sizeof(char));
    NewSegment.Max = StringBuilder->BufferElementSize;
    
    PushElement(NewSegment, &StringBuilder->Buffer);
}

internal void
StringBuilderPrintF (string_builder* StringBuilder, char* Format, ...)
{
    string Addition = {};
    Addition.Max = 2048;
    Addition.Memory = (char*)malloc(Addition.Max * sizeof(char));
    
    va_list Args;
    va_start(Args, Format);
    Addition.Length = PrintFArgsList(Addition.Memory, Addition.Max, Format, Args);
    
    s32 CharsCopied = 0;
    while (CharsCopied < Addition.Length)
    {
        s32 StringBuilderTailIndex = StringBuilder->Buffer.Used - 1;
        string* LastString = GetEntryAtIndex(StringBuilderTailIndex, StringBuilder->Buffer);
        if (!LastString || LastString->Length >= LastString->Max)
        {
            GrowStringBuilder(StringBuilder);
            StringBuilderTailIndex = StringBuilder->Buffer.Used - 1;
            LastString = GetEntryAtIndex(StringBuilderTailIndex, StringBuilder->Buffer);
        }
        
        while (CharsCopied < Addition.Length && LastString->Length < LastString->Max)
        {
            LastString->Memory[LastString->Length++] = Addition.Memory[CharsCopied++];
        }
    }
    
    free(Addition.Memory);
}