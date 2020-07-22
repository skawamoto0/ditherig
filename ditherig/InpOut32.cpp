#include "stdafx.h"
#include "InpOut32.h"

HMODULE g_hInpOut32;
InpOut32_Out32 Out32;
InpOut32_Inp32 Inp32;
InpOut32_IsInpOutDriverOpen IsInpOutDriverOpen;
InpOut32_IsXP64Bit IsXP64Bit;
InpOut32_DlPortReadPortUchar DlPortReadPortUchar;
InpOut32_DlPortWritePortUchar DlPortWritePortUchar;
InpOut32_DlPortReadPortUshort DlPortReadPortUshort;
InpOut32_DlPortWritePortUshort DlPortWritePortUshort;
InpOut32_DlPortReadPortUlong DlPortReadPortUlong;
InpOut32_DlPortWritePortUlong DlPortWritePortUlong;
InpOut32_MapPhysToLin MapPhysToLin;
InpOut32_UnmapPhysicalMemory UnmapPhysicalMemory;
InpOut32_GetPhysLong GetPhysLong;
InpOut32_SetPhysLong SetPhysLong;

BOOL LoadInpOut32(LPCTSTR FileName)
{
	BOOL bResult;
	bResult = FALSE;
	g_hInpOut32 = LoadLibrary(FileName);
	if(g_hInpOut32)
	{
		Out32 = (InpOut32_Out32)GetProcAddress(g_hInpOut32, "Out32");
		Inp32 = (InpOut32_Inp32)GetProcAddress(g_hInpOut32, "Inp32");
		IsInpOutDriverOpen = (InpOut32_IsInpOutDriverOpen)GetProcAddress(g_hInpOut32, "IsInpOutDriverOpen");
		IsXP64Bit = (InpOut32_IsXP64Bit)GetProcAddress(g_hInpOut32, "IsXP64Bit");
		DlPortReadPortUchar = (InpOut32_DlPortReadPortUchar)GetProcAddress(g_hInpOut32, "DlPortReadPortUchar");
		DlPortWritePortUchar = (InpOut32_DlPortWritePortUchar)GetProcAddress(g_hInpOut32, "DlPortWritePortUchar");
		DlPortReadPortUshort = (InpOut32_DlPortReadPortUshort)GetProcAddress(g_hInpOut32, "DlPortReadPortUshort");
		DlPortWritePortUshort = (InpOut32_DlPortWritePortUshort)GetProcAddress(g_hInpOut32, "DlPortWritePortUshort");
		DlPortReadPortUlong = (InpOut32_DlPortReadPortUlong)GetProcAddress(g_hInpOut32, "DlPortReadPortUlong");
		DlPortWritePortUlong = (InpOut32_DlPortWritePortUlong)GetProcAddress(g_hInpOut32, "DlPortWritePortUlong");
		MapPhysToLin = (InpOut32_MapPhysToLin)GetProcAddress(g_hInpOut32, "MapPhysToLin");
		UnmapPhysicalMemory = (InpOut32_UnmapPhysicalMemory)GetProcAddress(g_hInpOut32, "UnmapPhysicalMemory");
		GetPhysLong = (InpOut32_GetPhysLong)GetProcAddress(g_hInpOut32, "GetPhysLong");
		SetPhysLong = (InpOut32_SetPhysLong)GetProcAddress(g_hInpOut32, "SetPhysLong");
		if(IsInpOutDriverOpen)
		{
			if(IsInpOutDriverOpen())
				bResult = TRUE;
		}
	}
	return bResult;
}

void UnloadInpOut32()
{
	if(g_hInpOut32)
		FreeLibrary(g_hInpOut32);
}

