// NOTE(Peter): This stuff was all a test to see how I could do panel splitting. Thinking about moving away 
// from that for now. Might return later if necessary
// TODO(Peter): Finish this if necessary

struct interface_region
{ 
    v2 Min, Max;
    union
    {
        struct
        {
            interface_region* A;
            interface_region* B;
        };
        struct
        {
            interface_region* Left;
            interface_region* Right;
        };
        struct
        {
            interface_region* Top;
            interface_region* Bottom;
        };
    };
};

struct interface_tracker
{
    memory_arena* Storage;
    interface_region RootRegion;
};

enum interface_region_split
{
    InterfaceRegionSplit_Vertical,
    InterfaceRegionSplit_Horizontal,
};

inline s32
RegionWidth (interface_region Region)
{
    s32 Result = Region.Max.x - Region.Min.x;
    return Result;
}

inline s32
RegionHeight (interface_region Region)
{
    s32 Result = Region.Max.y - Region.Min.y;
    return Result;
}

internal void
SplitRegion (interface_tracker* Tracker, interface_region* Parent, s32 SplitPosition, interface_region_split SplitDirection)
{
    if (!Parent->A)
    {
        interface_region* A = PushStruct(Tracker->Storage, interface_region);
        A->A = 0;
        A->B = 0;
        Parent->A = A;
    }
    Parent->A->Min = Parent->Min;
    Parent->A->Max = Parent->Max;
    
    if (!Parent->B)
    {
        interface_region* B = PushStruct(Tracker->Storage, interface_region);
        B->A = 0;
        B->B = 0;
        Parent->B = B;
    }
    Parent->B->Min = Parent->Min;
    Parent->B->Max = Parent->Max;
    
    switch (SplitDirection)
    {
        case InterfaceRegionSplit_Vertical:
        {
            Parent->Left->Max.x = Parent->Min.x + SplitPosition;
            Parent->Right->Min.x = Parent->Min.x + SplitPosition;
        }break;
        
        case InterfaceRegionSplit_Horizontal:
        {
            Parent->Bottom->Max.y = Parent->Min.y + SplitPosition;
            Parent->Top->Min.y = Parent->Min.y + SplitPosition;
        }break;
    }
}

internal interface_tracker
CreateInterfaceTracker (memory_arena* Storage, s32 ScreenWidth, s32 ScreenHeight)
{
    interface_tracker Result = {};
    Result.Storage = Storage;
    Result.RootRegion.A = 0;
    Result.RootRegion.B = 0;
    Result.RootRegion.Min = v2{0, 0};
    Result.RootRegion.Max = v2{(r32)ScreenWidth, (r32)ScreenHeight};
    return Result;
}

internal v2
DrawString (render_command_buffer* RenderBuffer, string String, bitmap_font* Font, s32 PointSize, v2 Position, v4 Color)
{
    DEBUG_TRACK_FUNCTION;
    
    v2 LowerRight = Position;
    
    render_quad_batch_constructor BatchConstructor = PushRenderTexture2DBatch(RenderBuffer, String.Length,
                                                                              Font->BitmapMemory,
                                                                              Font->BitmapTextureHandle,
                                                                              Font->BitmapWidth,
                                                                              Font->BitmapHeight,
                                                                              Font->BitmapBytesPerPixel,
                                                                              Font->BitmapStride);
    
    r32 FontScale = (r32)PointSize / Font->PixelHeight;
    v2 RegisterPosition = Position;
    char* C = String.Memory;
    for (s32 i = 0; i < String.Length; i++)
    {
        s32 GlyphDataIndex = GetIndexForCodepoint(*Font, *C);
        codepoint_bitmap CodepointInfo = Font->CodepointValues[GlyphDataIndex];
        
        r32 MinX = RegisterPosition.x + CodepointInfo.XOffset * FontScale;
        r32 MinY = RegisterPosition.y + CodepointInfo.YOffset * FontScale;
        r32 MaxX = MinX + (CodepointInfo.Width) * FontScale;
        r32 MaxY = MinY + (CodepointInfo.Height) * FontScale;
        
        v2 MinUV = v2{(r32)CodepointInfo.BitmapX, (r32)CodepointInfo.BitmapY};
        v2 MaxUV = MinUV + v2{(r32)CodepointInfo.Width, (r32)CodepointInfo.Height};
        PushQuad2DOnBatch(&BatchConstructor, v2{MinX, MinY}, v2{MaxX, MinY}, v2{MaxX, MaxY}, v2{MinX, MaxY},  MinUV, MaxUV, Color);
        
        RegisterPosition.x += CodepointInfo.Width * FontScale;
        C++;
    }
    
    LowerRight.x = RegisterPosition.x;
    
    return LowerRight;
}

