//
// File: sculpture_gen.h
// Author: Peter Slattery
// Creation Date: 2021-01-06
//
#ifndef SCULPTURE_GEN_H

internal void
WriteIndented(gs_string* Buffer, u32 Indent, char* Format, ...)
{
  va_list Args;
  va_start(Args, Format);
  
  for (u32 i = 0; i < Indent; i++)
  {
    OutChar(Buffer, '\t');
  }
  
  PrintFArgsList(Buffer, Format, Args);
  va_end(Args);
}

internal void
WriteAssemblyCommonOpen(gs_string* Buffer, char* Name, u32 Scale, v3 Center, u32 StripCount)
{
  WriteIndented(Buffer, 0, "assembly_name: \"%s\";\n", Name);
  WriteIndented(Buffer, 0, "assembly_scale: %d;\n", Scale);
  WriteIndented(Buffer, 0, "assembly_center: (%f, %f, %f);\n", Center.x, Center.y, Center.z);
  WriteIndented(Buffer, 0, "led_strip_count: %d;\n", StripCount);
}

internal void
WriteAssemblyUARTOpen(gs_string* Buffer, char* Name, u32 Scale, v3 Center, u32 StripCount, char* ComPort)
{
  WriteAssemblyCommonOpen(Buffer, Name, Scale, Center, StripCount);
  WriteIndented(Buffer, 0, "output_mode: \"UART\";\n");
  
  if (ComPort)
  {
    WriteIndented(Buffer, 0, "com_port: \"%s\";\n", ComPort);
  }
}

internal void
WriteAssemblySACNOpen(gs_string* Buffer, char* Name, u32 Scale, v3 Center, u32 StripCount)
{
  WriteAssemblyCommonOpen(Buffer, Name, Scale, Center, StripCount);
  WriteIndented(Buffer, 0, "output_mode: \"SACN\";\n");
}

internal void
WriteLedStripOpen(gs_string* Buffer, u32 UARTChannel, char* UARTComPort, u32 SACNStartUniverse, u32 SACNStartChannel)
{
  WriteIndented(Buffer, 0, "led_strip:\n{\n");
  
  // SACN
  WriteIndented(Buffer, 1, "output_sacn: {\n");
  WriteIndented(Buffer, 2, "start_universe: %d;\n", SACNStartUniverse);
  WriteIndented(Buffer, 2, "start_channel: %d;\n", SACNStartChannel);
  WriteIndented(Buffer, 1, "};\n\n");
  
  // UART
  WriteIndented(Buffer, 1, "output_uart: {\n");
  WriteIndented(Buffer, 2, "channel: %d;\n", UARTChannel);
  WriteIndented(Buffer, 2, "com_port: \"%s\";\n", UARTComPort);
  WriteIndented(Buffer, 1, "};\n\n");
  
}

internal void
WriteSegmentSequenceOpen(gs_string* Buffer, u32 SegmentCount)
{
  WriteIndented(Buffer, 1, "segment: {\n");
  WriteIndented(Buffer, 2, "point_placement_type: \"SegmentSequence\";\n");
  WriteIndented(Buffer, 2, "segment_sequence:\n");
  WriteIndented(Buffer, 2, "{\n");
  WriteIndented(Buffer, 3, "segment_count: %d;\n", SegmentCount);
}

internal void
WriteSegmentSequenceSegment(gs_string* Buffer, v3 P0, v3 P1, u32 LedCount)
{
  WriteIndented(Buffer, 3, "segment: {\n");
  WriteIndented(Buffer, 4, "point_placement_type: \"InterpolatePoints\";\n");
  WriteIndented(Buffer, 4, "interpolate_points: {\n");
  WriteIndented(Buffer, 5, "start: (%f, %f, %f);\n", P0.x, P0.y, P0.z);
  WriteIndented(Buffer, 5, "end: (%f, %f, %f);\n", P1.x, P1.y, P1.z);
  WriteIndented(Buffer, 5, "led_count: %d;\n", LedCount);
  WriteIndented(Buffer, 4, "};\n");
  WriteIndented(Buffer, 3, "};\n");
}

internal void
WriteSegmentSequenceClose(gs_string* Buffer)
{
  WriteIndented(Buffer, 2, "};\n");
  WriteIndented(Buffer, 1, "};\n");
}

internal void
WriteSegmentTagsOpen(gs_string* Buffer, u32 TagCount)
{
  WriteIndented(Buffer, 1, "tags_count: %d;\n", TagCount);
}

internal void
WriteSegmentTag(gs_string* Buffer, char* TagName, char* TagValue)
{
  WriteIndented(Buffer, 1, "tag: {\n");
  WriteIndented(Buffer, 2, "name: \"%s\";\n", TagName);
  WriteIndented(Buffer, 2, "value: \"%s\";\n", TagValue);
  WriteIndented(Buffer, 1, "};\n");
  
}

internal void
WriteSegmentTagsClose(gs_string* Buffer)
{
}

internal void
WriteLedStripClose(gs_string* Buffer)
{
  WriteIndented(Buffer, 0, "};\n");
}

#define SCULPTURE_GEN_H
#endif // SCULPTURE_GEN_H