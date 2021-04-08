/* date = March 27th 2021 1:55 pm */

#ifndef PHRASE_HUE_MAP_H
#define PHRASE_HUE_MAP_H

enum p_hue_flag
{
    Hue_Value = 0,
    Hue_White = 1,
    Hue_Black = 2,
};

typedef struct p_hue
{
    r64 Hue;
    p_hue_flag Flags;
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
} phrase_hue_map;

typedef struct phrase_hue
{
    gs_const_string Phrase;
    u64 PhraseHash;
    p_hue Hue0;
    p_hue Hue1;
    p_hue Hue2;
} phrase_hue;

internal p_hue
CreateHueFromString(gs_const_string Str)
{
    p_hue Result = {};
    if (Str.Str[0] == 'b') {
        Result.Flags = Hue_Black;
    } else if (Str.Str[0] == 'w') {
        Result.Flags = Hue_White;
    } else {
        Result.Flags = Hue_Value;
        Result.Hue = (r64)ParseFloat(Str);
    }
    return Result;
}

internal v4
RGBFromPhraseHue (p_hue H)
{
    v4 Result = {};
    switch (H.Flags)
    {
        case Hue_Black: { Result = v4{1, 0, 0, 1}; } break;
        case Hue_White: { Result = v4{1, 0, 1, 1}; } break;
        case Hue_Value: { Result = v4{(r32)H.Hue, 1, 1, 1}; } break;
        InvalidDefaultCase;
    }
    Result = HSVToRGB(Result);
    return Result;
}

internal phrase_hue_map
PhraseHueMap_GenFromCSV(gscsv_sheet Sheet, gs_memory_arena* Arena)
{
    phrase_hue_map Result = {};
    Result.CountMax = Sheet.RowCount - 1; // we don't include the header row
    Result.Phrases = PushArray(Arena, gs_const_string, Result.CountMax);
    Result.PhraseHashes = PushArray(Arena, u64, Result.CountMax);
    Result.Hue0 = PushArray(Arena, p_hue, Result.CountMax);
    Result.Hue1 = PushArray(Arena, p_hue, Result.CountMax);
    Result.Hue2 = PushArray(Arena, p_hue, Result.CountMax);
    
    s32 DestOffset = 0;
    for (u32 Row = 1; Row < Sheet.RowCount; Row++)
    {
        s32 Index = (Row - 1) - DestOffset;
        Assert(Index >= 0 && Index < Result.CountMax);
        
        gs_const_string Phrase = CSVSheet_GetCell(Sheet,
                                                  0, Row);
        u64 PhraseHash = HashDJB2ToU32(StringExpand(Phrase));
        
        gs_const_string Hue0Str  = CSVSheet_GetCell(Sheet, 1, Row);
        gs_const_string Hue1Str  = CSVSheet_GetCell(Sheet, 2, Row);
        gs_const_string Hue2Str  = CSVSheet_GetCell(Sheet, 3, Row);
        gs_const_string Homonyms = CSVSheet_GetCell(Sheet, 4, Row);
        if (Phrase.Length == 0 ||
            Hue0Str.Length == 0 ||
            Hue1Str.Length == 0 ||
            Hue2Str.Length == 0)
        {
            DestOffset++;
            continue;
        }
        
        Result.Phrases[Index] = PushStringF(Arena, Phrase.Length, "%S", Phrase).ConstString;
        Result.PhraseHashes[Index] = PhraseHash;
        Result.Hue0[Index] = CreateHueFromString(Hue0Str);
        Result.Hue1[Index] = CreateHueFromString(Hue1Str);
        Result.Hue2[Index] = CreateHueFromString(Hue2Str);
    }
    
    Result.Count = Result.CountMax + DestOffset;
    
    return Result;
}

internal phrase_hue
PhraseHueMap_Get(phrase_hue_map Map, u64 PhraseHash)
{
    phrase_hue Result = {};
    
    for (u32 i = 0; i < Map.Count; i++)
    {
        if (Map.PhraseHashes[i] == PhraseHash)
        {
            Result.Phrase = Map.Phrases[i];
            Result.PhraseHash = Map.PhraseHashes[i];
            Result.Hue0 = Map.Hue0[i];
            Result.Hue1 = Map.Hue1[i];
            Result.Hue2 = Map.Hue2[i];
            
            break;
        }
    }
    
    return Result;
}

#endif //PHRASE_HUE_MAP_H
