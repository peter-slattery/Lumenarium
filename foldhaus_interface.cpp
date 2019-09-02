FOLDHAUS_INPUT_COMMAND_PROC(CameraMouseControl)
{
    if (State->NodeInteraction.NodeOffset >= 0) { return; }
    
    if (KeyTransitionedDown(Input, KeyCode_MouseLeftButton))
    {
        State->Camera_StartDragPos = V4(State->Camera.Position, 1);
    }
    
    if (Input.MouseDownY > State->InterfaceYMax)
    {
        if (!State->DrawUniverseOutputDisplay)
        {
            v2 DeltaPos = v2{
                (r32)(Input.New->MouseX - Input.MouseDownX),
                (r32)(Input.New->MouseY - Input.MouseDownY)
            };
            
            m44 XRotation = GetXRotation(-DeltaPos.y * State->PixelsToWorldScale);
            m44 YRotation = GetYRotation(DeltaPos.x * State->PixelsToWorldScale);
            m44 Combined = XRotation * YRotation;
            
            State->Camera.Position = V3(Combined * State->Camera_StartDragPos);
        }
        else
        {
            v2 DeltaPos = v2{
                (r32)(Input.New->MouseX - Input.Old->MouseX),
                (r32)(Input.New->MouseY - Input.Old->MouseY)
            };
            
            State->UniverseOutputDisplayOffset += DeltaPos;
        }
    }
}

FOLDHAUS_INPUT_COMMAND_PROC(CameraMouseZoom)
{
    if (State->DrawUniverseOutputDisplay)
    {
        r32 DeltaZoom = (r32)(Input.New->MouseScroll) / 120;
        State->UniverseOutputDisplayZoom = GSClamp(0.1f, State->UniverseOutputDisplayZoom + DeltaZoom, 4.f);
    }
    
}

FOLDHAUS_INPUT_COMMAND_PROC(ToggleUniverseDebugView)
{
    State->DrawUniverseOutputDisplay = !State->DrawUniverseOutputDisplay;
}


FOLDHAUS_INPUT_COMMAND_PROC(OpenNodeLister)
{
    State->InterfaceShowNodeList = true;
    State->NodeListMenuPosition = v2{(r32)Input.New->MouseX, (r32)Input.New->MouseY};
    SetTextInputDestinationToString(&State->ActiveTextEntry, &State->GeneralPurposeSearchString);
    State->ActiveCommands = &State->TextEntryCommandRegistry;
}

FOLDHAUS_INPUT_COMMAND_PROC(ToggleNodeDisplay)
{
    State->NodeRenderSettings.Display = !State->NodeRenderSettings.Display;
}