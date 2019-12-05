/*
 * yasapi.h
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
#ifndef __YASAPI_H__ // {
#define __YASAPI_H__
#include <ya.h>
#include <yasapi_guid.h>
#include <winamp/out.h>
#include <winamp/wa_cup.h>
#include <strsafe.h>

#ifdef __cpluplus
extern "C" {
#endif

extern Out_Module plugin;

///////////////////////////////////////////////////////////////////////////////
//#define YASAPI_ABOUT
#define YASAPI_GAPLESS
#define YASAPI_SURROUND
#define YASAPI_RING_REALLOC
#define YASAPI_RING_BUF_SIZE
#define YASAPI_FORCE24BIT
//#define YASAPI_EXECUTION_STATE
//#define YASAPI_READ_COUNT
//#define YASAPI_CONVERT_TIME
//#define YASAPI_CO_INITIALIZE
//#define YASAPI_CHANNEL_LAYOUT

#if defined (YASAPI_GAPLESS) // {
//#define YASAPI_CHECK_UNDERFLOW
#endif // }

#if defined (YASAPI_RING_REALLOC) // {
#define YASAPI_NOTIFY
#endif // }

///////////////////////////////////////////////////////////////////////////////
#if ! defined (YASAPI_VER) // {
  #define YASAPI_VER            1.7.25
  #define PLUGIN_VERSION        "1.1.5"
#endif // }

#define YASAPI_VERSION          YA_STR(YASAPI_VER)
#define PI_VER2                 "v" YASAPI_VERSION

#if defined (OUT_YASAPI_SFX) // {
  #define YASAPI_PREFIX         YA_CONCAT(out_yasapi,OUT_YASAPI_SFX)
#else // } {
  #define YASAPI_PREFIX         out_notsoyasapi
#endif // }

#define YASAPI_ID_BASE          65536
#define YASAPI_ID_OFFS          858

#if defined (YA_DEBUG) && defined (YA_SSE2) // {
  #define PI_VER                PI_VER2 " (Debug/SSE2)"
  #define YASAPI_ID             YASAPI_PREFIX ## -debug-sse2
  #define YASAPI_WA_ID          (YASAPI_ID_BASE+YASAPI_ID_OFFS+3)
#elif defined (YA_DEBUG) // {
  #define PI_VER                PI_VER2 " (Debug)"
  #define YASAPI_ID             YASAPI_PREFIX ## -debug
  #define YASAPI_WA_ID          (YASAPI_ID_BASE+YASAPI_ID_OFFS+0)
#elif defined (YA_SSE2) // } {
  #define PI_VER                PI_VER2 //" (SSE2)"
  #define YASAPI_ID             YASAPI_PREFIX ## -sse2
  #define YASAPI_WA_ID          (YASAPI_ID_BASE+YASAPI_ID_OFFS+1)
#else // } {
  #define PI_VER                PI_VER2
  #define YASAPI_ID             YASAPI_PREFIX
  #define YASAPI_WA_ID          (YASAPI_ID_BASE+YASAPI_ID_OFFS+2)
#endif // }

#define YASAPI_NAME             YA_STR(YASAPI_ID)
#define YASAPI_MODULE           YASAPI_NAME ".dll"
#define YASAPI_PROPERTY_FILE_NAME \
                                L"Plugins\\" YA_WSTR(YASAPI_ID) L".ini"
#define PI_LABEL                "Not So YASAPI Output " PI_VER

#define YASAPI_SHAREMODE_LABEL(eShareMode) \
    (AUDCLNT_SHAREMODE_SHARED==(eShareMode)?"SHARED":"EXCLUSIVE")

///////////////////////////////////////////////////////////////////////////////
#define YASAPI_FRAMES_MS(nFrames,pwfx) \
  MulDiv(1000,nFrames,(pwfx)->nSamplesPerSec)
#define YASAPI_FRAMES_TIMER(nFrames,pwfx) \
  (10000.0*1000*(nFrames)/(pwfx)->nSamplesPerSec)

#define YASAPI_MIN_BALANCE_COUNT  (8ull)

#define YASAPI_MIN_RING_SIZE      (1.0)
#define YASAPI_MAX_RING_SIZE      (40.0)

#define YASAPI_MIN_SHARE_SIZE     (1.0)
#define YASAPI_MAX_SHARE_SIZE     (20.0)

#if defined (YASAPI_UNDERFLOW) // {
#define YASAPI_MIN_UNDERFLOW      (0.0)
#define YASAPI_MAX_UNDERFLOW      (1.0)
#endif // }

///////////////////////////////////////////////////////////////////////////////
DEFINE_GUID(IID_IUnknown, 0x00000000, 0x0000, 0x0000, 0xc0,0x00,
    0x00,0x00,0x00,0x00,0x00,0x46);

DEFINE_GUID(IID_IAudioClient, 0x1cb9ad4c, 0xdbfa, 0x4c32, 0xb1,0x78,
    0xc2,0xf5,0x68,0xa7,0x03,0xb2);
DEFINE_GUID(IID_IAudioClock, 0xcd63314f, 0x3fba, 0x4a1b, 0x81,0x2c,
    0xef,0x96,0x35,0x87,0x28,0xe7);
DEFINE_GUID(IID_IAudioRenderClient, 0xf294acfc, 0x3146, 0x4483, 0xa7,0xbf,
    0xad,0xdc,0xa7,0xc2,0x60,0xe2);

DEFINE_GUID(CLSID_MMDeviceEnumerator, 0xbcde0395, 0xe52f, 0x467c, 0x8e,0x3d,
    0xc4,0x57,0x92,0x91,0x69,0x2e);
DEFINE_GUID(IID_IMMDeviceEnumerator, 0xa95664d2, 0x9614, 0x4f35, 0xa7,0x46,
    0xde,0x8d,0xb6,0x36,0x17,0xe6);
DEFINE_GUID(IID_IMMDevice, 0xd666063f, 0x1587, 0x4e43, 0x81,0xf1,
    0xb9,0x48,0xe8,0x07,0x36,0x3f);

DEFINE_GUID(IID_IMMNotificationClient, 0x7991eec9, 0x7e89, 0x4d85, 0x83,0x90,
    0x6c,0x70,0x3c,0xec,0x60,0xc0);
#if 0 // {
DEFINE_GUID(CLSID_MMPlayerNotificationClient, 0x1e45b183,0x7929,0x40e1,
    0xB1,0x9B,0x93,0x83,0x2A,0x44,0x49,0x24);
#endif // }

///////////////////////////////////////////////////////////////////////////////
#if defined (YASAPI_CHANNEL_LAYOUT) // {
typedef enum _ChannelLayout ChannelLayout;
#endif // }
typedef enum _PlayerState PlayerState;

typedef struct _RingIOError RingIOError;
typedef struct _Ring Ring;

typedef enum _TimeTag TimeTag;
typedef struct _OptionsCommon OptionsCommon;
typedef struct _OptionsDevice OptionsDevice;
typedef struct _Options Options;

typedef struct _Connection Connection;
typedef union _TimeSegment TimeSegment;
typedef struct _Time Time;
typedef struct _PlayerDevice PlayerDevice;
#if defined (YASAPI_FORCE24BIT) // {
typedef struct _Convert Convert;
#endif // }
typedef struct _PlayConfig PlayConfig;

typedef struct _Player Player;
typedef struct _Strategy Strategy;
typedef struct _Disconnect Disconnect;

typedef void DisconnectProc(Player *pPlayer, int bReset);

///////////////////////////////////////////////////////////////////////////////
#define YASAPI_PROPERTY_VIS     L"visualization"
#define YASAPI_PROPERTY_PAGE    L"page"

///////////////////////////////////////////////////////////////////////////////
extern const Control gcaCommonControls[];
extern const Control gcaDeviceControls[];
extern const Control gcaCoreDeviceControls[];
extern const Control gcaBuffersControls[];

///////////////////////////////////////////////////////////////////////////////
int gcd(int m, int n);

#if defined (YASAPI_CHANNEL_LAYOUT) // {
///////////////////////////////////////////////////////////////////////////////
enum _ChannelLayout {
  CHANNEL_LAYOUT_NULL,
  CHANNEL_LAYOUT_MONO,
  CHANNEL_LAYOUT_STEREO,
  CHANNEL_LAYOUT_3POINT1,             // CHANNEL_LAYOUT_QUAD
  CHANNEL_LAYOUT_3POINT1_SURROUND,    // CHANNEL_LAYOUT_SURROUND
  CHANNEL_LAYOUT_5POINT1,
  CHANNEL_LAYOUT_5POINT1_SURROUND,
  CHANNEL_LAYOUT_7POINT1,
  CHANNEL_LAYOUT_7POINT1_SURROUND,
  CHANNEL_LAYOUT_9POINT1,
  CHANNEL_LAYOUT_9POINT1_SURROUND
};
#endif // }

///////////////////////////////////////////////////////////////////////////////
typedef void (*RingCopy)(PVOID pClient, PVOID Destination,
    const VOID *Source, SIZE_T Length);

enum {
  RING_COPY=1,
  RING_COMMIT=2
};

struct _RingIOError {
  void (*Cleanup)(RingIOError *pError);
  //IAudioRenderClient *pRender;
  Player *pPlayer;
  UINT32 uFramesWrite;
};

struct _Ring {
  RingCopy Copy;
#if defined (YASAPI_FORCE24BIT) // {
  int nNumerator;
  int nDenominator;
#else // } {
  SIZE_T nShift;
#endif // }
  PVOID pClient;
  LPSTR pRep;
  LPSTR pMax;
  LPSTR pHead;
  LPSTR pTail;
  SIZE_T dwWritten;
  SIZE_T dwAvailable;
};

#if defined (YASAPI_FORCE24BIT) // {
DWORD RingCreate(Ring *pRing, SIZE_T dwSize, int nNumerator, int nDenominator);
#else // } {
DWORD RingCreate(Ring *pRing, SIZE_T dwSize, SIZE_T nShift);
#endif // }
void RingDestroy(Ring *pRing);

DWORD RingSourceSize(Ring *pRing, DWORD dwTargetSize);
DWORD RingTargetSize(Ring *pRing, DWORD dwSourceSize);

int RingGetSize(Ring *pRing);
void RingReset(Ring *pRing);
#if defined (YASAPI_RING_REALLOC) // {
int RingRealloc(Ring *pRing, SIZE_T uSize);
int RingReallocAvailable(Ring *pRing, SIZE_T dwAvailable);
#endif // }
DWORD RingWrite(Ring *pRing, LPCSTR pData, SIZE_T dwSize);
DWORD RingReadEx(Ring *pRing, LPSTR pData, SIZE_T dwSize, UINT uFlags,
    RingIOError *pError);
DWORD RingRead(Ring *pRing, LPSTR pData, SIZE_T dwSize, RingIOError *pError);

void RingCopyMemory(PVOID *Client, PVOID Destination, const VOID *Source,
    SIZE_T Length);
#if defined (YA_DEBUG) // {
void RingDump(Ring *pRing, SIZE_T dwSize);
#endif // }

///////////////////////////////////////////////////////////////////////////////
enum _OptionsSize {
  YASAPI_ID_SIZE=128
};

enum _TimeTag {
  TIME_POSITION,
  TIME_TIME
};

struct _OptionsCommon {
  // Persistent fields have to be of type "int", "double" or "wchar_t[]".
  wchar_t szId[YASAPI_ID_SIZE];
  int bMono2Stereo;
  int bVolume;
#if defined (YASAPI_SURROUND) // {
  int bSurround;
#endif // }
#if defined (YASAPI_GAPLESS) // {
  int bGapless;
  int bDisconnect;
  TimeTag eTimeTag;
#if defined (YASAPI_CHECK_UNDERFLOW) // {
  int nCheckUnderflow;
#endif // }
#else // } {
  int nWriteBlock;
#endif // }
  int bFormatSupported;
  int bAudioClock;
  int bVisualization;
  int nPage;
#if defined (YASAPI_ABOUT) // {
  int nConfigX;
  int nConfigY;
  int nAboutX;
  int nAboutY;
#else // } {
  int nPosX;
  int nPosY;
#endif // }
};

int OptionsCommonLoad(OptionsCommon *pOptions, const wchar_t *pfx,
    const wchar_t *path);
void OptionsCommonSave(OptionsCommon *pOptions, int mode, const wchar_t *path);
const OptionsCommon *OptionsCommonDefault(void);

///////////////////////////////////////////////////////////////////////////////
enum _OptionsSharemode {
  YASAPI_SHAREMODE_SHARE,
  YASAPI_SHAREMODE_EXCLUSIVE,
  YASAPI_SHAREMODE_AUTOMATIC
};

enum _OptionsDevicePeriod {
  YASAPI_DEVICE_PERIOD_DEFAULT,
  YASAPI_DEVICE_PERIOD_MINIMUM,
};

struct _OptionsDevice {
  // Persistent fields have to be of type "int", "double" or "wchar_t[]".
  int eShareMode;
  int eDevicePeriod;
  int bAutoConvertPCM;
  int bSRCDefaultQuality;
  int bPull;
#if defined (YASAPI_FORCE24BIT) // {
  int bForce24Bit;
#endif // }
  double qShareSize;

  struct {
    double qSize;
    double qFill;
  } ring;
};

int OptionsDeviceLoad(OptionsDevice *pOptions, const wchar_t *pfx,
    const wchar_t *pstrId, const wchar_t *path);
void OptionsDeviceSave(OptionsDevice *pOptions, const wchar_t *pstrId,
    const wchar_t *path);
const OptionsDevice *OptionsDeviceDefault(void);

///////////////////////////////////////////////////////////////////////////////
struct _Options {
  const wchar_t *path;
  OptionsCommon common;
  OptionsDevice device;
};

///////////////////////////////////////////////////////////////////////////////
struct _Connection {
#if defined (YASAPI_NOTIFY) // {
  int bInvalid;
#endif // }
  UINT32 uFrames;
  UINT32 uFramesMin;
  LONGLONG time;
#if defined (YASAPI_EXECUTION_STATE) // {
  EXECUTION_STATE eEsPrevFlags;
#endif // }
  IAudioClient *pClient;
  IAudioRenderClient *pRender;
  IAudioClock *pClock;
};

int ConnectionIsInvalid(Connection *pConnect);
int ConnectionSetInvalid(Connection *pConnect, int bInvalid);

int ConnectionGetPosition(Connection *pConnect, UINT64 *pu64Position);
int ConnectionGetFrequency(Connection *pConnect, UINT64 *pu64Frequency);

///////////////////////////////////////////////////////////////////////////////
union _TimeSegment {
  UINT64 u64Position;
  double xSeconds;
};

void TimeSegmentReset(TimeSegment *pSegment, UINT64 u64Frequency);
void TimeSegmentAdd(TimeSegment *pOffset, TimeSegment *pCurrent,
    UINT64 u64Frequency);
#if 0 // {
void TimeSegmentSub(TimeSegment *pOffset, TimeSegment *pCurrent,
    UINT64 u64Frequency);
#endif // }
int TimeSegmentGet(TimeSegment *pSegment, UINT64 u64Frequency,
    Connection *pConnect);

struct _Time {
  // time is pause + current - gapless
  UINT64 u64Frequency;
  TimeSegment gapless;
  TimeSegment pause;
  TimeSegment current;
};

int TimeReset(Time *pTime, TimeTag eTimeTag, Connection *pConnect);
int TimePause(Time *pTime, Connection *pConnect);
void TimeFlush(Time *pTime);
int TimeGapless(Time *pTime, Connection *pConnect);
#if defined (YASAPI_CONVERT_TIME) // {
int TimeMigrate(Time *pTime, TimeTag eTimeTag, Connection *pConnect);
#else // } {
int TimeMigrate(Time *pTime, Connection *pConnect);
#endif // }
int TimeGetMS(Time *pTime, Connection *pConnect, double *pms);

///////////////////////////////////////////////////////////////////////////////
struct _PlayerDevice {
  IMMDevice *pDevice;
  wchar_t szId[YASAPI_ID_SIZE];
};

int PlayerDeviceCreate(PlayerDevice *pPlayerDevice, LPCWSTR pstrId,
    IMMDeviceEnumerator *pEnumerator);
int PlayerDeviceDestroy(PlayerDevice *pPlayerDevice);
int PlayerDeviceGet(PlayerDevice *pPlayerDevice,
    IMMDeviceEnumerator *pEnumerator);

int PlayerDeviceCreateV(Player *pPlayer, Request *pRequst);
int PlayerDeviceDestroyV(Player *pPlayer, Request *pRequst);

///////////////////////////////////////////////////////////////////////////////
struct _PlayConfig {
  DWORD dwWrite;
  DWORD dwSize;
  DWORD dwWriteSize;
  UINT32 uFramesWrite;
};

#if defined (YASAPI_FORCE24BIT) // {
///////////////////////////////////////////////////////////////////////////////
struct _Convert {
  int nBytesPerFrame;
  int nBytesPerSample;
  int nChannels;
};
#endif // }

///////////////////////////////////////////////////////////////////////////////
#if defined (YASAPI_FORCE24BIT) // {
#define YASAPI_MAX_VOLUME 255
#endif // }

enum _PlayerState {
  PLAYER_STATE_NULL,        // not initialized
  PLAYER_STATE_BASE,        // base (i.e. non-WASAPI) objects available
  PLAYER_STATE_RUN,         // IMMDeviceEnumerator available
  PLAYER_STATE_PREPARED,    // Ring available
  PLAYER_STATE_PAUSE,       // possibly unconnected, not awaiting data
  PLAYER_STATE_UNDERFLOW,   // possibly unconnected, awaiting data
  PLAYER_STATE_CONNECTED,   // connected, ready to play
  PLAYER_STATE_PLAY,        // connected, playing
  // 1. client indicates draining.
  PLAYER_STATE_DRAIN_1,
  // 2. ring buffer is empty (drained).
  PLAYER_STATE_DRAIN_2,
  // 3. player awaits read request in order to switch to eot.
  PLAYER_STATE_DRAIN_3,
  // end of track.
  PLAYER_STATE_EOT
};

struct _Player {
  Options options;
  volatile HWND hDlgConfig;
  volatile PlayerState state;
  volatile int stamp;

  struct {
    //HINSTANCE hModule;
    //wchar_t aszModuleName[MAX_PATH];
    const wchar_t *pszFileName;
#if defined (YASAPI_FORCE24BIT) // {
    int nVolume;
#else // } {
    double qVolume;
#endif // }
    HANDLE hEvent;

    struct {
      HANDLE hRead;
#if defined (YASAPI_READ_COUNT) // {
      int nRead;
#endif // }
#if defined (YASAPI_CHECK_UNDERFLOW) // {
      HANDLE hUnderflow;
#endif // }
    } timer;

    PlayerStub *pStub;
  } base;

  struct {
    IMMDeviceEnumerator *pEnumerator;
#if defined (YASAPI_NOTIFY) // {
    IMMNotificationClient *pNotify;
#endif // }
  } run;

  PlayerDevice device;

  struct {
    const Strategy *pStrategy;
    const Disconnect *pDisconnect;
    WAVEFORMATEXTENSIBLE wfxx;
    WORD wBytesPerSample;
#if defined (YASAPI_SURROUND) // {
    int bSurround;
#endif // }
#if defined (YASAPI_FORCE24BIT) // {
    Convert source;
    Convert target;
#else // } {
    int nShift;
#endif // }
    AUDCLNT_SHAREMODE eShareMode;
    DWORD dwStreamFlags;
    REFERENCE_TIME hnsDevicePeriod;
#if 0 // {
#if defined (YASAPI_EXECUTION_STATE) // {
    EXECUTION_STATE ePrevState;
#endif // }
#endif // }
    Ring ring;
#if defined (YASAPI_RING_REALLOC) // {
    DWORD dwBufSize;
#endif // }
  } open;

  Connection connect;
  Time time;
};

#if 0 // {
int PlayerCreate(Player *pPlayer, HINSTANCE hModule, const wchar_t *path);
void PlayerDestroy(Player *pPlayer);
void PlayerKill(Player *pPlayer, int bReset, const PlayerState state);

int64_t PlayerRun(Player *pPlayer, Request *pRequest);
int64_t PlayerOpen(Player *pPlayer, Request *pRequest);
#if defined (YASAPI_GAPLESS) // {
int64_t PlayerReset(Player *pPlayer, Request *pRequest);
#endif // }
int64_t PlayerClose(Player *pPlayer, Request *pRequest);
int64_t PlayerCanWrite(Player *pPlayer, Request *pRequest);
int64_t PlayerWrite(Player *pPlayer, Request *pRequest);
int64_t PlayerFlush(Player *pPlayer, Request *pRequest);
int64_t PlayerIsPlaying(Player *pPlayer, Request *pRequest);
int64_t PlayerPause(Player *pPlayer, Request *pRequest);
int64_t PlayerGetTime(Player *pPlayer, Request *pRequest);
int64_t PlayerRead(Player *pPlayer, Request *pRequest);
#if defined (YASAPI_CHECK_UNDERFLOW) // {
int64_t PlayerShutdownUnderflow(Player *pPlayer, Request *pRequest);
#endif // }
#if defined (YASAPI_NOTIFY) // {
int64_t PlayerMigrate(Player *pPlayer, Request *pRequest);
#endif // }

int64_t PlayerSend(Player *pPlayer, int exit, PlayerProc *pPlayerProc, ...);
void PlayerPost(Player *pPlayer, int exit, PlayerProc *pPlayerProc);
void PlayerPostRead(void *data);

UINT32 PlayerGetFramesPadding(Player *pPlayer);

HWND PlayerGetDlgConfig(Player *pPlayer);
void PlayerPostUpdate(Player *pPlayer, WORD wRing, WORD wShared);

void PlayerPostRead(void *data);
VOID CALLBACK PlayerFireRead(LPVOID lpArg, DWORD dwLow, DWORD dwHigh);
void PlayerSetTimerRead(Player *pPlayer, LONGLONG time);
#if defined (YASAPI_CHECK_UNDERFLOW) // {
void PlayerSetTimerCheckUnderflow(Player *pPlayer);
#endif // }

DWORD WINAPI PlayerThread(LPVOID lpParameter);

void PlayerCopyMemory(PVOID p, PVOID Destination, const VOID *Source,
    SIZE_T Length);

#if defined (YASAPI_NOTIFY) // {
int PlayerAddNotify(Player *pPlayer);
void PlayerRemoveNotify(Player *pPlayer);
#endif // }
#else // } {
#if defined (YA_DEBUG) // {
  #define PLAYER_SEND(pPlayer,pPlayerProc,...) \
      PlayerSend(pPlayer,#pPlayerProc,pPlayerProc,__VA_ARGS__)
  #define PLAYER_POST(pPlayer,pPlayerProc) \
      PlayerPost(pPlayer,#pPlayerProc,pPlayerProc)
#else // } {
  #define PLAYER_SEND(pPlayer,pPlayerProc,...) \
      PlayerSend(pPlayer,pPlayerProc,__VA_ARGS__)
  #define PLAYER_POST(pPlayer,pPlayerProc) \
      PlayerPost(pPlayer,pPlayerProc)
#endif // }

int PlayerCreate(Player *pPlayer, HINSTANCE hModule, const wchar_t *path);
void PlayerDestroy(Player *pPlayer);
int PlayerRun(Player *pPlayer, Request *pRequest);
int PlayerCreateConnect(Player *pPlayer, int bNegociate, int bReset);
int PlayerCreateWFXX(Player *pPlayer, Request *pRequest);
int PlayerCreateRing(Player *pPlayer);
int PlayerReallocRing(Player *pPlayer);
int PlayerGetMaxLatency(Player *pPlayer);
int PlayerOpen(Player *pPlayer, Request *pRequest);
#if defined (YASAPI_NOTIFY) // {
int PlayerMigrate(Player *pPlayer, Request *pRequest);
#endif // }
void PlayerShutdown(Player *pPlayer);
void PlayerDisconnect(Player *pPlayer);
#if 0 // {
void PlayerDestroyEventSource(Player *pPlayer);
void PlayerDestroyConnect(Player *pPlayer);
#endif // }
#if defined (YASAPI_GAPLESS) // {
int PlayerReset(Player *pPlayer, Request *pRequest);
#endif // }
int PlayerClose(Player *pPlayer, Request *pRequest);
int PlayerCanWrite(Player *pPlayer, Request *pRequest);
int PlayerPlay(Player *pPlayer, UINT32 uFramesPadding, int bUnderflow);
int PlayerTryStart(Player *pPlayer, int bUnderflow);
int PlayerWrite(Player *pPlayer, Request *pRequest);
int PlayerFlush(Player *pPlayer, Request *pRequest);
int PlayerIsPlaying(Player *pPlayer, Request *pRequest);
int PlayerPause(Player *pPlayer, Request *pRequest);
int PlayerGetTime(Player *pPlayer, Request *pRequest);
int PlayerRead(Player *pPlayer, Request *pRequest);
#if defined (YASAPI_CHECK_UNDERFLOW) // {
int PlayerShutdownUnderflow(Player *pPlayer, Request *pRequest);
#endif // }
UINT32 PlayerGetFramesPadding(Player *pPlayer);
void PlayerKill(Player *pPlayer, int bReset, const PlayerState state);
int PlayerKillV(Player *pPlayer, Request *pRequest);
HWND PlayerGetDlgConfig(Player *pPlayer);
void PlayerPostUpdate(Player *pPlayer, WORD wRing, WORD wShared);
#if defined (YA_DEBUG) // {
int PlayerSend(Player *pPlayer, const char *id, PlayerProc *pPlayerProc, ...);
void PlayerPost(Player *pPlayer, const char *id, PlayerProc *pPlayerProc);
#else // } {
int PlayerSend(Player *pPlayer, PlayerProc *pPlayerProc, ...);
void PlayerPost(Player *pPlayer, PlayerProc *pPlayerProc);
#endif // }
void PlayerPostRead(void *data);
VOID CALLBACK PlayerFireRead(LPVOID lpArg, DWORD dwLow, DWORD dwHigh);
void PlayerSetTimerRead(Player *pPlayer, LONGLONG time);
void PlayerSetThreadCharacteristics(Player *pPlayer, HANDLE *pTask);
void PlayerRevertThreadCharacteristics(Player *pPlayer, HANDLE *pTask);
#if defined (YASAPI_CHECK_UNDERFLOW) // {
VOID CALLBACK PlayerFireShutdownUnderflow(LPVOID lpArg, DWORD dwLow,
    DWORD dwHigh);
void PlayerSetTimerCheckUnderflow(Player *pPlayer);
#endif // }
HANDLE PlayerGetEvent(Player *pPlayer);
int PlayerGetStamp(Player *pPlayer);
//DWORD WINAPI PlayerThread(LPVOID lpParameter);
void PlayerCopyMemory(PVOID p, PVOID Destination, const VOID *Source,
    SIZE_T Length);
#endif // }

///////////////////////////////////////////////////////////////////////////////
struct _Strategy {
  HRESULT (*Initialize)(Player *pPlayer);
  LONGLONG (*Interval)(Player *pPlayer, LONGLONG time);
  void (*SetTimer)(Player *pPlayer, LONGLONG time, BOOL bFlush);
  BOOL (*NeedDevicePeriod)(Player *pPlayer);
  BOOL (*NeedPadding)(Player *pPlayer);
  HRESULT (*SetEvent)(Player *pPlayer);
  HANDLE (*GetEvent)(Player *pPlayer);
  UINT32 (*GetFramesPadding)(Player *pPlayer);
};

extern const Strategy gcStrategyPush;
extern const Strategy gcStrategyPull;

///////////////////////////////////////////////////////////////////////////////
struct _Disconnect {
  void (*Disconnect)(Player *pPlayer, int bReset);
  int (*Reconnect)(Player *pPlayer);
  void (*Destroy)(Player *pPlayer, int bReset);
};

extern const Disconnect gcDisconnectNo;
extern const Disconnect gcDisconnectYes;

///////////////////////////////////////////////////////////////////////////////
enum {
  WM_CONFIG_UPDATE=WM_APP
};

int ConfigDialog(Player *pPlayer/*, HINSTANCE hInstance*/, HWND hWndParent);
#if defined (YASAPI_ABOUT) // {
void AboutDialog(Player *pPlayer, HINSTANCE hInstance, HWND hWndParent);
#else // } {
void AboutDialog(HWND hWndParent);
#endif // }

#ifdef __cpluplus
}
#endif
#endif // }