struct interface_config
{
    v4 PanelBGColors[4];
    v4 ButtonColor_Inactive, ButtonColor_Active, ButtonColor_Selected;
    v4 TextColor;
    bitmap_font* Font;
    r32 FontSize;
    v2 Margin;
};

struct button_result
{
    b32 Pressed;
    r32 Advance;
};

internal button_result
EvaluateButton_ (render_command_buffer* RenderBuffer, v2 Min, v2 Max, string Label, interface_config Config, input Input, v4 BGColor)
{
    button_result Result = {};
    Result.Pressed = false;
    
    v2 MousePos = v2{(r32)Input.New->MouseX, (r32)Input.New->MouseY};
    if (PointIsInRange(MousePos, Min, Max))
    {
        if (KeyTransitionedDown(Input, KeyCode_MouseLeftButton))
        {
            Result.Pressed = true;
        }
        else
        {
            BGColor = Config.ButtonColor_Active; 
        }
    }
    
    PushRenderQuad2D(RenderBuffer, Min, Max, BGColor);
    DrawString(RenderBuffer, Label, Config.Font, Config.Font->PixelHeight, Min + Config.Margin, Config.TextColor);
    
    Result.Advance = (Max.y - Min.y) + Config.Margin.y;
    return Result;
}

internal button_result
EvaluateButton (render_command_buffer* RenderBuffer, v2 Min, v2 Max, string Label, interface_config Config, input Input)
{
    button_result Result = EvaluateButton_(RenderBuffer, Min, Max, Label, Config, Input, Config.ButtonColor_Inactive);
    return Result;
}

internal button_result
EvaluateSelectableButton (render_command_buffer* RenderBuffer, v2 Min, v2 Max, string Label, interface_config Config, input Input, b32 Selected)
{
    v4 BGColor = Config.ButtonColor_Inactive;
    if (Selected)
    {
        BGColor = Config.ButtonColor_Selected;
    }
    
    button_result Result = EvaluateButton_(RenderBuffer, Min, Max, Label, Config, Input, BGColor);
    return Result;
}

struct multi_option_label_result
{
    b32 Pressed;
    s32 IndexPressed;
    r32 Advance;
};

internal multi_option_label_result
EvaluateMultiOptionLabel (render_command_buffer* RenderBuffer, 
                          v2 Min, v2 Max, string Label, string Options[], 
                          interface_config Config, input Input)
{
    multi_option_label_result Result = {};
    Result.Pressed = false;
    
    DrawString(RenderBuffer, Label, Config.Font, 14, Min + Config.Margin, Config.TextColor);
    
    r32 ButtonSide = (Max.y - Min.y) - (2 * Config.Margin.y);
    v2 ButtonDim = v2{ButtonSide, ButtonSide};
    v2 ButtonPos = Max - (ButtonDim + Config.Margin);
    
    for (s32 b = 0; b < sizeof(Options) / sizeof(Options[0]); b++)
    {
        button_result Button = EvaluateButton(RenderBuffer, ButtonPos, ButtonPos + ButtonDim,
                                              Options[b], Config, Input);
        if (Button.Pressed)
        {
            Result.Pressed = true;
            Result.IndexPressed = b;
        }
    }
    
    Result.Advance = (Max.y - Min.y) + Config.Margin.y;
    return Result;
}

// NOTE(Peter): returns IndexPressed = -1 if the button itself is pressed, as opposed
// to one of its options
internal multi_option_label_result
EvaluateMultiOptionButton (render_command_buffer* RenderBuffer, v2 Min, v2 Max, string Text, string Options[], b32 Selected,
                           interface_config Config, input Input)
{
    multi_option_label_result Result = {};
    Result.Pressed = false;
    
    s32 OptionsCount =  sizeof(Options) / sizeof(Options[0]);
    r32 ButtonSide = (Max.y - Min.y) - (2 * Config.Margin.y);
    v2 ButtonDim = v2{ButtonSide, ButtonSide};
    
    v2 FirstButtonPos = Max - ((ButtonDim + Config.Margin) * OptionsCount);
    v2 NewMax = v2{FirstButtonPos.x - Config.Margin.x, Max.y};
    
    button_result MainButton = EvaluateSelectableButton(RenderBuffer, Min, NewMax, Text, Config, Input, Selected);
    if (MainButton.Pressed)
    {
        Result.Pressed = true;
        Result.IndexPressed = -1;
    }
    
    v2 ButtonPos = Max - (ButtonDim + Config.Margin);
    
    for (s32 b = 0; b < OptionsCount; b++)
    {
        button_result Button = EvaluateButton(RenderBuffer, ButtonPos, ButtonPos + ButtonDim,
                                              Options[b], Config, Input);
        if (Button.Pressed)
        {
            Result.Pressed = true;
            Result.IndexPressed = b;
        }
    }
    
    Result.Advance = (Max.y - Min.y) + Config.Margin.y;
    return Result;
}

