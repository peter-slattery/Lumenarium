FOLDHAUS_INPUT_COMMAND_PROC(CameraMouseControl)
{
    if (State->NodeInteraction.NodeOffset >= 0) { return; }
    
    if (KeyTransitionedDown(Event))
    {
        State->Camera_StartDragPos = V4(State->Camera.Position, 1);
    }
    
    if (!State->DrawUniverseOutputDisplay)
    {
        v2 TotalDeltaPos = Mouse.Pos - Mouse.DownPos;
        
        m44 XRotation = GetXRotation(-TotalDeltaPos.y * State->PixelsToWorldScale);
        m44 YRotation = GetYRotation(TotalDeltaPos.x * State->PixelsToWorldScale);
        m44 Combined = XRotation * YRotation;
        
        State->Camera.Position = V3(Combined * State->Camera_StartDragPos);
    }
    else
    {
        State->UniverseOutputDisplayOffset += Mouse.DeltaPos;
    }
}

FOLDHAUS_INPUT_COMMAND_PROC(CameraMouseZoom)
{
    if (State->DrawUniverseOutputDisplay)
    {
        r32 DeltaZoom = (r32)(Mouse.Scroll) / 120;
        State->UniverseOutputDisplayZoom = GSClamp(0.1f, State->UniverseOutputDisplayZoom + DeltaZoom, 4.f);
    }
    
}

FOLDHAUS_INPUT_COMMAND_PROC(ToggleUniverseDebugView)
{
    State->DrawUniverseOutputDisplay = !State->DrawUniverseOutputDisplay;
}

FOLDHAUS_INPUT_COMMAND_PROC(ToggleNodeDisplay)
{
    State->NodeRenderSettings.Display = !State->NodeRenderSettings.Display;
}

////////////////////////////////////////
//
//     Universe View
//
///////////////////////////////////////

OPERATION_RENDER_PROC(RenderUniverseView)
{
    
}

////////////////////////////////////////
//
//     Node Lister
//
///////////////////////////////////////

struct node_lister_operation_state
{
    search_lister SearchLister;
};

OPERATION_RENDER_PROC(RenderNodeLister)
{
    node_lister_operation_state* OpState = (node_lister_operation_state*)Operation.OpStateMemory;
    
    v2 TopLeft = State->NodeListMenuPosition;
    v2 Dimension = v2{300, 30};
    
    // Filter the lister
    OpState->SearchLister.Filter = State->ActiveTextEntry.Buffer;
    FilterSearchLister(&OpState->SearchLister);
    
    // Display Search Lister
    search_lister_result NodeListerResult = EvaluateSearchLister (RenderBuffer, TopLeft, Dimension, 
                                                                  MakeStringLiteral("Nodes List"),
                                                                  OpState->SearchLister.SourceList,
                                                                  OpState->SearchLister.FilteredIndexLUT,
                                                                  OpState->SearchLister.FilteredListCount,
                                                                  OpState->SearchLister.HotItem,
                                                                  &State->ActiveTextEntry.Buffer,
                                                                  State->ActiveTextEntry.CursorPosition,
                                                                  State->Font, State->Interface, GuiMouse);
}

FOLDHAUS_INPUT_COMMAND_PROC(NodeListerNextItem)
{
    // TODO(Peter): Pass this in as a parameter
    operation_mode Mode = State->Modes.ActiveModes[State->Modes.ActiveModesCount - 1];
    node_lister_operation_state* OpState = (node_lister_operation_state*)Mode.OpStateMemory;
    OpState->SearchLister.HotItem = GetNextFilteredItem(OpState->SearchLister);
}

FOLDHAUS_INPUT_COMMAND_PROC(NodeListerPrevItem)
{
    // TODO(Peter): Pass this in as a parameter
    operation_mode Mode = State->Modes.ActiveModes[State->Modes.ActiveModesCount - 1];
    node_lister_operation_state* OpState = (node_lister_operation_state*)Mode.OpStateMemory;
    OpState->SearchLister.HotItem = GetPrevFilteredItem(OpState->SearchLister);
}

FOLDHAUS_INPUT_COMMAND_PROC(CloseNodeLister)
{
    DeactivateCurrentOperationMode(&State->Modes);
}

FOLDHAUS_INPUT_COMMAND_PROC(SelectAndCloseNodeLister)
{
    // TODO(Peter): Pass this in as a parameter
    operation_mode Mode = State->Modes.ActiveModes[State->Modes.ActiveModesCount - 1];
    node_lister_operation_state* OpState = (node_lister_operation_state*)Mode.OpStateMemory;
    
    s32 FilteredNodeIndex = OpState->SearchLister.HotItem;
    s32 NodeIndex = OpState->SearchLister.FilteredIndexLUT[FilteredNodeIndex];
    PushNodeOnListFromSpecification(State->NodeList, NodeSpecifications[NodeIndex],
                                    Mouse.Pos, State->NodeRenderSettings, State->Permanent);
    CloseNodeLister(State, Event, Mouse);
}

FOLDHAUS_INPUT_COMMAND_PROC(OpenNodeLister)
{
    // TODO(Peter): This won't work with hot code reloading
    operation_mode* AddNodeOperation = ActivateOperationMode(&State->Modes);
    
    { // Mode Commands
        InitializeInputCommandRegistry(&AddNodeOperation->Commands, 128, &State->Modes.Arena);
        RegisterKeyPressCommand(&AddNodeOperation->Commands, KeyCode_DownArrow, false, KeyCode_Invalid, NodeListerNextItem);
        RegisterKeyPressCommand(&AddNodeOperation->Commands, KeyCode_UpArrow, false, KeyCode_Invalid, NodeListerPrevItem);
        RegisterKeyPressCommand(&AddNodeOperation->Commands, KeyCode_Enter, false, KeyCode_Invalid, SelectAndCloseNodeLister);
        RegisterKeyPressCommand(&AddNodeOperation->Commands, KeyCode_MouseLeftButton, false, KeyCode_Invalid, CloseNodeLister);
        RegisterKeyPressCommand(&AddNodeOperation->Commands, KeyCode_Esc, false, KeyCode_Invalid, CloseNodeLister);
        InitializeTextInputCommands(&AddNodeOperation->Commands, &State->Modes.Arena);
    }
    
    AddNodeOperation->Render = RenderNodeLister;
    
    node_lister_operation_state* OpState = PushStruct(&State->Modes.Arena, node_lister_operation_state);
    AddNodeOperation->OpStateMemory = (u8*)OpState;
    {
        OpState->SearchLister.SourceListCount = NodeSpecificationsCount;
        OpState->SearchLister.SourceList = PushArray(&State->Modes.Arena, string, OpState->SearchLister.SourceListCount);
        {
            for (s32 i = 0; i < OpState->SearchLister.SourceListCount; i++)
            {
                OpState->SearchLister.SourceList[i] = MakeString(
                    NodeSpecifications[i].Name,
                    NodeSpecifications[i].NameLength);
            }
        }
        OpState->SearchLister.Filter = State->ActiveTextEntry.Buffer;
        
        
        OpState->SearchLister.FilteredListMax = OpState->SearchLister.SourceListCount;
        OpState->SearchLister.FilteredListCount = 0;
        OpState->SearchLister.FilteredIndexLUT = PushArray(&State->Modes.Arena, s32, OpState->SearchLister.SourceListCount);
    }
    
    State->NodeListMenuPosition = Mouse.Pos;
    SetTextInputDestinationToString(&State->ActiveTextEntry, &State->GeneralPurposeSearchString);
}
