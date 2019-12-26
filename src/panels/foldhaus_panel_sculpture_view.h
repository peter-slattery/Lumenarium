input_command SculptureView_Commands[] = {
    { KeyCode_MouseLeftButton, KeyCode_Invalid, Command_Began, Begin3DViewMouseRotate },
};

PANEL_INIT_PROC(SculptureView_Init)
{
    
}

PANEL_CLEANUP_PROC(SculptureView_Cleanup)
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

PANEL_RENDER_PROC(SculptureView_Render)
{
    DEBUG_TRACK_SCOPE(RenderSculpture);
    
r32 PanelWidth = PanelMax.x - PanelMin.x;
    r32 PanelHeight = PanelMax.y - PanelMin.y;
    State->Camera.AspectRatio = PanelWidth / PanelHeight;
    
    m44 ModelViewMatrix = GetCameraModelViewMatrix(State->Camera);
    m44 ProjectionMatrix = GetCameraPerspectiveProjectionMatrix(State->Camera);
    
    r32 LEDHalfWidth = .5f;
    
    PushRenderPerspective(RenderBuffer, PanelMin.x, PanelMin.y, PanelWidth, PanelHeight, State->Camera);
    
    // TODO(Peter): Pretty sure this isn't working right now
    m44 FaceCameraMatrix = GetLookAtMatrix(v4{0, 0, 0, 1}, V4(State->Camera.Position, 1));
    FaceCameraMatrix = FaceCameraMatrix;
    
    s32 MaxLEDsPerJob = 2048;
    render_quad_batch_constructor RenderLEDsBatch = PushRenderQuad3DBatch(RenderBuffer, State->TotalLEDsCount);
    
    for (s32 i = 0; i < State->ActiveAssemblyIndecies.Used; i++)
    {
        array_entry_handle AssemblyHandle = *GetElementAtIndex(i, State->ActiveAssemblyIndecies);
        assembly Assembly = *GetElementWithHandle(AssemblyHandle, State->AssemblyList);
        s32 JobsNeeded = IntegerDivideRoundUp(Assembly.LEDCount, MaxLEDsPerJob);
        
        for (s32 Job = 0; Job < JobsNeeded; Job++)
        {
            draw_leds_job_data* JobData = PushStruct(&State->Transient, draw_leds_job_data);
            JobData->LEDs = Assembly.LEDs;
            JobData->Colors = Assembly.Colors;
            JobData->StartIndex = Job * MaxLEDsPerJob;
            JobData->OnePastLastIndex = GSMin(JobData->StartIndex + MaxLEDsPerJob, Assembly.LEDCount);
            JobData->Batch = &RenderLEDsBatch;
            JobData->FaceCameraMatrix;
            JobData->ModelViewMatrix = ModelViewMatrix;
            JobData->LEDHalfWidth = LEDHalfWidth;
            
            Context.GeneralWorkQueue->PushWorkOnQueue(
                                                      Context.GeneralWorkQueue,
                                                      DrawLEDsInBufferRangeJob,
                                                      JobData);
        }
    }
    
    Context.GeneralWorkQueue->DoQueueWorkUntilDone(Context.GeneralWorkQueue, 0);
    Context.GeneralWorkQueue->ResetWorkQueue(Context.GeneralWorkQueue);
}
