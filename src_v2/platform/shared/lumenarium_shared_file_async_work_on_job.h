#if defined(PLATFORM_win32) || defined(PLATFORM_osx)

void
os_file_async_work_on_job(File_Async_Job* job)
{
  File_Handle file = {};
  if (has_flag(job->args.flags, FileAsyncJob_Read))
  {
    file = os_file_open(job->args.path, FileAccess_Read, FileCreate_OpenExisting);
    Data result = os_file_read_all(file, file_jobs_arena);
    if (result.base != 0) 
    {
      job->args.data = result;
      add_flag(job->args.flags, FileAsyncJob_Success);
    }
    else
    {
      add_flag(job->args.flags, FileAsyncJob_Failed);
    }
  }
  else if (has_flag(job->args.flags, FileAsyncJob_Write))
  {
    file = os_file_open(job->args.path, FileAccess_Write, FileCreate_OpenAlways);
    if (os_file_write_all(file, job->args.data))
    {
      add_flag(job->args.flags, FileAsyncJob_Success);
    }
    else
    {
      add_flag(job->args.flags, FileAsyncJob_Failed);
    }
  }
  os_file_close(file);
}

#endif // defined(win32) || defined(osx)