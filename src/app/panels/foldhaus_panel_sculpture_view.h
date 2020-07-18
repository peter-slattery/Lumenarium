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
    
    m44 XRotation = M44RotationX(-TotalDeltaPos.y * State->PixelsToWorldScale);
    m44 YRotation = M44RotationY(TotalDeltaPos.x * State->PixelsToWorldScale);
    m44 Combined = XRotation * YRotation;
    
    State->Camera.Position = (Combined * OpState->CameraStartPos).xyz;
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
    OpState->CameraStartPos = ToV4Point(State->Camera.Position);
}

// ----------------

GSMetaTag(panel_commands);
GSMetaTag(panel_type_sculpture_view);
global input_command SculptureView_Commands[] = {
    { KeyCode_MouseLeftButton, KeyCode_Invalid, Command_Began, Begin3DViewMouseRotate },
};
global s32 SculptureView_CommandsCount = 1;

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
    v4 CameraPosition;
    led_buffer LedBuffer;
    s32 StartIndex;
    s32 OnePastLastIndex;
    render_quad_batch_constructor* Batch;
    quad_batch_constructor_reserved_range BatchReservedRange;
    r32 LEDHalfWidth;
};

internal void
DrawLEDsInBufferRangeJob (gs_thread_context Context, gs_data JobData)
{
    DEBUG_TRACK_FUNCTION;
    
    draw_leds_job_data* Data = (draw_leds_job_data*)JobData.Memory;
    
    s32 LEDCount = Data->OnePastLastIndex - Data->StartIndex;
    
#if 0
    // TODO(Peter): Why are we doing this here? Shouldn't we be able to tell what the range
    // needs to be at the time of creation? That way its all on one thread and we're not
    // worried about locking up.
    quad_batch_constructor_reserved_range BatchReservedRange = ThreadSafeReserveRangeInQuadConstructor(Data->Batch, LEDCount * 2);
#endif
    
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
    
    for (s32 LedIndex = Data->StartIndex; LedIndex < Data->OnePastLastIndex; LedIndex++)
    {
        pixel PixelColor = Data->LedBuffer.Colors[LedIndex];
        v4 Color = v4{PixelColor.R / 255.f, PixelColor.G / 255.f, PixelColor.B / 255.f, 1.0f};
        
        v4 Position = Data->LedBuffer.Positions[LedIndex];
        m44 FaceCameraMatrix = M44LookAt(Position, Data->CameraPosition);
        v4 PositionOffset = ToV4Vec(Position.xyz);
        
        v4 P0 = (FaceCameraMatrix * P0_In) + PositionOffset;
        v4 P1 = (FaceCameraMatrix * P1_In) + PositionOffset;
        v4 P2 = (FaceCameraMatrix * P2_In) + PositionOffset;
        v4 P3 = (FaceCameraMatrix * P3_In) + PositionOffset;
        
        SetTri3DInBatch(Data->Batch, Data->BatchReservedRange.Start + TrisUsed++,
                        P0, P1, P2, UV0, UV1, UV2, Color, Color, Color);
        SetTri3DInBatch(Data->Batch, Data->BatchReservedRange.Start + TrisUsed++,
                        P0, P2, P3, UV0, UV2, UV3, Color, Color, Color);
    }
}

internal void
DrawQuad(render_command_buffer* RenderBuffer, v4 C, r32 Rad, v4 Color)
{
    v4 P0 = C + v4{-Rad,-Rad,0,0};
    v4 P1 = C + v4{ Rad,-Rad,0,0};
    v4 P2 = C + v4{ Rad,Rad,0,0};
    v4 P3 = C + v4{ -Rad,Rad,0,0};
    PushRenderQuad3D(RenderBuffer, P0, P1, P2, P3, Color);
}

