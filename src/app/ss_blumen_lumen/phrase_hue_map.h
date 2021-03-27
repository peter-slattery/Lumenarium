/* date = March 27th 2021 1:55 pm */

#ifndef PHRASE_HUE_MAP_H
#define PHRASE_HUE_MAP_H

typedef struct phrase_hue_map
{
    u64 Count;
    gs_const_string* Phrases;
    u64* PhraseHashes;
    r32* Hue0;
    r32* Hue1;
    r32* Hue2;
} phrase_hue_map;

typedef struct phrase_hue
{
    gs_const_string Phrase;
    u64 PhraseHash;
    r32 Hue0;
    r32 Hue1;
    r32 Hue2;
} phrase_hue;

internal phrase_hue_map
PhraseHueMap_GenFromCSV(gscsv_sheet Sheet, gs_memory_arena* Arena)
{
    phrase_hue_map Result = {};
    Result.Count = Sheet.RowCount - 1; // we don't include the header row
    Result.Phrases = PushArray(Arena, gs_const_string, Result.Count);
    Result.PhraseHashes = PushArray(Arena, u64, Result.Count);
    Result.Hue0 = PushArray(Arena, r32, Result.Count);
    Result.Hue1 = PushArray(Arena, r32, Result.Count);
    Result.Hue2 = PushArray(Arena, r32, Result.Count);
    
    for (u32 Row = 1; Row < Sheet.RowCount; Row++)
    {
        u32 Index = Row - 1;
        gs_const_string Phrase = CSVSheet_GetCell(Sheet,
                                                  0, Row);
        u64 PhraseHash = HashDJB2ToU32(StringExpand(Phrase));
        gs_const_string Hue0Str = CSVSheet_GetCell(Sheet,
                                                   1, Row);
        gs_const_string Hue1Str = CSVSheet_GetCell(Sheet,
                                                   2, Row);
        gs_const_string Hue2Str = CSVSheet_GetCell(Sheet,
                                                   3, Row);
        gs_const_string Homonyms = CSVSheet_GetCell(Sheet,
                                                    4, Row);
        
        Result.Phrases[Index] = PushStringF(Arena, Phrase.Length, "%S", Phrase).ConstString;
        Result.PhraseHashes[Index] = PhraseHash;
        Result.Hue0[Index] = (r64)ParseFloat(Hue0Str);
        Result.Hue1[Index] = (r64)ParseFloat(Hue1Str);
        Result.Hue2[Index] = (r64)ParseFloat(Hue2Str);
    }
    
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
