
/////////////////////////////////
//
//   Book-Keeping
//
/////////////////////////////////

internal void
InitializePanelLayout(panel_layout* Layout)
{
    Layout->FreeList.Free.Next = &Layout->FreeList;
}

internal panel_entry*
TakeNewPanelEntry(panel_layout* Layout)
{
    panel_entry* FreeEntry = 0;
    if (Layout->FreeList.Free.Next != &Layout->FreeList)
    {
        FreeEntry = Layout->FreeList.Free.Next;
        Layout->FreeList.Free.Next = FreeEntry->Free.Next;
    }
    else
    {
        Assert(Layout->PanelsUsed < PANELS_MAX);
        FreeEntry = Layout->Panels + Layout->PanelsUsed++;
    }
    return FreeEntry;
}

internal panel*
TakeNewPanel(panel_layout* Layout)
{
    panel* Result = 0;
    panel_entry* FreeEntry = TakeNewPanelEntry(Layout);
Result = &FreeEntry->Panel;
    *Result = {0};
    return Result;
}

internal void
FreePanelEntry(panel_entry* Entry, panel_layout* Layout)
{
    Assert(Entry >= Layout->Panels && Entry <= Layout->Panels + PANELS_MAX);
    Entry->Panel = {0};
    Entry->Free.Next = Layout->FreeList.Free.Next;
    Layout->FreeList.Free.Next = Entry;
}

internal void
FreePanelAtIndex(s32 Index, panel_layout* Layout)
{
    Assert(Index > 0 && Index < (s32)Layout->PanelsUsed);
    panel_entry* EntryToFree = Layout->Panels + Index;
    EntryToFree->Free.Next = Layout->FreeList.Free.Next;
    Layout->FreeList.Free.Next = EntryToFree;
}

internal void
SplitPanelVertically(panel* Parent, r32 Percent, v2 ParentMin, v2 ParentMax, panel_layout* Layout)
{
    r32 SplitX = GSLerp(ParentMin.x, ParentMax.x, Percent);
    if (SplitX > ParentMin.x && SplitX < ParentMax.x)
    {
Parent->SplitDirection = PanelSplit_Vertical;
    Parent->SplitPercent = Percent;

     Parent->Left = TakeNewPanelEntry(Layout);
    Parent->Left->Panel.Render = Parent->Render;
    
Parent->Right = TakeNewPanelEntry(Layout);
    Parent->Right->Panel.Render = Parent->Render;
    }
}

internal void
SplitPanelHorizontally(panel* Parent, r32 Percent, v2 ParentMin, v2 ParentMax, panel_layout* Layout)
{
    r32 SplitY = GSLerp(ParentMin.y, ParentMax.y, Percent);
    if (SplitY > ParentMin.y && SplitY < ParentMax.y)
    {
Parent->SplitDirection = PanelSplit_Horizontal;
    Parent->SplitPercent = Percent;

     Parent->Bottom = TakeNewPanelEntry(Layout);
    Parent->Bottom->Panel.Render = Parent->Render;
    
Parent->Top = TakeNewPanelEntry(Layout);
        Parent->Top->Panel.Render = Parent->Render;
    }
}

internal void
ConsolidatePanelsKeepOne(panel* Parent, panel_entry* PanelEntryToKeep, panel_layout* Layout)
{
    panel_entry* LeftChild = Parent->Left;
    panel_entry* RightChild = Parent->Right;
    
    *Parent = PanelEntryToKeep->Panel;
    Parent->SplitDirection = PanelSplit_NoSplit;
    
    FreePanelEntry(LeftChild, Layout);
    FreePanelEntry(RightChild, Layout);
}

internal void
SetPanelDefinition(panel* Panel, panel_definition Def)
{
    if(Panel->Cleanup)
    {
    Panel->Cleanup(Panel);
    }
    
Panel->Cleanup = Def.Cleanup;
    Panel->Render = Def.Render;
    Def.Init(Panel);
}

/////////////////////////////////
//
//   Rendering And Interaction
//
/////////////////////////////////

