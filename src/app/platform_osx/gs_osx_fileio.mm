#include <sys/stat.h>
#include <dirent.h>

internal gs_file_info
OsxGetFileInfo(int FileHandle, gs_const_string Path)
{
	gs_file_info Result = (gs_file_info){0};
	Result.Path = Path;
	Result.AbsolutePath = Path;

	struct stat FileStat;
	if (fstat(FileHandle, &FileStat) == 0)
	{
		Result.FileSize = (u32)FileStat.st_size;
		Result.LastWriteTime = (s32)FileStat.st_mtimespec.tv_sec;
	}
	else
	{
		// TODO(pjs): Error Handling
		InvalidCodePath;
	}

	Result.IsDirectory = false;
	return Result;
}

GET_FILE_INFO(OsxGetFileInfo)
{
	Assert(IsNullTerminated(Path));
	gs_file_info Result = (gs_file_info){0};
	
	int FileHandle = open(Path.Str, O_RDONLY);
	if (FileHandle != -1)
	{
		Result = OsxGetFileInfo(FileHandle, Path);
		close(FileHandle);
	}
	else
	{
		// TODO(pjs): Error Handling
		InvalidCodePath;
	}

	return Result;
}

READ_ENTIRE_FILE(OsxReadEntireFile)
{
	Assert(DataIsNonEmpty(Memory));
    Assert(IsNullTerminated(Path));

    gs_file Result = (gs_file){0};

    int FileHandle = open(Path.Str, O_RDONLY);
    if (FileHandle != -1)
    {
    	Result.FileInfo = OsxGetFileInfo(FileHandle, Path);
    	s32 BytesRead = read(FileHandle, (uint8_t*)Memory.Memory, Memory.Size - 1);
    	if (BytesRead != -1)
    	{
    		Memory.Memory[Memory.Size - 1] = 0;
    		Result.Data = Memory;

    		// TODO: Get Absolute Path
    	}
    	else
    	{
    		InvalidCodePath; // Error
    	}

    	close(FileHandle);
    }
    else
    {
    	InvalidCodePath; // Error
    }

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

WRITE_ENTIRE_FILE(OsxWriteEntireFile)
{
	Assert(DataIsNonEmpty(Data));
    Assert(IsNullTerminated(Path));

    bool Success = false;

    int FileHandle = open(Path.Str, O_WRONLY | O_CREAT, 0777);
    if (FileHandle != -1)
    {
    	ssize_t BytesWritten = write(FileHandle, Data.Memory, Data.Size);
    	if (BytesWritten == Data.Size)
    	{
    		Success = true;
    	}
    	else
    	{
    		InvalidCodePath; // Error
    	}

    	close(FileHandle);
    }
    else
    {
    	InvalidCodePath; // Error
    }

    return Success;
}

ENUMERATE_DIRECTORY(OsxEnumerateDirectory)
{
	Assert(IsNullTerminated(Path));
    gs_file_info_array Result = (gs_file_info_array){0};

    DIR* DirectoryHandle = opendir(Path.Str);
    if (DirectoryHandle != NULL)
    {
    	dirent* FileInfo;
    	while (NULL != (FileInfo = readdir(DirectoryHandle)))
    	{
    		// TODO
    	}
    	closedir(DirectoryHandle);
    }
    else
    {
    	InvalidCodePath; // Error;
    }

    return Result;
}






