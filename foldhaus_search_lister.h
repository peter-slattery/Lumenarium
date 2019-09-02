internal s32
GetNextHotItem (s32 CurrentHotItem, s32 ListLength)
{
    s32 Result = GSMin(CurrentHotItem + 1, ListLength - 1);
    return Result;
}

internal s32
GetPrevHotItem (s32 CurrentHotItem)
{
    s32 Result = GSMax(0, CurrentHotItem - 1);
    return Result;
}

FOLDHAUS_INPUT_COMMAND_PROC(SearchListerNextItem)
{
    State->GeneralPurposeSearchHotItem = GetNextHotItem(State->GeneralPurposeSearchHotItem, NodeSpecificationsCount);
}

FOLDHAUS_INPUT_COMMAND_PROC(SearchListerPrevItem)
{
    State->GeneralPurposeSearchHotItem = GetPrevHotItem(State->GeneralPurposeSearchHotItem);
}

FOLDHAUS_INPUT_COMMAND_PROC(CloseSearchLister)
{
    // TODO(Peter): This is to show the node list. Generalize to just a lister
    State->InterfaceShowNodeList = false;
    // TODO(Peter): This also assumes we know where we came from. Probably need to implement
    // push/pop functionality for the activecommands.
    State->ActiveCommands = &State->InputCommandRegistry;
    State->GeneralPurposeSearchHotItem = -1;
}
