//
// gs_list.h - cpp template implementation of a growable, freeable list
// Author: Peter Slattery
// Date: December 30, 2019
//
// The key difference between gs_list.h and gs_bucket.h is that gs_bucket.h keeps everything
// sequential (new elements are appended to the end, free elements are filled in from the end), 
// whereas gs_list.h maintiains a free list and deals with holes in the list

struct gs_list_handle
{
    // NOTE(Peter): A generation of 0 in a handle denotes an invalid handle
    u32 Generation;
    u32 Index;
};

#define ListHandleIsInvalid(handle) ((handle).Generation == 0)
#define ListHandleIsValid(handle) ((handle).Generation != 0)

internal b32 
GSListHandlesAreEqual(gs_list_handle A, gs_list_handle B)
{
    b32 Result = (A.Index == B.Index) && (A.Generation == B.Generation);
    return Result;
}

struct gs_free_list
{
    u8* NextFreeEntry;
};

template <typename T>
struct gs_list_entry
{
    gs_list_handle Handle;
    gs_free_list Free;
    T Value;
};

#define EntryIsFree(entry) ((entry)->Free.NextFreeEntry != 0)

template <typename T>
struct gs_list_bucket
{
    gs_list_entry<T>* Contents;
};

#define GS_LIST_DEFAULT_BUCKET_SIZE 256

template <typename T>
class gs_list {
    public:
    u32 BucketCapacity;
    u32 BucketCount;
    gs_list_bucket<T>* Buckets;
    gs_list_entry<T> FreeList;
    
    u32 Used;
    
    // NOTE(Peter): this is just here so we can get the next entry which is
    // not a part of the free list but is still unused.
    // It shouldn't be used outside of the actual list implementation.
    u32 OnePastLastUsed;
    
    gs_list();
    gs_list(u32 BucketSize);
    
    void GrowList();
    
    gs_list_entry<T>* GetEntryAtIndex(u32 Index);
    
    T* GetElementAtIndex(u32 Index);
    T* GetElementWithHandle(gs_list_handle Handle);
    
    gs_list_entry<T>* TakeFreeEntry();
    T* TakeElement();
    gs_list_handle PushElementOnList(T Ele);
    
    void FreeElementAtIndex(u32 Index);
    void FreeElementWithHandle(gs_list_handle Handle);
};

template <typename T>
gs_list<T>::gs_list()
{
    this->BucketCapacity = GS_LIST_DEFAULT_BUCKET_SIZE;
    this->BucketCount = 0;
    this->Buckets = 0;
    this->FreeList.Free.NextFreeEntry = (u8*)&this->FreeList;
    this->Used = 0;
    this->OnePastLastUsed = 0;
}

template <typename T>
gs_list<T>::gs_list(u32 BucketCapacity)
{
    this->BucketCapacity = BucketCapacity;
    this->BucketCount = 0;
    this->Buckets = 0;
    this->FreeList.Free.NextFreeEntry = (u8*)&this->FreeList;
    this->Used = 0;
    this->OnePastLastUsed = 0;
}

template <typename T>
void gs_list<T>::GrowList()
{
    if (this->BucketCapacity == 0)
    {
        this->BucketCapacity = GS_LIST_DEFAULT_BUCKET_SIZE;
    }
    
    this->BucketCount += 1;
    this->Buckets = (gs_list_bucket<T>*)realloc(this->Buckets, sizeof(gs_list_bucket<T>) * this->BucketCount);
    gs_list_bucket<T>* NewBucket = this->Buckets + (this->BucketCount - 1);
    NewBucket->Contents = (gs_list_entry<T>*)malloc(sizeof(gs_list_entry<T>) * this->BucketCapacity);
    
    u32 IndexOffset = (this->BucketCount - 1) * this->BucketCapacity;
    for (u32 i = 0; i < this->BucketCapacity; i++)
    {
        NewBucket->Contents[i].Handle.Index = IndexOffset + i;
        NewBucket->Contents[i].Handle.Generation = 0;
    }
}

