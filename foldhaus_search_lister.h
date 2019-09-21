struct search_lister
{
    s32 ListLength;
    u8* ListMemory;
    s32 OffsetToStringInElement;
    s32 ElementStride;
    
    string Filter;
};