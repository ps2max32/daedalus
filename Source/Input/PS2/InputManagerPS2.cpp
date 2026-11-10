/*
Copyright (C) 2026 the ps2max authors

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.
*/

// The PS2's pads (maxsdk, <mx/pad.h>) as N64 controllers 1 and 2:
//
//   left stick  N64 stick          D-pad       D-pad
//   cross       A                  square      B
//   L2, R2      Z                  L1, R1      L, R
//   right stick C buttons          triangle    C up, circle C down
//   start       start

#include "Base/Types.h"
#include "Input/InputManager.h"
#include "Ultra/ultra_os.h"

#include <cstdlib>
#include <cstring>

extern "C" {
#include <mx/pad.h>
}

extern u32 gPS2Frame;				// SysPS2/main.cpp
extern const char * gPS2PadScript;	// -pad F:B,..: buttons for tests

namespace
{
	// the buttons the -pad script holds at frame f (5 frames from each F)
	u16 ScriptButtons( u32 f )
	{
		static const struct { const char * name; u16 mask; } names[] = {
			{ "start", START_BUTTON }, { "a", A_BUTTON }, { "b", B_BUTTON }, { "z", Z_TRIG },
			{ "up", U_JPAD }, { "down", D_JPAD }, { "left", L_JPAD }, { "right", R_JPAD },
		};
		u16 held = 0;
		for( const char * p = gPS2PadScript; p != nullptr && *p != 0; )
		{
			char * e;
			u32 at = (u32)strtoul( p, &e, 10 );
			if( *e != ':' )
				break;
			p = e + 1;
			u16 b = 0;
			while( *p != 0 && *p != ',' )
			{
				size_t n = strcspn( p, "+," );
				for( auto & nm : names )
					if( strlen( nm.name ) == n && strncmp( nm.name, p, n ) == 0 )
						b |= nm.mask;
				p += n;
				if( *p == '+' )
					p++;
			}
			if( *p == ',' )
				p++;
			if( f >= at && f < at + 5 )
				held |= b;
		}
		return held;
	}

	// the N64 stick's range (about +-80 at the rim of a real controller)
	const int kStickRange = 80;
	const int kDeadzone   = 20;		// of 128
	const int kCThreshold = 64;		// right stick to a C button

	s8 StickValue( int raw )		// 0..255, middle 128
	{
		int v = raw - 128;
		if( v > -kDeadzone && v < kDeadzone )
			return 0;
		v = v > 0 ? v - kDeadzone : v + kDeadzone;
		v = v * kStickRange / ( 128 - kDeadzone );
		if( v > kStickRange )	v = kStickRange;
		if( v < -kStickRange )	v = -kStickRange;
		return (s8)v;
	}

	class IInputManager : public CInputManager
	{
	public:
		virtual u32				GetNumConfigurations() const						{ return 1; }
		virtual const char *	GetConfigurationName( u32 ) const					{ return "Default"; }
		virtual const char *	GetConfigurationDescription( u32 ) const			{ return "DualShock as an N64 controller"; }
		virtual void			SetConfiguration( u32 )								{}
		virtual u32				GetConfigurationFromName( const char * ) const		{ return 0; }

		virtual bool Initialise()	{ mx_pad_init(); return true; }
		virtual void Finalise()		{}
		virtual void GetState( OSContPad pPad[4] );
	};

	void IInputManager::GetState( OSContPad pPad[4] )
	{
		for( u32 cont = 0; cont < 4; cont++ )
		{
			pPad[cont].button = 0;
			pPad[cont].stick_x = 0;
			pPad[cont].stick_y = 0;
		}

		for( int port = 0; port < 2; port++ )
		{
			u32 b = mx_pad_buttons( port );
			u8 st[4];
			mx_pad_sticks( port, st );	// right x, right y, left x, left y

			u16 n = 0;
			if( b & MX_PAD_CROSS )		n |= A_BUTTON;
			if( b & MX_PAD_SQUARE )		n |= B_BUTTON;
			if( b & (MX_PAD_L2 | MX_PAD_R2) )	n |= Z_TRIG;
			if( b & MX_PAD_L1 )			n |= L_TRIG;
			if( b & MX_PAD_R1 )			n |= R_TRIG;
			if( b & MX_PAD_START )		n |= START_BUTTON;
			if( b & MX_PAD_UP )			n |= U_JPAD;
			if( b & MX_PAD_DOWN )		n |= D_JPAD;
			if( b & MX_PAD_LEFT )		n |= L_JPAD;
			if( b & MX_PAD_RIGHT )		n |= R_JPAD;
			if( b & MX_PAD_TRIANGLE )	n |= U_CBUTTONS;
			if( b & MX_PAD_CIRCLE )		n |= D_CBUTTONS;

			int rx = st[0] - 128, ry = st[1] - 128;
			if( rx < -kCThreshold )		n |= L_CBUTTONS;
			if( rx >  kCThreshold )		n |= R_CBUTTONS;
			if( ry < -kCThreshold )		n |= U_CBUTTONS;
			if( ry >  kCThreshold )		n |= D_CBUTTONS;

			if( port == 0 )
				n |= ScriptButtons( gPS2Frame );
			pPad[port].button  = n;
			pPad[port].stick_x = StickValue( st[2] );
			pPad[port].stick_y = (s8)-StickValue( st[3] );	// the N64's y grows upwards
		}
	}
}

template<> bool CSingleton< CInputManager >::Create()
{
	auto manager = std::make_shared<IInputManager>();
	if( !manager->Initialise() )
		return false;
	mpInstance = manager;
	return true;
}
