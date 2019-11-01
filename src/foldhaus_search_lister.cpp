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

FOLDHAUS_INPUT_COMMAND_PROC(SearchListerNextItem)
{
    State->SearchLister.HotItem = GetNextFilteredItem(State->SearchLister);
}

FOLDHAUS_INPUT_COMMAND_PROC(SearchListerPrevItem)
{
    State->SearchLister.HotItem = GetPrevFilteredItem(State->SearchLister);
}

FOLDHAUS_INPUT_COMMAND_PROC(CloseSearchLister)
{
    DeactivateCurrentOperationMode(&State->Modes);
    // TODO(Peter): This is to show the node list. Generalize to just a lister
    State->InterfaceShowNodeLister = false;
    // TODO(Peter): This also assumes we know where we came from. Probably need to implement
    // push/pop functionality for the activecommands.
    QueueNextFrameCommandRegistry(&State->InputCommandRegistry, State);
}

FOLDHAUS_INPUT_COMMAND_PROC(SelectAndCloseSearchLister)
{
    s32 FilteredNodeIndex = State->SearchLister.HotItem;
    s32 NodeIndex = State->SearchLister.FilteredIndexLUT[FilteredNodeIndex];
    PushNodeOnListFromSpecification(State->NodeList, NodeSpecifications[NodeIndex],
                                    Mouse.Pos, State->NodeRenderSettings, State->Permanent);
    CloseSearchLister(State, Event, Mouse);
}
