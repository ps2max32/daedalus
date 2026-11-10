/*
Copyright (C) 2026 the ps2max authors

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.
*/

// Time in microseconds (gettimeofday: maxsdk's libc glue).

#include "Base/Types.h"
#include "System/Timing/Timing.h"

#include <sys/time.h>

namespace NTiming {

bool GetPreciseFrequency( u64 * p_freq )
{
	*p_freq = 1000000;
	return true;
}

bool GetPreciseTime( u64 * p_time )
{
	struct timeval tv;
	gettimeofday( &tv, nullptr );
	*p_time = (u64)tv.tv_sec * 1000000 + (u64)tv.tv_usec;
	return true;
}

u64 ToMilliseconds( u64 ticks )
{
	return ticks / 1000;
}

} // NTiming
