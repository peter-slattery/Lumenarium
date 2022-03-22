#define WEBGL_EXPORT __attribute__((visibility("default")))
#define WEBGL_EXTERN extern "C" 

#define EXTERN_C_BEGIN extern "C" {
#define EXTERN_C_END }

WEBGL_EXTERN void print(char* text);

EXTERN_C_BEGIN;

WEBGL_EXPORT int 
main(void) 
{ 
  print("Hi there!\n");
  return 5; 
}

EXTERN_C_END;