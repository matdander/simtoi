/* 
 * Copyright (c) 2012 Brian Kloppenborg

 *
 * If you use this software as part of a scientific publication, please cite as:
 *
 * Kloppenborg, B.; Baron, F. (2012), "SIMTOI: The SImulation and Modeling 
 * Tool for Optical Interferometry" (Version X). 
 * Available from  <https://github.com/bkloppenborg/simtoi>.
 *
 * This file is part of the SImulation and Modeling Tool for Optical 
 * Interferometry (SIMTOI).
 * 
 * SIMTOI is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License 
 * as published by the Free Software Foundation version 3.
 * 
 * SIMTOI is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public 
 * License along with SIMTOI.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "CWorkerThread.h"
#include <QMutexLocker>
#include <stdexcept>
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace std;

#include "CGLWidget.h"
#include "CTaskList.h"
#include "CModelList.h"

// X11 "Status" definition causes namespace issues. Include after any QT headers (https://bugreports.qt-project.org/browse/QTBUG-54)
#include "COpenCL.hpp"

extern string EXE_FOLDER;

CWorkerThread::CWorkerThread(CGLWidget *glWidget, QString exe_folder)
	: QThread(), mGLWidget(glWidget)
{
	mRun = true;

    // Init datamembers to something reasonable.
    mImageWidth = 128;
    mImageHeight = 128;
    mImageDepth = 1;
    mImageScale = 0.05;
    mImageSamples = 4;

	// Initialize the model and task lists:
    mModelList = CModelListPtr(new CModelList());
	mTaskList = CTaskListPtr(new CTaskList(this));
}

CWorkerThread::~CWorkerThread()
{
	// Stop the thread if it is running. This is a blocking call.
	if(mRun)
		stop();

	// Free local OpenGL objects.
	glDeleteFramebuffers(1, &mFBO);
	glDeleteFramebuffers(1, &mFBO_texture);
	glDeleteFramebuffers(1, &mFBO_depth);
	glDeleteFramebuffers(1, &mFBO_storage);
	glDeleteFramebuffers(1, &mFBO_storage_texture);
}

/// Appends a model to the list of models.
void CWorkerThread::AddModel(CModelPtr model)
{
	// Get exclusive access to the worker
	QMutexLocker lock(&mWorkerMutex);

	// Modify the list of models:
	mModelList->AddModel(model);
	Enqueue(RENDER);
}

void CWorkerThread::BlitToBuffer(GLuint in_buffer, GLuint out_buffer)
{
	// TODO: Need to figure out how to use the layer
	glBindFramebuffer(GL_READ_FRAMEBUFFER, in_buffer);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, out_buffer);
	//glFramebufferTextureLayer(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, out_buffer, 0, out_layer);
	glBlitFramebuffer(0, 0, mImageWidth, mImageHeight, 0, 0, mImageWidth, mImageHeight, GL_COLOR_BUFFER_BIT, GL_LINEAR);
	glFinish();

  	CWorkerThread::CheckOpenGLError("CGLThread BlitToBuffer");
}

/// Blits the off-screen framebuffer to the foreground buffer.
void CWorkerThread::BlitToScreen(GLuint FBO)
{
    // Bind back to the default buffer (just in case something didn't do it),
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
CWorkerThread::CheckOpenGLError("A ");
    // Blit the application-defined render buffer to the on-screen render buffer.
    glBindFramebuffer(GL_READ_FRAMEBUFFER, FBO);
CWorkerThread::CheckOpenGLError("B ");
    /// TODO: In QT I don't know what GL_BACK is.  Seems GL_DRAW_FRAMEBUFFER is already set to it though.
    //glBindFramebuffer(GL_DRAW_FRAMEBUFFER, GL_BACK);

    glBlitFramebuffer(0, 0, mImageWidth, mImageHeight, 0, 0, mImageWidth, mImageHeight, GL_COLOR_BUFFER_BIT, GL_LINEAR);
CWorkerThread::CheckOpenGLError("C ");
    SwapBuffers();
}

/// \brief Instructs the task lists to prepare a bootstrapped data set
void CWorkerThread::BootstrapNext(unsigned int maxBootstrapFailures)
{
	// Get exclusive access to the worker
	QMutexLocker lock(&mWorkerMutex);

	mTempUint = maxBootstrapFailures;
	Enqueue(BOOTSTRAP_NEXT);

	// Wait for the operation to complete.
	mWorkerSemaphore.acquire(1);
}

/// Static function for checking OpenGL errors:
void CWorkerThread::CheckOpenGLError(string function_name)
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

/// Create an OpenGL framebuffer and storage buffers matching the default OpenGL image
void CWorkerThread::CreateGLBuffer(GLuint & FBO, GLuint & FBO_texture, GLuint & FBO_depth, GLuint & FBO_storage, GLuint & FBO_storage_texture)
{
	CreateGLBuffer(FBO, FBO_texture, FBO_depth, FBO_storage, FBO_storage_texture, mImageDepth);
}

/// Create an OpenGL framebuffer and storage buffer matching the default OpenGL image but with a user-defined
/// number of layers up to GL_MAX_FRAMEBUFFER_LAYERS.
void CWorkerThread::CreateGLBuffer(GLuint & FBO, GLuint & FBO_texture, GLuint & FBO_depth, GLuint & FBO_storage, GLuint & FBO_storage_texture, int n_layers)
{
	// enforce
	GLint max_layers = 1;
#ifndef __APPLE__	
	glGetIntegerv(GL_MAX_FRAMEBUFFER_LAYERS, &max_layers);
#endif // __APPLE__
	if(n_layers > max_layers)
		n_layers = max_layers;

	CreateGLMultisampleRenderBuffer(mImageWidth, mImageHeight, mImageSamples, FBO, FBO_texture, FBO_depth);
	CreateGLStorageBuffer(mImageWidth, mImageHeight, n_layers, FBO_storage, FBO_storage_texture);
}

void CWorkerThread::CreateGLMultisampleRenderBuffer(unsigned int width, unsigned int height, unsigned int samples,
		GLuint & FBO, GLuint & FBO_texture, GLuint & FBO_depth)
{
	glGenFramebuffers(1, &FBO);
	glBindFramebuffer(GL_FRAMEBUFFER, FBO);

	glGenRenderbuffers(1, &FBO_texture);
	glBindRenderbuffer(GL_RENDERBUFFER, FBO_texture);
	// Create a 2D multisample texture
	glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_RGBA32F, width, height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, FBO_texture);

	glGenRenderbuffers(1, &FBO_depth);
	glBindRenderbuffer(GL_RENDERBUFFER, FBO_depth);
	glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_DEPTH_COMPONENT, width, height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, FBO_depth);

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

    // Check that status of our generated frame buffer
    if (status != GL_FRAMEBUFFER_COMPLETE)
    {
    	const GLubyte * errStr = gluErrorString(status);
        printf("Couldn't create multisample frame buffer: %x %s\n", status, (char*)errStr);
        delete errStr;
        exit(0); // Exit the application
    }

    // All done, bind back to the default framebuffer.
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void CWorkerThread::CreateGLStorageBuffer(unsigned int width, unsigned int height, unsigned int depth, GLuint & FBO_storage, GLuint & FBO_storage_texture)
{
	glGenFramebuffers(1, &FBO_storage);
	glBindFramebuffer(GL_FRAMEBUFFER, FBO_storage);

	glGenTextures(1, &FBO_storage_texture);
	glBindTexture(GL_TEXTURE_2D_ARRAY, FBO_storage_texture);

	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
	glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_R32F, width, height, depth, 0, GL_RED, GL_FLOAT, NULL);

	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, FBO_storage_texture, 0);

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

    // Check that status of our generated frame buffer
    if (status != GL_FRAMEBUFFER_COMPLETE)
    {
    	const GLubyte * errStr = gluErrorString(status);
        printf("Couldn't create storage frame buffer: %x %s\n", status, (char*)errStr);
        delete errStr;
        exit(0); // Exit the application
    }

    // All done, bind back to the default framebuffer
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// Clears the worker task queue.
void CWorkerThread::ClearQueue()
{
	// get exclusive access to the operation queue
	QMutexLocker lock(&mTaskMutex);

	// Clear the queue
	while (!mTaskQueue.empty())
		mTaskQueue.pop();

	mTaskSemaphore.acquire(mTaskSemaphore.available());
}

void CWorkerThread::Enqueue(WorkerOperations op)
{
	// get exclusive access to the operation queue
	QMutexLocker lock(&mTaskMutex);

	// Push the operation onto the queue
	mTaskQueue.push(op);

	// Release the OpQueue semaphore to let the worker run.
	mTaskSemaphore.release(1);
}

void CWorkerThread::ExportResults(QString save_folder)
{
	// Get exclusive access to the worker
	QMutexLocker lock(&mWorkerMutex);

	// Make sure the save_folder ends in a slash
	if(save_folder[save_folder.size()] != QChar('/'))
		save_folder += "/";

	mTempString = save_folder.toStdString();
	Enqueue(EXPORT);

	// Wait for the export operation to finish
	mWorkerSemaphore.acquire(1);
}

void CWorkerThread::GetChi(double * chi, unsigned int size)
{
	// Get exclusive access to the worker
	QMutexLocker lock(&mWorkerMutex);

	// Assign temporary storage, enqueue the command, and wait for completion
	mTempArray = chi;
	mTempArraySize = size;
	Enqueue(GET_CHI);

	// Wait for the operation to complete
	mWorkerSemaphore.acquire(1);
}

unsigned int CWorkerThread::GetDataSize()
{
	// Get exclusive access to the worker
	QMutexLocker lock(&mWorkerMutex);

	// NOTE: this is a cross-thread call.
	return mTaskList->GetDataSize();
}

/// Gets a list of file filters for use in a QFileDialog
///
/// NOTE: This read-only operation is a cross-thread call
QStringList CWorkerThread::GetFileFilters()
{
	// Get exclusive access to the worker
	QMutexLocker lock(&mWorkerMutex);

	vector<string> filters = mTaskList->GetFileFilters();
	QStringList output;

	for(auto filter: filters)
		output.append(QString::fromStdString(filter));

	return output;
}

/// Get the next operation from the queue.  This is a blocking function.
WorkerOperations CWorkerThread::GetNextOperation(void)
{
	// Try to access the OpQueue semaphore.
	mTaskSemaphore.acquire();

	// get exclusive access to the operation queue
	QMutexLocker lock(&mTaskMutex);
	// get the top operation and pop it from the queue
	WorkerOperations tmp = mTaskQueue.top();
	mTaskQueue.pop();

	return tmp;
}

double CWorkerThread::GetTime()
{
	// Get exclusive access to the worker
	QMutexLocker lock(&mWorkerMutex);

	return mModelList->GetTime();
}

void CWorkerThread::GetUncertainties(double * uncertainties, unsigned int size)
{
	// Get exclusive access to the worker
	QMutexLocker lock(&mWorkerMutex);

	// Assign temporary storage, enqueue the command, and wait for completion
	mTempArray = uncertainties;
	mTempArraySize = size;
	Enqueue(GET_UNCERTAINTIES);

	// Wait for the operation to complete
	mWorkerSemaphore.acquire(1);
}

void CWorkerThread::OpenData(string filename)
{
	// Get exclusive access to the worker
	QMutexLocker lock(&mWorkerMutex);

	mTempString = filename;
	Enqueue(OPEN_DATA);

	// Wait for the operation to complete.
	mWorkerSemaphore.acquire(1);
}

/// Instructs the thread to render to the default framebuffer.
void CWorkerThread::Render()
{
	// Exclusive access is not required for a rendering operation.
	// Simply enqueue and return.
	Enqueue(RENDER);
}

void CWorkerThread::Restore(Json::Value input)
{
	// Get exclusive access to the worker
	QMutexLocker lock(&mWorkerMutex);

	// Note, this is a cross-thread call.
	mModelList->Restore(input);
}

// The main function of this thread
void CWorkerThread::run()
{
	// ########
	// CL/GL context initialization
	// ########
	// Immediately claim the OpenCL context
    mGLWidget->makeCurrent();
    // Create an OpenCL context
	mOpenCL = COpenCLPtr(new COpenCL(CL_DEVICE_TYPE_GPU));

	// ########
	// OpenGL display initialization
	// ########

    // Setup the OpenGL context
    // Set the clear color to black:
	glClearColor(0.0, 0.0, 0.0, 0.0);
	// Dithering is a fractional pixel filling technique that allows you to
	// combine some colors to create the effect of other colors. The non-full
	// fill fraction of pixels could negativly impact the interferometric
	// quantities we wish to simulate. So, disable dithering.
	glDisable(GL_DITHER);
	// Enable multi-sample anti-aliasing to improve the effective resolution
	// of the model area.
	glEnable(GL_MULTISAMPLE);
	// Enable depth testing to permit vertex culling
	glEnable(GL_DEPTH_TEST);
	// Enable alpha blending
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Enable for wireframe-only model
//	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE );

	// Initalize the window
	glViewport(0, 0, mImageWidth, mImageHeight);

	// Setup the view:
	double half_width = mImageWidth * mImageScale / 2;
	double half_height = mImageHeight * mImageScale / 2;
	double depth = 500; // hard-coded to 500 units (typically mas) in each direction.
	mView = glm::ortho(-half_width, half_width, -half_height, half_height, -depth, depth);

	CWorkerThread::CheckOpenGLError("Error occurred during GL Thread Initialization.");

	// Now create this thread's off-screen buffers to match other off-screen buffers
	// All rendering of objects from the UI happens here.
	CreateGLBuffer(mFBO, mFBO_texture, mFBO_depth, mFBO_storage, mFBO_storage_texture);

	// Now have the workers initialize any OpenGL objects they need
	mTaskList->InitGL();

	// ########
	// Remaining OpenCL initialization (context done above)
	// ########
	mTaskList->InitCL();

	// ########
	// Main thread.
	// ########
	WorkerOperations op;
	while(mRun)
	{
		op = GetNextOperation();
		switch(op)
		{

		case BOOTSTRAP_NEXT:
			mTaskList->BootstrapNext(mTempUint);
			mWorkerSemaphore.release(1);
			break;

		case EXPORT:
			// Instruct the worker to export data
			mTaskList->Export(mTempString);
			mWorkerSemaphore.release(1);
			break;

		case GET_CHI:
			// uses mTempArray
			mTaskList->GetChi(mTempArray, mTempArraySize);
			mWorkerSemaphore.release(1);
			break;

		case GET_UNCERTAINTIES:
			// uses mTempArray
			mTaskList->GetUncertainties(mTempArray, mTempArraySize);
			mWorkerSemaphore.release(1);
			break;

		case OPEN_DATA:
			// Instruct the task list to open the file.
			mTaskList->OpenData(mTempString);
			mWorkerSemaphore.release(1);

		case RENDER:
			mModelList->Render(mFBO, mView);
			BlitToScreen(mFBO);
			break;

		case SET_TIME:
			mModelList->SetTime(mTempDouble);
			break;

		default:
		case STOP:
			ClearQueue();
			mRun = false;
			break;

		}
	}

	emit finished();
}

void CWorkerThread::SetScale(double scale)
{
	// Get exclusive access to the worker
	QMutexLocker lock(&mWorkerMutex);

	if(scale < 0)
		throw runtime_error("Image scale cannot be negative.");

	mImageScale = scale;
}

void CWorkerThread::SetSize(unsigned int width, unsigned int height)
{
	// Get exclusive access to the worker
	QMutexLocker lock(&mWorkerMutex);

	if(width < 1 || height < 1)
		throw runtime_error("Image size must be at least 1x1 pixel");

	mImageWidth = width;
	mImageHeight = height;
}

void CWorkerThread::SetTime(double time)
{
	// Get exclusive access to the worker
	QMutexLocker lock(&mWorkerMutex);

	mTempDouble = time;
	Enqueue(SET_TIME);
	Enqueue(RENDER);
}


Json::Value CWorkerThread::Serialize()
{
	// Get exclusive access to the worker
	QMutexLocker lock(&mWorkerMutex);

	Json::Value temp = mModelList->Serialize();

	return temp;
}

void CWorkerThread::stop()
{
	// Get exclusive access to the worker
	QMutexLocker lock(&mWorkerMutex);
	// Equeue a stop command
	Enqueue(STOP);
}

void CWorkerThread::SwapBuffers()
{
	glFinish();
    mGLWidget->swapBuffers();
    CWorkerThread::CheckOpenGLError("CWorkerThread::SwapBuffers()");
}
