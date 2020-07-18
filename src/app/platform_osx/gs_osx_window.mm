@interface gsosx_ApplicationDelegate : NSObject <NSApplicationDelegate, NSWindowDelegate> @end

@implementation gsosx_ApplicationDelegate : NSObject
	- (void)
	applicationDidFinishLaunching: (NSNotification *)notification 
	{
		[NSApp stop: nil];
		NSAutoreleasePool* Pool = [[NSAutoreleasePool alloc] init];

		[Pool drain];
	}

	- (NSApplicationTerminateReply)
	applicationShouldTerminate: (NSApplication*) sender
	{
		return NO;
	}

	- (void)
	dealloc
	{
		[super dealloc];
	}
@end

@interface gsosx_WindowDelegate: NSObject<NSWindowDelegate> @end
@implementation gsosx_WindowDelegate : NSObject
	- (BOOL)
	windowShouldClose: (id)sender
	{
		// TODO(Peter): Stop Application Running?
		NSLog(@"Close button pressed");
		return NO;
	}

	- (void)
	windowDidBecomeKey: (NSNotification*)notification
	{
		// TODO: ???
	}

	- (void)
	windowDisResignKey: (NSNotification*)notification
	{
		// TODO: ???
	}
@end

static NSWindow*
gsosx_CreateWindow(NSApplication* App, int Width, int Height, id Title)
{
	int WindowStyleMask = NSWindowStyleMaskClosable;
	NSRect WindowRect = NSMakeRect(0, 0, Width, Height);
	
	NSWindow* Window = [[NSWindow alloc] initWithContentRect: WindowRect styleMask: WindowStyleMask backing: NSBackingStoreBuffered	 defer: YES];
	Window.styleMask = NSWindowStyleMaskResizable | NSWindowStyleMaskTitled | NSWindowStyleMaskClosable;

	gsosx_WindowDelegate* WindowDelegate = [[gsosx_WindowDelegate alloc] init];
	
	[Window setOpaque: YES];
	[Window setDelegate: WindowDelegate];
	[Window setTitle: Title];

	NSMenu* MenuBar = [NSMenu alloc];
	NSMenuItem* AppMenuItem = [NSMenuItem alloc];
	[MenuBar addItem: AppMenuItem];
	[App setMainMenu: MenuBar];

	NSMenu* AppMenu = [NSMenu alloc];
	id QuitTitle = [@"Quit " gs_stringByAppendinggs_string: Title];
	id QuitMenuItem = [[NSMenuItem alloc] initWithTitle: QuitTitle action: @selector(terminate:) keyEquivalent: @"q"];
	[AppMenu addItem: QuitMenuItem];
	[AppMenuItem setSubmenu: AppMenu];

	[App activateIgnoringOtherApps: YES];

	return Window;
}