struct slider_result
{
    r32 Percent;
    r32 Advance;
};

internal slider_result
EvaluateSlider (render_command_buffer* RenderBuffer, v2 Min, v2 Max, string Label, r32 Percent, interface_config Config, input Input)
{
    slider_result Result = {};
    
    v4 BGColor = Config.ButtonColor_Inactive;
    v4 FillColor = Config.ButtonColor_Selected;
    
    r32 DisplayPercent = Percent;
    
    v2 MousePos = v2{(r32)Input.New->MouseX, (r32)Input.New->MouseY};
    if (PointIsInRange(MousePos, Min, Max))
    {
        BGColor = Config.ButtonColor_Active;
    }
    
    if (KeyDown(Input, KeyCode_MouseLeftButton))
    {
        v2 MouseDownPos = v2{(r32)Input.MouseDownX, (r32)Input.MouseDownY};
        if (PointIsInRange(MouseDownPos, Min, Max))
        {
            r32 TempFillPercent = (MousePos.y - Min.y) / (Max.y - Min.y);
            
            DisplayPercent = GSClamp(0.0f, TempFillPercent, 1.0f);
        }
    }
    
    r32 FillHeight = ((Max.y - Min.y) - 4) * DisplayPercent;
    
    PushRenderQuad2D(RenderBuffer, Min, Max, BGColor);
    PushRenderQuad2D(RenderBuffer, Min + v2{2, 2}, v2{Max.x - 2, Min.y + 2 + FillHeight}, FillColor);
    
    // TODO(Peter): display the actual value of the slider
    
    DrawString(RenderBuffer, Label, Config.Font, 14, Min, Config.TextColor);
    
    Result.Percent = DisplayPercent;
    Result.Advance = (Max.y - Min.y) + Config.Margin.y; 
    
    return Result;
}

struct panel_result
{
    s32 Depth;
    v2 NextPanelMin;
    v2 ChildMin, ChildMax;
};

internal panel_result
EvaluatePanel (render_command_buffer* RenderBuffer, v2 Min, v2 Max, s32 Depth, interface_config Config, input Input)
{
    panel_result Result = {};
    
    Result.Depth = Depth;
    Result.ChildMin = Min + Config.Margin;
    Result.ChildMax = Max - Config.Margin; 
    Result.NextPanelMin = v2{Max.x, Min.y};
    
    v4 BG = Config.PanelBGColors[Depth];
    PushRenderQuad2D(RenderBuffer, Min, Max, BG);
    
    return Result;
}

internal panel_result
EvaluatePanel (render_command_buffer* RenderBuffer, v2 Min, v2 Max, string Label, s32 Depth, interface_config Config, input Input)
{
    panel_result Result = EvaluatePanel(RenderBuffer, Min, Max, Depth, Config, Input);
    
    v2 TextPos = v2{
        Min.x + Config.Margin.x,
        Max.y - ((r32)NewLineYOffset(*Config.Font) + Config.Margin.y)
    };
    DrawString(RenderBuffer, Label, Config.Font, 14, TextPos, Config.TextColor);
    Result.ChildMax = v2{Max.x, TextPos.y} - Config.Margin;
    
    return Result;
}

internal panel_result
EvaluatePanel(render_command_buffer* RenderBuffer, panel_result* ParentPanel, r32 Height, string Title, interface_config Config, input Input)
{
    v2 Min = v2{ParentPanel->ChildMin.x, ParentPanel->ChildMax.y - Height};
    v2 Max = ParentPanel->ChildMax;
    panel_result Result = EvaluatePanel(RenderBuffer, Min, Max, Title, ParentPanel->Depth + 1, Config, Input);
    
    ParentPanel->ChildMax.y = Min.y - Config.Margin.y;
    
    return Result;
}

enum selection_state
{
    Selection_None,
    Selection_Selected,
    Selection_Deselected,
};

