////////////////////////////////////////
//
//     Universe View
//
///////////////////////////////////////

struct universe_view_operation_state
{
    b32 MouseDown;
    v2 DisplayOffset;
    r32 Zoom;
};

OPERATION_RENDER_PROC(RenderUniverseView)
{
    InvalidCodePath;
    
#if 0
    DEBUG_TRACK_SCOPE(DrawUniverseOutputDisplay);
    
    universe_view_operation_state* OpState = (universe_view_operation_state*)Operation.OpStateMemory;
    
    string TitleBarString = InitializeEmptyString(PushArray(State->Transient, char, 64), 64);
    
    v2 DisplayArea_Dimension = v2{600, 600};
    
    v2 DisplayContents_Offset = OpState->DisplayOffset;
    
    //
    // TODO(Peter): I don't like this. Dragging the Universe view should be an operation mode, just
    // like rotating the 3D view, but modes don't have access to the state of modes above them in the stack
    // (and attempting to cast those states to the appropriate type seems risky)
    //
    // :NeedToPassStateDownModeChain
    //
    if (OpState->MouseDown)
    {
        DisplayContents_Offset += (Mouse.Pos - Mouse.DownPos);
    }
    
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
            DrawSACNUniversePixels(RenderBuffer, Universe, UniverseDisplayTopLeft, UniverseDisplayDimension);
            
            
            if (OpState->Zoom > .5f)
            {
                v2 TitleDisplayStart = UniverseDisplayTopLeft + v2{0, 12};
                PrintF(&TitleBarString, "Universe %d", Universe->Universe);
                DrawString(RenderBuffer, TitleBarString, State->Interface.Font, 
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
#endif
}

// TODO(Peter): Something isn't working with my laptop trackpad's zoom
FOLDHAUS_INPUT_COMMAND_PROC(UniverseZoom)
{
    universe_view_operation_state* OpState = GetCurrentOperationState(State->Modes, universe_view_operation_state);
    r32 DeltaZoom = (r32)(Mouse.Scroll) / 120;
    OpState->Zoom = GSClamp(0.1f, OpState->Zoom + DeltaZoom, 4.f);
}

FOLDHAUS_INPUT_COMMAND_PROC(UniverseViewEndPan)
{
    // :NeedToPassStateDownModeChain
    universe_view_operation_state* OpState = GetCurrentOperationState(State->Modes, universe_view_operation_state);
    OpState->MouseDown = false;
    OpState->DisplayOffset = OpState->DisplayOffset + (Mouse.Pos - Mouse.DownPos);
}

FOLDHAUS_INPUT_COMMAND_PROC(UniverseViewBeginPan)
{
    // :NeedToPassStateDownModeChain
    universe_view_operation_state* OpState = GetCurrentOperationState(State->Modes, universe_view_operation_state);
    OpState->MouseDown = true;
}

FOLDHAUS_INPUT_COMMAND_PROC(CloseUniverseView)
{
    DeactivateCurrentOperationMode(&State->Modes);
}

input_command UniverseViewCommands [] = {
    { KeyCode_MouseLeftButton, KeyCode_Invalid, Command_Began, UniverseViewBeginPan },
    { KeyCode_MouseLeftButton, KeyCode_Invalid, Command_Ended, UniverseViewEndPan },
    { KeyCode_U, KeyCode_Invalid, Command_Began, CloseUniverseView },
};

FOLDHAUS_INPUT_COMMAND_PROC(OpenUniverseView)
{
    operation_mode* UniverseViewMode = ActivateOperationModeWithCommands(&State->Modes, UniverseViewCommands);
    UniverseViewMode->Render = RenderUniverseView;
    
    // State Setup
    universe_view_operation_state* OpState = CreateOperationState(UniverseViewMode, 
                                                                  &State->Modes, 
                                                                  universe_view_operation_state); 
    OpState->DisplayOffset = v2{0, 0};
    OpState->Zoom = 1.0f;
}
