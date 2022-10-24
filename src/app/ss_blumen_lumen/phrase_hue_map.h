/* date = March 27th 2021 1:55 pm */

#ifndef PHRASE_HUE_MAP_H
#define PHRASE_HUE_MAP_H

enum p_hue_flag
{
    Hue_Value = 0,
    Hue_White = 1,
    Hue_Black = 2,
};

enum p_hue_pattern
{
    HuePattern_Patchy,
    HuePattern_Wavy,
    
    HuePattern_Count,
};

enum p_hue_add_in
{
    AddIn_None,
    AddIn_Waves,
    AddIn_Rotary,
    
    AddIn_Count,
};

typedef struct p_hue
{
    v4 HSV;
} p_hue;

typedef struct phrase_hue_map
{
    u64 CountMax;
    u64 Count;
    gs_const_string* Phrases;
    u64* PhraseHashes;
    p_hue* Hue0;
    p_hue* Hue1;
    p_hue* Hue2;
    u32*   Gran; // granularity
    r32*   Speed;
    u8*    Pattern;
    u8*    AddIn;
    bool*  OverrideAll;
} phrase_hue_map;

typedef struct phrase_hue
{
    gs_const_string Phrase;
    u64 PhraseHash;
    p_hue Hue0;
    p_hue Hue1;
    p_hue Hue2;
    u32   Granularity;
    r32   Speed;
    u8    Pattern;
    u8    AddIn;
    bool  OverrideAll;
} phrase_hue;

internal p_hue
LerpPHue(r32 T, p_hue A, p_hue B)
{
    p_hue Result = {};
    
    if (Abs(A.HSV.x - B.HSV.x) < 180.0f)
    {
        Result.HSV.x = LerpR64(T, A.HSV.x, B.HSV.x);
    } 
    else if (B.HSV.x > A.HSV.x)
    {
        Result.HSV.x = LerpR64(T, A.HSV.x, B.HSV.x - 360.0f);
    }
    else
    {
        Result.HSV.x = LerpR64(T, A.HSV.x - 360.0f, B.HSV.x);
    }
    if (Result.HSV.x < 360) Result.HSV.x += 360;
    if (Result.HSV.x > 360) Result.HSV.x -= 360;
    Result.HSV.x = Clamp(0, Result.HSV.x, 360);
    Result.HSV.y = LerpR32(T, A.HSV.y, B.HSV.y);
    Result.HSV.z = LerpR32(T, A.HSV.z, B.HSV.z);
    Result.HSV.w = LerpR32(T, A.HSV.w, B.HSV.w);
    
    return Result;
}

internal phrase_hue
LerpPhraseHue(r32 T, phrase_hue A, phrase_hue B)
{
    phrase_hue Result = {};
    Result.Hue0 = LerpPHue(T, A.Hue0, B.Hue0);
    Result.Hue1 = LerpPHue(T, A.Hue1, B.Hue1);
    Result.Hue2 = LerpPHue(T, A.Hue2, B.Hue2);
    Result.Granularity = (u32)LerpR32(T, (r32)A.Granularity, (r32)B.Granularity);
    Result.Speed = LerpR32(T, A.Speed, B.Speed);
    
    if (T < .5f) { 
        Result.Phrase = A.Phrase;
        Result.PhraseHash = A.PhraseHash;
        Result.Pattern = A.Pattern; 
        Result.AddIn = A.AddIn;
    }
    else { 
        Result.Phrase = B.Phrase;
        Result.PhraseHash = B.PhraseHash;
        Result.Pattern = B.Pattern; 
        Result.AddIn = B.AddIn;
    }
    
    return Result;
}

internal p_hue
CreateHueFromString(gs_const_string Str)
{
    p_hue Result = {};
    if (Str.Str[0] == 'b') {
        Result.HSV = v4{0, 0, 0, 1 };
    } else if (Str.Str[0] == 'w') {
        Result.HSV = v4{0, 0, 1, 1 };;
    } else {
        parse_float_result Parsed = ValidateAndParseFloat(Str);
        if (!Parsed.Success)
        {
            Log_Error(GlobalLogBuffer, "Failed to Parse CSV Float\n");
            Parsed.Value = 0.0;
        }
        Result.HSV = v4{ (r32)Parsed.Value, 1, 1, 1 };
        
    }
    return Result;
}

internal v4
RGBFromPhraseHue (p_hue H)
{
    v4 Result = H.HSV;
    Result = HSVToRGB(Result);
    return Result;
}

