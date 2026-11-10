/*
Copyright (C) 2026 the ps2max authors

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.
*/

// Audio on the PS2: for now without sound (as the PSP's plugin with audio
// off): audio lists are acknowledged, the AI's buffers are dropped.

#include "Base/Types.h"
#include "Core/Memory.h"
#include "Core/RSP_HLE.h"
#include "HLEAudio/AudioPlugin.h"
#include "Interface/ConfigOptions.h"

EAudioPluginMode gAudioPluginEnabled( APM_DISABLED );

namespace
{
	class AudioPluginPS2 : public CAudioPlugin
	{
	public:
		virtual bool			StartEmulation()			{ return true; }
		virtual void			StopEmulation()				{}
		virtual void			DacrateChanged( int )		{}
		virtual void			LenChanged()				{}
		virtual u32				ReadLength()				{ return 0; }
		virtual EProcessResult	ProcessAList()
		{
			Memory_SP_SetRegisterBits( SP_STATUS_REG, SP_STATUS_HALT );
			return PR_COMPLETED;
		}
	};
}

std::unique_ptr<CAudioPlugin> CreateAudioPlugin()
{
	return std::make_unique<AudioPluginPS2>();
}
