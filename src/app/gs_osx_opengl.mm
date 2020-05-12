#include <OpenGL/gl.h>

struct gsoo_opengl_state
{
	NSOpenGLContext* Context; 
};

@interface gsoo_OpenGLView: NSOpenGLView @end
@implementation gsoo_OpenGLView : NSOpenGLView
	- (void)
	reshape
	{
		// TODO: framebufferWidth and Height were globals in the code I was pulling from
		// Need some way to get the new window height in here.
		CGRect FrameRect = self.frame;
		glViewport(0, 0, FrameRect.size.width, FrameRect.size.height);
		//glViewport(0, 0, framebufferWidth, framebufferHeight);
	}
@end

static NSOpenGLContext*
gsoo_CreateOpenGLContext(NSWindow* Window, uint32_t Width, uint32_t Height, int32_t EnableVSync)
{
	NSOpenGLContext* Result = 0;
	NSOpenGLPixelFormatAttribute PixelFormatAttributes[] = {
		NSOpenGLPFAClosestPolicy,
		NSOpenGLPFADoubleBuffer,
		NSOpenGLPFASampleBuffers,
		0,
		0
	};

	NSOpenGLPixelFormat* PixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes: PixelFormatAttributes];
	Result = [[NSOpenGLContext alloc] initWithFormat: PixelFormat shareContext: 0];

	if (!Result) 
	{ 
		// TODO: Assert/Handle
		return 0; 
	}

	[Result makeCurrentContext];
	GLint VSync = EnableVSync;
	[Result setValues: &VSync forParameter: NSOpenGLCPSwapInterval];

	// Set Backbuffer Resolution
	GLint BackbufferDimensions[] = { Width, Height };
	CGLSetParameter(Result.CGLContextObj, kCGLCPSurfaceBackingSize, BackbufferDimensions);
	CGLEnable(Result.CGLContextObj, kCGLCESurfaceBackingSize);

	// 
	gsoo_OpenGLView* View = [[gsoo_OpenGLView alloc] init];
	[Window setContentView: View];
	[View setOpenGLContext: Result];
	[View setPixelFormat: PixelFormat];
	[Result setView: View];

	return Result;
}

static void
gsoo_SwapBuffers(NSOpenGLContext* OpenGLContext)
{
	[OpenGLContext flushBuffer];
}