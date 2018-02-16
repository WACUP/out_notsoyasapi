/*
 * yasapi_options_device.c
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
static const Property gcaDeviceProperties[]={
  { L"share_mode",&gcIntType,offsetof(OptionsDevice,eShareMode),0 },
  { L"device_period",&gcIntType,offsetof(OptionsDevice,eDevicePeriod),0 },
  { L"autoconvertpcm",&gcIntType,offsetof(OptionsDevice,bAutoConvertPCM),0 },
  { L"src_default_quality",&gcIntType,
      offsetof(OptionsDevice,bSRCDefaultQuality),0 },
  { L"pull",&gcIntType,offsetof(OptionsDevice,bPull),0 },
#if defined (YASAPI_FORCE24BIT) // {
  { L"force24bit",&gcIntType,offsetof(OptionsDevice,bForce24Bit),0 },
#endif // }
#if defined (YASAPI_BALANCE) // {
  { L"balance",&gcIntType,offsetof(OptionsDevice,bBalance),0 },
  { L"balance_start",&gcDoubleType,offsetof(OptionsDevice,qBalanceStart),0 },
#endif // }
  { L"ring_size",&gcDoubleType,offsetof(OptionsDevice,ring.qSize),0 },
  { L"ring_fill",&gcDoubleType,offsetof(OptionsDevice,ring.qFill),0 },
#if defined (YASAPI_UNDERFLOW) // {
  { L"ring_underflow",&gcDoubleType,
      offsetof(OptionsDevice,ring.qUnderflow),0 },
#endif // }
  { L"share_size",&gcDoubleType,offsetof(OptionsDevice,qShareSize),0 },
  { NULL,NULL,0,0 }
};

///////////////////////////////////////////////////////////////////////////////
int OptionsDeviceLoad(OptionsDevice *pOptions, const wchar_t *pfx,
    const wchar_t *pstrId, const wchar_t *path)
{
  PropertyIOConfig c;

  /////////////////////////////////////////////////////////////////////////////
  c.path=path;
  c.pData=pOptions;
  c.pDefault=OptionsDeviceDefault();
  c.group=pstrId;
#if 0 // {
  c.pfx=L"    ";
#else // } {
  c.pfx=pfx;
#endif // }
  PropertiesLoad(gcaDeviceProperties,&c);

  if (YASAPI_MAX_RING_SIZE<pOptions->ring.qSize)
    pOptions->ring.qSize=YASAPI_MAX_RING_SIZE;

  if (pOptions->ring.qSize<pOptions->ring.qFill)
    pOptions->ring.qFill=pOptions->ring.qSize;

  if (YASAPI_MAX_SHARE_SIZE<pOptions->qShareSize)
    pOptions->qShareSize=YASAPI_MAX_SHARE_SIZE;

#if defined (YASAPI_UNDERFLOW) // {
  if (YASAPI_MAX_UNDERFLOW<pOptions->ring.qUnderflow)
    pOptions->ring.qUnderflow=YASAPI_MAX_UNDERFLOW;
#endif // }

  /////////////////////////////////////////////////////////////////////////////
  return 0;
}

void OptionsDeviceSave(OptionsDevice *pOptions, const wchar_t *pstrId,
    const wchar_t *path)
{
  PropertyIOConfig c;

  c.path=path;
  c.pData=pOptions;
  c.group=pstrId;
  PropertiesSave(gcaDeviceProperties,&c);
}

///////////////////////////////////////////////////////////////////////////////
const OptionsDevice *OptionsDeviceDefault(void)
{
  static int initialized;
  static OptionsDevice options;

  if (!initialized) {
    options.eShareMode=YASAPI_SHAREMODE_SHARE;
    options.eDevicePeriod=YASAPI_DEVICE_PERIOD_DEFAULT;
    options.bAutoConvertPCM=TRUE;
    options.bSRCDefaultQuality=FALSE;
    options.bPull=FALSE;
#if defined (YASAPI_FORCE24BIT) // {
    options.bForce24Bit=FALSE;
#endif // }
#if defined (YASAPI_BALANCE) // {
    options.bBalance=FALSE;
    options.qBalanceStart=0.5;
#endif // }
    options.qShareSize=7.0;
    options.ring.qSize=5.0;
    options.ring.qFill=1.0;
#if defined (YASAPI_UNDERFLOW) // {
    options.ring.qUnderflow=0.25;
#endif // }

    initialized=1;
  }

  return &options;
}
