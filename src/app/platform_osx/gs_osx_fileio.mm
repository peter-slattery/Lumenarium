#include <sys/stat.h>

static uint32_t
gsosx_GetLastFileWriteTime(char* Path)
{
	int32_t Result = 0;
	struct stat FileStat = {0};
	if (stat(Path, &FileStat) == 0)
	{
		Result = FileStat.st_mtimespec.tv_sec;
	}
	else
	{
		// TODO: Asserts
	}
	return Result;
}

static uint32_t
gsosx_GetFilesize(char* Path)
{
	uint32_t Result = 0;

	int FileHandle = open(Path, O_RDONLY);
	struct stat FileStat = {0};
	fstat(FileHandle, &FileStat);
	close(FileHandle);

	Result = (uint32_t)FileStat.st_size;
	return Result;
}

static bool
gsosx_LoadFileIntoMemory(char* Path, uint32_t FileSize, uint8_t* FileMemory)
{
	bool Result = false;
	int FileHandle = open(Path, O_RDONLY);

	struct stat FileStat = {0};
	fstat(FileHandle, &FileStat);
	if (FileStat.st_size <= FileSize)
	{
		read(FileHandle, FileMemory, FileSize);
		Result = true;
	}
	close(FileHandle);

	return Result;
}

static bool
gsosx_WriteEntireFile(char* Path, uint32_t FileSize, uint8_t* FileMemory)
{
	bool Result = false;
	int FileHandle = open(Path, O_WRONLY | O_CREAT, 0777);
	ssize_t SizeWritten = write(FileHandle, FileMemory, FileSize);
	if (SizeWritten == FileSize)
	{
		Result = true;
	}
	close(FileHandle);
	return Result;
}