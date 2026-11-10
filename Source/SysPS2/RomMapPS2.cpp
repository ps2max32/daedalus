/*
Copyright (C) 2026 the ps2max authors

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.
*/

#include "SysPS2/RomMapPS2.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern "C" {
#include <mx/vm.h>
const char *ps2_host_path(const char *path, char *out);
}

namespace
{
	// the first word of the header read as a little-endian u32, by layout
	const u32 kMagicN64 = 0x80371240;	// 40 12 37 80: the core's layout
	const u32 kMagicZ64 = 0x40123780;	// 80 37 12 40: big-endian
	const u32 kMagicV64 = 0x12408037;	// 37 80 40 12: 16-bit swapped

	u32 ReadMagic( const char * path, long * size )
	{
		FILE * f = fopen( path, "rb" );
		if( f == nullptr )
			return 0;
		u32 magic = 0;
		if( fread( &magic, 4, 1, f ) != 1 )
			magic = 0;
		fseek( f, 0, SEEK_END );
		*size = ftell( f );
		fclose( f );
		return magic;
	}

	void Swap( u8 * p, u32 n, u32 magic )
	{
		for( u32 i = 0; i + 4 <= n; i += 4 )
		{
			u8 a = p[i], b = p[i+1], c = p[i+2], d = p[i+3];
			if( magic == kMagicZ64 )	{ p[i] = d; p[i+1] = c; p[i+2] = b; p[i+3] = a; }
			else						{ p[i] = c; p[i+1] = d; p[i+2] = a; p[i+3] = b; }
		}
	}

	// src (magic) into dst in the core's layout
	bool Convert( const char * src, const char * dst, u32 magic )
	{
		printf( "daedalus: converting %s to %s\n", src, dst );
		FILE * in = fopen( src, "rb" );
		if( in == nullptr )
			return false;
		FILE * out = fopen( dst, "wb" );
		if( out == nullptr )
		{
			fclose( in );
			return false;
		}
		const u32 kChunk = 256 * 1024;
		u8 * buf = static_cast< u8 * >( malloc( kChunk ) );
		bool ok = buf != nullptr;
		size_t n;
		while( ok && ( n = fread( buf, 1, kChunk, in ) ) > 0 )
		{
			Swap( buf, (u32)n, magic );
			ok = fwrite( buf, 1, n, out ) == n;
		}
		free( buf );
		fclose( in );
		if( fclose( out ) != 0 )
			ok = false;
		if( !ok )
			remove( dst );
		return ok;
	}
}

const void * PS2_MapRom( const std::filesystem::path & filename, u32 * size )
{
	std::string path = filename.string();
	long file_size = 0;
	u32 magic = ReadMagic( path.c_str(), &file_size );
	if( magic != kMagicN64 )
	{
		if( magic != kMagicZ64 && magic != kMagicV64 )
		{
			printf( "daedalus: %s is not an N64 ROM (%08x)\n", path.c_str(), (unsigned)magic );
			return nullptr;
		}
		std::filesystem::path converted = filename;
		converted.replace_extension( ".n64" );
		long conv_size = 0;
		if( ReadMagic( converted.string().c_str(), &conv_size ) != kMagicN64 || conv_size != file_size )
		{
			if( !Convert( path.c_str(), converted.string().c_str(), magic ) )
			{
				printf( "daedalus: cannot write %s\n", converted.string().c_str() );
				return nullptr;
			}
		}
		path = converted.string();
	}

	char host[PATH_MAX + 8];
	const void * p = mx_vm_map_file( ps2_host_path( path.c_str(), host ), size );
	if( p == nullptr )
		printf( "daedalus: cannot map %s\n", host );
	else
		printf( "daedalus: %s mapped, %u KiB\n", host, (unsigned)( *size >> 10 ) );
	return p;
}

void PS2_UnmapRom( const void * p )
{
	mx_vm_unmap_file( p );
}
