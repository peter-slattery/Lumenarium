//
// File: interface_test.cpp
// Author: Peter Slattery
// Creation Date: 2020-11-15
//
#ifndef INTERFACE_TEST_CPP

global r32 TestSlider_Value = 5;
global r32 TestSlider_Min = 0;
global r32 TestSlider_Max = 10;
global bool TestToggle = true;
global r64 TestTextEntry = 3.1415f;

internal void
InterfaceTest_Render(app_state* State, context* Context, render_command_buffer* RenderBuffer)
{
    ui_InterfaceReset(&State->Interface);
    State->Interface.RenderBuffer = RenderBuffer;
    State->Interface.WindowBounds = Context->WindowBounds;
    
    gs_string A = MakeString("TestRender Layout");
    
    ui_PushLayout(&State->Interface, A);
    {
#if 1
        TestTextEntry = ui_TextEntryR64(&State->Interface, MakeString("Spacer"), TestTextEntry);
        ui_Button(&State->Interface, MakeString("A"));
        TestSlider_Value = ui_RangeSlider(&State->Interface, MakeString("TestSlider"), TestSlider_Value, TestSlider_Min, TestSlider_Max);
#elif 0
        ui_PushLayout(&State->Interface, MakeString("Outer"));
        {
            for (u32 i = 0; i < 3; i++)
            {
                ui_Button(&State->Interface, MakeString("A"));
            }
        }
        ui_PopLayout(&State->Interface);
        
        ui_BeginRow(&State->Interface, 2);
        {
            ui_PushLayout(&State->Interface, MakeString("TestLayout"));
            {
                for (u32 i = 0; i < 5; i++)
                {
                    ui_Button(&State->Interface, MakeString("TestButon"));
                }
            }
            ui_PopLayout(&State->Interface);
            
            ui_PushLayout(&State->Interface, MakeString("TestLayout"));
            {
                ui_Button(&State->Interface, MakeString("TestButon"));
                TestToggle = ui_Toggle(&State->Interface, MakeString("Toggle"), TestToggle);
                TestSlider_Value = ui_RangeSlider(&State->Interface, MakeString("TestSlider"), TestSlider_Value, TestSlider_Min, TestSlider_Max);
                if (ui_BeginDropdown(&State->Interface, MakeString("TestDropdown")))
                {
                    ui_Button(&State->Interface, MakeString("TestButon"));
                    ui_Button(&State->Interface, MakeString("TestButon"));
                    ui_Button(&State->Interface, MakeString("TestButon"));
                }
                ui_EndDropdown(&State->Interface);
            }
            ui_PopLayout(&State->Interface);
        }
        ui_EndRow(&State->Interface);
        
        ui_PushLayout(&State->Interface, MakeString("Outer"));
        {
            for (u32 i = 0; i < 3; i++)
            {
                ui_Button(&State->Interface, MakeString("B"));
            }
        }
        ui_PopLayout(&State->Interface);
        
#else
        ui_BeginList(&State->Interface, MakeString("Test List"), 10);
        {
            for (u32 i = 0; i < 32; i++)
            {
                ui_Button(&State->Interface, MakeString("Option"));
            }
        }
        ui_EndList(&State->Interface);
#endif
        
        ui_PushOverlayLayout(&State->Interface, rect2{25, 25, 400, 200}, LayoutDirection_TopDown, MakeString("t"));
        {
            ui_Label(&State->Interface, PushStringF(State->Interface.PerFrameMemory, 256, "Mouse Pos - %f %f", State->Interface.Mouse.Pos.x, State->Interface.Mouse.Pos.y));
            ui_Label(&State->Interface, PushStringF(State->Interface.PerFrameMemory, 256, "Hot - %lld | Active - %lld",
                                                    State->Interface.HotWidget.Id, State->Interface.ActiveWidget.Id));
            ui_Label(&State->Interface, PushStringF(State->Interface.PerFrameMemory, 256, "Last Active - %lld",
                                                    State->Interface.LastActiveWidget.Id));
        }
        ui_PopLayout(&State->Interface);
    }
    ui_PopLayout(&State->Interface);
}


#define INTERFACE_TEST_CPP
#endif // INTERFACE_TEST_CPP