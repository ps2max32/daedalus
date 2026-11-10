/*
Copyright (C) 2026 the ps2max authors

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.
*/

#ifndef SYSPS2_ROMMAPPS2_H_
#define SYSPS2_ROMMAPPS2_H_

#include "Base/Types.h"
#include <filesystem>

// The ROM mapped read-only into virtual memory (maxsdk's mx_vm_map_file), in
// the word order the core keeps it in (.n64: 32-bit words, little-endian).
// A .z64 or .v64 is converted once into <name>.n64 next to it. Returns the
// mapping and its size, or nullptr.
const void *	PS2_MapRom( const std::filesystem::path & filename, u32 * size );
void			PS2_UnmapRom( const void * p );

#endif // SYSPS2_ROMMAPPS2_H_
