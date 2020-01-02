//
// File: foldhaus_network_ordering.h
// Author: Peter Slattery
// Creation Date: 2020-01-01
//
#ifndef FOLDHAUS_NETWORK_ORDERING_H

// Packs a u8 to a known big endian buffer
inline u8*	
PackB1(u8* ptr, u8 val)
{
	*ptr = val;
    return ptr + sizeof(val);
}

//Unpacks a u8 from a known big endian buffer
inline u8 
UpackB1(const u8* ptr)
{
	return *ptr;
}

//Packs a u8 to a known little endian buffer
inline u8*	
PackL1(u8* ptr, u8 val)
{
	*ptr = val;
    return ptr + sizeof(val);
}

//Unpacks a u8 from a known little endian buffer
inline u8 
UpackL1(const u8* ptr)
{
	return *ptr;
}

//Packs a u16 to a known big endian buffer
inline u8* 
PackB2(u8* ptr, u16 val)
{
	ptr[1] = (u8)(val & 0xff);
	ptr[0] = (u8)((val & 0xff00) >> 8);
    return ptr + sizeof(val);
}

//Unpacks a u16 from a known big endian buffer
inline u16 
UpackB2(const u8* ptr)
{
	return (u16)(ptr[1] | ptr[0] << 8);
}

//Packs a u16 to a known little endian buffer
inline u8* 
PackL2(u8* ptr, u16 val)
{
	*((u16*)ptr) = val;
    return ptr + sizeof(val);
}

//Unpacks a u16 from a known little endian buffer
inline u16 
UpackL2(const u8* ptr)
{
	return *((u16*)ptr);
}

//Packs a u32 to a known big endian buffer
inline u8* 
PackB4(u8* ptr, u32 val)
{
	ptr[3] = (u8) (val & 0xff);
	ptr[2] = (u8)((val & 0xff00) >> 8);
	ptr[1] = (u8)((val & 0xff0000) >> 16);
	ptr[0] = (u8)((val & 0xff000000) >> 24);
    return ptr + sizeof(val);
}

//Unpacks a u32 from a known big endian buffer
inline u32 
UpackB4(const u8* ptr)
{
	return (u32)(ptr[3] | (ptr[2] << 8) | (ptr[1] << 16) | (ptr[0] << 24));
}

//Packs a u32 to a known little endian buffer
inline u8* 
PackL4(u8* ptr, u32 val)
{
	*((u32*)ptr) = val;
    return ptr + sizeof(val);
}

//Unpacks a u32 from a known little endian buffer
inline u32 
UpackL4(const u8* ptr)
{
	return *((u32*)ptr);
}

//Packs a u64 to a known big endian buffer
inline u8* 
PackB8(u8* ptr, u64 val)
{
	ptr[7] = (u8) (val & 0xff);
	ptr[6] = (u8)((val & 0xff00) >> 8);
	ptr[5] = (u8)((val & 0xff0000) >> 16);
	ptr[4] = (u8)((val & 0xff000000) >> 24);
	ptr[3] = (u8)((val & 0xff00000000) >> 32);
	ptr[2] = (u8)((val & 0xff0000000000) >> 40);
	ptr[1] = (u8)((val & 0xff000000000000) >> 48);
	ptr[0] = (u8)((val & 0xff00000000000000) >> 56);
    return ptr + sizeof(val);
}

//Unpacks a uint64 from a known big endian buffer
inline u64 
UpackB8(const u8* ptr)
{
	return ((u64)ptr[7]) | (((u64)ptr[6]) << 8) | (((u64)ptr[5]) << 16) |
        (((u64)ptr[4]) << 24) | (((u64)ptr[3]) << 32) | 
        (((u64)ptr[2]) << 40) | (((u64)ptr[1]) << 48) | 
        (((u64)ptr[0]) << 56);
}

//Packs a u64 to a known little endian buffer
inline u8* 
PackL8(u8* ptr, u64 val)
{
	*((u64*)ptr) = val;
    return ptr + sizeof(val);
}

//Unpacks a u64 from a known little endian buffer
inline u64 
UpackL8(const u8* ptr)
{
	return *((u64*)ptr);
}


#define FOLDHAUS_NETWORK_ORDERING_H
#endif // FOLDHAUS_NETWORK_ORDERING_H