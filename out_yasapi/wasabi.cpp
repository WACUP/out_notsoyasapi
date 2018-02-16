#include <yasapi.h>
#include <nu/servicebuilder.h>
#include <wasabi/api/service/api_service.h>
#include <Agave/Language/api_language.h>
#include <wasabi/api/service/waServiceFactory.h>
#include <resource.h>

// TODO add to lang.h
// {6C82B613-30DD-4d1e-89D2-23672CD87679}
static const GUID OutNotSoYASAPILangGUID = 
{ 0x6c82b613, 0x30dd, 0x4d1e, { 0x89, 0xd2, 0x23, 0x67, 0x2c, 0xd8, 0x76, 0x79 } };

api_service* WASABI_API_SVC = NULL;
api_language* WASABI_API_LNG = NULL;
HINSTANCE WASABI_API_LNG_HINST = 0, WASABI_API_ORIG_HINST = 0;

static wchar_t pluginTitle[256] = {0};

extern "C" void SetupWasabiServices(Out_Module *plugin)
{
	// load all of the required wasabi services from the winamp client
	if (WASABI_API_SVC == NULL)
	{
		/*WASABI_API_SVC = GetServiceAPIPtr();/*/
		WASABI_API_SVC = reinterpret_cast<api_service*>(SendMessage(plugin->hMainWindow, WM_WA_IPC, 0, IPC_GET_API_SERVICE));
		if (WASABI_API_SVC == reinterpret_cast<api_service*>(1)) WASABI_API_SVC = NULL;/**/
	}
	if (WASABI_API_SVC != NULL)
	{
		if (WASABI_API_LNG == NULL)
		{
			ServiceBuild(WASABI_API_SVC, WASABI_API_LNG, languageApiGUID);
			WASABI_API_START_LANG(plugin->hDllInstance, OutNotSoYASAPILangGUID);

			StringCchPrintf(pluginTitle, ARRAYSIZE(pluginTitle), WASABI_API_LNGSTRINGW(IDS_PLUGIN_NAME), TEXT(PLUGIN_VERSION));
			plugin->description = (char*)pluginTitle;
		}
	}
}

extern "C" LPWSTR GetLangString(UINT id)
{
	return WASABI_API_LNGSTRINGW(id);
}

extern "C" INT_PTR WADialogBoxParam(UINT id, HWND parent, DLGPROC proc, LPARAM param)
{
	return WASABI_API_DIALOGBOXPARAMW(id, parent, proc, param);
}

extern "C" HWND WACreateDialogParam(UINT id, HWND parent, DLGPROC proc, LPARAM param)
{
	return WASABI_API_CREATEDIALOGPARAMW(id, parent, proc, param);
}

extern "C" LPWSTR GetTextResource(UINT id)
{
	DWORD data_size = 0;
	return (LPWSTR)WASABI_API_LOADRESFROMFILEW(L"TEXT", MAKEINTRESOURCE(id), &data_size);
}