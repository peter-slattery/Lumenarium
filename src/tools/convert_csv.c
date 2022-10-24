#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <math.h>
#include <stdbool.h>
#include <errno.h>
#include <limits.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../platform/linux/lumenarium_linux_memory.h"
#include "../core/lumenarium_core.h"
#include "../platform/lumenarium_os.h"

#define linux_err_print(sub_proc) linux_err_print_((char*)__FUNCTION__, (char*)(sub_proc), errno)
void 
linux_err_print_(char* proc, char* sub_proc, s32 errsv)
{
  printf("Error: %s:%s - %d\n\t%s\n\n", proc, sub_proc, errsv, strerror(errsv));
}

#define OS_FILE_HANDLE_TYPE s32
#define OS_FILE_MAX_PATH PATH_MAX
#define OS_FILE_INVALID_HANDLE -1
#define OS_SOCKET_TYPE s32
#define OS_SOCKET_INVALID_HANDLE -1
#include "../platform/shared/lumenarium_shared_file_tracker.h"
#include "../platform/linux/lumenarium_linux_file.h"

char* cvtcsv_header = "";

Allocator* a = 0;

internal char*
cvtcsv_seek_next_line(char* at)
{
  char* result = at;
  while(*result && !(*result == '\n' || *result == '\r')) result++;
  while(*result && (*result == '\n' || *result == '\r')) result++;
  return result;
}

internal char*
cvtcsv_seek_comma(char* at) 
{
  char* result = at;
  while (*result && *result != ',') { result++; }
  return result;
}

internal void
cvtcsv_process_line(u32 line_number, char* start, char* end, FILE* out)
{
  char* col_end_0 = cvtcsv_seek_comma(start);
  char* col_end_1 = cvtcsv_seek_comma(col_end_0 + 1);
  char* col_end_2 = cvtcsv_seek_comma(col_end_1 + 1);
  char* col_end_3 = end;

  char* col_start_1 = col_end_0 + 1;
  char* col_start_2 = col_end_1 + 1;
  char* col_start_3 = col_end_2 + 1;
  
  s32 col_len_0 = (u32)(col_end_0 - start);
  s32 col_len_1 = (u32)(col_end_1 - (col_end_0 + 1));
  s32 col_len_2 = (u32)(col_end_2 - (col_end_1 + 1));
  s32 col_len_3 = (u32)(col_end_3 - (col_end_2 + 1));

  fprintf(out, "[%d] = {", line_number);
  fprintf(out, " %.*s, ", col_len_0, start);       // city id
  fprintf(out, " %.*s, ", col_len_1, col_start_1); // year
  fprintf(out, " %.*s, ", col_len_2, col_start_2); // month
  fprintf(out, " %.*s, ", col_len_3, col_start_3); // prop
  fprintf(out, "},\n");
}

internal void
cvtcsv_convert(char* path)
{
  // Reading in the csv
  String in_path = string_f(a, "../../../data/incenter_data/csv/%s.csv", path);
  File_Handle csv_file = os_file_open(in_path, FileAccess_Read, FileCreate_OpenExisting);
  if (csv_file.value == 0) { printf("could not open %.*s\n", str_varg(in_path)); return; }
  Data csv_ = os_file_read_all(csv_file, a);
  os_file_close(csv_file);
  if (csv_.size == 0) { printf("could not read %.*s\n", str_varg(in_path)); return; }

  String csv = (String){ 
    .str = csv_.base, 
    .len = csv_.size, 
    .cap = csv_.size 
  };

  // Writing out the c file
  String out_path = string_f(a, "../../../data/incenter_data/c/%s.h", path);
  printf("%.*s -> %.*s\n", str_varg(in_path), str_varg(out_path));
  FILE* out = fopen((char*)out_path.str, "wb");
  if (!out) { printf("Cannot write to file: %.*s\n", str_varg(out_path)); return; }
  fprintf(out, "%s", cvtcsv_header);
  fprintf(out, "static Incenter_Data_Row %s_data[] = {\n", path);

  char* at = cvtcsv_seek_next_line((char*)csv.str);
  u32 col = 0;
  u32 line = 0;
  char* line_start = at;
  while (*at != 0)
  {
    if (*at == ',') col += 1;
    if (*at == '\n') { 
      cvtcsv_process_line(line, line_start, at, out);
      line += 1; 
      col = 0; 
      line_start = at + 1; 
    }
    at++;
  }
//  cvtcsv_process_line(line, line_start, at, out);
  fprintf(out, "};\nglobal u32 %s_len = sizeof(%s_data) / sizeof(%s_data[0]);", path, path, path);

  fclose(out);
}

int main (int arg_c, char** args)
{
  a = bump_allocator_create_reserve(MB(1));
  
  cvtcsv_convert("question_1");
  cvtcsv_convert("question_2");
  cvtcsv_convert("question_3");
  cvtcsv_convert("question_4");
  cvtcsv_convert("question_5");
  cvtcsv_convert("question_6");
  cvtcsv_convert("question_7");
  cvtcsv_convert("question_8");
  cvtcsv_convert("question_9");
  cvtcsv_convert("question_10");
  cvtcsv_convert("question_11");
  cvtcsv_convert("question_12");
  cvtcsv_convert("question_13");
  cvtcsv_convert("question_14");
  cvtcsv_convert("question_15");
  cvtcsv_convert("question_16");
  cvtcsv_convert("question_17");
  cvtcsv_convert("question_18");
  cvtcsv_convert("question_19");
  cvtcsv_convert("question_20");
  cvtcsv_convert("question_21");



  printf("Done\n");
  return 0;
}