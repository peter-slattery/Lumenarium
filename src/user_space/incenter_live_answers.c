
internal void
live_answers_set_magic(char dst[4], char value[4])
{
  dst[0] = value[0];
  dst[1] = value[1];
  dst[2] = value[2];
  dst[3] = value[3];
}

internal bool
live_answers_valid_magic(char dst[4], char value[4])
{
  return ((dst[0] == value[0]) &&
    (dst[1] == value[1]) &&
    (dst[2] == value[2]) &&
    (dst[3] == value[3]));
}

internal void
live_answers_init_bucket_u32(Live_Answers_File_Bucket* bucket, u32 answer, u32 count)
{
  live_answers_set_magic((char*)bucket->magic, LIVE_DATA_BUCKET_MAGIC_NUMBER);
  bucket->answer_u32 = answer;
  bucket->count = count;
}

internal void
live_answers_init_bucket_r32(Live_Answers_File_Bucket* bucket, r32 answer, u32 count)
{
  live_answers_set_magic((char*)bucket->magic, LIVE_DATA_BUCKET_MAGIC_NUMBER);
  bucket->answer_r32 = answer;
  bucket->count = count;
}

internal bool
live_answers_validate(Live_Answers_File file)
{
  bool valid = true;
  if (!live_answers_valid_magic((char*)file.header->magic, LIVE_DATA_HEADER_MAGIC_NUMBER)) {
    printf("Error: Live Answers File has corrupted header.\n");
    valid = false;
  }
  
  // if there was an error in the header, we can't assume that the count
  // its keeping of the number of buckets is correct
  if (valid) 
  {
    for (u32 i = 0; i < file.header->buckets_count; i++)
    {
      Live_Answers_File_Bucket* b = file.buckets + i;
      if (!live_answers_valid_magic((char*)b->magic, LIVE_DATA_BUCKET_MAGIC_NUMBER))
      {
        printf("Error: Live Answers Bucket has corrupted header.\n");
        valid = false;
      }
    }
  }
  
  if (!valid) 
  {
    printf("  Invalid Live Answers File: %.*s\n", str_varg(file.path));
  }
  
  return valid;
}

internal Live_Answers_File
live_answers_load(Incenter_Scene scene, Allocator* allocator)
{
  Live_Answers_File result = {0};
  
  // Read the File
#define USING_PLATFORM_LAYER 0
#if !USING_PLATFORM_LAYER
  result.path = string_f(allocator, "data/live_data/%s.incenterdata", scene.name);
  Data file_data = {0};
  FILE* file = fopen((const char*)result.path.str, "rb");
  if (file) {
    fseek(file, 0, SEEK_END);
    s32 size = ftell(file);
    fseek(file, 0, SEEK_SET);
    file_data.base = allocator_alloc(allocator, size);
    file_data.size = fread((void*)file_data.base, size, 1, file);
    fclose(file);
  } else {
    printf("Creating live data file: %s\n", (char*)result.path.str);
  }
#else
  File_Handle file = os_file_open(result.path, FileAccess_Read, FileCreate_OpenExisting);
  Data file_data = {0};
  if (file.value != 0)
  {
    file_data = os_file_read_all(file, allocator);
    os_file_close(file);
  }
#endif
  
  // Obtain structure of file data
  if (file_data.size > 1) {
    result.header = (Live_Answers_File_Header*)file_data.base;
    result.buckets = (Live_Answers_File_Bucket*)(result.header + 1);
  } else {
    // create the file's initial contents    
    file_data.size = sizeof(Live_Answers_File_Header);
    file_data.base = allocator_alloc(allocator, file_data.size);
    
    // obtain structure
    result.header = (Live_Answers_File_Header*)file_data.base;
    result.buckets = (Live_Answers_File_Bucket*)(result.header + 1);
    
    // initialize 
    live_answers_set_magic((char*)result.header->magic, LIVE_DATA_HEADER_MAGIC_NUMBER);
    result.header->buckets_count = 0;
    result.header->answers_total_count = 0;
  } 
  
  if (!live_answers_validate(result)) {
    return (Live_Answers_File){0};
  }
  
  return result;
}

