struct free_list
{
    free_list* Next;
    s32 Index;
};

struct bucket_index
{
    s32 Bucket;
    s32 IndexInBucket;
};

inline bucket_index
GetBucketIndexForIndex (s32 Index, s32 BucketSize)
{
    bucket_index Result = {};
    Result.Bucket = Index / BucketSize;
    Result.IndexInBucket = Index % BucketSize;
    return Result;
};

struct array_entry_handle
{
    s32 Generation;
    s32 Index;
};

// NOTE(Peter): This is a total bastardization of the preprocessor but it works and is
// easier than writing a metaprogramming system at the moment. 
// TODO(Peter): Write a metaprogramming version of this that is actually easy to debug

#define TYPEDEF_ARRAY(element_type) \
struct element_type##_array_entry { \
    s32 Generation; \
    union \
    { \
        element_type Entry; \
        free_list Free; \
    }; \
}; \
\
struct element_type##_array \
{ \
    element_type##_array_entry** Buckets; \
    s32 BucketSize; \
    s32 BucketCount; \
    s32 Used; \
    free_list FreeList; \
}; \
\
internal void \
GrowBuffer(element_type##_array* Buffer) \
{ \
    s32 NewBucketSize = sizeof(element_type##_array_entry) * Buffer->BucketSize; \
    element_type##_array_entry* NewBucket = (element_type##_array_entry*)malloc(NewBucketSize); \
    GSZeroMemory((u8*)NewBucket, NewBucketSize); \
    \
    s32 NewBucketIndex = Buffer->BucketCount++; \
    if (!Buffer->Buckets) \
    { \
        Buffer->Buckets = (element_type##_array_entry**)malloc(sizeof(element_type##_array_entry*)); \
    } \
    else \
    { \
        Buffer->Buckets = (element_type##_array_entry**)realloc(Buffer->Buckets, sizeof(element_type##_array_entry*) * Buffer->BucketCount); \
    } \
    Buffer->Buckets[NewBucketIndex] = NewBucket; \
}  \
\
internal element_type##_array_entry* \
GetEntryAtIndex (s32 Index, element_type##_array Buffer) \
{ \
    bucket_index BucketIndex = GetBucketIndexForIndex(Index, Buffer.BucketSize); \
    element_type##_array_entry* Entry = Buffer.Buckets[BucketIndex.Bucket] + BucketIndex.IndexInBucket; \
    return Entry; \
} \
\
internal array_entry_handle \
PushElement (element_type Data, element_type##_array* Buffer) \
{ \
    array_entry_handle Result = {}; \
    \
    if (Buffer->FreeList.Next != &Buffer->FreeList) \
    { \
        free_list* FreeList = Buffer->FreeList.Next; \
        element_type##_array_entry* Entry = GetEntryAtIndex(FreeList->Index, *Buffer); \
        Buffer->FreeList.Next = Entry->Free.Next; \
        \
        Result.Index = Entry->Free.Index; \
        Result.Generation = Entry->Generation; \
        Entry->Entry = Data; \
        \
        ++Buffer->Used; \
    } \
    else \
    { \
        if (Buffer->Used >= Buffer->BucketSize * Buffer->BucketCount) \
        { \
            GrowBuffer(Buffer); \
        } \
        \
        s32 Index = Buffer->Used++; \
        s32 BucketIndex = Index / Buffer->BucketSize; \
        s32 IndexInBucket = Index % Buffer->BucketSize; \
        \
        Buffer->Buckets[BucketIndex][IndexInBucket].Entry = Data; \
        Result.Index = Index; \
        Result.Generation = Buffer->Buckets[BucketIndex][IndexInBucket].Generation; \
    } \
    \
    return Result; \
} \
\
internal element_type* \
GetElementAtIndex (s32 Index, element_type##_array Buffer) \
{ \
    Assert(Index < Buffer.Used); \
    element_type##_array_entry* Entry = GetEntryAtIndex(Index, Buffer); \
    element_type* Result = &Entry->Entry; \
    return Result; \
} \
\
internal element_type* \
GetElementWithHandle (array_entry_handle Handle, element_type##_array Buffer) \
{ \
    element_type* Result = 0; \
    \
    element_type##_array_entry* Entry = GetEntryAtIndex(Handle.Index, Buffer); \
    \
    if (Entry->Generation == Handle.Generation) \
    { \
        Result = &Entry->Entry; \
    } \
    \
    return Result; \
} \
\
internal void \
RemoveElementAtIndex (s32 Index, element_type##_array* Buffer) \
{ \
    Assert(Index < Buffer->Used); \
    \
    element_type##_array_entry* Entry = GetEntryAtIndex(Index, *Buffer); \
    ++Entry->Generation; \
    Entry->Free.Index = Index; \
    \
    Entry->Free.Next = Buffer->FreeList.Next; \
    Buffer->FreeList.Next = &Entry->Free; \
    \
} \

// END OF CRAZY MACRO


#define TYPEDEF_CONTIGUOUS_ARRAY(element_type) \
struct element_type##_contiguous_array \
{ \
    element_type** Buckets; \
    s32 BucketSize; \
    s32 BucketCount; \
    s32 Used; \
}; \
\
internal void \
GrowBuffer(element_type##_contiguous_array* Buffer) \
{ \
    s32 NewBucketSize = sizeof(element_type) * Buffer->BucketSize; \
    element_type* NewBucket = (element_type*)malloc(NewBucketSize); \
    GSZeroMemory((u8*)NewBucket, NewBucketSize); \
    \
    s32 NewBucketIndex = Buffer->BucketCount++; \
    if (!Buffer->Buckets) \
    { \
        Buffer->Buckets = (element_type**)malloc(sizeof(element_type*)); \
    } \
    else \
    { \
        Buffer->Buckets = (element_type**)realloc(Buffer->Buckets, sizeof(element_type*) * Buffer->BucketCount); \
    } \
    Buffer->Buckets[NewBucketIndex] = NewBucket; \
}  \
\
internal element_type* \
GetElementAtIndex (s32 Index, element_type##_contiguous_array Buffer) \
                   { \
                       element_type* Entry = 0; \
                       if (Index <= Buffer.Used) \
                       { \
bucket_index BucketIndex = GetBucketIndexForIndex(Index, Buffer.BucketSize); \
                           Entry = Buffer.Buckets[BucketIndex.Bucket] + BucketIndex.IndexInBucket; \
                       } \
    return Entry; \
} \
\
internal s32 \
PushElement (element_type Data, element_type##_contiguous_array* Buffer) \
{ \
    s32 Result = -1; \
    \
    if (Buffer->Used >= Buffer->BucketSize * Buffer->BucketCount) \
    { \
        GrowBuffer(Buffer); \
    } \
    \
    s32 Index = Buffer->Used++; \
    s32 BucketIndex = Index / Buffer->BucketSize; \
    s32 IndexInBucket = Index % Buffer->BucketSize; \
    \
    Buffer->Buckets[BucketIndex][IndexInBucket] = Data; \
    Result = Index; \
    return Result; \
} \
\
internal void \
RemoveElementAtIndex (s32 Index, element_type##_contiguous_array* Buffer) \
{ \
    Assert(Index < Buffer->Used); \
    \
    bucket_index IndexToRemove = GetBucketIndexForIndex(Index, Buffer->BucketSize); \
    bucket_index LastIndex = GetBucketIndexForIndex(Buffer->Used - 1, Buffer->BucketSize); \
    element_type ValueAtLastIndex = Buffer->Buckets[LastIndex.Bucket][LastIndex.IndexInBucket]; \
    Buffer->Buckets[IndexToRemove.Bucket][IndexToRemove.IndexInBucket] = ValueAtLastIndex; \
    --Buffer->Used; \
} \

// END OF CRAZY MACRO

TYPEDEF_ARRAY(array_entry_handle);
TYPEDEF_CONTIGUOUS_ARRAY(array_entry_handle);