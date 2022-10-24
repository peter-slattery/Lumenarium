
// NOTE(PS): should call performance.now() which is in milliseconds
WASM_EXTERN u32 wasm_performance_now();

WASM_EXTERN void wasm_sleep(u32 milliseconds);

Platform_Ticks
platform_get_ticks()
{
  Platform_Ticks result = {};
  result.value = (u64)wasm_performance_now();
  return result;
}

r64
platform_ticks_to_seconds(Platform_Ticks ticks)
{
  r64 result = (r64)(ticks.value * 1000);
  return result;
}

void
platform_sleep(r64 milliseconds)
{
  wasm_sleep(milliseconds);
}