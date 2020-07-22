#include "stdafx.h"
#include "WinRing0.h"

HMODULE g_hWinRing0;
WinRing0_InitializeOls InitializeOls;
WinRing0_DeinitializeOls DeinitializeOls;
WinRing0_Rdmsr Rdmsr;
WinRing0_Wrmsr Wrmsr;
WinRing0_ReadPciConfigWordEx ReadPciConfigWordEx;
WinRing0_ReadPciConfigDwordEx ReadPciConfigDwordEx;
WinRing0_FindPciDeviceById FindPciDeviceById;
WinRing0_FindPciDeviceByClass FindPciDeviceByClass;

BOOL LoadWinRing0(LPCTSTR FileName)
{
	BOOL bResult;
	bResult = FALSE;
	g_hWinRing0 = LoadLibrary(FileName);
	if(g_hWinRing0)
	{
		InitializeOls = (WinRing0_InitializeOls)GetProcAddress(g_hWinRing0, "InitializeOls");
		DeinitializeOls = (WinRing0_DeinitializeOls)GetProcAddress(g_hWinRing0, "DeinitializeOls");
		Rdmsr = (WinRing0_Rdmsr)GetProcAddress(g_hWinRing0, "Rdmsr");
		Wrmsr = (WinRing0_Wrmsr)GetProcAddress(g_hWinRing0, "Wrmsr");
		ReadPciConfigWordEx = (WinRing0_ReadPciConfigWordEx)GetProcAddress(g_hWinRing0, "ReadPciConfigWordEx");
		ReadPciConfigDwordEx = (WinRing0_ReadPciConfigDwordEx)GetProcAddress(g_hWinRing0, "ReadPciConfigDwordEx");
		FindPciDeviceById = (WinRing0_FindPciDeviceById)GetProcAddress(g_hWinRing0, "FindPciDeviceById");
		FindPciDeviceByClass = (WinRing0_FindPciDeviceByClass)GetProcAddress(g_hWinRing0, "FindPciDeviceByClass");
		if(InitializeOls)
		{
			if(InitializeOls())
				bResult = TRUE;
		}
	}
	return bResult;
}

void UnloadWinRing0()
{
	if(DeinitializeOls)
		DeinitializeOls();
	if(g_hWinRing0)
		FreeLibrary(g_hWinRing0);
}

