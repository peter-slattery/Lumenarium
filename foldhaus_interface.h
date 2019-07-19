enum interface_selection_type
{
    InterfaceSelection_None,
    InterfaceSelection_Channel,
    InterfaceSelection_Pattern,
};

struct interface_state
{
    b32 AddingPattern;
    
    s32 ChannelSelectorStart;
    s32 ChannelSelected;
    
    s32 ActiveChannelPatternSelectorStart;
    s32 ActiveChannelPatternSelected;
    
    s32 PatternSelectorStart;
    
    b32 ChooseOperationPanelOpen;
    
    interface_selection_type SelectionType;
};
