

// NOTE(Peter): returns the rightmost bound of the panel
internal r32
DrawLeftHandInterface (app_state* State, input Input, r32 WindowHeight, render_command_buffer* RenderBuffer)
{
    DEBUG_TRACK_FUNCTION;
    
    s32 StringLength = 128;
    
    panel_result LeftHandPanel = EvaluatePanel(RenderBuffer, v2{0, 0}, v2{250, WindowHeight},
                                               MakeStringLiteral("Channel Ops"), 0, State->Interface, Input);
    
    r32 ListHeight = (LeftHandPanel.ChildMax.y - LeftHandPanel.ChildMin.y) / 2;
    panel_result ChannelListPanel = EvaluatePanel(RenderBuffer, &LeftHandPanel, 
                                                  ListHeight, MakeStringLiteral("Channels"), 
                                                  State->Interface, Input);
    panel_result PatternsListPanel = EvaluatePanel(RenderBuffer, &LeftHandPanel, ListHeight, MakeStringLiteral("Patterns"),
                                                   State->Interface, Input);
    
    // NOTE(Peter): have to do this before we open it otherwise, it'll just close again immediately
    // due to the mouse being pressed, outside the box, on the frame it is opened;
    if (State->InterfaceState.AddingPattern &&
        State->InterfaceState.ChannelSelected >= 0)
    {
        v2 PatternSelectorDim = v2{300, 200};
        v2 PatternSelectorPosition = PatternsListPanel.NextPanelMin;
        PatternSelectorPosition.y = PatternsListPanel.ChildMax.y - PatternSelectorDim.y;
        
        panel_result AddPatternPanel = EvaluatePanel(
            RenderBuffer,
            PatternSelectorPosition, PatternSelectorPosition + PatternSelectorDim,
            MakeStringLiteral("Add Pattern"), 0, State->Interface, Input);
        
        if (KeyTransitionedDown(Input, KeyCode_MouseLeftButton) &&
            !PointIsInRange(v2{(r32)Input.New->MouseX, (r32)Input.New->MouseY},
                            AddPatternPanel.ChildMin, AddPatternPanel.ChildMax))
        {
            State->InterfaceState.AddingPattern = false;
        }
        
        s32 PatternsCount = sizeof(PatternRegistry)/sizeof(PatternRegistry[0]);
        string* PatternNames = PushArray(State->Transient, string, PatternsCount);
        
        for (s32 i = 0; i < PatternsCount; i++)
        {
            PushString(&PatternNames[i], State->Transient, StringLength);
            CopyCharArrayToString(PatternRegistry[i].Name, &PatternNames[i]);
        }
        
        scroll_list_result PatternsResult = DrawOptionsList(RenderBuffer, AddPatternPanel.ChildMin, 
                                                            AddPatternPanel.ChildMax,
                                                            PatternNames,
                                                            PatternsCount, 
                                                            State->InterfaceState.PatternSelectorStart,
                                                            State->Interface,
                                                            Input);
        if (PatternsResult.IndexSelected >= 0)
        {
            led_channel* ActiveChannel = GetChannelByIndex(State->InterfaceState.ChannelSelected,
                                                           State->ChannelSystem);
            pattern_index_id_key PatternKey = AddPattern(
                &State->PatternSystem, 
                &PatternRegistry[PatternsResult.IndexSelected]);
            AddPatternKeyToChannel(PatternKey, ActiveChannel);
        }
        
        State->InterfaceState.PatternSelectorStart = PatternsResult.StartIndex;
    }
    
    s32 ChannelCount = State->ChannelSystem.ChannelCount;
    // NOTE(Peter): adding one to the channel count here so that we can tack on the '+ Add Channel' lable at the end.
    // NOTE(Peter): I think I've spelled lable as label all throughout here now... oops.
    s32 ChannelLabelsCount = ChannelCount + 1;
    string* ChannelTitles = PushArray(State->Transient, string, ChannelLabelsCount);
    
    led_channel* Channel = State->ChannelSystem.Channels;
    for (s32 ChannelIdx = 0; ChannelIdx < ChannelCount; ChannelIdx++)
    {
        PushString(&ChannelTitles[ChannelIdx], State->Transient, StringLength);
        PrintF(&ChannelTitles[ChannelIdx], "Channel %d", Channel->ChannelID);
        Channel = Channel->Next;
    }
    
    ChannelTitles[ChannelCount] = MakeStringLiteral("+ Add Channel");
    
    scroll_list_result ChannelList = DrawSelectableOptionsList(
        RenderBuffer,
        ChannelListPanel.ChildMin,
        ChannelListPanel.ChildMax,
        ChannelTitles,
        ChannelLabelsCount,
        State->InterfaceState.ChannelSelectorStart, 
        State->InterfaceState.ChannelSelected,
        State->Interface, Input);
    
    State->InterfaceState.ChannelSelectorStart = ChannelList.StartIndex;
    if (ChannelList.Selection == Selection_Selected)
    {
        if (ChannelList.IndexSelected >= ChannelCount)
        {
            led_channel* NewChannel = AddLEDChannel(&State->ChannelSystem);
        }
        else
        {
            State->InterfaceState.SelectionType = InterfaceSelection_Channel;
            State->InterfaceState.ChannelSelected = ChannelList.IndexSelected;
            State->InterfaceState.ActiveChannelPatternSelected = -1;
        }
    }
    else if (ChannelList.Selection == Selection_Deselected)
    {
        State->InterfaceState.ChannelSelected = -1;
        State->InterfaceState.ActiveChannelPatternSelected = -1;
    }
    
    s32 ActiveChannelPatternCount = 0;
    s32 PatternLabelsCount =  1;
    string* PatternTitles = PushArray(State->Transient, string, 1);; 
    
    if (State->InterfaceState.ChannelSelected >= 0)
    {
        led_channel* ActiveChannel = GetChannelByIndex(State->InterfaceState.ChannelSelected,
                                                       State->ChannelSystem);
        ActiveChannelPatternCount = ActiveChannel->ActivePatterns;
        
        // NOTE(Peter): We're just growing the PatternTitles array allocated above. 
        // IMPORTANT(Peter): make sure no allocations happen between the PushArray to PatternTitles and this one vvv
        PushArray(State->Transient, string, ActiveChannelPatternCount);
        PatternLabelsCount += ActiveChannelPatternCount;
        
        for (s32 P = 0; P < ActiveChannelPatternCount; P++)
        {
            led_pattern* Pattern = FindPatternAndUpdateIDKey(&ActiveChannel->Patterns[P],
                                                             &State->PatternSystem);
            
            PushString(&PatternTitles[P], State->Transient, StringLength);
            PrintF(&PatternTitles[P], "%s %d", Pattern->Name, ActiveChannel->Patterns[P].ID);
            
            Pattern++;
        }
    }
    
    PatternTitles[ActiveChannelPatternCount]= MakeStringLiteral("+ Add Pattern");
    
    scroll_list_result PatternsList = DrawSelectableOptionsList(
        RenderBuffer,
        PatternsListPanel.ChildMin,
        PatternsListPanel.ChildMax,
        PatternTitles, 
        PatternLabelsCount,
        State->InterfaceState.ActiveChannelPatternSelectorStart, 
        State->InterfaceState.ActiveChannelPatternSelected,
        State->Interface, Input
        );
    
    State->InterfaceState.ActiveChannelPatternSelectorStart = PatternsList.StartIndex;
    if (PatternsList.Selection == Selection_Selected)
    {
        if (PatternsList.IndexSelected >= ActiveChannelPatternCount)
        {
            State->InterfaceState.AddingPattern = true;
        }
        else if (PatternsList.IndexSelected >= 0)
        {
            State->InterfaceState.SelectionType = InterfaceSelection_Pattern;
            OutputDebugStringA("Pattern\n");
            State->InterfaceState.ActiveChannelPatternSelected = PatternsList.IndexSelected;
        }
    }
    else if (PatternsList.Selection == Selection_Deselected)
    {
        State->InterfaceState.SelectionType = InterfaceSelection_None;
        OutputDebugStringA("None\n");
        State->InterfaceState.ActiveChannelPatternSelected = -1;
    }
    
    return LeftHandPanel.NextPanelMin.x;
}

