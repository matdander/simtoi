/* CDensityDisk.h
 *
 *  Created on: Nov. 5, 2013
 *      Author: bkloppenborg
 */

#include "CDensityDisk.h"
#include "CShaderFactory.h"
#include "CCylinder.h"

CDensityDisk::CDensityDisk(int n_additional_params)
: 	CModel(n_additional_params + 4)
{
	// give this object a name
	mName = "Density disk base class";

	// Define the placement of the density disk model parameters
	mParamNames.push_back("r_in");
	SetParam(mBaseParams + 1, 0.1);
	SetFree(mBaseParams + 1, false);
	SetMax(mBaseParams + 1, 10);
	SetMin(mBaseParams + 1, 0.1);

	mParamNames.push_back("radial cutoff");
	SetParam(mBaseParams + 2, 20);
	SetFree(mBaseParams + 2, false);
	SetMax(mBaseParams + 2, 10);
	SetMin(mBaseParams + 2, 0.1);

	mParamNames.push_back("height cutoff");
	SetParam(mBaseParams + 3, 5);
	SetFree(mBaseParams + 3, false);
	SetMax(mBaseParams + 3, 10);
	SetMin(mBaseParams + 3, 0.1);

	mParamNames.push_back("n rings (int)");
	SetParam(mBaseParams + 4, 50);
	SetFree(mBaseParams + 4, false);
	SetMax(mBaseParams + 4, 100);
	SetMin(mBaseParams + 4, 1);

	// We load the default shader, but this should be replaced by something more
	// specific later.
	auto shaders = CShaderFactory::Instance();
	mShader = shaders.CreateShader("default");
}

CDensityDisk::~CDensityDisk()
{

}

///// Draws a flat (planar) ring between r_in and r_out.
//void CDensityDisk::DrawDisk(double r_in, double r_out)
//{
//	// lookup constants
//	double color = mParams[3];
//
//	// Compute the transparency at the inner/out radii
//	const double trans_in = Transparency(r_in, 0, 0);
//	const double trans_out = Transparency(r_out, 0, 0);
//
//	glBegin(GL_QUAD_STRIP);
//
//	glNormal3d(0.0, 1.0, 0.0);
//
//	for(int j = 0; j <= mSlices; j++ )
//	{
//		glColor4d(color, 0.0, 0.0, trans_in);
//		glVertex3d(mCosT[ j ] * r_in,  0, mSinT[ j ] * r_in);
//		glColor4d(color, 0.0, 0.0, trans_out);
//		glVertex3d(mCosT[ j ] * r_out, 0, mSinT[ j ] * r_out);
//	}
//
//	glEnd();
//}

void CDensityDisk::Init()
{
	// Generate the verticies and elements
	vector<vec3> vbo_data;
	vector<unsigned int> elements;
	unsigned int z_divisions = 20;
	unsigned int phi_divisions = 50;
	unsigned int r_divisions = 20;

	mRimStart = 0;
	CCylinder::GenreateRim(vbo_data, elements, z_divisions, phi_divisions);
	mRimSize = elements.size();
	// Calculate the offset in vertex indexes for the GenerateMidplane function
	// to generate correct element indices.
	unsigned int vertex_offset = vbo_data.size() / 2;
	mMidplaneStart = mRimSize;
	CCylinder::GenerateMidplane(vbo_data, elements, vertex_offset, r_divisions, phi_divisions);
	mMidplaneSize = elements.size() - mRimSize;

	// Create a new Vertex Array Object, Vertex Buffer Object, and Element Buffer
	// object to store the model's information.
	//
	// First generate the VAO, this stores all buffer information related to this object
	glGenVertexArrays(1, &mVAO);
	glBindVertexArray(mVAO);
	// Generate and bind to the VBO. Upload the verticies.
	glGenBuffers(1, &mVBO); // Generate 1 buffer
	glBindBuffer(GL_ARRAY_BUFFER, mVBO);
	glBufferData(GL_ARRAY_BUFFER, vbo_data.size() * sizeof(vec3), &vbo_data[0], GL_STATIC_DRAW);
	// Generate and bind to the EBO. Upload the elements.
	glGenBuffers(1, &mEBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mEBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, elements.size() * sizeof(unsigned int), &elements[0], GL_STATIC_DRAW);

	// Next we need to define the storage format for this object for the shader.
	// First get the shader and activate it
	GLuint shader_program = mShader->GetProgram();
	glUseProgram(shader_program);
	// Now start defining the storage for the VBO.
	// The 'vbo_data' variable stores a unit cylindrical shell (i.e. r = 1,
	// h = -0.5 ... 0.5).
	GLint posAttrib = glGetAttribLocation(shader_program, "position");
	glEnableVertexAttribArray(posAttrib);
	glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), 0);
	// Now define the normals, if they are used
	GLint normAttrib = glGetAttribLocation(shader_program, "normal");
	if(normAttrib > -1)
	{
		glEnableVertexAttribArray(normAttrib);
		glVertexAttribPointer(normAttrib, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)(3*sizeof(float)));
	}

	// Check that things loaded correctly.
	CWorkerThread::CheckOpenGLError("CDensityDisk.Init()");

	// All done. Un-bind from the VAO, VBO, and EBO to prevent it from being
	// modified by subsequent calls.
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	// Indicate the model is ready to use.
	mModelReady = true;
}

