#include <mach/mach_time.h>

static const uint64_t NANOS_PER_USEC = 1000ULL;
static const uint64_t NANOS_PER_MILLISEC = 1000ULL * NANOS_PER_USEC;
static const uint64_t NANOS_PER_SEC = 1000ULL * NANOS_PER_MILLISEC;

struct gsosx_time_info
{
	uint64_t StartTimeAbsolute;
	mach_timebase_info_data_t MachTimeInfo;
};

static gsosx_time_info
gsosx_InitTime()
{
	gsosx_time_info Result = {0};
	Result.StartTimeAbsolute = mach_absolute_time();
	mach_timebase_info(&Result.MachTimeInfo);
	return Result;
}

static uint64_t
gsosx_GetTime(gsosx_time_info TimeInfo)
{
	uint64_t Result = mach_absolute_time() - TimeInfo.StartTimeAbsolute;
	return Result;
}

static double
gsosx_GetSecondsElapsed(uint64_t Start, uint64_t End, gsosx_time_info TimeInfo)
{
	double Result = 0;
	double Elapsed = (double)(End - Start);
	Result = Elapsed * (double)(TimeInfo.MachTimeInfo.numer) / (double)NANOS_PER_SEC / (double)TimeInfo.MachTimeInfo.denom;
	return Result;
}

static void
gsosx_Sleep(double Seconds, gsosx_time_info TimeInfo)
{
	// NOTE(Peter): This isn't entirely precise, and can vary by up to 500 microseconds. We could implement a sleep combined witha busy wait
	// to get more precise sleeping. Do this if necessary
	uint64_t WaitTimeAbs = Seconds / (double)TimeInfo.MachTimeInfo.numer * (double)NANOS_PER_SEC * (double)TimeInfo.MachTimeInfo.denom;
	uint64_t NowAbs = mach_absolute_time();
	mach_wait_until(NowAbs + WaitTimeAbs);
}