struct scroll_list_result
{
    s32 IndexSelected;
    s32 StartIndex;
    selection_state Selection;
};

internal scroll_list_result
DrawOptionsList(render_command_buffer* RenderBuffer, v2 Min, v2 Max, 
                string* Options, s32 OptionsCount,
                s32 Start, interface_config Config, input Input)
{
    scroll_list_result Result = {};
    Result.IndexSelected = -1;
    Result.StartIndex = Start;
    Result.Selection = Selection_None;
    
    r32 OptionHeight = NewLineYOffset(*Config.Font) + (2 * Config.Margin.y);
    r32 OptionOffset = OptionHeight + Config.Margin.y;
    
    s32 OptionsToDisplay = ((Max.y - Min.y) / OptionHeight) - 2;
    OptionsToDisplay = GSMin(OptionsToDisplay, (OptionsCount - Start));
    
    v2 ButtonMin = v2{Min.x, Max.y - OptionHeight};
    v2 ButtonMax = v2{Max.x, Max.y};
    
    string* OptionCursor = Options + Start;
    for (s32 i = 0; i < OptionsToDisplay; i++)
    {
        button_result Button = EvaluateButton(RenderBuffer, ButtonMin, ButtonMax,
                                              *OptionCursor,
                                              Config, Input);
        if (Button.Pressed)
        {
            Result.IndexSelected = Start + i;
            Result.Selection = Selection_Selected;
        }
        OptionCursor++;
        ButtonMin.y -= OptionOffset;
        ButtonMax.y -= OptionOffset;
    }
    
    r32 HalfWidthWithMargin = ((Max.x - Min.x) / 2.0f) - Config.Margin.x;
    string DownArrowString = MakeStringLiteral(" v ");
    string UpArrowString = MakeStringLiteral(" ^ ");
    button_result Down = EvaluateButton(RenderBuffer, Min, v2{Min.x + HalfWidthWithMargin, Min.y + OptionHeight},
                                        DownArrowString, Config, Input);
    button_result Up = EvaluateButton(RenderBuffer, v2{Min.x + HalfWidthWithMargin + Config.Margin.x, Min.y},
                                      v2{Max.x, Min.y + OptionHeight},
                                      UpArrowString, Config, Input);
    if (Down.Pressed)
    {
        Result.StartIndex += 1;
    }
    if (Up.Pressed)
    {
        Result.StartIndex -= 1;
    }
    
    Result.StartIndex = GSClamp(0, Result.StartIndex, OptionsCount);
    
    return Result;
}

internal scroll_list_result
DrawSelectableOptionsList(render_command_buffer* RenderBuffer, v2 Min, v2 Max, 
                          string* Options, s32 OptionsCount,
                          s32 Start, s32 Selected, interface_config Config, input Input)
{
    scroll_list_result Result = {};
    Result.IndexSelected = Selected;
    Result.StartIndex = Start;
    Result.Selection = Selection_None;
    
    r32 OptionHeight = NewLineYOffset(*Config.Font) + (2 * Config.Margin.y);
    r32 OptionOffset = OptionHeight + Config.Margin.y;
    
    s32 OptionsToDisplay = ((Max.y - Min.y) / OptionHeight) - 2;
    OptionsToDisplay = GSMin(OptionsToDisplay, (OptionsCount - Start));
    
    string* OptionCursor = 0;
    OptionCursor = Options + Start;
    
    v2 ButtonMin = v2{Min.x, Max.y - OptionHeight};
    v2 ButtonMax = v2{Max.x, Max.y};
    
    for (s32 i = 0; i < OptionsToDisplay; i++)
    {
        button_result Button = EvaluateSelectableButton(RenderBuffer, ButtonMin, ButtonMax,
                                                        *OptionCursor,
                                                        Config, Input, (Selected == Start + i));
        if (Button.Pressed)
        {
            s32 SelectedIndex = Start + i;
            if (SelectedIndex == Result.IndexSelected)
            {
                Result.Selection = Selection_Deselected;
                Result.IndexSelected = -1;
            }
            else
            {
                Result.Selection = Selection_Selected;
                Result.IndexSelected = Start + i;
            }
        }
        
        OptionCursor++;
        
        ButtonMin.y -= OptionOffset;
        ButtonMax.y -= OptionOffset;
    }
    
    r32 HalfWidthWithMargin = ((Max.x - Min.x) / 2.0f) - Config.Margin.x;
    string DownArrowString = MakeStringLiteral(" v ");
    string UpArrowString = MakeStringLiteral(" ^ ");
    button_result Down = EvaluateButton(RenderBuffer, Min, v2{Min.x + HalfWidthWithMargin, Min.y + OptionHeight},
                                        DownArrowString, Config, Input);
    button_result Up = EvaluateButton(RenderBuffer, v2{Min.x + HalfWidthWithMargin + Config.Margin.x, Min.y},
                                      v2{Max.x, Min.y + OptionHeight},
                                      UpArrowString, Config, Input);
    if (Down.Pressed)
    {
        Result.StartIndex += 1;
    }
    if (Up.Pressed)
    {
        Result.StartIndex -= 1;
    }
    
    Result.StartIndex = GSClamp(0, Result.StartIndex, OptionsCount);
    
    return Result;
}

