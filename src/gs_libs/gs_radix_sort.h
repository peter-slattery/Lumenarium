/*
gs_radix_sort.h - An implementation of radix sort for fixed size unsigned 32bit integer buffers

TODO
*/

#ifndef GS_RADIX_SORT_H

#ifdef DEBUG
#if !defined(GSRad_Assert)
#define GSRad_Assert(expression) \
if(!(expression)) { \
    *((int *)0) = 5; \
}
#endif // !defined(GSRad_Assert)
#endif // DEBUG

typedef unsigned long long int gs_rad_u64;
typedef unsigned int gs_rad_b32;

struct gs_radix_entry
{
    gs_rad_u64 Radix;
    gs_rad_u64 ID;
};

static void
RadixSortInPlace_ (gs_radix_entry* Data, gs_rad_u64 Start, gs_rad_u64 End, gs_rad_u64 Iteration)
{
    gs_rad_u64 Shift = Iteration;
    gs_rad_u64 ZerosBoundary = Start;
    gs_rad_u64 OnesBoundary = End - 1;
    
    for (gs_rad_u64 d = Start; d < End; d++)
    {
        gs_radix_entry Entry = Data[ZerosBoundary];
        gs_rad_u64 Place = (Entry.Radix >> Shift) & 0x1;
        if (Place)
        {
            gs_radix_entry Evicted = Data[OnesBoundary];
            Data[OnesBoundary] = Entry;
            Data[ZerosBoundary] = Evicted;
            OnesBoundary -= 1;
        }
        else
        {
            ZerosBoundary += 1;
        }
    }
    
    if (Iteration > 0)
    {
        RadixSortInPlace_(Data, Start, ZerosBoundary, Iteration - 1);
        RadixSortInPlace_(Data, ZerosBoundary, End, Iteration - 1);
    }
}

static void
RadixSortInPlace (gs_radix_entry* Data, gs_rad_u64 Count)
{
    gs_rad_u64 Highest = 0;
    for (gs_rad_u64 i = 0; i < Count; i++)
    {
        if (Data[i].Radix > Highest)
        {
            Highest = Data[i].Radix;
        }
    }
    
    gs_rad_u64 Iterations = 0;
    while (Highest > 1)
    {
        ++Iterations;
        Highest = Highest >> 1;
    }
    
    RadixSortInPlace_(Data, 0, Count, Iterations);
}

#define GS_RADIX_SORT_H
#endif // GS_RADIX_SORT_H