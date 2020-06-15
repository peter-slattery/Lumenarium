//
// File: foldhaus_panel_sculpture_view.h
// Author: Peter Slattery
// Creation Date: 2020-01-01
//
#ifndef FOLDHAUS_PANEL_SCULPTURE_VIEW_H

// 3D Mouse View

OPERATION_STATE_DEF(mouse_rotate_view_operation_state)
{
    v4 CameraStartPos;
};

OPERATION_RENDER_PROC(Update3DViewMouseRotate)
{
    mouse_rotate_view_operation_state* OpState = (mouse_rotate_view_operation_state*)Operation.OpStateMemory;
    
    v2 TotalDeltaPos = Mouse.Pos - Mouse.DownPos;
    
    m44 XRotation = GetXRotation(-TotalDeltaPos.y * State->PixelsToWorldScale);
    m44 YRotation = GetYRotation(TotalDeltaPos.x * State->PixelsToWorldScale);
    m44 Combined = XRotation * YRotation;
    
    State->Camera.Position = V3(Combined * OpState->CameraStartPos);
}

FOLDHAUS_INPUT_COMMAND_PROC(End3DViewMouseRotate)
{
    DeactivateCurrentOperationMode(&State->Modes);
}

input_command MouseRotateViewCommands [] = {
    { KeyCode_MouseLeftButton, KeyCode_Invalid, Command_Ended, End3DViewMouseRotate},
};

FOLDHAUS_INPUT_COMMAND_PROC(Begin3DViewMouseRotate)
{
    operation_mode* RotateViewMode = ActivateOperationModeWithCommands(&State->Modes, MouseRotateViewCommands, Update3DViewMouseRotate);
    mouse_rotate_view_operation_state* OpState = CreateOperationState(RotateViewMode,
                                                                      &State->Modes,
                                                                      mouse_rotate_view_operation_state);
    OpState->CameraStartPos = V4(State->Camera.Position, 1);
}

// ----------------

GSMetaTag(panel_commands);
GSMetaTag(panel_type_sculpture_view);
global_variable input_command SculptureView_Commands[] = {
    { KeyCode_MouseLeftButton, KeyCode_Invalid, Command_Began, Begin3DViewMouseRotate },
};
global_variable s32 SculptureView_CommandsCount = 1;

GSMetaTag(panel_init);
GSMetaTag(panel_type_sculpture_view);
internal void
SculptureView_Init(panel* Panel, app_state* State)
{
    
}

GSMetaTag(panel_cleanup);
GSMetaTag(panel_type_sculpture_view);
internal void
SculptureView_Cleanup(panel* Panel, app_state* State)
{
    
}


struct draw_leds_job_data
{
    led* LEDs;
    pixel* Colors;
    s32 StartIndex;
    s32 OnePastLastIndex;
    
    render_quad_batch_constructor* Batch;
    
    m44 FaceCameraMatrix;
    m44 ModelViewMatrix;
    r32 LEDHalfWidth;
};

internal void
DrawLEDsInBufferRangeJob (s32 ThreadID, void* JobData)
{
    DEBUG_TRACK_FUNCTION;
    
    draw_leds_job_data* Data = (draw_leds_job_data*)JobData;
    
    s32 LEDCount = Data->OnePastLastIndex - Data->StartIndex;
    
    quad_batch_constructor_reserved_range BatchReservedRange = ThreadSafeReserveRangeInQuadConstructor(Data->Batch, LEDCount * 2);
    s32 TrisUsed = 0;
    
    r32 HalfWidth = Data->LEDHalfWidth;
    
    v4 P0_In = v4{-HalfWidth, -HalfWidth, 0, 1};
    v4 P1_In = v4{HalfWidth, -HalfWidth, 0, 1};
    v4 P2_In = v4{HalfWidth, HalfWidth, 0, 1};
    v4 P3_In = v4{-HalfWidth, HalfWidth, 0, 1};
    
    v2 UV0 = v2{0, 0};
    v2 UV1 = v2{1, 0};
    v2 UV2 = v2{1, 1};
    v2 UV3 = v2{0, 1};
    
    led* LED = Data->LEDs + Data->StartIndex;
    for (s32 LEDIdx = 0;
         LEDIdx < LEDCount;
         LEDIdx++)
    {
        pixel PixelColor = Data->Colors[LED->Index];
        v4 Color = v4{PixelColor.R / 255.f, PixelColor.G / 255.f, PixelColor.B / 255.f, 1.0f};
        
        v4 V4Position = LED->Position;
        V4Position.w = 0;
        v4 P0 = P0_In + V4Position;
        v4 P1 = P1_In + V4Position;
        v4 P2 = P2_In + V4Position;
        v4 P3 = P3_In + V4Position;
        
        SetTri3DInBatch(Data->Batch, BatchReservedRange.Start + TrisUsed++,
                        P0, P1, P2, UV0, UV1, UV2, Color, Color, Color);
        SetTri3DInBatch(Data->Batch, BatchReservedRange.Start + TrisUsed++,
                        P0, P2, P3, UV0, UV2, UV3, Color, Color, Color);
        
        LED++;
    }
}

