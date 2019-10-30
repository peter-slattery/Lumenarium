internal string*
GetStringAtIndex (search_lister Lister, s32 Index)
{
    string* Result = 0;
    Assert(Index >= 0 && Index < Lister.ListLength);
    
    u8* Element = Lister.ListMemory + (Lister.ElementStride * Index);
    Result = (string*)(Element + Lister.OffsetToStringInElement);
    
    return Result;
}

#define InitializeSearchLister(SearchLister, List, ListLength, StringFieldName) \
s32 ElementStride = (u8*)(List + 1) - (u8*)(List); \
s32 OffsetToString = (u8*)(List->#StringFieldName) - (u8*)(List); \
InitializeSearchLister_((u8*)List, ListLength, ElementStride, OffsetToString); \

internal void
InitializeSearchLister_(search_lister* Lister, 
                        u8* ListMemory, 
                        s32 ListLength, 
                        s32 ElementStride, 
                        s32 OffsetToString)
{
    Lister->ListLength = ListLength;
    Lister->ListMemory = ListMemory;
    Lister->OffsetToStringInElement = OffsetToString;
    Lister->ElementStride = ElementStride;
    Lister->Filter.Length = 0;
    
}

internal s32
GetNextHotItemIndex (s32 CurrentHotItem, s32 ListLength)
{
    s32 Result = GSMin(CurrentHotItem + 1, ListLength - 1);
    return Result;
}

internal s32
GetPrevHotItemIndex (s32 CurrentHotItem)
{
    s32 Result = GSMax(0, CurrentHotItem - 1);
    return Result;
}

FOLDHAUS_INPUT_COMMAND_PROC(SearchListerNextItem)
{
    State->GeneralPurposeSearchHotItem = GetNextHotItemIndex(State->GeneralPurposeSearchHotItem, NodeSpecificationsCount);
}

FOLDHAUS_INPUT_COMMAND_PROC(SearchListerPrevItem)
{
    State->GeneralPurposeSearchHotItem = GetPrevHotItemIndex(State->GeneralPurposeSearchHotItem);
}

FOLDHAUS_INPUT_COMMAND_PROC(CloseSearchLister)
{
    // TODO(Peter): This is to show the node list. Generalize to just a lister
    State->InterfaceShowNodeList = false;
    // TODO(Peter): This also assumes we know where we came from. Probably need to implement
    // push/pop functionality for the activecommands.
    QueueNextFrameCommandRegistry(&State->InputCommandRegistry, State);
    State->GeneralPurposeSearchHotItem = -1;
}

FOLDHAUS_INPUT_COMMAND_PROC(SelectAndCloseSearchLister)
{
    s32 HotItem = State->GeneralPurposeSearchHotItem;
    PushNodeOnListFromSpecification(State->NodeList, NodeSpecifications[HotItem],
                                    Mouse.Pos, State->NodeRenderSettings, State->Permanent);
    CloseSearchLister(State, Event, Mouse);
}
