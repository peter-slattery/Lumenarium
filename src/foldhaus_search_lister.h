struct search_lister
{
    // TODO(Peter): Giving up trying to just use the source list for now. At the moment
    // we are copying the strings you want to filter from and storing them here. Come back
    // once its working and make the memory efficient version (ie store the existing memory
    // location, the element stride and the offset to the char*)
    s32 SourceListCount;
    string* SourceList;
    
    // NOTE(Peter): stores the source indecies of each filtered item
    // For example:
    //   Index 0 in this array contains 3. This means the first item that passes the filter
    //   is index 3 in ListMemory
    s32 FilteredListMax;
    s32 FilteredListCount;
    s32* FilteredIndexLUT;
    s32 HotItem;
    
    string Filter;
};