FOLDHAUS_INPUT_COMMAND_PROC(DeleteSelectedChannelOrPattern)
{
    if (State->InterfaceState.ChannelSelected >= 0)
    {
        switch (State->InterfaceState.SelectionType)
        {
            case InterfaceSelection_Channel:
            {
                led_channel* DeleteCandidate = GetChannelByIndex(
                    State->InterfaceState.ChannelSelected,
                    State->ChannelSystem);
                
                for (s32 i = 0; i < DeleteCandidate->ActivePatterns; i++)
                {
                    RemovePattern(DeleteCandidate->Patterns[i],
                                  &State->PatternSystem);
                }
                
                RemoveLEDChannel(State->InterfaceState.ChannelSelected, &State->ChannelSystem);
                State->InterfaceState.ChannelSelected--;
                
            }break;
            
            case InterfaceSelection_Pattern:
            {
                if (State->InterfaceState.ActiveChannelPatternSelected >= 0)
                {
                    led_channel* ActiveChannel = GetChannelByIndex(State->InterfaceState.ChannelSelected,
                                                                   State->ChannelSystem);
                    s32 KeyIndex = State->InterfaceState.ActiveChannelPatternSelected;
                    pattern_index_id_key Key = ActiveChannel->Patterns[KeyIndex];
                    if (RemovePattern(Key, &State->PatternSystem))
                    {
                        RemovePatternKeyFromChannel(Key, ActiveChannel);
                    }
                }
            }break;
        }
    }
}

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


FOLDHAUS_INPUT_COMMAND_PROC(AddNode)
{
    State->InterfaceShowNodeList = true;
    State->NodeListMenuPosition = v2{(r32)Input.New->MouseX, (r32)Input.New->MouseY};
}