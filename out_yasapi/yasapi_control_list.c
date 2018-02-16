/*
 * yasapi_config_list.c
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
#include <resource.h>

///////////////////////////////////////////////////////////////////////////////
////////////////
static const ControlCheckBoxConfig gcMono2StereoConfig=
  { IDC_CHECKBOX_MONO2STEREO,offsetof(OptionsCommon,bMono2Stereo),0,1,IDS_MONO_TO_STEREO };

////////////////
static const ControlCheckBoxConfig gcVolumeConfig=
  { IDC_CHECKBOX_VOLUME,offsetof(OptionsCommon,bVolume),0,1,IDS_USE_WINAMP_VOLUME };

#if defined (YASAPI_GAPLESS) // {
////////////////
static const ControlCheckBoxConfig gcGapless=
  { IDC_CHECKBOX_GAPLESS,offsetof(OptionsCommon,bGapless),0,1,IDS_GAPLESS_PLAYBACK };

////////////////
static const ControlCheckBoxConfig gcDisconnect=
  { IDC_CHECKBOX_DISCONNECT,offsetof(OptionsCommon,bDisconnect),0,1,IDS_DISCONNECT_WHEN_IDLE };

#if defined (YASAPI_CHECK_UNDERFLOW) // {
////////////////
static const ControlComboBoxList gcaCheckUnderflowList[]={
  { L"100 ms",100 },
  { L"200 ms",200 },
  { L"500 ms",500 },
  { L"1 sec",1000 },
  { L"2 sec",2000 },
  { NULL,0 }
};

static const ControlComboBoxConfig gcCheckUnderflowConfig=
  { IDC_COMBOBOX_CHECK_UNDERFLOW,offsetof(OptionsCommon,nCheckUnderflow),
      gcaCheckUnderflowList,
      L"In gapless mode, the time interval after which WASAPI should"
      L" automatically disconnect the device." };
#endif // }
#endif // }

#if defined (YASAPI_SORROUND) // {
static const ControlCheckBoxConfig gcSorround=
  { IDC_CHECKBOX_SORROUND,offsetof(OptionsCommon,bSorround),0,1,IDS_SURROUND_MODE };
#endif // }

////////////////
static const ControlCheckBoxConfig gcFormatSupported=
  { IDC_CHECKBOX_FORMAT_SUPPORTED,offsetof(OptionsCommon,bFormatSupported),0,1,IDS_TEST_FORMAT };

////////////////
static const ControlComboBoxList gcaAudioClockList[]={
  { IDS_WASAPI,1 },
  { IDS_SYSTEM,0 },
  { NULL,0 }
};

static const ControlComboBoxConfig gcAudioClockConfig=
  { IDC_COMBOBOX_CLOCK,offsetof(OptionsCommon,bAudioClock),gcaAudioClockList,IDS_CLOCK_MODE };

////////////////
static const ControlCheckBoxConfig gcVisualizationConfig=
  { IDC_CHECKBOX_VISUALIZATION,offsetof(OptionsCommon,bVisualization),0,1,IDS_VISUALISE_BUFFERS };

#if defined (YASAPI_GAPLESS) // {
static const ControlRadioButtonConfig gcTimeTagRadioButtons[]={
  { IDC_RADIOBUTTON_TIME_POSITION,offsetof(OptionsCommon,eTimeTag),TIME_POSITION,IDS_TIME_POSITION },
  { IDC_RADIOBUTTON_TIME_TIME,offsetof(OptionsCommon,eTimeTag),TIME_TIME,IDS_TIME_TIME },
  { 0,0,0,NULL }
};
#endif // }

////////////////
const Control gcaCommonControls[]={
  { &gcCheckBoxType,&gcMono2StereoConfig },
  { &gcCheckBoxType,&gcVolumeConfig },
#if defined (YASAPI_GAPLESS) // {
  { &gcCheckBoxType,&gcGapless },
  { &gcCheckBoxType,&gcDisconnect },
  { &gcRadioButtonType,gcTimeTagRadioButtons },
#if defined (YASAPI_CHECK_UNDERFLOW) // {
  { &gcComboBoxType,&gcCheckUnderflowConfig },
#endif // }
#endif // }
#if defined (YASAPI_SORROUND) // {
  { &gcCheckBoxType,&gcSorround },
#endif // }
  { &gcCheckBoxType,&gcFormatSupported },
  { &gcComboBoxType,&gcAudioClockConfig },
  { &gcCheckBoxType,&gcVisualizationConfig },
  { NULL,0 }
};

///////////////////////////////////////////////////////////////////////////////
static const ControlComboBoxList gcaShareModeList[]={
  { IDS_SHARE,YASAPI_SHAREMODE_SHARE },
  { IDS_EXCLUSIVE,YASAPI_SHAREMODE_EXCLUSIVE },
  { IDS_AUTOMATIC,YASAPI_SHAREMODE_AUTOMATIC },
  { NULL,0 }
};

static const ControlComboBoxConfig gcShareModeConfig=
  { IDC_COMBOBOX_SHAREMODE,offsetof(OptionsDevice,eShareMode),gcaShareModeList,IDS_SHAREMODE };

////////////////
static const ControlCheckBoxConfig gcAutoConvertPCMConfig=
  { IDC_CHECKBOX_AUTOCONVERT_PCM,offsetof(OptionsDevice,bAutoConvertPCM),FALSE,TRUE,IDS_AUTO_CONVERT_PCM };

////////////////
static const ControlCheckBoxConfig gcSRCDefaultQualityConfig=
  { IDC_CHECKBOX_SRC_DEFAULT_QUALITY,offsetof(OptionsDevice,bSRCDefaultQuality),FALSE,TRUE,IDS_SRC_DEFAULT_QUALITY };

////////////////
static const ControlComboBoxList gcaPullList[]={
  { IDS_PUSH,FALSE },
  { IDS_PULL,TRUE },
  { NULL,0 }
};

static const ControlComboBoxConfig gcPullConfig=
  { IDC_COMBOBOX_PULL,offsetof(OptionsDevice,bPull),gcaPullList,IDS_PUSH_OR_PULL };

#if defined (YASAPI_FORCE24BIT) // {
////////////////
static const ControlComboBoxList gcaForce24BitList[]={
  { IDS_NO,FALSE },
  { IDS_YES,TRUE },
  { NULL,0 }
};

static const ControlCheckBoxConfig gcForce24BitConfig=
  { IDC_CHECKBOX_FORMAT_FORCE24BIT,
      offsetof(OptionsDevice,bForce24Bit),FALSE,TRUE,IDS_16BIT_AS_24BIT };
#endif // }

#if defined (YASAPI_BALANCE) // {
static const ControlCheckBoxConfig gcBalanceConfig=
  { IDC_CHECKBOX_BALANCE,offsetof(OptionsDevice,bBalance),FALSE,TRUE,
      L"In push mode, whether YASAPI should balance the time interval"
      L" towards 0.5 of the frame buffer." };

////////////////
static const ControlSliderConfig gcBalanceStartConfig=
  { IDC_SLIDER_BALANCE_START,offsetof(OptionsDevice,qBalanceStart),
      0.0,1.0,0l,
      IDC_STATIC_BALANCE_START,
      L"Initial timer invocation after %0.2f of buffer duration",
      L"Initial time interval for balancing towards 0.5 of the frame buffer."
      L" In theory the time interval is 0.5 of the buffer duration,"
      L" in practice it may be much shorter." };
#endif // }

////////////////
const Control gcaDeviceControls[]={
  { &gcComboBoxType,&gcShareModeConfig },
  { &gcCheckBoxType,&gcAutoConvertPCMConfig },
  { &gcCheckBoxType,&gcSRCDefaultQualityConfig },
  { &gcComboBoxType,&gcPullConfig },
#if defined (YASAPI_FORCE24BIT) // {
  { &gcCheckBoxType,&gcForce24BitConfig },
#endif // }
#if defined (YASAPI_BALANCE) // {
  { &gcCheckBoxType,&gcBalanceConfig },
  { &gcSliderType,&gcBalanceStartConfig },
#endif // }
  { NULL,0 }
};

const Control gcaCoreDeviceControls[]={
  { &gcComboBoxType,&gcShareModeConfig },
  { &gcComboBoxType,&gcPullConfig },
  { NULL,0 }
};

///////////////////////////////////////////////////////////////////////////////
static const ControlComboBoxList gcaDevicePeriodList[]={
  { IDS_DEFAULT,YASAPI_DEVICE_PERIOD_DEFAULT },
  { IDS_MINIMUM,YASAPI_DEVICE_PERIOD_MINIMUM },
  { NULL,0 }
};

static const ControlComboBoxConfig gcDevicePeriodConfig=
  { IDC_COMBOBOX_DEVICE_PERIOD,offsetof(OptionsDevice,eDevicePeriod),gcaDevicePeriodList,IDS_DEVICE_PERIOD };

////////////////
static const ControlSliderConfig gcShareSizeConfig=
  { IDC_SLIDER_SHARE_SIZE,offsetof(OptionsDevice,qShareSize),
      YASAPI_MIN_SHARE_SIZE,YASAPI_MAX_SHARE_SIZE,2000l,
      IDC_STATIC_SHARE_SIZE,IDS_SLIDER_MINIMUM_DEFAULT,IDS_SHARE_SIZE };

static const ControlSliderCascadeList gcaBuffersList[]= {
  { IDC_SLIDER_RING_SIZE,offsetof(OptionsDevice,ring.qSize),
      IDC_STATIC_RING_SIZE,IDS_SLIDER_SHARED_BUFFER,IDS_RING_SIZE },
  { IDC_SLIDER_RING_FILL,offsetof(OptionsDevice,ring.qFill),
      IDC_STATIC_RING_FILL,IDS_SLIDER_PREBUFFER,IDS_RING_FILL },
  { 0,0,0,NULL,NULL }
};

static const ControlSliderCascadeConfig gcBuffersConfig=
  { YASAPI_MIN_RING_SIZE,YASAPI_MAX_RING_SIZE,2000,gcaBuffersList };

#if defined (YASAPI_UNDERFLOW) // {
static const ControlSliderConfig gcRingUnderflowConfig=
  { IDC_SLIDER_RING_UNDERFLOW,offsetof(OptionsDevice,ring.qUnderflow),
      YASAPI_MIN_UNDERFLOW,YASAPI_MAX_UNDERFLOW,100l,
      IDC_STATIC_RING_UNDERFLOW,L"Underflow: %0.2f \xD7 size of shared buffer",
      L"The number of samples which should always remain in the ring buffer,"
      L" i.e. when YASAPI shold stop playing." };
#endif // }

////////////////
const Control gcaBuffersControls[]={
  { &gcComboBoxType,&gcDevicePeriodConfig },
  { &gcSliderType,&gcShareSizeConfig },
  { &gcSliderCascadeType,&gcBuffersConfig },
#if defined (YASAPI_UNDERFLOW) // {
  { &gcSliderType,&gcRingUnderflowConfig },
#endif // }
  { NULL,0 }
};
