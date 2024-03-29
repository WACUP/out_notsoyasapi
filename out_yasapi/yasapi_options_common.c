/*
 * yasapi_options_common.c
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
#include <yasapi.h>

///////////////////////////////////////////////////////////////////////////////
static const Property gcaCommonProperties[]={
  { L"id",&gcStringType,offsetof(OptionsCommon,szId),YASAPI_ID_SIZE },
  { L"mono2stereo",&gcIntType,offsetof(OptionsCommon,bMono2Stereo),0 },
#if defined (YASAPI_SURROUND) // {
  { L"surround",&gcIntType,offsetof(OptionsCommon,bSurround),0 },
#endif // }
  { L"volume",&gcIntType,offsetof(OptionsCommon,bVolume),0 },
#if defined (YASAPI_GAPLESS) // {
  { L"gapless",&gcIntType,offsetof(OptionsCommon,bGapless),0 },
  { L"disconnect",&gcIntType,offsetof(OptionsCommon,bDisconnect),0 },
  { L"gapless_offset",&gcIntType,offsetof(OptionsCommon,eTimeTag),0 },
#if defined (YASAPI_CHECK_UNDERFLOW) // {
  { L"check_underflow",&gcIntType,offsetof(OptionsCommon,nCheckUnderflow),0 },
#endif // }
#endif // }
  { L"format_supported",&gcIntType,offsetof(OptionsCommon,bFormatSupported),0 },
  { L"audioclock",&gcIntType,offsetof(OptionsCommon,bAudioClock),0 },
  { YASAPI_PROPERTY_VIS,&gcIntType,offsetof(OptionsCommon,bVisualization),0 },
  { YASAPI_PROPERTY_PAGE,&gcIntType,offsetof(OptionsCommon,nPage),0 },
  { NULL,NULL,0,0 }
};

static const Property gcaConfigProperties[]={
  { YASAPI_PROPERTY_PAGE,&gcIntType,offsetof(OptionsCommon,nPage),0 },
  { NULL,NULL,0,0 }
};

///////////////////////////////////////////////////////////////////////////////
void OptionsCommonLoad(OptionsCommon *pOptions, const wchar_t *pfx,
    const wchar_t *path)
{
  PropertyIOConfig c;

  c.path=path;
  c.pData=pOptions;
  c.pDefault=OptionsCommonDefault();
  c.group=YA_GROUP_COMMON;
#if 0 // {
  c.pfx=L"  ";
#else // } {
  c.pfx=pfx;
#endif // }
  PropertiesLoad(gcaCommonProperties,&c);
}

void OptionsCommonSave(OptionsCommon *pOptions, int mode, const wchar_t *path)
{
  PropertyIOConfig c;

  c.path=path;
  c.pData=pOptions;
  c.group=YA_GROUP_COMMON;
  // this change allows us to save only the config window
  // state if we are not doing a full configuration save.
  PropertiesSave((!mode ? gcaCommonProperties : gcaConfigProperties),&c);
}

///////////////////////////////////////////////////////////////////////////////
const OptionsCommon *OptionsCommonDefault(void)
{
  static int initialized;
  static OptionsCommon options;

  if (!initialized) {
    options.bMono2Stereo=TRUE;
	options.bVolume=TRUE;
#if defined (YASAPI_SURROUND) // {
    options.bSurround=FALSE;
#endif // }
#if defined (YASAPI_GAPLESS) // {
    options.bGapless=TRUE;
    options.bDisconnect=TRUE;
    options.eTimeTag=TIME_POSITION;
#if defined (YASAPI_CHECK_UNDERFLOW) // {
    options.nCheckUnderflow=500;
#endif // }
#endif // }
    options.bFormatSupported=FALSE;
    options.bAudioClock=TRUE;
    options.bVisualization=TRUE;
    options.nPage=0;

    initialized=1;
  }

  return &options;
}
