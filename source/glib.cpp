#include "glib.h"
#include "vshader_shbin.h"

#define DISPLAY_TRANSFER_FLAGS \
	(GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) | \
	GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) | \
	GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))

glib::glib()
{
	gfxInitDefault();
	C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);

	target = C3D_RenderTargetCreate(240, 400, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
	C3D_RenderTargetSetOutput(target, GFX_TOP, GFX_LEFT, DISPLAY_TRANSFER_FLAGS);

	vshader_dvlb = DVLB_ParseFile((u32*)vshader_shbin, (u32)vshader_shbin_size);
	shaderProgramInit(&program);
	shaderProgramSetVsh(&program, vshader_dvlb->DVLE);
	C3D_BindProgram(&program);

	uLoc_projection = shaderInstanceGetUniformLocation(program.vertexShader, "projection");
	uLoc_modelView = shaderInstanceGetUniformLocation(program.vertexShader, "modelView");

	C3D_AttrInfo* attrInfo = C3D_GetAttrInfo();
	AttrInfo_Init(attrInfo);
	AttrInfo_AddLoader(attrInfo, 0, GPU_FLOAT, 3);
	AttrInfo_AddLoader(attrInfo, 1, GPU_FLOAT, 4);
	AttrInfo_AddLoader(attrInfo, 2, GPU_FLOAT, 3);

	vbo = (vertex*)linearAlloc(VBO_SIZE * sizeof(vertex));

	C3D_BufInfo* bufInfo = C3D_GetBufInfo();
	BufInfo_Init(bufInfo);
	BufInfo_Add(bufInfo, vbo, sizeof(vertex), 3, 0x210);

	C3D_TexEnv* env = C3D_GetTexEnv(0);
	C3D_TexEnvInit(env);
	C3D_TexEnvSrc(env, C3D_Both, GPU_PRIMARY_COLOR);
	C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE);

	clearColor = color(0, 0, 0, 255);
	Mtx_OrthoTilt(&_projection, 0.0f, 400.0f, 0.0f, 240.0f, 0.0f, 1.0f, true);
	Mtx_Identity(&_model);
}

glib::~glib()
{
	linearFree(vbo);
	shaderProgramFree(&program);
	DVLB_Free(vshader_dvlb);
	C3D_Fini();
	gfxExit();
}

glib::color glib::setClearColor(color newColor)
{
	color old = clearColor;
	clearColor = newColor;
	return old;
}

C3D_Mtx* glib::projection()
{
	return &_projection;
}

C3D_Mtx* glib::model()
{
	return &_model;
}

void glib::beginFrame()
{
	C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
	C3D_RenderTargetClear(target, C3D_CLEAR_ALL, clearColor, 0);
	C3D_FrameDrawOn(target);
}

void glib::endFrame()
{
	if (vbo_offset > 0) { flush(); }
	C3D_FrameEnd(0);
}

void glib::rect(float x, float y, float w, float h, color col)
{
	vertex bl{ x, y, col };
	vertex br{ x + w, y, col };
	vertex tl{ x, y + h, col };
	vertex tr{ x + w, y + h, col };

	push(bl, br, tl);
	push(tl, br, tr);
}

void glib::push(vertex v1, vertex v2, vertex v3)
{
	if (vbo_offset + 3 >= VBO_SIZE) { flush(); }
	vbo[vbo_offset++] = v1;
	vbo[vbo_offset++] = v2;
	vbo[vbo_offset++] = v3;
}

void glib::push(vertex* vertices, int cnt)
{
	if (vbo_offset + cnt >= VBO_SIZE)
	{
		for (int i = 0; i < cnt; i++)
		{
			if (vbo_offset + 1 >= VBO_SIZE) { flush(); }
			vbo[vbo_offset++] = vertices[i];
		}
	}
	else
	{
		for (int i = 0; i < cnt; i++)
		{
			vbo[vbo_offset++] = vertices[i];
		}
	}
}

void glib::push(vertex* vertices, int* indices, int cnt)
{
	if (vbo_offset + cnt >= VBO_SIZE)
	{
		for (int i = 0; i < cnt; i++)
		{
			if (vbo_offset + 1 >= VBO_SIZE) { flush(); }
			vbo[vbo_offset++] = vertices[indices[i]];
		}
	}
	else
	{
		for (int i = 0; i < cnt; i++)
		{
			vbo[vbo_offset++] = vertices[indices[i]];
		}
	}
}

void glib::flush()
{
	if (vbo_offset <= 0) { return; }
	C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uLoc_projection, &_projection);
	C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uLoc_modelView, &_model);
	C3D_DrawArrays(GPU_TRIANGLES, 0, vbo_offset);
	vbo_offset = 0;
}

glib::color::color(unsigned char r, unsigned char g, unsigned char b, unsigned char a) : r(r), g(g), b(b), a(a) { }
glib::color::color(int c) : r((c >> 24) & 0xFF), g((c >> 16) & 0xFF), b((c >> 8) & 0xFF), a(c & 0xFF) { }
glib::color::color(unsigned int c) : r((c >> 24) & 0xFF), g((c >> 16) & 0xFF), b((c >> 8) & 0xFF), a(c & 0xFF) { }

glib::color::operator int() const
{
	return (r << 24) | (g << 16) | (b << 8) | a;
}

glib::color::operator unsigned int() const
{
	return (r << 24) | (g << 16) | (b << 8) | a;
}

glib::vertex::vertex(float x, float y, glib::color c) : vertex(x, y, 0.5f, c) { }
glib::vertex::vertex(float x, float y, float z, glib::color c) : vertex(x, y, z, 0.0f, 0.0f, 0.0f, c) { }
glib::vertex::vertex(float x, float y, float z, float nx, float ny, float nz, glib::color c) : position{ x, y, z }, color{ c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, c.a / 255.0f }, normal{ nx, ny, nz } { }