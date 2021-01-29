/*
 * yasapi_guid.h
 * Copyright (C) 2015-2016 Peter Belkner <pbelkner@snafu.de>
 *
 * This file is part of out_yasapi.
 *
 * out_yasapi is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * out_yasapi is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with out_yasapi.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef __YASAPI_GUID_H__ // {
#define __YASAPI_GUID_H__
#include <windows.h>
#include <commctrl.h>
#pragma component(browser, off, references)
#include <shlobj.h>
#pragma component(browser, on, references)
#include <audioclient.h>
#include <mmdeviceapi.h>
#include <mftransform.h>
#include <mferror.h>
#include <mfapi.h>
#define tagTIMECODE_SAMPLE ____tagTIMECODE_SAMPLE
#define TIMECODE_SAMPLE ____TIMECODE_SAMPLE
#define PTIMECODE_SAMPLE ____PTIMECODE_SAMPLE
#define _DDPIXELFORMAT _____DDPIXELFORMAT
#define DDPIXELFORMAT ____DDPIXELFORMAT
#define LPDDPIXELFORMAT ____LPDDPIXELFORMAT
#include <wmcodecdsp.h>
#undef LPDDPIXELFORMAT
#undef DDPIXELFORMAT
#undef _DDPIXELFORMAT
#undef PTIMECODE_SAMPLE
#undef TIMECODE_SAMPLE
#undef tagTIMECODE_SAMPLE
#endif // }