internal r32
EvaluateColorChannelSlider (render_command_buffer* RenderBuffer, v4 ChannelMask, v2 Min, v2 Max, r32 Current, input Input)
{
    r32 Result = Current;
    
    render_quad_batch_constructor Batch = PushRenderQuad2DBatch(RenderBuffer, 2);
    
    v4 LeftColor = ChannelMask * 0;
    LeftColor.a = 1.f;
    v4 RightColor = ChannelMask;
    PushQuad2DOnBatch(&Batch, 
                      Min, v2{Max.x, Min.y}, Max, v2{Min.x, Max.y},
                      v2{0, 0}, v2{1, 0}, v2{1, 1}, v2{0, 1},
                      LeftColor, RightColor, RightColor, LeftColor);
    
    if (KeyDown(Input, KeyCode_MouseLeftButton))
    {
        v2 MouseDownPos = v2{(r32)Input.MouseDownX, (r32)Input.MouseDownY};
        if (PointIsInRange(MouseDownPos, Min, Max))
        {
            Result = ((r32)Input.New->MouseX - Min.x) / (Max.x - Min.x);
            Result = GSClamp01(Result);
        }
    }
    
    r32 DragBarWidth = 8;
    v2 DragBarMin = v2{GSLerp(Min.x, Max.x, Result) - (DragBarWidth / 2), Min.y - 2};
    v2 DragBarMax = DragBarMin + v2{DragBarWidth, (Max.y - Min.y) + 4};
    
    PushQuad2DOnBatch(&Batch, DragBarMin, DragBarMax, v4{.3f, .3f, .3f, 1.f});
    
    return Result;
}

internal b32
EvaluateColorPicker (render_command_buffer* RenderBuffer, v4* Value, v2 PanelMin, interface_config Config, input Input)
{
    b32 ShouldClose = false;
    
    v2 PanelMax = v2{400, 500};
    if (KeyTransitionedDown(Input, KeyCode_MouseLeftButton) && 
        !PointIsInRange(v2{(r32)Input.New->MouseX, (r32)Input.New->MouseY}, PanelMin, PanelMax))
    {
        ShouldClose = true;
    }
    else
    {
        PushRenderQuad2D(RenderBuffer, PanelMin, PanelMax, v4{.5f, .5f, .5f, 1.f});
        
        v2 TitleMin = v2{PanelMin.x + 5, PanelMax.y - (Config.Font->PixelHeight + 5)};
        DrawString(RenderBuffer, MakeStringLiteral("Color Picker"), Config.Font, Config.Font->PixelHeight, 
                   TitleMin, WhiteV4);
        
        v2 SliderDim = v2{(PanelMax.x - PanelMin.x) - 20, 32};
        // channel sliders
        v2 SliderMin = TitleMin - v2{0, SliderDim.y + 10};
        Value->r = EvaluateColorChannelSlider(RenderBuffer, RedV4, SliderMin, SliderMin + SliderDim, Value->r, Input);
        SliderMin.y -= SliderDim.y + 10;
        Value->g = EvaluateColorChannelSlider(RenderBuffer, GreenV4, SliderMin, SliderMin + SliderDim, Value->g, Input);
        SliderMin.y -= SliderDim.y + 10;
        Value->b = EvaluateColorChannelSlider(RenderBuffer, BlueV4, SliderMin, SliderMin + SliderDim, Value->b, Input);
        SliderMin.y -= SliderDim.y + 10;
        Value->a = EvaluateColorChannelSlider(RenderBuffer, WhiteV4, SliderMin, SliderMin + SliderDim, Value->a, Input);
        
        // Output Color Display
        SliderMin.y -= 100;
        PushRenderQuad2D(RenderBuffer, SliderMin, SliderMin + v2{75, 75}, *Value);
    }
    
    return ShouldClose;
}