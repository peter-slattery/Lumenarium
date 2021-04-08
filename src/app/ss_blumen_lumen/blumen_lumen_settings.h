/* date = March 27th 2021 2:50 pm */

#ifndef BLUMEN_LUMEN_SETTINGS_H
#define BLUMEN_LUMEN_SETTINGS_H

// Hey you never know, might need to change this some day lololol
// The number of flowers in the sculpture. Used to size all sorts of
// arrays. Maybe don't touch this unless you really know what you're doing?
#define BL_FLOWER_COUNT 3

// The path to the three flower assembly files
// PS is 90% sure you don't need to touch these ever
gs_const_string Flower0AssemblyPath = ConstString("data/ss_blumen_one.fold");
gs_const_string Flower1AssemblyPath = ConstString("data/ss_blumen_two.fold");
gs_const_string Flower2AssemblyPath = ConstString("data/ss_blumen_three.fold");

// The path to the phrase map CSV. Can be an absolute path, or relative
// to the app_run_tree folder
gs_const_string PhraseMapCSVPath = ConstString("data/flower_codes.csv");
char PhraseMapCSVSeparator = ',';

// Search Strings for which folders to find ambient animation files and 
// voice animation files in. 
// these search patterns should always end in *.foldanim so they only
// return valid animation files
gs_const_string AmbientPatternFolder = ConstString("data/blumen_animations/ambient_patterns/*.foldanim");
gs_const_string VoicePatternFolder = ConstString("data/blumen_animations/audio_responses/*.foldanim");

// The times of day when the motors should be open.
// these are in the format { Start_Hour, Start_Minute, End_Hour, End_Minute }
// Hours are in the range 0-23 inclusive
// Minutes are in the range 0-59 inclusive
// NOTE: There is no need to modify the MotorOpenTimesCount variable - 
// it is a compile time constant that gets calculated automatically
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
global u32 MotorOpenTimesCount = CArrayLength(MotorOpenTimes); // do not edit

// How long it takes to fade from the default pattern to the 
// voice activated pattern
r32 VoiceCommandFadeDuration = 1.0f; // in seconds

// How long the voice activated pattern will remain active
// without additional voice commands, before fading back to
// default behaviour.
// ie.
//    if this is set to 30 seconds, upon receiving a voice command
//    lumenarium will fade to the requested pattern/color palette
//    and then wait 30 seconds before fading back to the original 
//    pattern. If, in that 30 second window, another voice command
//    is issued, lumenarium will reset the 30 second counter.
r64 VoiceCommandSustainDuration = 30.0; // in seconds

// When we send a Motor Close command, we don't want the upper leds to
// immediately turn off. Instead, we want to wait until the flower is
// at least some of the way closed. This variable dictates how long 
// we wait for.
// For example:
//     1. We send a 'motor close' command to the clear core
//     2. the clear core sends back a 'motor closed' state packet
//     3. We begin a timer
//     4. When the timer reaches the value set in this variable,
//        we turn the upper leds off.
// 
// NOTE: This is not a symmetric operation. When we send a 'motor open'
// command, we want to immediately turn the upper leds on so they appear
// to have been on the whole time.
r64 TurnUpperLedsOffAfterMotorCloseCommandDelay = 5.0; // in seconds


// NOTE: Temperature & Time of Day Based Led Brightness Settings

// The temperature above which we dim the leds to
// HighTemperatureBrightnessPercent (below)
//
// NOTE: this is an 8bit signed integer so its range is
// -128 to 127, not that we should need either of those extremes for 
// this, but just a note that you can't just put anything in here.
s8 MinHighTemperature = 0;

// The percent brightness we set leds to during high temperatures.
// A value in the range 0:1 inclusive
// This is multiplied by each pixels R, G, & B channels before being
// sent. So if it is set to .1f, then the maximum brightness value sent
// to any channel of any pixel will be 25 (255 * .1 = 25).
r32 HighTemperatureBrightnessPercent = .25f;

// The percent brightness we set leds to when no other conditions apply
// A value in the range 0:1 inclusive.
// Probably wants to be something high like 1 but we might want to 
// lower it for heat reasons?
r32 FullBrightnessPercent = 1.0f;

// A global modifier so Joerg can just slow all the patterns right down
// XD
// This is a percent - so 1 is full speed, 0.1f is 1/10 full speed and 
// 2 is 2x as fast. 
r32 GlobalAnimSpeed = 1.0f;

r64 PhrasePriorityMessageGroupingTime = 1.0f;

// How often should Lumenarium send its status to the python server?
// 
#define STATUS_PACKET_FREQ_SECONDS 10 // in seconds

#endif //BLUMEN_LUMEN_SETTINGS_H
