FOLDHAUS_INPUT_COMMAND_PROC(CameraMouseControl)
{
    if (State->NodeInteraction.NodeOffset >= 0) { return; }
    
    if (KeyTransitionedDown(Event))
    {
        State->Camera_StartDragPos = V4(State->Camera.Position, 1);
    }
    
    v2 TotalDeltaPos = Mouse.Pos - Mouse.DownPos;
    
    m44 XRotation = GetXRotation(-TotalDeltaPos.y * State->PixelsToWorldScale);
    m44 YRotation = GetYRotation(TotalDeltaPos.x * State->PixelsToWorldScale);
    m44 Combined = XRotation * YRotation;
    
    State->Camera.Position = V3(Combined * State->Camera_StartDragPos);
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

struct universe_view_operation_state
{
    v2 DisplayOffset;
    r32 Zoom;
};

OPERATION_RENDER_PROC(RenderUniverseView)
{
    DEBUG_TRACK_SCOPE(DrawUniverseOutputDisplay);
    
    // TODO(Peter): Pass this in as a parameter
    operation_mode Mode = State->Modes.ActiveModes[State->Modes.ActiveModesCount - 1];
    universe_view_operation_state* OpState = (universe_view_operation_state*)Mode.OpStateMemory;
    
    string TitleBarString = InitializeEmptyString(PushArray(State->Transient, char, 64), 64);
    
    v2 DisplayArea_Dimension = v2{600, 600};
    v2 DisplayContents_Offset = OpState->DisplayOffset;
    v2 DisplayArea_TopLeft = v2{300, (r32)RenderBuffer->ViewHeight - 50} + DisplayContents_Offset;
    v2 UniverseDisplayDimension = v2{100, 100} * OpState->Zoom;
    v2 Padding = v2{25, 50} * OpState->Zoom;
    
    v2 UniverseDisplayTopLeft = DisplayArea_TopLeft;
    
    sacn_universe_buffer* UniverseList = State->SACN.UniverseBuffer;
    while(UniverseList)
    {
        for (s32 UniverseIdx = 0;
             UniverseIdx < UniverseList->Used;
             UniverseIdx++)
        {
            sacn_universe* Universe = UniverseList->Universes + UniverseIdx;
            
            DrawSACNUniversePixels(RenderBuffer, Universe, 
                                   UniverseDisplayTopLeft, UniverseDisplayDimension);
            
            if (OpState->Zoom > .5f)
            {
                v2 TitleDisplayStart = UniverseDisplayTopLeft + v2{0, 12};
                PrintF(&TitleBarString, "Universe %d", Universe->Universe);
                DrawString(RenderBuffer, TitleBarString, State->Interface.Font, 12, 
                           TitleDisplayStart, WhiteV4);
            }
            
            UniverseDisplayTopLeft.x += UniverseDisplayDimension.x + Padding.x;
            if (UniverseDisplayTopLeft.x > DisplayArea_TopLeft.x + DisplayArea_Dimension.x)
            {
                UniverseDisplayTopLeft.x = DisplayArea_TopLeft.x;
                UniverseDisplayTopLeft.y -= UniverseDisplayDimension.y + Padding.y;
            }
            
            if (UniverseDisplayTopLeft.y < DisplayArea_TopLeft.y - DisplayArea_Dimension.y)
            {
                break;
            }
        }
        UniverseList = UniverseList->Next;
    }
}

// TODO(Peter): Something isn't working with my laptop trackpad's zoom
FOLDHAUS_INPUT_COMMAND_PROC(UniverseZoom)
{
    // TODO(Peter): Pass this in as a parameter
    operation_mode Mode = State->Modes.ActiveModes[State->Modes.ActiveModesCount - 1];
    universe_view_operation_state* OpState = (universe_view_operation_state*)Mode.OpStateMemory;
    r32 DeltaZoom = (r32)(Mouse.Scroll) / 120;
    OpState->Zoom = GSClamp(0.1f, OpState->Zoom + DeltaZoom, 4.f);
}

FOLDHAUS_INPUT_COMMAND_PROC(UniverseViewPan)
{
    // TODO(Peter): Pass this in as a parameter
    operation_mode Mode = State->Modes.ActiveModes[State->Modes.ActiveModesCount - 1];
    universe_view_operation_state* OpState = (universe_view_operation_state*)Mode.OpStateMemory;
    OpState->DisplayOffset += Mouse.DeltaPos;
}

FOLDHAUS_INPUT_COMMAND_PROC(CloseUniverseView)
{
    DeactivateCurrentOperationMode(&State->Modes);
}

FOLDHAUS_INPUT_COMMAND_PROC(OpenUniverseView)
{
    operation_mode* UniverseViewMode = ActivateOperationMode(&State->Modes);
    UniverseViewMode->Render = RenderUniverseView;
    
    { // Mode Commands
        InitializeInputCommandRegistry(&UniverseViewMode->Commands, 3, &State->Modes.Arena);
        RegisterKeyPressCommand(&UniverseViewMode->Commands, KeyCode_MouseLeftButton, true, KeyCode_Invalid, UniverseViewPan);
        RegisterKeyPressCommand(&UniverseViewMode->Commands, KeyCode_U, false, KeyCode_Invalid, CloseUniverseView);
        RegisterMouseWheelCommand(&UniverseViewMode->Commands, UniverseZoom);
    }
    
    // State Setup
    universe_view_operation_state* OpState = CreateOperationState(UniverseViewMode, 
                                                                  &State->Modes, 
                                                                  universe_view_operation_state); 
    OpState->DisplayOffset = v2{0, 0};
    OpState->Zoom = 1.0f;
}

////////////////////////////////////////
//
//     Node Lister
//
///////////////////////////////////////

struct node_lister_operation_state
{
    search_lister SearchLister;
    v2 ListPosition;
};

OPERATION_RENDER_PROC(RenderNodeLister)
{
    node_lister_operation_state* OpState = (node_lister_operation_state*)Operation.OpStateMemory;
    
    v2 TopLeft = OpState->ListPosition;
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
    
    node_lister_operation_state* OpState = CreateOperationState(AddNodeOperation, 
                                                                &State->Modes, 
                                                                node_lister_operation_state);
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
    
    OpState->ListPosition = Mouse.Pos;
    SetTextInputDestinationToString(&State->ActiveTextEntry, &State->GeneralPurposeSearchString);
}

////////////////////////////////////////
//
//    Node Color Picker
//
///////////////////////////////////////

struct color_picker_operation_state
{
    v4* ValueAddr;
};

internal void
CloseColorPicker(app_state* State)
{
    DeactivateCurrentOperationMode(&State->Modes);
}

OPERATION_RENDER_PROC(RenderColorPicker)
{
    // TODO(Peter): Pass this in as a parameter
    operation_mode Mode = State->Modes.ActiveModes[State->Modes.ActiveModesCount - 1];
    color_picker_operation_state* OpState = (color_picker_operation_state*)Mode.OpStateMemory;
    
    
    b32 ShouldClose = EvaluateColorPicker(RenderBuffer, OpState->ValueAddr, 
                                          v2{200, 200}, State->Interface, GuiMouse);
    
    if (ShouldClose)
    {
        CloseColorPicker(State);
    }
}

internal void
OpenColorPicker(app_state* State, v4* ValueAddr)
{
    // TODO(Peter): This won't work with hot code reloading
    operation_mode* ColorPickerMode = ActivateOperationMode(&State->Modes);
    ColorPickerMode->Render = RenderColorPicker;
    
    color_picker_operation_state* OpState = CreateOperationState(ColorPickerMode, 
                                                                 &State->Modes, 
                                                                 color_picker_operation_state);
    OpState->ValueAddr = ValueAddr;
}