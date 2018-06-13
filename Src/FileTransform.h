/////////////////////////////////////////////////////////////////////////////
//    WinMerge:  an interactive diff/merge utility
//    Copyright (C) 1997-2000  Thingamahoochie Software
//    Author: Dean Grimm
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 3 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see <http://www.gnu.org/licenses/>.
/////////////////////////////////////////////////////////////////////////////
/**
 *  @file FileTransform.h
 *
 *  @brief Declaration of file transformations
 */ 
#pragma once

#include "Common/UniFile.h"
#include "Common/MyCom.h"

/**
 * @brief Unpacking/packing information for a given file
 *
 * @note Can be be copied between threads
 * Each thread really needs its own instance so that subcode is really defined
 * during the unpacking (open file) of the thread
 */
class PackingInfo
{
public:
	PackingInfo()
	: method(Default)
	, disallowMixedEOL(false)
	, readOnly(false)
	, canWrite(false)
	{
	}

	static UniFile *Default(PackingInfo *);
	static UniFile *XML(PackingInfo *);
	static UniFile *Plugin(PackingInfo *);

	/// "3rd path" where output saved if given
	String saveAsPath;
	/// text type to override syntax highlighting
	String textType;
	/// plugin moniker
	String pluginMoniker;
	/// plugin options
	CMyComPtr<IDispatch> pluginOptionsOwner;
	CMyComPtr<IDispatch> pluginOptions;
	/// custom UniFile
	UniFile *(*method)(PackingInfo *);
	bool disallowMixedEOL;
	bool readOnly;
	bool canWrite;
};
