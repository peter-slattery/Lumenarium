#include <stdlib.h>

static uint8_t*
gsosx_Alloc(size_t Size)
{
	uint8_t* Result = 0;
	char* StartAddress = (char*)0;
	int Prot = PROT_READ | PROT_WRITE;
	int Flags = MAP_PRIVATE | MAP_ANON;
	Result = (uint8_t*)mmap(StartAddress, Size, Prot, Flags, -1, 0);
	return Result;
}

static void
gsosx_Free(uint8_t* Base, uint32_t Size)
{
	munmap((void*)Base, (size_t)Size);
}

static uint8_t*
gsosx_Realloc(uint8_t* Base, uint32_t OldSize, uint32_t NewSize)
{
	uint8_t* Result = gsosx_Alloc(NewSize);
	for (int32_t i = 0; i < OldSize; i++)
	{
		Result[i] = Base[i];
	}
	gsosx_Free(Base, OldSize	);
	return Result;
}