internal void
live_answers_save(Live_Answers_File file, Live_Answers_File_Bucket* new_bucket)
{
  if (!live_answers_validate(file)) {
    printf("  Not saving invalid live answers data\n");
    return;
  }
  
#if !USING_PLATFORM_LAYER
  FILE* fh = fopen((const char*)file.path.str, "wb");
  if (!fh) {
    printf("Error: Unable to open live data file for writing\n");
    printf("  Live Data File: %.*s\n", str_varg(file.path));
    return;
  }
#else
  File_Handle fh = os_file_open(file.path, FileAccess_Write, FileCreate_OpenAlways);
  if (fh.value == 0) {
    printf("Error: Unable to open live data file for writing\n");
    printf("  Live Data File: %.*s\n", str_varg(file.path));
    
    // TODO(PS): maybe print out what the values should be so we can 
    // recover if necessary?
    
    return;
  }
#endif

  Data existing_data = {
    .base = (u8*)file.header,
    .size = sizeof(Live_Answers_File_Header) + (sizeof(Live_Answers_File_Bucket) * file.header->buckets_count)
  };
#if !USING_PLATFORM_LAYER
  s32 size_written = fwrite((void*)existing_data.base, 1, existing_data.size, fh);
  if (size_written != existing_data.size) {
    printf("Unable to write full file: %.*s\n", str_varg(file.path));
    printf("  Size Needed: %llu, Size Written: %d\n", existing_data.size, size_written);
  }
#else
  if (!os_file_write(fh, existing_data)) {
    printf("Error: Could not write existing data to Live Data File\n");
    printf("  Live Data File: %.*s\n", str_varg(file.path));
  }
#endif
  
  if (new_bucket) {
    Data new_data = {
      .base = (u8*)new_bucket,
      .size = sizeof(Live_Answers_File_Bucket)
    };
#if !USING_PLATFORM_LAYER
    size_written = fwrite(new_data.base, 1, new_data.size, fh);
    if (size_written != new_data.size) {
      printf("Unable to add new data to file %.*s\n", str_varg(file.path));
      printf("  Size Needed: %llu, Size Written: %d\n", new_data.size, size_written);
    }
#else
    if (!os_file_write(fh, new_data)) {
      printf("Error: Could not write new bucket data to Live Data File\n");
      printf("  Live Data File: %.*s\n", str_varg(file.path));
    }
#endif
  }
  
#if !USING_PLATFORM_LAYER
  fclose(fh);
#else
  os_file_close(fh);
#endif
}

#if 1
#  define MAYBE_SKIP_SAVING_INPUT
#else
#  define MAYBE_SKIP_SAVING_INPUT return
#endif

internal void
live_answers_input_u32(Incenter_State* ins, Incenter_Scene scene, u32 value)
{
  MAYBE_SKIP_SAVING_INPUT;

  scratch_get(scratch);
  Live_Answers_File file = live_answers_load(scene, scratch.a);
  if (file.header == 0) {
    printf("Unable to save live data file\n");
    return;
  }
  
  bool found = false;
  for (u32 i = 0; i < file.header->buckets_count; i++)
  {
    Live_Answers_File_Bucket* b = file.buckets + i;
    if (b->answer_u32 == value) {
      b->count += 1;
      found = true;
      break;
    }
  }
  
  Live_Answers_File_Bucket* new_bucket = 0;
  if (!found) {
    new_bucket = allocator_alloc_struct(scratch.a, Live_Answers_File_Bucket);
    live_answers_init_bucket_u32(new_bucket, value, 1);
    file.header->buckets_count += 1;
  }
  
  file.header->answers_total_count += 1;
  
  live_answers_save(file, new_bucket);
  
  scratch_release(scratch);
}

internal void
live_answers_input_r32(Incenter_State* ins, Incenter_Scene scene, r32 value)
{
  MAYBE_SKIP_SAVING_INPUT;

  scratch_get(scratch);
  Live_Answers_File file = live_answers_load(scene, scratch.a);
  if (file.header == 0) {
    printf("Unable to save live data file\n");
    return;
  }
  
  bool found = false;
  for (u32 i = 0; i < file.header->buckets_count; i++)
  {
    Live_Answers_File_Bucket* b = file.buckets + i;
    if (b->answer_r32 == value) {
      b->count += 1;
      found = true;
      break;
    }
  }
  
  Live_Answers_File_Bucket* new_bucket = 0;
  if (!found) {
    new_bucket = allocator_alloc_struct(scratch.a, Live_Answers_File_Bucket);
    live_answers_init_bucket_r32(new_bucket, value, 1);
  }
  
  live_answers_save(file, new_bucket);
  
  scratch_release(scratch);
}