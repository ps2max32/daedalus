/*
Copyright (C) 2026 the ps2max authors

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.
*/

// The PS2's _DaedalusICacheInvalidate (the PSP's is in DynaRecStubs.S): code
// written through the data cache is written back, then the instruction
// cache's lines of it invalidated (CACHE DHWBIN, then IHIN, 64-byte lines:
// EE Core Instruction Set Manual).

#include "Base/Types.h"
#include "Core/CPU.h"

#include <cstddef>

// the offsets DynaRecStubsPS2.S uses (_C0_Count, _AuxBase ...): checked here,
// since the stubs cannot
static_assert( offsetof( SCPUState, CPUControl ) == 0x100 && sizeof( gCPUState.CPUControl[0] ) == 4, "_C0_Count" );
static_assert( offsetof( SCPUState, CurrentPC ) == 0x280, "_AuxBase" );
static_assert( offsetof( SCPUState, StuffToDo ) == 0x28c, "_StuffToDo" );
static_assert( offsetof( SCPUState, MultLo ) == 0x290 && offsetof( SCPUState, MultHi ) == 0x298, "_MultLo, _MultHi" );
static_assert( offsetof( SCPUState, Temp1 ) == 0x2a0, "_Temp1" );
static_assert( offsetof( SCPUState, Events ) == 0x2b0 && offsetof( CPUEvent, mCount ) == 0, "_Events" );

extern "C" void _DaedalusICacheInvalidate( const void * address, u32 length )
{
	if( length == 0 )
		return;
	uintptr_t p = reinterpret_cast<uintptr_t>( address ) & ~uintptr_t( 63 );
	uintptr_t end = reinterpret_cast<uintptr_t>( address ) + length;
	// out of the data cache: written back and invalidated (CACHE DHWBIN,
	// each directly between two SYNC.Ls). Written back only (DHWOIN,
	// mx_dcache_writeback), the PS2 ran stale code within seconds (the
	// SM64 title: a fragment's indirect exit with a freed map, a load
	// from a register without its value); PCSX2 has no caches to show it.
	for( uintptr_t q = p; q < end; q += 64 )
		__asm__ volatile( "sync.l\n\tcache 0x18, 0(%0)\n\tsync.l" :: "r"( q ) : "memory" );
	// then out of the instruction cache (IHIN, after a SYNC.P; it also
	// drops the BTAC's entries of the line)
	for( ; p < end; p += 64 )
		__asm__ volatile( "sync.p\n\tcache 0x0b, 0(%0)" :: "r"( p ) : "memory" );
	__asm__ volatile( "sync.p" ::: "memory" );
}
