//
// File: foldhaus_search_lister.cpp
// Author: Peter Slattery
// Creation Date: 2020-01-01
//
#ifndef FOLDHAUS_SEARCH_LISTER_CPP

internal b32
NamePassesFilter (gs_string Target, gs_string Filter)
{
    return (Filter.Length == 0 || StringContainsStringCaseInsensitive(Target, Filter));
}

internal void
FilterSearchLister (search_lister* SearchLister)
{
    Assert(SearchLister->FilteredIndexLUT != 0);
    Assert(SearchLister->FilteredListMax == SearchLister->SourceListCount);
    
    SearchLister->FilteredListCount = 0;
    
    for (s32 i = 0; i < SearchLister->SourceListCount; i++)
    {
        gs_string* Namegs_string = SearchLister->SourceList + i;
        if (NamePassesFilter(*Namegs_string, SearchLister->Filter))
        {
            SearchLister->FilteredIndexLUT[SearchLister->FilteredListCount++] = i;
        }
    }
    
    SearchLister->HotItem = Clamp(0, SearchLister->HotItem, SearchLister->FilteredListCount - 1);
    
    if (SearchLister->FilteredListCount == 0)
    {
        SearchLister->HotItem = -1;
    }
}

internal s32
GetNextFilteredItem (search_lister SearchLister)
{
    s32 Result = Min(SearchLister.HotItem + 1, SearchLister.FilteredListCount - 1);
    return Result;
}

internal s32
GetPrevFilteredItem (search_lister SearchLister)
{
    s32 Result = Max(SearchLister.HotItem - 1, 0);
    return Result;
}


#define FOLDHAUS_SEARCH_LISTER_CPP
#endif // FOLDHAUS_SEARCH_LISTER_CPP