GSMetaTag(panel_render);
GSMetaTag(panel_type_sculpture_view);
internal void
SculptureView_Render(panel Panel, rect PanelBounds, render_command_buffer* RenderBuffer, app_state* State, context Context, mouse_state Mouse)
{
    DEBUG_TRACK_SCOPE(RenderSculpture);
    
    r32 PanelWidth = PanelBounds.Max.x - PanelBounds.Min.x;
    r32 PanelHeight = PanelBounds.Max.y - PanelBounds.Min.y;
    State->Camera.AspectRatio = PanelWidth / PanelHeight;
    
    m44 ModelViewMatrix = GetCameraModelViewMatrix(State->Camera);
    m44 ProjectionMatrix = GetCameraPerspectiveProjectionMatrix(State->Camera);
    
    r32 LEDHalfWidth = .5f;
    
    PushRenderPerspective(RenderBuffer, PanelBounds.Min.x, PanelBounds.Min.y, PanelWidth, PanelHeight, State->Camera);
    
    // TODO(Peter): Pretty sure this isn't working right now
    m44 FaceCameraMatrix = GetLookAtMatrix(v4{0, 0, 0, 1}, V4(State->Camera.Position, 1));
    FaceCameraMatrix = FaceCameraMatrix;
    
    u32 MaxLEDsPerJob = 2048;
    render_quad_batch_constructor RenderLEDsBatch = PushRenderQuad3DBatch(RenderBuffer, State->LedSystem.LedsCountTotal);
    
    for (u32 i = 0; i < State->Assemblies.Count; i++)
    {
        assembly Assembly = State->Assemblies.Values[i];
        led_buffer* LedBuffer = LedSystemGetBuffer(&State->LedSystem, Assembly.LedBufferIndex);
        u32 JobsNeeded = IntegerDivideRoundUp(LedBuffer->LedCount, MaxLEDsPerJob);
        
        for (u32 Job = 0; Job < JobsNeeded; Job++)
        {
            draw_leds_job_data* JobData = PushStruct(&State->Transient, draw_leds_job_data);
            JobData->LEDs = LedBuffer->Leds;
            JobData->Colors = LedBuffer->Colors;
            JobData->StartIndex = Job * MaxLEDsPerJob;
            JobData->OnePastLastIndex = GSMin(JobData->StartIndex + MaxLEDsPerJob, LedBuffer->LedCount);
            JobData->Batch = &RenderLEDsBatch;
            JobData->FaceCameraMatrix;
            JobData->ModelViewMatrix = ModelViewMatrix;
            JobData->LEDHalfWidth = LEDHalfWidth;
            
            Context.GeneralWorkQueue->PushWorkOnQueue(Context.GeneralWorkQueue, DrawLEDsInBufferRangeJob, JobData, "Sculpture Draw LEDS");
        }
    }
    Context.GeneralWorkQueue->DoQueueWorkUntilDone(Context.GeneralWorkQueue, 0);
}

#define FOLDHAUS_PANEL_SCULPTURE_VIEW_H
#endif // FOLDHAUS_PANEL_SCULPTURE_VIEW_H