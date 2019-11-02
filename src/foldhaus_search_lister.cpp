internal void
FilterSearchLister (search_lister* SearchLister)
{
    Assert(SearchLister->FilteredIndexLUT != 0);
    Assert(SearchLister->FilteredListMax == SearchLister->SourceListCount);
    
    SearchLister->FilteredListCount = 0;
    
    for (s32 i = 0; i < SearchLister->SourceListCount; i++)
    {
        string* NameString = SearchLister->SourceList + i;
        if (SpecificationPassesFilter(*NameString, SearchLister->Filter))
        {
            SearchLister->FilteredIndexLUT[SearchLister->FilteredListCount++] = i;
        }
    }
    
    if (SearchLister->FilteredListCount == 0)
    {
        SearchLister->HotItem = -1;
    }
}

internal s32
GetNextFilteredItem (search_lister SearchLister)
{
    s32 Result = GSMin(SearchLister.HotItem + 1, SearchLister.FilteredListCount - 1);
    return Result;
}

internal s32
GetPrevFilteredItem (search_lister SearchLister)
{
    s32 Result = GSMax(SearchLister.HotItem - 1, 0);
    return Result;
}