internal void
HandleMousePanelInteractionOrRecurse(panel* Panel, v2 PanelMin, v2 PanelMax, panel_layout* PanelLayout, mouse_state Mouse)
{
    r32 PanelEdgeClickMaxDistance = 4;
    
    // TODO(Peter): Need a way to calculate this button's position more systemically
    if (Panel->SplitDirection == PanelSplit_NoSplit 
        && PointIsInRange(Mouse.DownPos, PanelMin, PanelMin + v2{25, 25}))
    {
        r32 XDistance = GSAbs(Mouse.Pos.x - Mouse.DownPos.x);
        r32 YDistance = GSAbs(Mouse.Pos.y - Mouse.DownPos.y);
        
        if (XDistance > YDistance)
        {
            r32 XPercent = (Mouse.Pos.x - PanelMin.x) / (PanelMax.x - PanelMin.x);
            SplitPanelVertically(Panel, XPercent, PanelMin, PanelMax, PanelLayout);
        }
        else
        {
            r32 YPercent = (Mouse.Pos.y - PanelMin.y) / (PanelMax.y - PanelMin.y);
            SplitPanelHorizontally(Panel, YPercent, PanelMin, PanelMax, PanelLayout);
        }
    }
    else if (Panel->SplitDirection == PanelSplit_Horizontal)
    {
        r32 SplitY = GSLerp(PanelMin.y, PanelMax.y, Panel->SplitPercent);
        r32 ClickDistanceFromSplit = GSAbs(Mouse.DownPos.y - SplitY);
        if (ClickDistanceFromSplit < PanelEdgeClickMaxDistance)
        {
            r32 NewSplitY = Mouse.Pos.y;
            if (NewSplitY <= PanelMin.y)
            {
                ConsolidatePanelsKeepOne(Panel, Panel->Top, PanelLayout);
            }
            else if (NewSplitY >= PanelMax.y)
            {
                ConsolidatePanelsKeepOne(Panel, Panel->Bottom, PanelLayout);
            }
            else
            {
                Panel->SplitPercent = (NewSplitY  - PanelMin.y) / (PanelMax.y - PanelMin.y);
            }
        }
        else
        {
            HandleMousePanelInteractionOrRecurse(&Panel->Bottom->Panel, PanelMin, v2{PanelMax.x, SplitY}, PanelLayout, Mouse);
            HandleMousePanelInteractionOrRecurse(&Panel->Top->Panel, v2{PanelMin.x, SplitY}, PanelMax, PanelLayout, Mouse);
        }
    }
    else if (Panel->SplitDirection == PanelSplit_Vertical)
    {
        r32 SplitX = GSLerp(PanelMin.x, PanelMax.x, Panel->SplitPercent);
        r32 ClickDistanceFromSplit = GSAbs(Mouse.DownPos.x - SplitX);
        if (ClickDistanceFromSplit < PanelEdgeClickMaxDistance)
        {
            r32 NewSplitX = Mouse.Pos.x;
            if (NewSplitX <= PanelMin.x)
            {
                ConsolidatePanelsKeepOne(Panel, Panel->Right, PanelLayout);
            }
            else if (NewSplitX >= PanelMax.x)
            {
                ConsolidatePanelsKeepOne(Panel, Panel->Left, PanelLayout);
            }
            else
            {
                Panel->SplitPercent = (NewSplitX  - PanelMin.x) / (PanelMax.x - PanelMin.x);
            }
        }
        else
        {
            HandleMousePanelInteractionOrRecurse(&Panel->Left->Panel, PanelMin, v2{SplitX, PanelMax.y}, PanelLayout, Mouse);
            HandleMousePanelInteractionOrRecurse(&Panel->Right->Panel, v2{SplitX, PanelMin.y}, PanelMax, PanelLayout, Mouse);
        }
    }
}

internal void
HandleMousePanelInteraction(panel_layout* PanelLayout, v2 WindowMin, v2 WindowMax, mouse_state Mouse)
{
    r32 PanelEdgeClickMaxDistance = 4;
    
    if (MouseButtonTransitionedUp(Mouse.LeftButtonState))
    {
        Assert(PanelLayout->PanelsUsed > 0);
        panel* FirstPanel = &PanelLayout->Panels[0].Panel;
        HandleMousePanelInteractionOrRecurse(FirstPanel, WindowMin, WindowMax, PanelLayout, Mouse);
    }
}

