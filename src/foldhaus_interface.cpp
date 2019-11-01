FOLDHAUS_INPUT_COMMAND_PROC(CameraMouseControl)
{
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
    universe_view_operation_state* OpState = (universe_view_operation_state*)Operation.OpStateMemory;
    
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
        RegisterKeyPressCommand(&UniverseViewMode->Commands, KeyCode_MouseLeftButton, Command_Began | Command_Ended, KeyCode_Invalid, UniverseViewPan);
        RegisterKeyPressCommand(&UniverseViewMode->Commands, KeyCode_U, Command_Began, KeyCode_Invalid, CloseUniverseView);
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
                                                                  State->Font, State->Interface, Mouse);
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
                                    Mouse.Pos, State->Permanent);
    CloseNodeLister(State, Event, Mouse);
}

FOLDHAUS_INPUT_COMMAND_PROC(OpenNodeLister)
{
    // TODO(Peter): This won't work with hot code reloading
    operation_mode* AddNodeOperation = ActivateOperationMode(&State->Modes);
    
    { // Mode Commands
        InitializeInputCommandRegistry(&AddNodeOperation->Commands, 128, &State->Modes.Arena);
        RegisterKeyPressCommand(&AddNodeOperation->Commands, KeyCode_DownArrow, Command_Began, KeyCode_Invalid, NodeListerNextItem);
        RegisterKeyPressCommand(&AddNodeOperation->Commands, KeyCode_UpArrow, Command_Began, KeyCode_Invalid, NodeListerPrevItem);
        RegisterKeyPressCommand(&AddNodeOperation->Commands, KeyCode_Enter, Command_Began, KeyCode_Invalid, SelectAndCloseNodeLister);
        RegisterKeyPressCommand(&AddNodeOperation->Commands, KeyCode_MouseLeftButton, Command_Began, KeyCode_Invalid, CloseNodeLister);
        RegisterKeyPressCommand(&AddNodeOperation->Commands, KeyCode_Esc, Command_Began, KeyCode_Invalid, CloseNodeLister);
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
    color_picker_operation_state* OpState = (color_picker_operation_state*)Operation.OpStateMemory;
    
    
    b32 ShouldClose = EvaluateColorPicker(RenderBuffer, OpState->ValueAddr, 
                                          v2{200, 200}, State->Interface, Mouse);
    
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


////////////////////////////////////////
//
//    Node View
//
///////////////////////////////////////

struct node_view_operation_state
{
    node_interaction Interaction;
    node_render_settings RenderSettings;
};

FOLDHAUS_INPUT_COMMAND_PROC(NodeViewMousePickNode)
{
    // TODO(Peter): Pass this in as a parameter
    operation_mode Mode = State->Modes.ActiveModes[State->Modes.ActiveModesCount - 1];
    node_view_operation_state* OpState = (node_view_operation_state*)Mode.OpStateMemory;
    
    if (Mouse.LeftButtonTransitionedDown)
    {
        node_offset Node = GetNodeUnderPoint(State->NodeList, Mouse.Pos, OpState->RenderSettings);
        if (Node.Node)
        {
            OpState->Interaction = GetNodeInteractionType(Node.Node, Node.Offset, Mouse.Pos, OpState->RenderSettings);
        }
    }
    else if (Mouse.LeftButtonTransitionedUp)
    {
        if (IsDraggingNodePort(OpState->Interaction))
        {
            TryConnectNodes(OpState->Interaction, Mouse.Pos, State->NodeList, OpState->RenderSettings);
            OpState->Interaction = NewEmptyNodeInteraction();
        }
        else if(IsDraggingNodeValue(OpState->Interaction))
        {
            // This is just a click
            if (Mag(Mouse.DeltaPos) < 10)
            {
                node_interaction Interaction = OpState->Interaction;
                interface_node* Node = GetNodeAtOffset(State->NodeList, Interaction.NodeOffset);
                node_connection* Connection = Node->Connections + Interaction.InputValue;
                struct_member_type InputType = Connection->Type;
                if (InputType == MemberType_r32)
                {
                    SetTextInputDestinationToFloat(&State->ActiveTextEntry, &Connection->R32Value);
                    // TODO(Peter): This is wrong, should be something to do with capturing text input
                    State->ActiveCommands = &State->NodeListerCommandRegistry;
                }
                OpState->Interaction = NewEmptyNodeInteraction();
            }
            else // This is the case where you dragged the value
            {
                OpState->Interaction = NewEmptyNodeInteraction();
            }
        }
        else
        {
            OpState->Interaction = NewEmptyNodeInteraction();
        }
        
    }
}

OPERATION_RENDER_PROC(RenderNodeView)
{
    // TODO(Peter): Pass this in as a parameter
    node_view_operation_state* OpState = (node_view_operation_state*)Operation.OpStateMemory;
    
    UpdateDraggingNode(Mouse.Pos, OpState->Interaction, State->NodeList, 
                       OpState->RenderSettings);
    UpdateDraggingNodePort(Mouse.Pos, OpState->Interaction, State->NodeList, 
                           OpState->RenderSettings, RenderBuffer);
    UpdateDraggingNodeValue(Mouse.Pos, Mouse.OldPos, OpState->Interaction, State->NodeList, OpState->RenderSettings, State);
    
    ResetNodesUpdateState(State->NodeList);
    
    RenderNodeList(State->NodeList, OpState->RenderSettings, RenderBuffer);
}

FOLDHAUS_INPUT_COMMAND_PROC(CloseNodeView)
{
    DeactivateCurrentOperationMode(&State->Modes);
}

FOLDHAUS_INPUT_COMMAND_PROC(OpenNodeView)
{
    // TODO(Peter): This won't work with hot code reloading
    operation_mode* NodeViewMode = ActivateOperationMode(&State->Modes);
    NodeViewMode->Render = RenderNodeView;
    
    { // Mode Commands
        InitializeInputCommandRegistry(&NodeViewMode->Commands, 3, &State->Modes.Arena);
        
        RegisterKeyPressCommand(&NodeViewMode->Commands, KeyCode_Tab, Command_Began, KeyCode_Invalid, CloseNodeView);
        RegisterKeyPressCommand(&NodeViewMode->Commands, KeyCode_A, Command_Began, KeyCode_Invalid, OpenNodeLister);
        RegisterKeyPressCommand(&NodeViewMode->Commands, KeyCode_MouseLeftButton, Command_Began | Command_Ended, KeyCode_Invalid,
                                NodeViewMousePickNode);
    }
    
    node_view_operation_state* OpState = CreateOperationState(NodeViewMode, 
                                                              &State->Modes, 
                                                              node_view_operation_state);
    OpState->Interaction = NewEmptyNodeInteraction();
    OpState->RenderSettings.PortColors[MemberType_r32] = RedV4;
    OpState->RenderSettings.PortColors[MemberType_s32] = GreenV4;
    OpState->RenderSettings.PortColors[MemberType_v4] = BlueV4;
    OpState->RenderSettings.Font = State->Font;
}
