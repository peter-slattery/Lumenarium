internal v2
DrawCharacter (render_quad_batch_constructor* BatchConstructor, char C, bitmap_font Font, v2 Position, v4 Color, r32 FontScale)
{
    s32 GlyphDataIndex = GetIndexForCodepoint(Font, C);
    codepoint_bitmap CodepointInfo = Font.CodepointValues[GlyphDataIndex];
    
    r32 MinX = Position.x + CodepointInfo.XOffset * FontScale;
    r32 MinY = Position.y + CodepointInfo.YOffset * FontScale;
    r32 MaxX = MinX + (CodepointInfo.Width) * FontScale;
    r32 MaxY = MinY + (CodepointInfo.Height) * FontScale;
    
    PushQuad2DOnBatch(BatchConstructor, 
                      v2{MinX, MinY}, v2{MaxX, MinY}, 
                      v2{MaxX, MaxY}, v2{MinX, MaxY},  
                      CodepointInfo.UVMin, CodepointInfo.UVMax, 
                      Color);
    
    return v2{Position.x + CodepointInfo.Width * FontScale, Position.y};
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
        v2 PositionAfterCharacter = DrawCharacter(&BatchConstructor, *C, *Font, RegisterPosition, Color, FontScale);
        RegisterPosition.x = PositionAfterCharacter.x;
        C++;
    }
    
    LowerRight.x = RegisterPosition.x;
    
    return LowerRight;
}

internal void
DrawCursor (render_quad_batch_constructor* BatchConstructor, v2 Position, v4 Color, bitmap_font Font, r32 FontScale)
{
    v2 Min = Position;
    v2 Max = Position + v2{(r32)Font.MaxCharWidth * FontScale, Font.Ascent + Font.Descent * FontScale};
    PushQuad2DOnBatch(BatchConstructor, Min, Max, Color);
}

