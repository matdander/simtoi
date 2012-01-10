/*
 * COpenGL.cpp
 *
 *  Created on: Dec 15, 2011
 *      Author: bkloppenborg
 */

#include "COpenGL.h"

#include <cstdio>

#include "CGLWidget.h"
#include "CModelList.h"
#include "CGLShaderList.h"

using namespace std;

COpenGL::COpenGL(CGLWidget * gl_widget)
{
	// Set the OpenGL widget, make this thread the current rendering context
	mGLWidget = gl_widget;
}

COpenGL::~COpenGL()
{
	// Delete the shader list.
	delete this->shader_list;

   // Free OpenGL memory:
	if(mFBO_depth) glDeleteRenderbuffers(1, &mFBO_depth);
	if(mFBO_texture) glDeleteTextures(1, &mFBO_texture);
	if(mFBO) glDeleteFramebuffers(1, &mFBO);

	// Signal that we are done rendering.
	mGLWidget->doneCurrent();
}

/// Static function for checking OpenGL errors:
void COpenGL::CheckOpenGLError(string function_name)
{
    GLenum status = glGetError(); // Check that status of our generated frame buffer
    // If the frame buffer does not report back as complete
    if (status != 0)
    {
        string errstr =  (const char *) gluErrorString(status);
        printf("Encountered OpenGL Error %x %s\n %s", status, errstr.c_str(), function_name.c_str());
        throw;
    }
}

/// Copy the contents from the internal rendering framebuffer to GL_BACK
/// calling thread is responsible for swapping the buffers.
void COpenGL::BlitToScreen()
{
	// Get exclusive access to OpenGL:
	QMutexLocker locker(&mGLMutex);
	mGLWidget->makeCurrent();

    // Bind back to the default buffer (just in case something didn't do it),
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Blit the application-defined render buffer to the on-screen render buffer.
    glBindFramebuffer(GL_READ_FRAMEBUFFER, mFBO);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, GL_BACK);
    glBlitFramebuffer(0, 0, mWindow_width, mWindow_height, 0, 0, mWindow_width, mWindow_height, GL_COLOR_BUFFER_BIT, GL_LINEAR);

    glFinish();
    mGLWidget->swapBuffers();
    mGLWidget->doneCurrent();
    printf("Blit called.\n");
}

/// Gets the named shader.
CShader * COpenGL::GetShader(eGLShaders shader)
{
	// TODO: implement this in a thread-safe fashion.
	// Really this is only called during initialization and should be ok not being thread-friendly.
	return shader_list->GetShader(shader);
};

/// Initiales class memory, sets window width, height and the shader source directory
void COpenGL::Init(int window_width, int window_height, double scale, string shader_source_dir)
{
	mWindow_width = window_width;
	mWindow_height = window_height;
	mScale = scale;
	this->shader_list = new CGLShaderList(shader_source_dir);
}

void COpenGL::InitOpenGL()
{
	// Get exclusive access to OpenGL:
	QMutexLocker locker(&mGLMutex);
	mGLWidget->makeCurrent();

    // ########
    // OpenGL initialization
    // ########
    glClearColor(0.0, 0.0, 0.0, 0.0);
    // Set to flat (non-interpolated) shading:
    glShadeModel(GL_FLAT);
    glEnable(GL_DEPTH_TEST);    // enable the Z-buffer depth testing
    glDisable(GL_DITHER);
    glHint(GL_POLYGON_SMOOTH_HINT, GL_DONT_CARE);

    // Now setup the projection system to be orthographic
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    // Note, the coordinates here are in object-space, not coordinate space.
    double half_width = mWindow_width * mScale / 2;
    glOrtho(-half_width, half_width, -half_width, half_width, -half_width, half_width);

    // Init the off-screen frame buffer.
    InitFrameBuffer();

    mGLWidget->doneCurrent();
}

void COpenGL::InitFrameBuffer(void)
{
    InitFrameBufferDepthBuffer();
    InitFrameBufferTexture();

    glGenFramebuffers(1, &mFBO); // Generate one frame buffer and store the ID in mFBO
    glBindFramebuffer(GL_FRAMEBUFFER, mFBO); // Bind our frame buffer

    // Attach the depth and texture buffer to the frame buffer
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mFBO_texture, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, mFBO_depth);

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

    // Check that status of our generated frame buffer
    if (status != GL_FRAMEBUFFER_COMPLETE)
    {
        string errorstring = (char *) gluErrorString(status);
        printf("Couldn't create frame buffer: %x %s\n", status, errorstring.c_str());
        exit(0); // Exit the application
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0); // Unbind our frame buffer
}

/// Initializes the framebuffer's depth buffer.
void COpenGL::InitFrameBufferDepthBuffer(void)
{

    glGenRenderbuffers(1, &mFBO_depth); // Generate one render buffer and store the ID in mFBO_depth
    CheckOpenGLError("initFrameBufferDepthBuffer1");
    glBindRenderbuffer(GL_RENDERBUFFER, mFBO_depth); // Bind the mFBO_depth render buffer
    CheckOpenGLError("initFrameBufferDepthBuffer2");

    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, mWindow_width, mWindow_height); // Set the render buffer storage to be a depth component, with a width and height of the window

    glBindRenderbuffer(GL_RENDERBUFFER, 0); // Unbind the render buffer
}

/// Initalizes the framebuffer's texture buffer.
void COpenGL::InitFrameBufferTexture(void)
{
    glGenTextures(1, &mFBO_texture); // Generate one texture
    glBindTexture(GL_TEXTURE_2D, mFBO_texture); // Bind the texture mFBOtexture

    // Create the texture in red channel only 8-bit (256 levels of gray) in GL_BYTE (CL_UNORM_INT8) format.
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, mWindow_width, mWindow_height, 0, GL_RED, GL_BYTE, NULL);
    //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, mWindow_width, mWindow_height, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, NULL);
    // These other formats might work, check that GL_BYTE is still correct for the higher precision.
    // I don't think we'll ever need floating point numbers, but those are here too:
    //glTexImage2D(GL_TEXTURE_2D, 0, GL_R16, mWindow_width, mWindow_height, 0, GL_RED, GL_BYTE, NULL);
    //glTexImage2D(GL_TEXTURE_2D, 0, GL_R32, mWindow_width, mWindow_height, 0, GL_RED, GL_BYTE, NULL);
    //glTexImage2D(GL_TEXTURE_2D, 0, GL_R16F, mWindow_width, mWindow_height, 0, GL_RED, CL_HALF_FLOAT, NULL);
    //glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, mWindow_width, mWindow_height, 0, GL_RED, GL_FLOAT, NULL);


    // Setup the basic texture parameters
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    // Unbind the texture
    glBindTexture(GL_TEXTURE_2D, 0);
}

/// Render the models.
/// Don't call this routine using a different thread unless you have control of the OpenGL context.
void COpenGL::RenderModels()
{
	// Get exclusive control of the OpenGL context (unlocking is implicit when this function returns)
	QMutexLocker locker(&mGLMutex);
	printf("Render called.\n");

	if(mModels != NULL)
		mModels->Render(this);

	// Unlock the mutex and signal that rendering is completed
	emit RenderComplete();
}

/// Resize the window
void COpenGL::Resize(int x, int y)
{
	// Do nothing (for now).
}