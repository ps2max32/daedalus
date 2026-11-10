/*
Copyright (C) 2026 the ps2max authors

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.
*/

// The picture through libs/libgl (GL 1.x on the GS), after the 3DS backend:
// the renderer draws vertex arrays filled per frame. The emulator sees a
// window of 640 x 480 (mxgl_window), mapped onto the TV mode's picture.

#include "Base/Types.h"
#include "Graphics/ColourValue.h"
#include "Graphics/GraphicsContext.h"
#include "Interface/Preferences.h"

#include <stdio.h>
#include <stdlib.h>

#include <GL/gl.h>
extern "C" {
#include <mxgl.h>
}

extern void HandleEndOfFrame();

#define SCR_WIDTH  640
#define SCR_HEIGHT 480

uint32_t  gVertexCount = 0;
uint32_t  gMaxVertices = 30000;
float    *gVertexBuffer;
uint32_t *gColorBuffer;
float    *gTexCoordBuffer;
static float    *gVertexBufferPtr;
static uint32_t *gColorBufferPtr;
static float    *gTexCoordBufferPtr;

// the picture's mode (mxgl_set_mode), from the command line (main.cpp)
int gPS2VideoMode = MXGL_SD;

namespace
{
	class IGraphicsContext : public CGraphicsContext
	{
	public:
		IGraphicsContext();
		virtual ~IGraphicsContext();

		bool				Initialise();
		bool				IsInitialised() const { return mInitialised; }

		void				ClearAllSurfaces();
		void				ClearToBlack();
		void				ClearZBuffer();
		void				ClearColBuffer(const c32 &colour);
		void				ClearColBufferAndDepth(const c32 &colour);
		void				ResetVertexBuffer();

		void				BeginFrame();
		void				EndFrame();
		void				UpdateFrame(bool wait_for_vbl);
		void				GetScreenSize(u32 * width, u32 * height) const;
		void				SetDebugScreenTarget( ETargetSurface ) {}
		void				ViewportType(u32 *d_width, u32 *d_height) const;
		void				DumpScreenShot() {}
		void				DumpNextScreen() {}

	private:
		bool				mInitialised;
		bool				mNewFrame;
	};

	IGraphicsContext::IGraphicsContext() : mInitialised(false), mNewFrame(true)
	{
		gVertexBufferPtr   = static_cast<float *>(malloc(gMaxVertices * sizeof(float) * 3));
		gTexCoordBufferPtr = static_cast<float *>(malloc(gMaxVertices * sizeof(float) * 2));
		gColorBufferPtr    = static_cast<uint32_t *>(malloc(gMaxVertices * sizeof(uint32_t)));
		ResetVertexBuffer();
	}

	IGraphicsContext::~IGraphicsContext()
	{
		free(gVertexBufferPtr);
		free(gTexCoordBufferPtr);
		free(gColorBufferPtr);
	}

	bool IGraphicsContext::Initialise()
	{
		mxgl_init();
		if (gPS2VideoMode != MXGL_SD && mxgl_set_mode(gPS2VideoMode) < 0)
			printf("daedalus: the video mode %#x without beam racing (memory)\n", gPS2VideoMode);
		mxgl_window(SCR_WIDTH, SCR_HEIGHT);
		mInitialised = true;
		ClearAllSurfaces();
		return true;
	}

	void IGraphicsContext::ClearAllSurfaces()
	{
		ClearToBlack();
	}

	void IGraphicsContext::ClearToBlack()
	{
		glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
		glDisable(GL_SCISSOR_TEST);
		glDepthMask(GL_TRUE);
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClearDepth(1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	void IGraphicsContext::ClearZBuffer()
	{
		glDepthMask(GL_TRUE);
		glClearDepth(1.0f);
		glClear(GL_DEPTH_BUFFER_BIT);
	}

	void IGraphicsContext::ClearColBuffer(const c32 & colour)
	{
		glClearColor(colour.GetRf(), colour.GetGf(), colour.GetBf(), colour.GetAf());
		glClear(GL_COLOR_BUFFER_BIT);
	}

	void IGraphicsContext::ClearColBufferAndDepth(const c32 & colour)
	{
		glDepthMask(GL_TRUE);
		glClearDepth(1.0f);
		glClearColor(colour.GetRf(), colour.GetGf(), colour.GetBf(), colour.GetAf());
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	void IGraphicsContext::ResetVertexBuffer()
	{
		gVertexBuffer   = gVertexBufferPtr;
		gColorBuffer    = gColorBufferPtr;
		gTexCoordBuffer = gTexCoordBufferPtr;
		gVertexCount    = 0;
	}

	void IGraphicsContext::BeginFrame()
	{
		if (mNewFrame)
		{
			ClearToBlack();
			ResetVertexBuffer();
			mNewFrame = false;
		}
		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_COLOR_ARRAY);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glVertexPointer(3, GL_FLOAT, 0, gVertexBufferPtr);
		glColorPointer(4, GL_UNSIGNED_BYTE, 0, gColorBufferPtr);
		glTexCoordPointer(2, GL_FLOAT, 0, gTexCoordBufferPtr);
	}

	void IGraphicsContext::EndFrame()
	{
		HandleEndOfFrame();
	}

	void IGraphicsContext::UpdateFrame(bool wait_for_vbl [[maybe_unused]])
	{
		mxgl_end_frame();
		mxgl_frame_done();
		mNewFrame = true;
	}

	void IGraphicsContext::ViewportType(u32 *d_width, u32 *d_height) const
	{
		*d_width  = SCR_WIDTH;
		*d_height = SCR_HEIGHT;
	}

	void IGraphicsContext::GetScreenSize(u32 * p_width, u32 * p_height) const
	{
		*p_width  = SCR_WIDTH;
		*p_height = SCR_HEIGHT;
	}
}

template<> bool CSingleton< CGraphicsContext >::Create()
{
	mpInstance = std::make_shared<IGraphicsContext>();
	return mpInstance->Initialise();
}