void CDensityDisk::Render(GLuint framebuffer_object, const glm::mat4 & view)
{
	if(!mModelReady)
		Init();

	// Look up the parameters:
	const double r_in = mParams[mBaseParams + 1];
	const double r_cutoff  = mParams[mBaseParams + 2];
	const double h_cutoff  = mParams[mBaseParams + 3];
	int n_rings  = ceil(mParams[mBaseParams + 4]);
	vec2 color = vec2(mParams[3], 1.0);

	// Bind to the framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_object);

	// Activate the shader
	GLuint shader_program = mShader->GetProgram();
	mShader->UseShader();
	GLint uniColorFlag = glGetUniformLocation(shader_program, "color_from_uniform");
	glUniform1i(uniColorFlag, true);

	// bind back to the VAO
	glBindVertexArray(mVAO);

	// Define the color
    GLint uniColor = glGetUniformLocation(shader_program, "uni_color");
    glUniform2fv(uniColor, 1, glm::value_ptr(color));

	// Define the view:
	GLint uniView = glGetUniformLocation(shader_program, "view");
	glUniformMatrix4fv(uniView, 1, GL_FALSE, glm::value_ptr(view));

	GLint uniTranslation = glGetUniformLocation(shader_program, "translation");
	glUniformMatrix4fv(uniTranslation, 1, GL_FALSE, glm::value_ptr(Translate()));

	GLint uniRotation = glGetUniformLocation(shader_program, "rotation");
	glUniformMatrix4fv(uniRotation, 1, GL_FALSE, glm::value_ptr(Rotate()));

	GLint uniScale = glGetUniformLocation(shader_program, "scale");

	glDisable(GL_DEPTH_TEST);

	// Render each cylindrical wall:

    // Init scale variables
	double radius = 0;
	double height = h_cutoff;

    glm::mat4 scale;
    glm::mat4 r_scale;
    glm::mat4 h_scale = glm::scale(mat4(), glm::vec3(1.0, 1.0, height));

	double dr = (r_cutoff - r_in) / (n_rings - 1);
	for(unsigned int i = 0; i < n_rings; i++)
	{
		// Scale the radius:
		radius = r_in + i * dr;
		r_scale = glm::scale(mat4(), glm::vec3(radius, radius, 1.0));
		scale = h_scale * r_scale;

		glUniformMatrix4fv(uniScale, 1, GL_FALSE, glm::value_ptr(scale));
		glDrawElements(GL_TRIANGLE_STRIP, mRimSize, GL_UNSIGNED_INT, 0);
	}

	// Draw the midplane
	glDrawElements(GL_TRIANGLE_STRIP, mMidplaneSize, GL_UNSIGNED_INT, (void*) (mMidplaneStart * sizeof(float)));

	glEnable(GL_DEPTH_TEST);

	// Return to the default framebuffer before leaving.
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	CWorkerThread::CheckOpenGLError("CDensityDisk.Render()");
}

/// Overrides the default CModel::SetShader function.
void CDensityDisk::SetShader(CShaderPtr shader)
{
	// This mode does not accept different shaders, do nothing here.
}
