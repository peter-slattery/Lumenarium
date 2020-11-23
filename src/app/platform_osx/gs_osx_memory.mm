#include <stdlib.h>

ALLOCATOR_ALLOC(OsxAlloc)
{
	uint8_t* Result = 0;
	char* StartAddress = (char*)0;

	int Prot = PROT_READ | PROT_WRITE;
	int Flags = MAP_PRIVATE | MAP_ANON;

	Result = (uint8_t*)mmap(StartAddress, Size, Prot, Flags, -1, 0);
	return (u8*)Result;
}

ALLOCATOR_FREE(OsxFree)
{
	munmap((void*)Ptr, (size_t)Size);
}