GSMetaTag(panel_render);
GSMetaTag(panel_type_sculpture_view);
internal void
SculptureView_Render(panel Panel, rect2 PanelBounds, render_command_buffer* RenderBuffer, app_state* State, context Context)
{
    DEBUG_TRACK_SCOPE(RenderSculpture);
    
    // TODO(Peter): @MajorFix
    // NOTE(Peter): Just returning from this function to make sure that this isn't a problem as I go and try to fix
    // the other panels
    return;
    
    
    State->Camera.AspectRatio = RectAspectRatio(PanelBounds);
    
    PushRenderPerspective(RenderBuffer, PanelBounds, State->Camera);
    
    u32 MaxLEDsPerJob = 2048;
    render_quad_batch_constructor RenderLEDsBatch = PushRenderQuad3DBatch(RenderBuffer, State->LedSystem.LedsCountTotal);
    
    u32 FocusPixel = 256;
    
    for (u32 BufferIndex = 0; BufferIndex < State->LedSystem.BuffersCount; BufferIndex++)
    {
        led_buffer* LedBuffer = LedSystemGetBuffer(&State->LedSystem, BufferIndex);
        u32 JobsNeeded = U32DivideRoundUp(LedBuffer->LedCount, MaxLEDsPerJob);
        
#if 1
        u32 NextLEDIndex = 0;
        for (u32 Job = 0; Job < JobsNeeded; Job++)
        {
            gs_data Data = PushSizeToData(&State->Transient, sizeof(draw_leds_job_data));
            draw_leds_job_data* JobData = (draw_leds_job_data*)Data.Memory;
            JobData->LedBuffer = *LedBuffer;
            JobData->StartIndex = NextLEDIndex;
            JobData->OnePastLastIndex = Min(JobData->StartIndex + MaxLEDsPerJob, LedBuffer->LedCount);
            s32 JobLedCount = JobData->OnePastLastIndex - JobData->StartIndex;
            JobData->Batch = &RenderLEDsBatch;
            JobData->BatchReservedRange = ReserveRangeInQuadConstructor(JobData->Batch, JobLedCount * 2);
            JobData->LEDHalfWidth = .5f;
            
            JobData->CameraPosition = ToV4Point(State->Camera.Position);
            
            Context.GeneralWorkQueue->PushWorkOnQueue(Context.GeneralWorkQueue, (thread_proc*)DrawLEDsInBufferRangeJob, Data, ConstString("Sculpture Draw LEDS"));
            
            NextLEDIndex = JobData->OnePastLastIndex;
        }
#else
        gs_data Data = PushSizeToData(&State->Transient, sizeof(draw_leds_job_data));
        draw_leds_job_data* JobData = (draw_leds_job_data*)Data.Memory;
        JobData->LedBuffer = *LedBuffer;
        JobData->StartIndex = 0;
        JobData->OnePastLastIndex = LedBuffer->LedCount;
        s32 JobLedCount = JobData->OnePastLastIndex - JobData->StartIndex;
        JobData->Batch = &RenderLEDsBatch;
        JobData->BatchReservedRange = ReserveRangeInQuadConstructor(JobData->Batch, JobLedCount * 2);
        JobData->LEDHalfWidth = .5f;
        
        JobData->CameraPosition = ToV4Point(State->Camera.Position);
        
        Context.GeneralWorkQueue->PushWorkOnQueue(Context.GeneralWorkQueue, (thread_proc*)DrawLEDsInBufferRangeJob, Data, ConstString("Sculpture Draw LEDS"));
#endif
        
        u32 f = 0;
    }
    
    // TODO(Peter): I don't like the fact that setting an orthographic view inside a panel render function
    // needs to relyon the window bounds rather than the panel bounds. Ideally the panel only needs to know where
    // itself is, and nothing else.
    PushRenderOrthographic(RenderBuffer, State->WindowBounds);
    
    if (State->Assemblies.Count > 0)
    {
        assembly Assembly = State->Assemblies.Values[0];
        led_buffer* LedBuffer = LedSystemGetBuffer(&State->LedSystem, Assembly.LedBufferIndex);
        
        //__debugbreak();
        v4 LedPosition = LedBuffer->Positions[FocusPixel];
        m44 Matrix = GetCameraPerspectiveProjectionMatrix(State->Camera) * GetCameraModelViewMatrix(State->Camera);
        v4 LedProjectedPosition = Matrix * LedPosition;
        v2 LedOnScreenPosition = LedProjectedPosition.xy;
        
        gs_string Tempgs_string = PushString(&State->Transient, 256);
        PrintF(&Tempgs_string, "%f %f", LedOnScreenPosition.x, LedOnScreenPosition.y);
        DrawString(RenderBuffer, Tempgs_string, State->Interface.Style.Font, v2{PanelBounds.Min.x + 100, PanelBounds.Max.y - 200}, WhiteV4);
        
        v2 BoxHalfDim = v2{ 25, 25 };
        v2 BoxMin = LedOnScreenPosition - BoxHalfDim;
        v2 BoxMax = LedOnScreenPosition + BoxHalfDim;
        PushRenderBoundingBox2D(RenderBuffer, BoxMin, BoxMax, 2.0f, TealV4);
    }
    
    Context.GeneralWorkQueue->CompleteQueueWork(Context.GeneralWorkQueue, Context.ThreadContext);
}

#define FOLDHAUS_PANEL_SCULPTURE_VIEW_H
#endif // FOLDHAUS_PANEL_SCULPTURE_VIEW_H