internal v2
DrawStringWithCursor (render_command_buffer* RenderBuffer, string String, s32 CursorPosition, bitmap_font* Font, s32 PointSize, v2 Position, v4 Color, v4 CursorColor)
{
    DEBUG_TRACK_FUNCTION;
    v2 LowerRight = Position;
    
    // NOTE(Peter): We push this on first so that the cursor will be drawn underneath any character it may overlap with
    render_quad_batch_constructor CursorBatch = PushRenderQuad2DBatch(RenderBuffer, 1);
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
        if (i == CursorPosition)
        {
            DrawCursor(&CursorBatch, RegisterPosition, GreenV4, *Font, FontScale);
        }
        
        v2 PositionAfterCharacter = DrawCharacter(&BatchConstructor, *C, *Font, RegisterPosition, Color, FontScale);
        RegisterPosition.x = PositionAfterCharacter.x;
        C++;
    }
    
    if (CursorPosition == String.Length)
    {
        DrawCursor(&CursorBatch, RegisterPosition, GreenV4, *Font, FontScale);
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
EvaluateButton (render_command_buffer* RenderBuffer, 
                v2 Min, v2 Max, v2 Margin, string Label,  
                v4 IdleBGColor, v4 HotBGColor, v4 IdleTextColor, v4 HotTextColor, 
                bitmap_font* Font, input Input)
{
    button_result Result = {};
    Result.Pressed = false;
    
    v4 BGColor = IdleBGColor;
    v4 TextColor = IdleTextColor;
    
    v2 MousePos = v2{(r32)Input.New->MouseX, (r32)Input.New->MouseY};
    if (PointIsInRange(MousePos, Min, Max))
    {
        if (KeyTransitionedDown(Input, KeyCode_MouseLeftButton))
        {
            Result.Pressed = true;
        }
        else
        {
            BGColor = HotBGColor; 
            TextColor = HotTextColor;
        }
    }
    
    PushRenderQuad2D(RenderBuffer, Min, Max, BGColor);
    DrawString(RenderBuffer, Label, Font, Font->PixelHeight, Min + Margin, TextColor);
    
    Result.Advance = (Max.y - Min.y) + Margin.y;
    return Result;
}

internal button_result
EvaluateButton (render_command_buffer* RenderBuffer, v2 Min, v2 Max, string Label, interface_config Config, input Input)
{
    button_result Result = EvaluateButton(RenderBuffer, 
                                          Min, Max, Config.Margin, Label, 
                                          Config.ButtonColor_Inactive, Config.ButtonColor_Active,
                                          Config.TextColor, Config.TextColor, 
                                          Config.Font, Input);
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
    
    button_result Result = EvaluateButton(RenderBuffer, 
                                          Min, Max, Config.Margin, Label, 
                                          Config.ButtonColor_Inactive, Config.ButtonColor_Active,
                                          Config.TextColor, Config.TextColor, 
                                          Config.Font, Input);
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

struct search_lister_result
{
    s32 HotItem;
    b32 ShouldRemainOpen;
};

typedef char* search_lister_get_list_item_at_offset(u8* ListMemory, s32 ListLength, s32 Offset);

internal search_lister_result
EvaluateSearchLister (render_command_buffer* RenderBuffer, v2 TopLeft, v2 Dimension, string Title, 
                      s32 ListLength, u8* ListMemory, s32 HotItem, search_lister_get_list_item_at_offset* GetListItem, 
                      string* SearchString, s32 SearchStringCursorPosition,
                      bitmap_font* Font, interface_config Config, input Input)
{
    Assert(GetListItem != 0);
    
    search_lister_result Result = {};
    Result.ShouldRemainOpen = true;
    Result.HotItem = HotItem;
    
    // NOTE(Peter): These are direction reversed because going up the list in terms of indicies is
    // visually displayed as going down.
    if (KeyTransitionedDown(Input, KeyCode_DownArrow))
    {
        Result.HotItem = GSMin(Result.HotItem + 1, ListLength - 1);
    }
    if (KeyTransitionedDown(Input, KeyCode_UpArrow))
    {
        Result.HotItem = GSMax(0, Result.HotItem - 1); 
    }
    
    // Title Bar
    PushRenderQuad2D(RenderBuffer, v2{TopLeft.x, TopLeft.y - 30}, v2{TopLeft.x + 300, TopLeft.y}, v4{.3f, .3f, .3f, 1.f});
    DrawString(RenderBuffer, Title, Font, 14, v2{TopLeft.x, TopLeft.y - 25}, WhiteV4);
    TopLeft.y -= 30;
    
    // Search Bar
    PushRenderQuad2D(RenderBuffer, v2{TopLeft.x, TopLeft.y - 30}, v2{TopLeft.x + 300, TopLeft.y}, v4{.3f, .3f, .3f, 1.f});
    DrawStringWithCursor(RenderBuffer, *SearchString, SearchStringCursorPosition, Font, 14, v2{TopLeft.x, TopLeft.y - 25}, WhiteV4, GreenV4);
    TopLeft.y -= 30;
    
    s32 VisibleItemIndex = 0;
    for (s32 i = 0; i < ListLength; i++)
    {
        char* ListItemText = GetListItem(ListMemory, ListLength, i);
        Assert(ListItemText);
        string ListItemString = MakeStringLiteral(ListItemText);
        
        if (SearchString->Length == 0 ||
            StringContainsStringCaseInsensitive(ListItemString, *SearchString))
        {
            v2 Min = v2{TopLeft.x, TopLeft.y - 30};
            v2 Max = Min + Dimension - v2{0, Config.Margin.y};
            
            v4 ButtonColor = Config.ButtonColor_Inactive;
            if (VisibleItemIndex == HotItem)
            {
                ButtonColor = Config.ButtonColor_Active;
            }
            
            button_result Button = EvaluateButton(RenderBuffer, Min, Max, Config.Margin, ListItemString, 
                                                  ButtonColor, ButtonColor, Config.TextColor, Config.TextColor, 
                                                  Config.Font, Input);
            if (Button.Pressed)
            {
                Result.HotItem = i;
            }
            
            TopLeft.y -= 30;
            VisibleItemIndex++;
        }
    }
    
    return Result;
}