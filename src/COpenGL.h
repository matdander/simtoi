/*
 * COpenGL.h
 *
 *  Created on: Nov 9, 2011
 *      Author: bkloppenborg
 *
 * A minimial wrapper class to initialize OpenGL and keep track of
 * open contexts and/or OpenGL allocated memory.
 */

#ifndef COPENGL_H_
#define COPENGL_H_

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#include <string>

#include "CGLShaderList.h"

class CShader;

using namespace std;

void CheckOpenGLError(string function_name);

class COpenGL
{
protected:
	// Class variables for (a single) off-screen frame buffer.
	GLuint fbo;
	GLuint fbo_texture;
	GLuint fbo_depth;
	int window_width;
	int window_height;
	double scale;

	CGLShaderList shader_list;

public:
	COpenGL(int window_width, int window_height, double scale);
	~COpenGL();

	CShader * GetShader(eGLShaders shader);

	void init(int argc, char *argv[]);

protected:
	void initFrameBuffer(void);

	void initFrameBufferDepthBuffer(void);

	void initFrameBufferTexture(void);
};

#endif /* COPENGL_H_ */