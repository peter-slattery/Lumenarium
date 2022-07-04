char* cvtcsv_header = ""
"typedef struct {\n"
"  Incenter_City_Id city;\n"
"  u32 year;\n"
"  Incenter_Month_Id month;\n"
"  r32 value_0;\n"
"  r32 value_1;\n"
"  r32 value_2;\n"
"} Incenter_Test_Data_Row;\n";

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
  char* col_end_3 = cvtcsv_seek_comma(col_end_2 + 1);
  char* col_end_4 = end - 1; // -1 because line endings are \r\n

  char* col_start_1 = col_end_0 + 1;
  char* col_start_2 = col_end_1 + 1;
  char* col_start_3 = col_end_2 + 1;
  char* col_start_4 = col_end_3 + 1;

  s32 col_len_0 = (u32)(col_end_0 - start);
  s32 col_len_1 = (u32)(col_end_1 - (col_end_0 + 1));
  s32 col_len_2 = (u32)(col_end_2 - (col_end_1 + 1));
  s32 col_len_3 = (u32)(col_end_3 - (col_end_2 + 1));
  s32 col_len_4 = (u32)(col_end_4 - (col_end_3 + 1));

  fprintf(out, "[%d] = {", line_number);
  fprintf(out, " %.*s, ", col_len_1, col_start_1); // city id
  fprintf(out, " %.*s, ", col_len_2, col_start_2); // year
  fprintf(out, " %.*s, ", col_len_3, col_start_3); // month
  fprintf(out, " %.*sf, ", col_len_4, col_start_4); // value 1
  fprintf(out, " 0, 0 },\n");
}

internal void
cvtcsv_convert(String path)
{
  // this is hardcoded for a particular kind of csv
  scratch_get(scratch);
  File_Handle csv_file = os_file_open(path, FileAccess_Read, FileCreate_OpenExisting);
  Data csv_data = os_file_read_all(csv_file, scratch.a);
  String csv = (String){ 
    .str = csv_data.base, 
    .len = csv_data.size, 
    .cap = csv_data.size 
  };

  FILE* out = fopen("./data/incenter_test_data.c", "wb");
  fprintf(out, "%s", cvtcsv_header);
  fprintf(out, "static Incenter_Test_Data_Row test_data[] = {\n");

  char* at = (char*)csv.str;
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
  cvtcsv_process_line(line, line_start, at, out);
  fprintf(out, "};\nglobal u32 test_data_len = sizeof(test_data) / sizeof(test_data[0]);");
  printf("Done");

  fclose(out);

  scratch_release(scratch);
}