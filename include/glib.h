#ifndef _GLIB_H_
#define _GLIB_H_

#include <3ds.h>
#include <citro3d.h>

class glib
{
public:
	struct color
	{
		color(unsigned char r, unsigned char g, unsigned char b, unsigned char a = 255);
		color(int c);
		color(unsigned int c);

		unsigned char r;
		unsigned char g;
		unsigned char b;
		unsigned char a;

		operator int() const;
		operator unsigned int() const;
	};

	struct vertex
	{
		vertex(float x, float y, color c);
		vertex(float x, float y, float z, color c);
		vertex(float x, float y, float z, float nx, float ny, float nz, color c);

		float position[3];
		float color[4];
		float normal[3];
	};

	glib();
	~glib();

	glib(const glib&) = delete;
	glib(glib&&) = delete;

	glib& operator=(const glib&) = delete;
	glib& operator=(glib&&) = delete;

	color setClearColor(color newColor);
	C3D_Mtx* projection();
	C3D_Mtx* model();
	void beginFrame();
	void endFrame();

	void rect(float x, float y, float w, float h, color col);
	
	void push(vertex v1, vertex v2, vertex v3);
	void push(vertex* vertices, int cnt);
	void push(vertex* vertices, int* indices, int cnt);
	void flush();

private:
	static constexpr int VBO_SIZE = 3 * 500;

	DVLB_s* vshader_dvlb;
	shaderProgram_s program;
	int uLoc_projection;
	int uLoc_modelView;
	C3D_Mtx _projection;
	C3D_Mtx _model;
	C3D_RenderTarget* target;
	int clearColor;
	vertex* vbo;
	int vbo_offset;
};

#endif // _GLIB_H_