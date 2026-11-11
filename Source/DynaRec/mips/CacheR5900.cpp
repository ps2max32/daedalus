/*
Copyright (C) 2026 the ps2max authors

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.
*/

// The PS2's _DaedalusICacheInvalidate (the PSP's is in DynaRecStubs.S): code
// written through the data cache is written back, then the instruction
// cache's lines of it invalidated (CACHE IHIN, 64-byte lines; the sequence is
// preceded by SYNC.P: EE Core Instruction Set Manual).

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

extern "C" {
#include <mx/cpu.h>
}

extern "C" void _DaedalusICacheInvalidate( const void * address, u32 length )
{
	if( length == 0 )
		return;
	uintptr_t p = reinterpret_cast<uintptr_t>( address ) & ~uintptr_t( 63 );
	uintptr_t end = reinterpret_cast<uintptr_t>( address ) + length;
	mx_dcache_writeback( reinterpret_cast<const void *>( p ), u32( end - p ) );
	__asm__ volatile( "sync.l" ::: "memory" );
	for( ; p < end; p += 64 )
		__asm__ volatile( "sync.p\n\tcache 0x0b, 0(%0)" :: "r"( p ) : "memory" );
	__asm__ volatile( "sync.p" ::: "memory" );
}
