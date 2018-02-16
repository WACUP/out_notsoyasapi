/*
 * ya_wfxx.c
 * Copyright (C) 2015-2016 Peter Belkner <pbelkner@snafu.de>
 *
 * This file is part of libya.
 *
 * libya is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * libya is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with libya.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <ya.h>
#include <ks.h>
#include <ksmedia.h>

void WFXXSetup(WAVEFORMATEXTENSIBLE *pwfxx, int samplerate, int numchannels,
    int bitspersamp, BOOL bSurround, BOOL bFloat)
{
  WAVEFORMATEX *pwfx=&pwfxx->Format;

  pwfx->wFormatTag=WAVE_FORMAT_EXTENSIBLE;
  pwfx->nChannels=numchannels;
  pwfx->nSamplesPerSec=samplerate;
  pwfx->wBitsPerSample=bitspersamp;
  pwfx->cbSize=(sizeof *pwfxx)-(sizeof *pwfx);
  ////
  pwfx->nBlockAlign=pwfx->nChannels*(pwfx->wBitsPerSample>>3);
  pwfx->nAvgBytesPerSec=pwfx->nSamplesPerSec*pwfx->nBlockAlign;
  ////
  pwfxx->SubFormat
      =bFloat?KSDATAFORMAT_SUBTYPE_IEEE_FLOAT:KSDATAFORMAT_SUBTYPE_PCM;
  pwfxx->Samples.wValidBitsPerSample=pwfx->wBitsPerSample;

  switch (pwfx->nChannels) {
  case 1:
    pwfxx->dwChannelMask
        =KSAUDIO_SPEAKER_MONO;
    break;
  case 2:
    pwfxx->dwChannelMask
        =KSAUDIO_SPEAKER_STEREO;
    break;
  case 3:
    pwfxx->dwChannelMask
        =SPEAKER_FRONT_LEFT
        |SPEAKER_FRONT_RIGHT
        |SPEAKER_LOW_FREQUENCY;
    break;
  case 4:
    pwfxx->dwChannelMask
        =bSurround?KSAUDIO_SPEAKER_SURROUND:KSAUDIO_SPEAKER_QUAD;
    break;
  case 5:
    pwfxx->dwChannelMask
        =SPEAKER_BACK_RIGHT
        |SPEAKER_FRONT_RIGHT
        |SPEAKER_FRONT_CENTER
        |SPEAKER_FRONT_LEFT
        |SPEAKER_BACK_LEFT;
    break;
  case 6:
    pwfxx->dwChannelMask
        =bSurround?KSAUDIO_SPEAKER_5POINT1_SURROUND:KSAUDIO_SPEAKER_5POINT1;
    break;
  case 8:
    pwfxx->dwChannelMask
        =bSurround?KSAUDIO_SPEAKER_7POINT1_SURROUND:KSAUDIO_SPEAKER_7POINT1;
    break;
  default:
    pwfxx->dwChannelMask=0;
    break;
  }
}

#if defined (YA_DEBUG) // {
const char *WFXXGetSubFormat(const WAVEFORMATEXTENSIBLE *pwfxx)
{
  if (IsEqualGUID(&KSDATAFORMAT_SUBTYPE_PCM,&pwfxx->SubFormat))
    return "KSDATAFORMAT_SUBTYPE_PCM";
  else if (IsEqualGUID(&KSDATAFORMAT_SUBTYPE_IEEE_FLOAT,&pwfxx->SubFormat))
    return "KSDATAFORMAT_SUBTYPE_IEEE_FLOAT";
  else if (IsEqualGUID(&KSDATAFORMAT_SUBTYPE_DRM,&pwfxx->SubFormat))
    return "KSDATAFORMAT_SUBTYPE_DRM";
  else if (IsEqualGUID(&KSDATAFORMAT_SUBTYPE_ALAW,&pwfxx->SubFormat))
    return "KSDATAFORMAT_SUBTYPE_ALAW";
  else if (IsEqualGUID(&KSDATAFORMAT_SUBTYPE_MULAW,&pwfxx->SubFormat))
    return "KSDATAFORMAT_SUBTYPE_MULAW";
  else if (IsEqualGUID(&KSDATAFORMAT_SUBTYPE_ADPCM,&pwfxx->SubFormat))
    return "KSDATAFORMAT_SUBTYPE_ADPCM";
  else
    return "UNKNOWN";
}

void WFXPrint(const WAVEFORMATEX *pwfx, FILE *f)
{
  const WAVEFORMATEXTENSIBLE *pwfxx=WAVE_FORMAT_EXTENSIBLE==pwfx->wFormatTag
      ?(const WAVEFORMATEXTENSIBLE *)pwfx
      :NULL;

  switch (pwfx->wFormatTag) {
  case WAVE_FORMAT_PCM:
    fputs("WAVE_FORMAT_PCM",f);
    break;
  case WAVE_FORMAT_EXTENSIBLE:
    fputs("WAVE_FORMAT_EXTENSIBLE",f);
    break;
  default:
    fputs("UNKNOWN",f);
    break;
  }

  fprintf(f,", channels: %d",pwfx->nChannels);
  fprintf(f,", rate: %d",pwfx->nSamplesPerSec);
  fprintf(f,", bps: %d",pwfx->wBitsPerSample);

  if (pwfxx) {
    /*
    if (IsEqualGUID(&KSDATAFORMAT_SUBTYPE_PCM,&pwfxx->SubFormat))
      fputs(", KSDATAFORMAT_SUBTYPE_PCM",f);
    else if (IsEqualGUID(&KSDATAFORMAT_SUBTYPE_IEEE_FLOAT,&pwfxx->SubFormat))
      fputs(", KSDATAFORMAT_SUBTYPE_IEEE_FLOAT",f);
    else if (IsEqualGUID(&KSDATAFORMAT_SUBTYPE_DRM,&pwfxx->SubFormat))
      fputs(", KSDATAFORMAT_SUBTYPE_DRM",f);
    else if (IsEqualGUID(&KSDATAFORMAT_SUBTYPE_ALAW,&pwfxx->SubFormat))
      fputs(", KSDATAFORMAT_SUBTYPE_ALAW",f);
    else if (IsEqualGUID(&KSDATAFORMAT_SUBTYPE_MULAW,&pwfxx->SubFormat))
      fputs(", KSDATAFORMAT_SUBTYPE_MULAW",f);
    else if (IsEqualGUID(&KSDATAFORMAT_SUBTYPE_ADPCM,&pwfxx->SubFormat))
      fputs(", KSDATAFORMAT_SUBTYPE_ADPCM",f);
    else
      fputs(", UNKNOWN",f);
      */
    fprintf(f,", %s",WFXXGetSubFormat(pwfxx));

    if (SPEAKER_FRONT_LEFT&pwfxx->dwChannelMask)
      fputs(", SPEAKER_FRONT_LEFT",f);

    if (SPEAKER_FRONT_RIGHT&pwfxx->dwChannelMask)
      fputs(", SPEAKER_FRONT_RIGHT",f);

    if (SPEAKER_FRONT_CENTER&pwfxx->dwChannelMask)
      fputs(", SPEAKER_FRONT_CENTER",f);

    if (SPEAKER_LOW_FREQUENCY&pwfxx->dwChannelMask)
      fputs(", SPEAKER_LOW_FREQUENCY",f);

    if (SPEAKER_BACK_LEFT&pwfxx->dwChannelMask)
      fputs(", SPEAKER_BACK_LEFT",f);

    if (SPEAKER_BACK_RIGHT&pwfxx->dwChannelMask)
      fputs(", SPEAKER_BACK_RIGHT",f);

    if (SPEAKER_FRONT_LEFT_OF_CENTER&pwfxx->dwChannelMask)
      fputs(", SPEAKER_FRONT_LEFT_OF_CENTER",f);

    if (SPEAKER_FRONT_RIGHT_OF_CENTER&pwfxx->dwChannelMask)
      fputs(", SPEAKER_FRONT_RIGHT_OF_CENTER",f);

    if (SPEAKER_BACK_CENTER&pwfxx->dwChannelMask)
      fputs(", SPEAKER_BACK_CENTER",f);

    if (SPEAKER_SIDE_LEFT&pwfxx->dwChannelMask)
      fputs(", SPEAKER_SIDE_LEFT",f);

    if (SPEAKER_SIDE_RIGHT&pwfxx->dwChannelMask)
      fputs(", SPEAKER_SIDE_RIGHT",f);

    if (SPEAKER_TOP_CENTER&pwfxx->dwChannelMask)
      fputs(", SPEAKER_TOP_CENTER",f);

    if (SPEAKER_TOP_FRONT_LEFT&pwfxx->dwChannelMask)
      fputs(", SPEAKER_TOP_FRONT_LEFT",f);

    if (SPEAKER_TOP_FRONT_CENTER&pwfxx->dwChannelMask)
      fputs(", SPEAKER_TOP_FRONT_CENTER",f);

    if (SPEAKER_TOP_FRONT_RIGHT&pwfxx->dwChannelMask)
      fputs(", SPEAKER_TOP_FRONT_RIGHT",f);

    if (SPEAKER_TOP_BACK_LEFT&pwfxx->dwChannelMask)
      fputs(", SPEAKER_TOP_BACK_LEFT",f);

    if (SPEAKER_TOP_BACK_CENTER&pwfxx->dwChannelMask)
      fputs(", SPEAKER_TOP_BACK_CENTER",f);

    if (SPEAKER_TOP_BACK_RIGHT&pwfxx->dwChannelMask)
      fputs(", SPEAKER_TOP_BACK_RIGHT",f);
  }
}
#endif // }