internal void
DrawPanelFooter(panel* Panel, render_command_buffer* RenderBuffer, v2 FooterMin, v2 FooterMax, interface_config Interface, mouse_state Mouse)
{
    PushRenderQuad2D(RenderBuffer, FooterMin, v2{FooterMax.x, FooterMin.y + 25}, v4{.5f, .5f, .5f, 1.f});
    PushRenderQuad2D(RenderBuffer, FooterMin, FooterMin + v2{25, 25}, WhiteV4);
    
    v2 PanelSelectButtonMin = FooterMin + v2{30, 1};
    v2 PanelSelectButtonMax = PanelSelectButtonMin + v2{100, 23};
    
    if (Panel->PanelSelectionMenuOpen)
    {
        v2 ButtonDimension = v2{100, 25};
        v2 ButtonMin = v2{PanelSelectButtonMin.x, FooterMax.y};
        
        v2 MenuMin = ButtonMin;
        v2 MenuMax = v2{ButtonMin.x + ButtonDimension.x, ButtonMin.y + (ButtonDimension.y * GlobalPanelDefsCount)};
        if (MouseButtonTransitionedDown(Mouse.LeftButtonState)
            && !PointIsInRange(Mouse.DownPos, MenuMin, MenuMax))
    {
        Panel->PanelSelectionMenuOpen = false;
    }


for (s32 i = 0; i < GlobalPanelDefsCount; i++)
        {
            panel_definition Def = GlobalPanelDefs[i];
            string DefName = MakeString(Def.PanelName, Def.PanelNameLength);
            button_result DefinitionButton = EvaluateButton(RenderBuffer,
                                                            ButtonMin, ButtonMin + ButtonDimension,
                                                            DefName, Interface, Mouse);
            if (DefinitionButton.Pressed)
            {
                SetPanelDefinition(Panel, Def);
                Panel->PanelSelectionMenuOpen = false;
            }
            
            ButtonMin.y += ButtonDimension.y;
        }
    }

button_result ButtonResult = EvaluateButton(RenderBuffer,
                                                PanelSelectButtonMin, 
PanelSelectButtonMax,
                                                MakeStringLiteral("Select"), Interface, Mouse);
    if (ButtonResult.Pressed)
    {
        Panel->PanelSelectionMenuOpen = !Panel->PanelSelectionMenuOpen;
    }
    
}

internal void
DrawPanelBorder(panel Panel, v2 PanelMin, v2 PanelMax, v4 Color, render_command_buffer* RenderBuffer)
{
    PushRenderBoundingBox2D(RenderBuffer, PanelMin, PanelMax, 1, Color);
}

internal void
DrawPanelOrRecurse(panel* Panel, v2 PanelMin, v2 PanelMax, v2 WindowMin, v2 WindowMax, render_command_buffer* RenderBuffer, interface_config Interface, mouse_state Mouse, app_state* State, context Context)
{
    if (Panel->SplitDirection == PanelSplit_NoSplit)
    {
        v2 FooterMin = PanelMin;
        v2 FooterMax = v2{PanelMax.x, PanelMin.y + 25};
        v2 PanelViewMin = v2{PanelMin.x, FooterMax.y};
        v2 PanelViewMax = PanelMax;
        
        Panel->Render(*Panel, PanelViewMin, PanelViewMax, RenderBuffer, State, Context, Mouse);
        v4 BorderColor = v4{0, 1, 1, 1};
        if (PointIsInRange(Mouse.Pos, PanelMin, PanelMax))
        {
            BorderColor = v4{1, 0, 1, 1};
        }
        
PushRenderOrthographic(RenderBuffer, WindowMin.x, WindowMin.y, WindowMax.x, WindowMax.y);
DrawPanelBorder(*Panel, PanelMin, PanelMax, BorderColor, RenderBuffer);
DrawPanelFooter(Panel, RenderBuffer, FooterMin, FooterMax, Interface, Mouse);
    }
    else if (Panel->SplitDirection == PanelSplit_Horizontal)
    {
        r32 SplitY = GSLerp(PanelMin.y, PanelMax.y, Panel->SplitPercent);
        DrawPanelOrRecurse(&Panel->Bottom->Panel, PanelMin, v2{PanelMax.x, SplitY}, WindowMin, WindowMax, RenderBuffer, Interface, Mouse, State, Context);
        DrawPanelOrRecurse(&Panel->Top->Panel, v2{PanelMin.x, SplitY}, PanelMax, WindowMin, WindowMax, RenderBuffer, Interface, Mouse, State, Context);
    }
    else if (Panel->SplitDirection == PanelSplit_Vertical)
    {
        r32 SplitX = GSLerp(PanelMin.x, PanelMax.x, Panel->SplitPercent);
        DrawPanelOrRecurse(&Panel->Left->Panel, PanelMin, v2{SplitX, PanelMax.y}, WindowMin, WindowMax, RenderBuffer, Interface, Mouse, State, Context);
        DrawPanelOrRecurse(&Panel->Right->Panel, v2{SplitX, PanelMin.y}, PanelMax, WindowMin, WindowMax, RenderBuffer, Interface, Mouse, State, Context);
    }
}

internal void
DrawAllPanels(panel_layout PanelLayout, v2 WindowMin, v2 WindowMax, render_command_buffer* RenderBuffer, interface_config Interface, mouse_state Mouse, app_state* State, context Context)
{
    Assert(PanelLayout.PanelsUsed > 0);
    panel* FirstPanel = &PanelLayout.Panels[0].Panel;
    DrawPanelOrRecurse(FirstPanel, WindowMin, WindowMax, WindowMin, WindowMax, RenderBuffer, Interface, Mouse, State, Context);
}
