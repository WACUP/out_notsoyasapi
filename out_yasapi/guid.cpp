/*
 * guid.cpp
 * Copyright (C) 2015 Peter Belkner <pbelkner@snafu.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301  USA
 */
#include <yasapi_guid.h>
#include <stdio.h>

#define PRINT_IID(id,f) \
  print_guid(__uuidof(I##id),"IID_I" #id,f)
#define PRINT_CLSID(id,f) \
  print_guid(__uuidof(id),"CLSID_" #id,f)

void print_guid(const GUID &guid, const char *name, FILE *f)
{
  fprintf(f,"DEFINE_GUID("
      "%s, "                  // IID_IAudioClient,
      "0x%02x%02x%02x%02x, "  // 0x1cb9ad4c,
      "0x%02x%02x, "          // 0xdbfa,
      "0x%02x%02x, "          // 0x4c32,
      "0x%02x,0x%02x,\n"       // 0xb1,0x78,
      "    0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x"
                              // 0xc2,0xf5,0x68,0xa7,0x03,0xb2
      ");\n",
      name,
      ////////
      ((const unsigned char *)&guid.Data1)[3],
      ((const unsigned char *)&guid.Data1)[2],
      ((const unsigned char *)&guid.Data1)[1],
      ((const unsigned char *)&guid.Data1)[0],
      ////////
      ((const unsigned char *)&guid.Data2)[1],
      ((const unsigned char *)&guid.Data2)[0],
      ////////
      ((const unsigned char *)&guid.Data3)[1],
      ((const unsigned char *)&guid.Data3)[0],
      ////////
      guid.Data4[0],
      guid.Data4[1],
      ////////
      guid.Data4[2],
      guid.Data4[3],
      guid.Data4[4],
      guid.Data4[5],
      guid.Data4[6],
      guid.Data4[7]);
}

int main(int argc, char **argv)
{
  PRINT_IID(Unknown,stdout);
  fputs("\n",stdout);

  PRINT_IID(AudioClient,stdout);
  PRINT_IID(AudioClock,stdout);
  PRINT_IID(AudioRenderClient,stdout);
  fputs("\n",stdout);

  PRINT_IID(MMNotificationClient,stdout);
  fputs("\n",stdout);

  PRINT_CLSID(MMDeviceEnumerator,stdout);
  PRINT_IID(MMDeviceEnumerator,stdout);
  PRINT_IID(MMDevice,stdout);
  fputs("\n",stdout);

  PRINT_CLSID(CResamplerMediaObject,stdout);
  PRINT_IID(MFTransform,stdout);
  PRINT_IID(WMResamplerProps,stdout);

  return 0;
}
