//
// File: gs_path.h
// Author: Peter Slattery
// Creation Date: 2021-03-06
//
#ifndef GS_PATH_H

internal gs_const_string
ClearString()
{
    gs_const_string R = {};
    R.Str = 0;
    R.Length = 0;
    return R;
}

internal void
SanitizePath (gs_const_string Path, gs_string* Dest, gs_memory_arena* Scratch)
{
    Dest->Length = 0;
    
    // Put all slashes in the same format
    s32 SlashCount = 0;
    for (u64 i = 0; i < Path.Length; i++)
    {
        char At = Path.Str[i];
        if (At == '\\' || At == '/') {
            SlashCount += 1;
        }
    }
    
    // we add one to slash count in case the last element is a file or
    // doesn't end in a slash (even if it should)
    u32 PathEleCountMax = SlashCount + 1;
    u32 PathEleCount = 0;
    gs_const_string* PathEle = PushArray(Scratch, gs_const_string, PathEleCountMax);
    
    u64 OnePastLastEleEnd = 0;
    for (s64 i = 0; i < (s64)Path.Length; i++)
    {
        char At = Path.Str[i];
        if (At == '\\' || At == '/')
        {
            gs_const_string* NewEle = PathEle + PathEleCount++;
            *NewEle = Substring(Path, OnePastLastEleEnd, i + 1);
            OnePastLastEleEnd = i + 1;
        }
    }
    
    if (OnePastLastEleEnd != Path.Length)
    {
        gs_const_string* NewEle = PathEle + PathEleCount++;
        *NewEle = Substring(Path, OnePastLastEleEnd, Path.Length);
        OnePastLastEleEnd = Path.Length;
    }
    
    // Remove redundant elements
    for (u32 i = 0; i < PathEleCount; i++)
    {
        gs_const_string* At = PathEle + i;
        bool ShouldRemove = false;
        if (i != 0)
        {
            if (StringsEqual(*At, ConstString(".\\")) ||
                StringsEqual(*At, ConstString("./")))
            {
                *At = ClearString();
            }
            else if (StringsEqual(*At, ConstString("..\\")) ||
                     StringsEqual(*At, ConstString("../")))
            {
                PathEle[i - 1] = ClearString();
                if (i != 1) {
                    PathEle[i] = ClearString();
                }
            }
        }
    }
    
    for (u32 i = 0; i < PathEleCount; i++)
    {
        if (PathEle[i].Str) {
            AppendPrintF(Dest, "%S", PathEle[i]);
        }
    }
    
    // Put all slashes in the same format
    for (u64 i = 0; i < Dest->Length; i++)
    {
        if (Dest->Str[i] == '/') {
            Dest->Str[i] = '\\';
        }
    }
}

internal gs_const_string
SanitizePath (gs_const_string Path, gs_memory_arena* Scratch)
{
    gs_string Result = PushString(Scratch, Path.Length);
    SanitizePath(Path, &Result, Scratch);
    return Result.ConstString;
}


#define GS_PATH_H
#endif // GS_PATH_H