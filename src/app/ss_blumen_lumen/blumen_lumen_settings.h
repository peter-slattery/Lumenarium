/* date = March 27th 2021 2:50 pm */

#ifndef BLUMEN_LUMEN_SETTINGS_H
#define BLUMEN_LUMEN_SETTINGS_H

gs_const_string Flower0AssemblyPath = ConstString("data/ss_blumen_one.fold");
gs_const_string Flower1AssemblyPath = ConstString("data/ss_blumen_two.fold");
gs_const_string Flower2AssemblyPath = ConstString("data/ss_blumen_three.fold");

gs_const_string PhraseMapCSVPath = ConstString("data/flower_codes.csv");
char PhraseMapCSVSeparator = ',';

gs_const_string AmbientPatternFolder = ConstString("data/blumen_animations/ambient_patterns/*.foldanim");
gs_const_string VoicePatternFolder = ConstString("data/blumen_animations/audio_responses/*.foldanim");

global time_range MotorOpenTimes[] = {
    { 00, 30, 00, 40 },
    { 00, 50, 01, 00 },
    { 01, 10, 01, 20 },
    { 01, 30, 01, 40 },
    { 01, 50, 02, 00 },
    { 02, 10, 02, 20 },
    { 02, 30, 02, 40 },
    { 02, 50, 03, 00 },
    { 03, 10, 03, 20 },
    { 03, 30, 03, 40 },
    { 03, 50, 04, 00 },
    { 04, 10, 04, 20 },
    { 04, 30, 04, 40 },
    { 04, 50, 05, 00 },
    { 05, 10, 05, 20 },
    { 05, 30, 05, 40 },
    { 05, 50, 06, 00 },
    { 06, 10, 06, 20 },
    { 06, 30, 06, 40 },
    { 06, 50, 07, 00 },
    { 07, 10, 07, 20 },
    { 07, 30, 07, 40 },
    { 07, 50,  8, 00 },
    {  8, 10,  8, 20 },
    {  8, 30,  8, 40 },
    {  8, 50,  9, 00 },
    {  9, 10,  9, 20 },
    {  9, 30,  9, 40 },
    {  9, 50, 10, 00 },
    { 10, 10, 10, 20 },
    { 10, 30, 10, 40 },
    { 10, 50, 11, 00 },
    { 11, 10, 11, 20 },
    { 11, 30, 11, 40 },
    { 11, 50, 12, 00 },
    { 12, 10, 12, 20 },
    { 12, 30, 12, 40 },
    { 12, 50, 13, 00 },
    { 13, 10, 13, 20 },
    { 13, 30, 13, 40 },
    { 13, 50, 14, 00 },
    { 14, 10, 14, 20 },
    { 14, 30, 14, 40 },
    { 14, 50, 15, 00 },
};
global u32 MotorOpenTimesCount = CArrayLength(MotorOpenTimes);

r32 VoiceCommandFadeDuration = 1.0f; // in seconds

#endif //BLUMEN_LUMEN_SETTINGS_H
