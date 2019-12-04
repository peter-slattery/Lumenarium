// NOTE(Peter): stuff this in a function and itll print out the code needed to generate a blumen
// TODO(Peter): Modify this when you get actual blumen measurements
MakeStringBuffer(Buffer, 256);
v3 InnerVectors[12];
v3 OuterVectors[12];
for (s32 i = 0; i < 12; i++)
{
    r32 Theta = ((r32)i / 12.0f) * 2 * PI;
    
    v3 Direction = v3{GSSin(Theta), GSCos(Theta), 0};
    
    InnerVectors[i] = Direction;
    OuterVectors[i] = v3{Direction.x * 0.05f, Direction.y * 0.05f, 1};
    
    PrintF(&Buffer, "led_strip { 0, %d, 0, INTERPOLATE_POINTS, (%f, %f, %f), (%f, %f, %f), 70 }\n",
           i,
           InnerVectors[i].x, InnerVectors[i].y, InnerVectors[i].z,
           OuterVectors[i].x, OuterVectors[i].y, OuterVectors[i].z);
    NullTerminate(&Buffer);
    OutputDebugStringA(Buffer.Memory);
}
