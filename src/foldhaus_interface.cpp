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


FOLDHAUS_INPUT_COMMAND_PROC(OpenNodeLister)
{
    State->InterfaceShowNodeLister = true;
    State->NodeListMenuPosition = Mouse.Pos;
    SetTextInputDestinationToString(&State->ActiveTextEntry, &State->GeneralPurposeSearchString);
    State->ActiveCommands = &State->NodeListerCommandRegistry;
}

FOLDHAUS_INPUT_COMMAND_PROC(ToggleNodeDisplay)
{
    State->NodeRenderSettings.Display = !State->NodeRenderSettings.Display;
}