internal phrase_hue_map
PhraseHueMap_GenFromCSV(gscsv_sheet Sheet, gs_memory_arena* Arena)
{
    phrase_hue_map Result = {};
    if (Sheet.RowCount == 0) return Result;
    
    Result.CountMax = Sheet.RowCount - 1; // we don't include the header row
    Result.Phrases = PushArray(Arena, gs_const_string, Result.CountMax);
    Result.PhraseHashes = PushArray(Arena, u64, Result.CountMax);
    Result.Hue0 =  PushArray(Arena, p_hue, Result.CountMax);
    Result.Hue1 =  PushArray(Arena, p_hue, Result.CountMax);
    Result.Hue2 =  PushArray(Arena, p_hue, Result.CountMax);
    Result.Gran =  PushArray(Arena, u32,   Result.CountMax);
    Result.Pattern = PushArray(Arena, u8,   Result.CountMax);
    Result.Speed = PushArray(Arena, r32,   Result.CountMax);
    Result.AddIn = PushArray(Arena, u8,    Result.CountMax);
    Result.OverrideAll = PushArray(Arena, bool, Result.CountMax);
    
    // this lets us tightly pack phrase_hues even if there is a
    // row in the csv that is empty or invalid
    s32 DestOffset = 0;
    for (u32 Row = 1; Row < Sheet.RowCount; Row++)
    {
        s32 Index = (Row - 1) - DestOffset;
        Assert(Index >= 0 && Index < Result.CountMax);
        
        gs_const_string Phrase = CSVSheet_GetCell(Sheet,
                                                  0, Row);
        gs_const_string Hue0Str     = CSVSheet_GetCell(Sheet, 1, Row);
        gs_const_string Hue1Str     = CSVSheet_GetCell(Sheet, 2, Row);
        gs_const_string Hue2Str     = CSVSheet_GetCell(Sheet, 3, Row);
        gs_const_string Homonyms    = CSVSheet_GetCell(Sheet, 4, Row);
        gs_const_string Granularity = CSVSheet_GetCell(Sheet, 5, Row);
        gs_const_string Speed       = CSVSheet_GetCell(Sheet, 6, Row);
        gs_const_string Pattern     = CSVSheet_GetCell(Sheet, 7, Row);
        gs_const_string AddIn       = CSVSheet_GetCell(Sheet, 8, Row);
        gs_const_string OverrideAll = CSVSheet_GetCell(Sheet, 9, Row);
        
        // essential parameters
        if (Phrase.Length == 0 ||
            Hue0Str.Length == 0 ||
            Hue1Str.Length == 0 ||
            Hue2Str.Length == 0)
        {
            DestOffset++;
            continue;
        }
        
        Result.Phrases[Index] = PushStringF(Arena, Phrase.Length, "%S", Phrase).ConstString;
        for (u64 i = 0; i < Result.Phrases[Index].Length; i++)
        {
            char C = Result.Phrases[Index].Str[i];
            if (IsAlpha(C))
            {
                Result.Phrases[Index].Str[i] = ToLower(C);
            }
        }
        
        u64 PhraseHash = HashDJB2ToU32(StringExpand(Result.Phrases[Index]));
        
        Result.PhraseHashes[Index] = PhraseHash;
        Result.Hue0[Index] = CreateHueFromString(Hue0Str);
        Result.Hue1[Index] = CreateHueFromString(Hue1Str);
        Result.Hue2[Index] = CreateHueFromString(Hue2Str);
        
        parse_float_result ParsedSpeed = ValidateAndParseFloat(Speed);
        if (!ParsedSpeed.Success)
        {
            ParsedSpeed.Value = 1.0;
        }
        Result.Speed[Index] = ParsedSpeed.Value;
        
        if (StringsEqual(Pattern, ConstString("wavy")))
        {
            Result.Pattern[Index] = HuePattern_Wavy;
        } else {
            Result.Pattern[Index] = HuePattern_Patchy;
        }
        
        if (StringsEqual(AddIn, ConstString("waves")))
        {
            Result.AddIn[Index] = AddIn_Waves;
        } else if (StringsEqual(AddIn, ConstString("rotary"))) {
            Result.AddIn[Index] = AddIn_Rotary;
        } else {
            Result.AddIn[Index] = AddIn_None;
        }
        
        parse_uint_result ParsedGranularity = ValidateAndParseUInt(Granularity);
        if (!ParsedGranularity.Success)
        {
            ParsedGranularity.Value = 1;
        }
        Result.Gran[Index] = ParsedGranularity.Value;
        
        Result.OverrideAll[Index] = StringsEqualUpToLength(OverrideAll, ConstString("yes"), 3);
    }
    
    Result.Count = Result.CountMax + DestOffset;
    
    return Result;
}

internal phrase_hue
PhraseHueMap_Get(phrase_hue_map Map, u32 Index)
{
    Assert(Index < Map.Count);
    phrase_hue Result = {};
    Result.Phrase = Map.Phrases[Index];
    Result.PhraseHash = Map.PhraseHashes[Index];
    Result.Hue0 = Map.Hue0[Index];
    Result.Hue1 = Map.Hue1[Index];
    Result.Hue2 = Map.Hue2[Index];
    Result.Granularity = Map.Gran[Index];
    Result.Speed = Map.Speed[Index];
    Result.AddIn = Map.AddIn[Index];
    Result.Pattern = Map.Pattern[Index];
    Result.OverrideAll = Map.OverrideAll[Index];
    return Result;
}

internal phrase_hue
PhraseHueMap_Find(phrase_hue_map Map, u64 PhraseHash)
{
    phrase_hue Result = {};
    for (u32 i = 0; i < Map.Count; i++)
    {
        if (Map.PhraseHashes[i] == PhraseHash)
        {
            Result = PhraseHueMap_Get(Map, i);
            break;
        }
    }
    return Result;
}

#endif //PHRASE_HUE_MAP_H
