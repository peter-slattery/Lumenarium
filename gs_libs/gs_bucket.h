//
// gs_bucket.h - cpp template implementation of a growable, freeable list
// Author: Peter Slattery
// Date: December 30, 2019
//
// The key difference between gs_list.h and gs_bucket.h is that gs_bucket.h keeps everything
// sequential (new elements are appended to the end, free elements are filled in from the end), 
// whereas gs_list.h maintiains a free list and deals with holes in the list

template <typename T>
struct gs_bucket_bucket
{
    T* Contents;
};

#define GS_LIST_DEFAULT_BUCKET_SIZE 256

template <typename T>
class gs_bucket {
    public:
    u32 BucketSize;
    u32 BucketCount;
    gs_bucket_bucket<T>* Buckets;
    
    u32 Used;
    
    void GrowBucket();
    
    T* GetElementAtIndex(u32 Index);
    T* TakeElement();
    u32 PushElementOnBucket(T Ele);
    
    void FreeElementAtIndex(u32 Index);
};

template <typename T>
void gs_bucket<T>::GrowBucket()
{
    if (this->BucketSize == 0)
    {
        this->BucketSize = GS_LIST_DEFAULT_BUCKET_SIZE;
    }
    
    this->BucketCount += 1;
    this->Buckets = (gs_bucket_bucket<T>*)realloc(this->Buckets, sizeof(gs_bucket_bucket<T>) * this->BucketCount);
    gs_bucket_bucket<T>* NewBucket = this->Buckets + (this->BucketCount - 1);
    NewBucket->Contents = (T*)malloc(sizeof(T) * this->BucketSize);
}

template <typename T>
T* gs_bucket<T>::GetElementAtIndex(u32 Index)
{
    Assert(Index < this->BucketSize * this->BucketCount);
    Assert(Index < this->Used);
    
    u32 BucketIndex = Index / this->BucketSize;
    u32 IndexInBucket = Index % this->BucketSize;
    
    T* Result = this->Buckets[BucketIndex].Contents + IndexInBucket;
    
    return Result;
}

template <typename T>
T* gs_bucket<T>::TakeElement()
{
    if (this->Used >= this->BucketSize * this->BucketCount)
    {
        this->GrowBucket();
    }
    
    u32 Index = this->Used++;
    T* Result = this->GetElementAtIndex(Index);
    
    return Result;
}

template <typename T>
u32 gs_bucket<T>::PushElementOnBucket(T Ele)
{
    u32 ResultIndex = 0;
    
    if (this->Used >= this->BucketSize * this->BucketCount)
    {
        this->GrowBucket();
    }
    
    ResultIndex = this->Used++;
    T* FreeElement = this->GetElementAtIndex(ResultIndex);
    *FreeElement = Ele;
    
    return ResultIndex;
}

template <typename T>
void gs_bucket<T>::FreeElementAtIndex(u32 Index)
{
    Assert(Index < this->BucketSize * this->BucketCount);
    
    T* ToFillIn = this->GetElementAtIndex(Index);
    T* ToFree = this->GetElementAtIndex(--this->Used);
    *ToFillIn = *ToFree;
}
