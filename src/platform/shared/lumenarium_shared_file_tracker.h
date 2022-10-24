#if !defined(LUMENARIUM_SHARED_FILE_TRACKER_H)
#define LUMENARIUM_SHARED_FILE_TRACKER_H

#if !defined(OS_FILE_HANDLE_TYPE)
#  error "You must define an OS_FILE_HANDLE_TYPE"
#endif

#if !defined(OS_FILE_MAX_PATH)
#  error "You must define an OS_FILE_MAX_PATH"
#endif

#if !defined(OS_FILE_INVALID_HANDLE)
#  error "You must define an OS_FILE_INVALID_HANDLE"
#endif

#define                    open_files_cap   32
global u64                 open_files_len = 1; // zero is invalid
global char                open_file_paths[open_files_cap][OS_FILE_MAX_PATH];
global u64                 open_file_paths_len[open_files_cap];
global OS_FILE_HANDLE_TYPE open_files[open_files_cap];

void                open_files_init();
bool                open_files_has_room();
OS_FILE_HANDLE_TYPE open_files_get_handle(File_Handle handle);
String              open_files_get_path(File_Handle handle);
File_Handle         open_files_put_handle(OS_FILE_HANDLE_TYPE os_handle, String path);
void                open_files_rem_file(File_Handle handle);

////////////////////////////////////////////////
// IMPLEMENTATION

void 
open_files_init()
{
  for (u32 i = 0; i < open_files_cap; i++)
  {
    open_files[i] = OS_FILE_INVALID_HANDLE;
  }
}

bool open_files_has_room() { return open_files_len < open_files_cap; }

OS_FILE_HANDLE_TYPE
open_files_get_handle(File_Handle handle)
{
  assert(handle.value < open_files_len);
  return open_files[handle.value];
}

String
open_files_get_path(File_Handle handle)
{
  assert(handle.value < open_files_len);
  String result = {
    .str = (u8*)open_file_paths[handle.value],
    .len = open_file_paths_len[handle.value],
    .cap = open_file_paths_len[handle.value],
  };
  return result;
}

File_Handle
open_files_put_handle(OS_FILE_HANDLE_TYPE os_handle, String path)
{
  assert(path.len < OS_FILE_MAX_PATH);

  File_Handle result = {};
  if (os_handle != OS_FILE_INVALID_HANDLE)
  {
    if (open_files_has_room())
    {
      result.value = open_files_len++;
    }
    else
    {
      // search for emtpy index
      for (u32 i = 1; i < open_files_cap; i++)
      {
        if (open_files[i] == OS_FILE_INVALID_HANDLE)
        {
          result.value = i;
        }
      }
    }

    if (result.value != 0)
    {
      open_files[result.value] = os_handle;
      memory_copy(path.str, (u8*)open_file_paths[result.value], path.len);
      open_file_paths[result.value][path.len] = 0; // null term
      open_file_paths_len[result.value] = path.len;
    }
  }
  return result;
}

void
open_files_rem_file(File_Handle handle)
{
  assert(handle.value < open_files_len);
  open_files[handle.value] = OS_FILE_INVALID_HANDLE;;
}



#endif // LUMENARIUM_SHARED_FILE_TRACKER_H