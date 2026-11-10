/*
Copyright (C) 2026 the ps2max authors

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.
*/

// Threads on maxsdk (<mx/thread.h>): fixed priorities, no time slicing. The
// emulator's own thread runs at the main thread's priority; a lower number
// runs first.

#include "Base/Types.h"
#include "System/Thread/Thread.h"
#include "System/Timing/Timing.h"

extern "C" {
#include <mx/mem.h>
#include <mx/thread.h>
}

namespace
{
	const u32 kStackBytes = 64 * 1024;

	struct SThread
	{
		DaedThread	Function;
		void *		Argument;
		void *		Stack;
		int			Id;
	};

	// priority offsets from the main thread's: TP_LOW runs below it
	const int gPriorityOffsets[ TP_NUM_PRIORITIES ] = { 1, 0, -1, -2 };

	void ThreadEntry( void * arg )
	{
		SThread * t = static_cast< SThread * >( arg );
		t->Function( t->Argument );
	}

	SThread * gThreads[ 16 ];

	SThread * Find( ThreadHandle handle )
	{
		return handle >= 0 && handle < 16 ? gThreads[ handle ] : nullptr;
	}
}

ThreadHandle CreateThread( const char * name [[maybe_unused]], DaedThread function, void * argument )
{
	int slot = 0;
	while( slot < 16 && gThreads[ slot ] != nullptr )
		slot++;
	if( slot == 16 )
		return kInvalidThreadHandle;

	SThread * t = new SThread;
	t->Function = function;
	t->Argument = argument;
	// a thread's stack is pinned, never VM (<mx/thread.h>)
	t->Stack = mx_dma_alloc( kStackBytes );
	if( t->Stack == nullptr )
	{
		delete t;
		return kInvalidThreadHandle;
	}
	int prio = mx_thread_prio( mx_thread_self() ) + gPriorityOffsets[ TP_NORMAL ];
	t->Id = mx_thread_start( ThreadEntry, t, t->Stack, kStackBytes, prio );
	if( t->Id < 0 )
	{
		mx_dma_free( t->Stack );
		delete t;
		return kInvalidThreadHandle;
	}
	gThreads[ slot ] = t;
	return slot;
}

void SetThreadPriority( ThreadHandle handle, EThreadPriority pri )
{
	if( SThread * t = Find( handle ) )
		mx_thread_set_prio( t->Id, mx_thread_prio( mx_thread_self() ) + gPriorityOffsets[ pri ] );
}

void ReleaseThreadHandle( ThreadHandle handle [[maybe_unused]] )
{
}

void TerminateThread( ThreadHandle handle [[maybe_unused]] )
{
	// maxsdk has no way to end another thread; the emulator's threads end
	// when asked (JoinThread)
}

bool JoinThread( ThreadHandle handle, s32 timeout [[maybe_unused]] )
{
	SThread * t = Find( handle );
	if( t == nullptr )
		return true;
	mx_thread_join( t->Id );
	mx_dma_free( t->Stack );
	delete t;
	gThreads[ handle ] = nullptr;
	return true;
}

void ThreadSleepMs( u32 ms )
{
	mx_sleep_us( ms * 1000 );
}

void ThreadSleepTicks( u32 ticks )
{
	mx_sleep_us( (u32)NTiming::ToMilliseconds( ticks ) * 1000 );
}

void ThreadYield()
{
	mx_thread_yield();
}
