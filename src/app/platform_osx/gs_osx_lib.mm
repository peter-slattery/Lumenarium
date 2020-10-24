#include <dlfcn.h>

static void*
gsosx_LoadDLL(char* Path)
{
	void* LibHandle = 0;

	LibHandle = dlopen(Path, RTLD_LAZY);
	if (LibHandle)
	{
		dlerror(); // Clear Last Error
	}
	else
	{
		LibHandle = 0;
	}

	return LibHandle;
}

#define gsosx_GetProcAddress(libHandle, type, name) (type*)dlsym((libHandle), name)

static void
gsosx_UnloadDLL(void* LibHandle)
{
	if (LibHandle)
	{
		dlclose(LibHandle);
	}
}