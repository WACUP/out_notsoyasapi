#include <yasapi.h>
#include <nu/servicebuilder.h>
#include <wasabi/api/service/api_service.h>
#include <wasabi/api/memmgr/api_memmgr.h>
#include <Agave/Language/api_language.h>
#include <wasabi/api/service/waServiceFactory.h>
#include <loader/hook/get_api_service.h>
#include <loader/hook/squash.h>
#include <loader/loader/utils.h>
#include <resource.h>

#ifdef __cplusplus
extern "C" {
#endif

// TODO add to lang.h
// {6C82B613-30DD-4d1e-89D2-23672CD87679}
static const GUID OutNotSoYASAPILangGUID = 
{ 0x6c82b613, 0x30dd, 0x4d1e, { 0x89, 0xd2, 0x23, 0x67, 0x2c, 0xd8, 0x76, 0x79 } };

api_service* WASABI_API_SVC = NULL;
api_memmgr* WASABI_API_MEMMGR = NULL;
api_language* WASABI_API_LNG = NULL;
extern "C" HINSTANCE WASABI_API_LNG_HINST = 0;
HINSTANCE WASABI_API_ORIG_HINST = 0;

static wchar_t pluginTitle[256] = {0};

extern "C" void* safe_calloc(const size_t size)
{
	return WASABI_API_MEMMGR->sysMalloc(size);
}

extern "C" void __forceinline safe_free(void* ptr)
{
	WASABI_API_MEMMGR->sysFree(ptr);
}

extern "C" void SetupWasabiServices(Out_Module *_plugin)
{
	// load all of the required wasabi services from the winamp client
	if (WASABI_API_SVC == NULL)
	{
		WASABI_API_SVC = GetServiceAPIPtr();
	}
	if (WASABI_API_SVC != NULL)
	{
		if (WASABI_API_LNG == NULL)
		{
			ServiceBuild(WASABI_API_SVC, WASABI_API_LNG, languageApiGUID);

			WASABI_API_START_LANG_DESC(WASABI_API_LNG, _plugin->hDllInstance,
									   OutNotSoYASAPILangGUID, IDS_PLUGIN_NAME,
									   TEXT(PLUGIN_VERSION), &_plugin->description);
		}

		if (WASABI_API_MEMMGR == NULL)
		{
			ServiceBuild(WASABI_API_SVC, WASABI_API_MEMMGR, memMgrApiServiceGuid);
		}
	}
}

extern "C" LPWSTR GetLangStringBuf(const UINT id, LPWSTR buffer, const size_t buffer_len)
{
	return WASABI_API_LNGSTRINGW_BUF(id, buffer, buffer_len);
}

extern "C" LPWSTR GetLangString(const UINT id)
{
	return WASABI_API_LNGSTRINGW(id);
}

extern "C" LPWSTR GetLangStringDup(const UINT id)
{
	return WASABI_API_LNGSTRINGW_DUP(id);
}

extern "C" INT_PTR WADialogBoxParam(UINT id, HWND parent, DLGPROC proc, LPARAM param)
{
	return WASABI_API_DIALOGBOXPARAMW(id, parent, proc, param);
}

extern "C" HWND WACreateDialogParam(UINT id, HWND parent, DLGPROC proc, LPARAM param)
{
	return WASABI_API_CREATEDIALOGPARAMW(id, parent, proc, param);
}

extern "C" LPWSTR GetTextResource(const UINT id, LPWSTR* text)
{
	// the resource is utf-8 encoded so we convert
	// before passing it on to then be displayed
	*text = (LPWSTR)DecompressResourceText(WASABI_API_LNG_HINST,
						 WASABI_API_ORIG_HINST, id, NULL, true);
	return *text;
}

void __cdecl about(HWND hWndParent)
{
	wchar_t message[4096] = { 0 };

	const unsigned char* output = DecompressResourceText(WASABI_API_LNG_HINST, WASABI_API_ORIG_HINST, IDR_ABOUT_GZ, NULL, true);

	StringCchPrintf(message, ARRAYSIZE(message), (LPCWSTR)output, TEXT(PLUGIN_VERSION),
					TEXT(YASAPI_VERSION), WACUP_Author(), WACUP_Copyright(), TEXT(__DATE__));
	AboutMessageBox(hWndParent, message, GetLangString(IDS_ABOUT_TITLE));

	safe_free((void*)output);
}

extern "C" float safe_w_to_f(LPCWSTR str)
{
	return (float)WASABI_API_LNG->SafeWtofL(str);
}

extern "C" wchar_t* safe_wcsdup(LPCWSTR str)
{
	return WASABI_API_MEMMGR->sysDupStr((wchar_t*)str);
}

#ifdef __cplusplus
}
#endif