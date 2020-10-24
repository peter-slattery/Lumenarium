#include <Cocoa/Cocoa.h>
#include "gs_osx_memory.mm"
#include "gs_osx_window.mm"
#include "gs_osx_fileio.mm"
#include "gs_osx_lib.mm"
#include "gs_osx_opengl.mm"
#include "gs_osx_time.mm"

static void
gsosx_ProcessWindowEvents(NSApplication* App, NSWindow* Window)
{
	// Process Events
	while (true)
	{
		NSEvent* Event = [App nextEventMatchingMask: NSEventMaskAny untilDate: [NSDate distantPast] inMode: NSDefaultRunLoopMode dequeue: YES];
		if (!Event) { break; }

		switch([Event type])
		{
			case NSEventTypeKeyUp:
			case NSEventTypeKeyDown:
			{
				// TODO: Handle Key Presses	
			}break;

			// TODO: Mouse Input

			default:
			{
				[App sendEvent: Event];
			}break;
		}
	}
}

int main(int ArgCount, char** Args)
{
	NSApplication* Application = [NSApplication sharedApplication];
	[Application setActivationPolicy: NSApplicationActivationPolicyRegular];

	gsosx_ApplicationDelegate* Delegate = [[gsosx_ApplicationDelegate alloc] init];
	[Application setDelegate: Delegate];

	int WindowWidth = 1024;
	int WindowHeight = 768;
	id AppName = @"Lumenarium";
	NSWindow* Window = gsosx_CreateWindow(Application, WindowWidth, WindowHeight, AppName);

	// A really cryptic way of asking the window to open
	[Window makeKeyAndOrderFront: Application];

	NSOpenGLContext* OpenGLContext = gsoo_CreateOpenGLContext(Window, WindowWidth, WindowHeight, true);

	gsosx_time_info TimeInfo = gsosx_InitTime();
	double TargetSecondsPerFrame = 1.0 / 60;
	uint64_t LastFrameEnd = gsosx_GetTime(TimeInfo);
	while (true)
	{
		gsosx_ProcessWindowEvents(Application, Window);

		glClearColor(1, 0, 1, 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		gsoo_SwapBuffers(OpenGLContext);

		uint64_t ThisFrameEnd = gsosx_GetTime(TimeInfo);
		double FrameSeconds = gsosx_GetSecondsElapsed(LastFrameEnd, ThisFrameEnd, TimeInfo);
		double SleepSeconds = TargetSecondsPerFrame - FrameSeconds;
		if (SleepSeconds > 0)
		{
			gsosx_Sleep(SleepSeconds, TimeInfo);
		}
	}

	return (0);
}
