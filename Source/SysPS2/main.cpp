/*
Copyright (C) 2026 the ps2max authors

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.
*/

// DaedalusX64 on the PS2 (maxsdk, mx OS).
//
//   daedalus.elf [options] <rom>       .z64, .v64 or .n64 (a .n64 copy is made once)
//
//   -hd         1080i, beam racing (libgl's MXGL_1080I | MXGL_BEAM)
//   -beam       beam racing in SD
//   -rgb24      24-bit colour (with -beam)
//   -frames N   end after N frames (tests); -shot <file>: the last as a TGA
//   -nogfx      no display lists (the CPU and the RSP's other tasks only)
//   -pad F:B,.. press N64 buttons B (start, a, b, z, up, down, left, right;
//               joined with +) at frame F for 5 frames (tests)
//
// The ROM is mapped into virtual memory (SysPS2/RomMapPS2.cpp); paths are
// relative to the ELF's directory (roms.ini, Preferences.ini, SaveGames/).

#include "Base/Types.h"
#include "Core/CPU.h"
#include "Interface/Preferences.h"
#include "System/SystemInit.h"
#include "System/Timing/Timing.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern "C" {
#include <mxgl.h>
}

extern "C" {
#include <mx/vm.h>
}

// virtual memory from the start (the ROM's window: up to 64 MiB, the largest
// cartridge); the swap file is the shell's shared one
extern "C" const struct mx_vm_config mx_vm_config = {
	.swap = "swap.bin",
	.map_size = 64u << 20,
	.clean_frames = 32,
};

extern int gPS2VideoMode;		// SysPS2/Graphics/GraphicsContextPS2.cpp
bool gPS2NoGfx = false;			// SysPS2/HLEGraphics/GraphicsPluginPS2.cpp
u32 gPS2Frame = 0;				// the frames so far (Input/PS2: -pad)
const char * gPS2PadScript = nullptr;	// -pad

namespace
{
	u32			gFrameLimit = 0;
	const char *gShotPath = nullptr;
	u32			gFrames = 0;
	u64			gLastReport = 0;
	u32			gFramesAtReport = 0;
}

// each frame (GraphicsContext::EndFrame), and each VI with -nogfx
void HandleEndOfFrame()
{
	gFrames++;
	gPS2Frame = gFrames;
	u64 now;
	NTiming::GetPreciseTime( &now );
	if( gLastReport == 0 )
		gLastReport = now;
	if( now - gLastReport >= 5000000 )
	{
		u32 n = gFrames - gFramesAtReport;
		printf( "daedalus: frame %u, %.1f frames/s\n", (unsigned)gFrames,
				(double)n * 1000000.0 / (double)( now - gLastReport ) );
		gLastReport = now;
		gFramesAtReport = gFrames;
	}
	if( gFrameLimit != 0 && gFrames >= gFrameLimit )
	{
		if( gShotPath != nullptr && !gPS2NoGfx )
			mxgl_screenshot( gShotPath );
		CPU_Halt( "frames" );
	}
}

int main( int argc, char * argv[] )
{
	const char * rom = nullptr;
	for( int i = 1; i < argc; i++ )
	{
		if( strcmp( argv[i], "-hd" ) == 0 )					gPS2VideoMode = MXGL_1080I | MXGL_BEAM;
		else if( strcmp( argv[i], "-beam" ) == 0 )			gPS2VideoMode |= MXGL_BEAM;
		else if( strcmp( argv[i], "-rgb24" ) == 0 )			gPS2VideoMode |= MXGL_RGB24;
		else if( strcmp( argv[i], "-nogfx" ) == 0 )			gPS2NoGfx = true;
		else if( strcmp( argv[i], "-frames" ) == 0 && i + 1 < argc )	gFrameLimit = (u32)atoi( argv[++i] );
		else if( strcmp( argv[i], "-shot" ) == 0 && i + 1 < argc )		gShotPath = argv[++i];
		else if( strcmp( argv[i], "-pad" ) == 0 && i + 1 < argc )		gPS2PadScript = argv[++i];
		else if( argv[i][0] != '-' )						rom = argv[i];
		else
			printf( "daedalus: unknown option %s\n", argv[i] );
	}
	if( rom == nullptr )
	{
		printf( "usage: daedalus.elf [-hd] [-beam] [-rgb24] [-frames N] [-shot file] [-nogfx] <rom>\n" );
		return 64;
	}

	if( !System_Init() )
	{
		printf( "daedalus: System_Init failed\n" );
		return 1;
	}
	printf( "daedalus: %s\n", rom );
	if( !System_Open( rom ) )
	{
		printf( "daedalus: cannot open %s\n", rom );
		System_Finalize();
		return 1;
	}
	CPU_Run();
	System_Close();
	System_Finalize();
	printf( "daedalus: %u frames\n", (unsigned)gFrames );
	return 0;
}