template <typename T>
gs_list_entry<T>* gs_list<T>::GetEntryAtIndex(u32 Index)
{
    gs_list_entry<T>* Result = 0;
    
    u32 BucketIndex = Index / this->BucketCapacity;
    u32 IndexInBucket = Index % this->BucketCapacity;
    
    Assert(BucketIndex < this->BucketCount);
    Assert(IndexInBucket < this->BucketCapacity); // this should always be true no matter what
    
    Result = this->Buckets[BucketIndex].Contents + IndexInBucket;
    return Result;
    
}

template <typename T>
T* gs_list<T>::GetElementAtIndex(u32 Index)
{
    T* Result = 0;
    gs_list_entry<T>* Entry = this->GetEntryAtIndex(Index);
    Result = &Entry->Value;
    return Result;
}

template <typename T>
T* gs_list<T>::GetElementWithHandle(gs_list_handle Handle)
{
    T* Result = 0;
    gs_list_entry<T>* Entry = this->GetEntryAtIndex(Handle.Index);
    if (Entry->Handle.Generation == Handle.Generation)
    {
        Result = &Entry->Value;
    }
    return Result;
}

template <typename T>
gs_list_entry<T>* gs_list<T>::TakeFreeEntry()
{
    gs_list_entry<T>* FreeEntry = (gs_list_entry<T>*)this->FreeList.Free.NextFreeEntry;
    if (FreeEntry == 0 || this->BucketCapacity == 0 || this->BucketCount == 0)
    {
        this->FreeList.Free.NextFreeEntry = (u8*)&this->FreeList;
        FreeEntry = (gs_list_entry<T>*)this->FreeList.Free.NextFreeEntry;
    }
    
    this->FreeList.Free.NextFreeEntry = FreeEntry->Free.NextFreeEntry;
    if (FreeEntry == &this->FreeList)
    {
        if (this->Used >= this->BucketCapacity * this->BucketCount)
        {
            this->GrowList();
        }
        FreeEntry = this->GetEntryAtIndex(this->OnePastLastUsed++);
    }
    Assert(FreeEntry != 0);
    
    FreeEntry->Handle.Generation++;
    FreeEntry->Free.NextFreeEntry = 0;
    
    this->Used++;
    
    return FreeEntry;
}

template <typename T>
T* gs_list<T>::TakeElement()
{
    T* Result = 0;
    
    gs_list_entry<T>* FreeEntry = this->TakeFreeEntry();
    Result = &FreeEntry->Value;
    
    return Result;
}

template <typename T>
gs_list_handle gs_list<T>::PushElementOnList(T Ele)
{
    gs_list_handle Result = {0};
    
    gs_list_entry<T>* FreeEntry = this->TakeFreeEntry();
    FreeEntry->Value = Ele;
    Result = FreeEntry->Handle;
    
    return Result;
}

template <typename T>
void gs_list<T>::FreeElementAtIndex(u32 Index)
{
    Assert(Index < this->BucketCapacity * this->BucketCount);
    
    gs_list_entry<T>* EntryToFree = this->GetEntryAtIndex(Index);
    
    // If the entry has a value in NextFreeEntry, then it is already free
    Assert(EntryToFree->Free.NextFreeEntry == 0);
    
    EntryToFree->Free.NextFreeEntry = this->FreeList.Free.NextFreeEntry;
    this->FreeList.Free.NextFreeEntry = (u8*)EntryToFree;
    
    this->Used--;
}

template <typename T>
void gs_list<T>::FreeElementWithHandle(gs_list_handle Handle)
{
    Assert(Handle.Index < this->BucketCapacity * this->BucketCount);
    
    gs_list_entry<T>* EntryToFree = this->GetEntryAtIndex(Handle.Index);
    
    // If the entry has a value in NextFreeEntry, then it is already free
    Assert(EntryToFree->Free.NextFreeEntry == 0);
    
    if (EntryToFree->Handle.Generation == Handle.Generation)
    {
        EntryToFree->Free.NextFreeEntry = this->FreeList.Free.NextFreeEntry;
        this->FreeList.Free.NextFreeEntry = (u8*)EntryToFree;
        this->Used--;
    }
}
