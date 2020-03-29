#include <stdio.h>

#include <objc/message.h>
#include <objc/runtime.h>

typedef struct AppDel {
	Class isa;
	id window;
} AppDelegate;

BOOL AppDel_didFinishLaunching(AppDelegate* self, SEL _command, id Notification)
{
	return YES;
}

int main(int ArgCount, char** Args)
{
	Class NSApplicationClass = (Class)objc_getClass("NSApplication");
	Class AppDelegateClass = objc_allocateClassPair(NSApplicationClass, "AppDelegate", 0);

	SEL MethodSelector = sel_getUid("applicationDidFinishLaunching:");
	// NOTE(Peter): i = int, @ = object, : = method selector (SEL)
	char MethodSignature[] = "i@:@";
	class_addMethod(AppDelegateClass, MethodSelector, (IMP)AppDel_didFinishLaunching, "i@:@");
	objc_registerClassPair(AppDelegateClass);

	return 0;	
}