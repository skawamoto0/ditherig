// ditherig.cpp : コンソール アプリケーションのエントリ ポイントを定義します。
//

#include "stdafx.h"
#include "resource.h"
#include "InpOut32.h"
#include "WinRing0.h"


#define APPLICATION_NAME				_T("Dithering Settings for Integrated Graphics")
#define APPLICATION_NAME_OLD			_T("Dithering Settings for Intel Graphics")

HINSTANCE g_hInstance;
TCHAR g_AppPath[MAX_PATH];
TCHAR g_AppDir[MAX_PATH];
HWND g_hWnd;
HICON g_hIcon;
HMENU g_hMenu;
NOTIFYICONDATA g_Notify;
BOOL g_bSupportedGPU;
TCHAR g_GPUInfo1[256];
TCHAR g_GPUInfo2[256];
DWORD g_Selection;
BOOL g_bFreqNominal;
BOOL g_bDisableEIST;
BOOL g_bDisableIDA;
BOOL g_bDisableC1E;
BOOL g_bDisableBDPROCHOT;
BOOL g_bDisableRAPL;
BOOL g_bDisableMCH59A0;
DWORD g_GPUFreq;
DWORD g_TypeOverride;
CHAR* g_pDatabase;
DWORD_PTR g_PhysicalAddress;
DWORD_PTR g_PhysicalSize;
HANDLE g_hPhysicalMemory;
PBYTE g_pPhysicalMemory;
DWORD g_TimerWaitCount;
UINT g_TaskbarCreated;
HPOWERNOTIFY g_hPowerNotify;

void GetDirFromFullPath(LPTSTR Dest, LPCTSTR File)
{
	TCHAR* p;
	_tcscpy(Dest, File);
	p = _tcsrchr(Dest, _T('\\'));
	if(p)
		*p = _T('\0');
}

void SetAppDir()
{
	GetModuleFileName(NULL, g_AppPath, MAX_PATH - 1);
	GetDirFromFullPath(g_AppDir, g_AppPath);
}

BOOL Load()
{
	BOOL bResult;
	DWORD i;
	bResult = FALSE;
	i = 30;
	while(i > 0)
	{
#if defined(_X86_)
		if(LoadInpOut32(_T("inpout32.dll")))
#elif defined(_AMD64_)
		if(LoadInpOut32(_T("inpoutx64.dll")))
#else
#error Not supported.
#endif
			break;
		Sleep(1000);
		i--;
	}
	while(i > 0)
	{
#if defined(_X86_)
		if(LoadWinRing0(_T("WinRing0.dll")))
#elif defined(_AMD64_)
		if(LoadWinRing0(_T("WinRing0x64.dll")))
#else
#error Not supported.
#endif
			break;
		Sleep(1000);
		i--;
	}
	if(i > 0)
		bResult = TRUE;
	return bResult;
}

void Unload()
{
	UnloadInpOut32();
	UnloadWinRing0();
}

void AltCopyMemory1(void* pDst, void* pSrc, DWORD_PTR Size)
{
	volatile BYTE* p;
	BYTE* pEnd;
	volatile BYTE* p0;
	p = (BYTE*)pDst;
	pEnd = (BYTE*)((DWORD_PTR)pDst + Size);
	p0 = (BYTE*)pSrc;
	while(p < pEnd)
	{
		*p = *p0;
		p++;
		p0++;
	}
}

void AltCopyMemory2(void* pDst, void* pSrc, DWORD_PTR Size)
{
	volatile WORD* p;
	WORD* pEnd;
	volatile WORD* p0;
	p = (WORD*)pDst;
	pEnd = (WORD*)((DWORD_PTR)pDst + Size);
	p0 = (WORD*)pSrc;
	while(p < pEnd)
	{
		*p = *p0;
		p++;
		p0++;
	}
}

void AltCopyMemory4(void* pDst, void* pSrc, DWORD_PTR Size)
{
	volatile DWORD* p;
	DWORD* pEnd;
	volatile DWORD* p0;
	p = (DWORD*)pDst;
	pEnd = (DWORD*)((DWORD_PTR)pDst + Size);
	p0 = (DWORD*)pSrc;
	while(p < pEnd)
	{
		*p = *p0;
		p++;
		p0++;
	}
}

void AltCopyMemory(void* pDst, void* pSrc, DWORD_PTR Size)
{
	DWORD_PTR Align;
	Align = (DWORD_PTR)pDst | (DWORD_PTR)pSrc | Size;
	if(!(Align & (sizeof(DWORD) - 1)))
		AltCopyMemory4(pDst, pSrc, Size);
	else if(!(Align & (sizeof(WORD) - 1)))
		AltCopyMemory2(pDst, pSrc, Size);
	else
		AltCopyMemory1(pDst, pSrc, Size);
}

BOOL ReadPhysicalMemory(DWORD_PTR Address, void* pBuffer, DWORD_PTR Size)
{
	BOOL bResult;
	bResult = FALSE;
	if(Address != g_PhysicalAddress || Size != g_PhysicalSize)
	{
		if(g_hPhysicalMemory)
			UnmapPhysicalMemory(g_hPhysicalMemory, g_pPhysicalMemory);
		g_PhysicalAddress = Address;
		g_PhysicalSize = Size;
		g_hPhysicalMemory = NULL;
		g_pPhysicalMemory = NULL;
		if(Size > 0)
			g_pPhysicalMemory = MapPhysToLin((PBYTE)Address, (DWORD)Size, &g_hPhysicalMemory);
	}
	if(g_pPhysicalMemory)
	{
		AltCopyMemory(pBuffer, g_pPhysicalMemory, Size);
		bResult = TRUE;
	}
	return bResult;
}

BOOL WritePhysicalMemory(DWORD_PTR Address, void* pBuffer, DWORD_PTR Size)
{
	BOOL bResult;
	bResult = FALSE;
	if(Address != g_PhysicalAddress || Size != g_PhysicalSize)
	{
		if(g_hPhysicalMemory)
			UnmapPhysicalMemory(g_hPhysicalMemory, g_pPhysicalMemory);
		g_PhysicalAddress = Address;
		g_PhysicalSize = Size;
		g_hPhysicalMemory = NULL;
		g_pPhysicalMemory = NULL;
		if(Size > 0)
			g_pPhysicalMemory = MapPhysToLin((PBYTE)Address, (DWORD)Size, &g_hPhysicalMemory);
	}
	if(g_pPhysicalMemory)
	{
		AltCopyMemory(g_pPhysicalMemory, pBuffer, Size);
		bResult = TRUE;
	}
	return bResult;
}

BOOL LoadDatabase(LPCTSTR FileName)
{
	BOOL bResult;
	HANDLE hFile;
	LARGE_INTEGER li;
	bResult = FALSE;
	hFile = CreateFile(FileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if(hFile != INVALID_HANDLE_VALUE)
	{
		if(GetFileSizeEx(hFile, &li))
		{
			g_pDatabase = new CHAR[li.LowPart + 1];
			if(g_pDatabase)
			{
				if(ReadFile(hFile, g_pDatabase, li.LowPart, &li.LowPart, NULL))
				{
					g_pDatabase[li.LowPart] = '\0';
					bResult = TRUE;
				}
			}
		}
		CloseHandle(hFile);
	}
	return bResult;
}

void UnloadDatabase()
{
	if(g_pDatabase)
		delete []g_pDatabase;
	g_pDatabase = NULL;
}

BOOL FindConfigFromDatabase(WORD VendorID, WORD DeviceID, DWORD Selection, DWORD Count, DWORD* pBAR1Address, DWORD* pBAR2Address, DWORD_PTR* pRegisterAddress, DWORD_PTR* pRegisterSize, DWORD* pRegisterMask, DWORD* pRegisterData, CHAR* Info)
{
	BOOL bResult;
	DWORD Type;
	CHAR* p;
	CHAR* pNextRow;
	CHAR* pNextColumn;
	bResult = FALSE;
	Type = 0;
	p = g_pDatabase;
	while(*p != '\0')
	{
		pNextRow = strchr(p, '\r');
		if(!pNextRow)
		{
			pNextRow = strchr(p, '\n');
			if(!pNextRow)
				pNextRow = strchr(p, '\0');
		}
		if(strtoul(p, &p, 0) == 2)
		{
			pNextColumn = strchr(p, ',');
			if(pNextColumn && pNextColumn < pNextRow)
			{
				p = pNextColumn + 1;
				if((WORD)strtoul(p, &p, 0) == VendorID)
				{
					pNextColumn = strchr(p, ',');
					if(pNextColumn && pNextColumn < pNextRow)
					{
						p = pNextColumn + 1;
						if((WORD)strtoul(p, &p, 0) == DeviceID)
						{
							pNextColumn = strchr(p, ',');
							if(pNextColumn && pNextColumn < pNextRow)
							{
								p = pNextColumn + 1;
								Type = strtoul(p, &p, 0);
								if(Info)
								{
									pNextColumn = strchr(p, ',');
									if(pNextColumn && pNextColumn < pNextRow)
									{
										p = pNextColumn + 1;
										memcpy(Info, p, (size_t)pNextRow - (size_t)p);
										Info[pNextRow - p] = '\0';
									}
								}
								break;
							}
						}
					}
				}
			}
		}
		p = pNextRow;
		if(*p == '\r')
			p++;
		if(*p == '\n')
			p++;
	}
	if(g_TypeOverride != 0)
		Type = g_TypeOverride;
	if(Type != 0)
	{
		p = g_pDatabase;
		while(*p != '\0')
		{
			pNextRow = strchr(p, '\r');
			if(!pNextRow)
			{
				pNextRow = strchr(p, '\n');
				if(!pNextRow)
					pNextRow = strchr(p, '\0');
			}
			if(strtoul(p, &p, 0) == 1)
			{
				pNextColumn = strchr(p, ',');
				if(pNextColumn && pNextColumn < pNextRow)
				{
					p = pNextColumn + 1;
					if((DWORD)strtoul(p, &p, 0) == Type)
					{
						pNextColumn = strchr(p, ',');
						if(pNextColumn && pNextColumn < pNextRow)
						{
							p = pNextColumn + 1;
							if((DWORD)strtoul(p, &p, 0) == Selection)
							{
								pNextColumn = strchr(p, ',');
								if(pNextColumn && pNextColumn < pNextRow)
								{
									p = pNextColumn + 1;
									if((DWORD)strtoul(p, &p, 0) == Count)
									{
										pNextColumn = strchr(p, ',');
										if(pNextColumn && pNextColumn < pNextRow)
										{
											p = pNextColumn + 1;
											*pBAR1Address = strtoul(p, &p, 0);
											pNextColumn = strchr(p, ',');
											if(pNextColumn && pNextColumn < pNextRow)
											{
												p = pNextColumn + 1;
												*pBAR2Address = strtoul(p, &p, 0);
												pNextColumn = strchr(p, ',');
												if(pNextColumn && pNextColumn < pNextRow)
												{
													p = pNextColumn + 1;
													*pRegisterAddress = strtoul(p, &p, 0);
													pNextColumn = strchr(p, ',');
													if(pNextColumn && pNextColumn < pNextRow)
													{
														p = pNextColumn + 1;
														*pRegisterSize = strtoul(p, &p, 0);
														pNextColumn = strchr(p, ',');
														if(pNextColumn && pNextColumn < pNextRow)
														{
															p = pNextColumn + 1;
															*pRegisterMask = strtoul(p, &p, 0);
															pNextColumn = strchr(p, ',');
															if(pNextColumn && pNextColumn < pNextRow)
															{
																p = pNextColumn + 1;
																*pRegisterData = strtoul(p, &p, 0);
																bResult = TRUE;
																break;
															}
														}
													}
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
			p = pNextRow;
			if(*p == '\r')
				p++;
			if(*p == '\n')
				p++;
		}
	}
	return bResult;
}

BOOL FindQuestionFromDatabase(DWORD Case, BOOL* Branch, DWORD* CaseYes, DWORD* CaseNo, CHAR* Question)
{
	BOOL bResult;
	CHAR* p;
	CHAR* pNextRow;
	CHAR* pNextColumn;
	bResult = FALSE;
	p = g_pDatabase;
	while(*p != '\0')
	{
		pNextRow = strchr(p, '\r');
		if(!pNextRow)
		{
			pNextRow = strchr(p, '\n');
			if(!pNextRow)
				pNextRow = strchr(p, '\0');
		}
		if(strtoul(p, &p, 0) == 3)
		{
			pNextColumn = strchr(p, ',');
			if(pNextColumn && pNextColumn < pNextRow)
			{
				p = pNextColumn + 1;
				if((DWORD)strtoul(p, &p, 0) == Case)
				{
					pNextColumn = strchr(p, ',');
					if(pNextColumn && pNextColumn < pNextRow)
					{
						p = pNextColumn + 1;
						*Branch = strtoul(p, &p, 0);
						pNextColumn = strchr(p, ',');
						if(pNextColumn && pNextColumn < pNextRow)
						{
							p = pNextColumn + 1;
							*CaseYes = strtoul(p, &p, 0);
							pNextColumn = strchr(p, ',');
							if(pNextColumn && pNextColumn < pNextRow)
							{
								p = pNextColumn + 1;
								*CaseNo = strtoul(p, &p, 0);
								pNextColumn = strchr(p, ',');
								if(pNextColumn && pNextColumn < pNextRow)
								{
									p = pNextColumn + 1;
									memcpy(Question, p, (size_t)pNextRow - (size_t)p);
									Question[pNextRow - p] = '\0';
									bResult = TRUE;
									break;
								}
							}
						}
					}
				}
			}
		}
		p = pNextRow;
		if(*p == '\r')
			p++;
		if(*p == '\n')
			p++;
	}
	return bResult;
}

BOOL IsSupportedGPU()
{
	BOOL bResult;
	BYTE Index;
	DWORD PCIAddress;
	WORD VendorID;
	WORD DeviceID;
	DWORD BAR1Address;
	DWORD BAR2Address;
	DWORD_PTR RegisterAddress;
	DWORD_PTR RegisterSize;
	DWORD RegisterMask;
	DWORD RegisterData;
	bResult = FALSE;
	Index = 0;
	while((PCIAddress = FindPciDeviceByClass(0x03, 0x00, 0x00, Index)) != 0xffffffff)
	{ 
		if(ReadPciConfigWordEx(PCIAddress, 0x00000000, &VendorID) && ReadPciConfigWordEx(PCIAddress, 0x00000002, &DeviceID))
		{
			if(FindConfigFromDatabase(VendorID, DeviceID, 0, 0, &BAR1Address, &BAR2Address, &RegisterAddress, &RegisterSize, &RegisterMask, &RegisterData, NULL))
			{
				bResult = TRUE;
				break;
			}
		}
		Index++;
	}
	return bResult;
}

BOOL GetGPUInfo(TCHAR* Text1, TCHAR* Text2)
{
	BOOL bResult;
	BYTE Index;
	DWORD PCIAddress;
	WORD VendorID;
	WORD DeviceID;
	DWORD BAR1Address;
	DWORD BAR2Address;
	DWORD_PTR RegisterAddress;
	DWORD_PTR RegisterSize;
	DWORD RegisterMask;
	DWORD RegisterData;
	CHAR Info[256];
	bResult = FALSE;
	Index = 0;
	VendorID = 0;
	DeviceID = 0;
	strcpy(Info, "Graphics not found in the database");
	while((PCIAddress = FindPciDeviceByClass(0x03, 0x00, 0x00, Index)) != 0xffffffff)
	{ 
		if(ReadPciConfigWordEx(PCIAddress, 0x00000000, &VendorID) && ReadPciConfigWordEx(PCIAddress, 0x00000002, &DeviceID))
		{
			if(FindConfigFromDatabase(VendorID, DeviceID, 0, 0, &BAR1Address, &BAR2Address, &RegisterAddress, &RegisterSize, &RegisterMask, &RegisterData, Info))
			{
				bResult = TRUE;
				break;
			}
		}
		Index++;
	}
	_stprintf(Text1, _T("Vendor ID = %04X, Device ID = %04X"), VendorID, DeviceID);
#ifdef UNICODE
	MultiByteToWideChar(CP_UTF8, 0, Info, -1, Text2, 256);
#else
	strcpy(Text2, Info);
#endif
	return bResult;
}

BOOL IdentifyGPUByQuestions()
{
	BOOL bResult;
	DWORD Case;
	BOOL Branch;
	DWORD CaseYes;
	DWORD CaseNo;
	CHAR Question[256];
	TCHAR Question1[256];
	bResult = FALSE;
	Case = 0;
	while(FindQuestionFromDatabase(Case, &Branch, &CaseYes, &CaseNo, Question))
	{
#ifdef UNICODE
		MultiByteToWideChar(CP_UTF8, 0, Question, -1, Question1, 256);
#else
		strcpy(Question1, Question);
#endif
		if(Branch)
		{
			switch(MessageBox(g_hWnd, Question1, APPLICATION_NAME, MB_YESNOCANCEL))
			{
			case IDYES:
				Case = CaseYes;
				break;
			case IDNO:
				Case = CaseNo;
				break;
			default:
				bResult = TRUE;
				break;
			}
		}
		else
		{
			MessageBox(g_hWnd, Question1, APPLICATION_NAME, MB_OK);
			g_TypeOverride = CaseYes;
			bResult = TRUE;
		}
		if(bResult)
			break;
	}
	return bResult;
}

BOOL ConfigureGPURegister(DWORD Selection)
{
	BOOL bResult;
	BYTE Index;
	DWORD PCIAddress;
	WORD VendorID;
	WORD DeviceID;
	DWORD Count;
	DWORD BAR1Address;
	DWORD BAR2Address;
	DWORD_PTR RegisterAddress;
	DWORD_PTR RegisterSize;
	DWORD RegisterMask;
	DWORD RegisterData;
	DWORD BAR1;
	DWORD BAR2;
	DWORD_PTR Address;
	DWORD Old;
	DWORD New;
	bResult = FALSE;
	Index = 0;
	while((PCIAddress = FindPciDeviceByClass(0x03, 0x00, 0x00, Index)) != 0xffffffff)
	{ 
		if(ReadPciConfigWordEx(PCIAddress, 0x00000000, &VendorID) && ReadPciConfigWordEx(PCIAddress, 0x00000002, &DeviceID))
		{
			Count = 0;
			while(FindConfigFromDatabase(VendorID, DeviceID, Selection, Count, &BAR1Address, &BAR2Address, &RegisterAddress, &RegisterSize, &RegisterMask, &RegisterData, NULL))
			{
				if(RegisterMask != 0)
				{
					if(ReadPciConfigDwordEx(PCIAddress, BAR1Address, &BAR1) && ReadPciConfigDwordEx(PCIAddress, BAR2Address, &BAR2))
					{
#if defined(_X86_)
						Address = (DWORD_PTR)BAR1 & ~((DWORD_PTR)0x10 - 1);
#elif defined(_AMD64_)
						if(BAR2Address != 0)
							Address = ((DWORD_PTR)BAR1 | ((DWORD_PTR)BAR2 << 32)) & ~((DWORD_PTR)0x10 - 1);
						else
							Address = (DWORD_PTR)BAR1 & ~((DWORD_PTR)0x10 - 1);
#endif
						if(ReadPhysicalMemory(Address + RegisterAddress, &Old, RegisterSize))
						{
							New = Old;
							New = (New & ~RegisterMask) | (RegisterData & RegisterMask);
							if(New == Old)
								bResult = TRUE;
							else
							{
								if(WritePhysicalMemory(Address + RegisterAddress, &New, RegisterSize))
									bResult = TRUE;
							}
						}
					}
				}
				else
					bResult = TRUE;
				Count++;
			}
		}
		Index++;
	}
	return bResult;
}

BOOL IsEnabledEIST(BOOL* pbEnable)
{
	BOOL bResult;
	DWORD MSR[2];
	bResult = FALSE;
	if(Rdmsr)
	{
		if(Rdmsr(0x000001a0, &MSR[0], &MSR[1]))
		{
			*pbEnable = (MSR[0] & 0x00010000) ? TRUE : FALSE;
			bResult = TRUE;
		}
	}
	return bResult;
}

BOOL EnableEIST(BOOL bEnable)
{
	BOOL bResult;
	DWORD MSR[2];
	bResult = FALSE;
	if(Rdmsr)
	{
		if(Rdmsr(0x000001a0, &MSR[0], &MSR[1]))
		{
			if(Wrmsr)
			{
				MSR[0] = bEnable ? (MSR[0] | 0x00010000) : (MSR[0] & ~0x00010000);
				bResult = Wrmsr(0x000001a0, MSR[0], MSR[1]);
			}
		}
	}
	return bResult;
}

BOOL IsEnabledIDA(BOOL* pbEnable)
{
	BOOL bResult;
	DWORD MSR[2];
	bResult = FALSE;
	if(Rdmsr)
	{
		if(Rdmsr(0x000001a0, &MSR[0], &MSR[1]))
		{
			*pbEnable = (MSR[1] & ~0x00000040) ? FALSE : TRUE;
			bResult = TRUE;
		}
	}
	return bResult;
}

BOOL EnableIDA(BOOL bEnable)
{
	BOOL bResult;
	DWORD MSR[2];
	bResult = FALSE;
	if(Rdmsr)
	{
		if(Rdmsr(0x000001a0, &MSR[0], &MSR[1]))
		{
			if(Wrmsr)
			{
				MSR[1] = bEnable ? (MSR[1] & ~0x00000040) : (MSR[1] | 0x00000040);
				bResult = Wrmsr(0x000001a0, MSR[0], MSR[1]);
			}
		}
	}
	return bResult;
}

BOOL IsEnabledC1E(BOOL* pbEnable)
{
	BOOL bResult;
	DWORD MSR[2];
	bResult = FALSE;
	if(Rdmsr)
	{
		if(Rdmsr(0x000001fc, &MSR[0], &MSR[1]))
		{
			*pbEnable = (MSR[0] | 0x00000002) ? TRUE : FALSE;
			bResult = TRUE;
		}
	}
	return bResult;
}

BOOL EnableC1E(BOOL bEnable)
{
	BOOL bResult;
	DWORD MSR[2];
	bResult = FALSE;
	if(Rdmsr)
	{
		if(Rdmsr(0x000001fc, &MSR[0], &MSR[1]))
		{
			if(Wrmsr)
			{
				MSR[0] = bEnable ? (MSR[0] | 0x00000002) : (MSR[0] & ~0x00000002);
				bResult = Wrmsr(0x000001fc, MSR[0], MSR[1]);
			}
		}
	}
	return bResult;
}

BOOL IsEnabledBDPROCHOT(BOOL* pbEnable)
{
	BOOL bResult;
	DWORD MSR[2];
	bResult = FALSE;
	if(Rdmsr)
	{
		if(Rdmsr(0x000001fc, &MSR[0], &MSR[1]))
		{
			*pbEnable = (MSR[0] | 0x00000001) ? TRUE : FALSE;
			bResult = TRUE;
		}
	}
	return bResult;
}

BOOL EnableBDPROCHOT(BOOL bEnable)
{
	BOOL bResult;
	DWORD MSR[2];
	bResult = FALSE;
	if(Rdmsr)
	{
		if(Rdmsr(0x000001fc, &MSR[0], &MSR[1]))
		{
			if(Wrmsr)
			{
				MSR[0] = bEnable ? (MSR[0] | 0x00000001) : (MSR[0] & ~0x00000001);
				bResult = Wrmsr(0x000001fc, MSR[0], MSR[1]);
			}
		}
	}
	return bResult;
}

BOOL EnableRAPL(BOOL bEnable)
{
	BOOL bResult;
	DWORD MSR[2];
	bResult = FALSE;
	if(Rdmsr)
	{
		if(Rdmsr(0x00000638, &MSR[0], &MSR[1]))
		{
			if(Wrmsr)
			{
				MSR[0] = bEnable ? (MSR[0] | 0x00008000) : (MSR[0] & ~0x00008000);
				bResult = Wrmsr(0x00000638, MSR[0], MSR[1]);
			}
		}
		if(Rdmsr(0x00000640, &MSR[0], &MSR[1]))
		{
			if(Wrmsr)
			{
				MSR[0] = bEnable ? (MSR[0] | 0x00008000) : (MSR[0] & ~0x00008000);
				bResult = Wrmsr(0x00000640, MSR[0], MSR[1]);
			}
		}
	}
	return bResult;
}

BOOL EnableMCH59A0(BOOL bEnable)
{
	BOOL bResult;
	DWORD PCIAddress;
	WORD VendorID;
	DWORD BAR1;
	DWORD BAR2;
	DWORD_PTR Address;
	DWORD Register;
	bResult = FALSE;
	PCIAddress = FindPciDeviceByClass(0x06, 0x00, 0x00, 0);
	if(PCIAddress != 0xffffffff)
	{ 
		if(ReadPciConfigWordEx(PCIAddress, 0x00000000, &VendorID))
		{
			if(VendorID == 0x8086)
			{
				if(ReadPciConfigDwordEx(PCIAddress, 0x00000048, &BAR1) && ReadPciConfigDwordEx(PCIAddress, 0x0000004c, &BAR2))
				{
					if(BAR1 & 0x00000001)
					{
#if defined(_X86_)
						Address = (DWORD_PTR)BAR1 & ~((DWORD_PTR)0x10 - 1);
#elif defined(_AMD64_)
						Address = ((DWORD_PTR)BAR1 | ((DWORD_PTR)BAR2 << 32)) & ~((DWORD_PTR)0x10 - 1);
#endif
						if(bEnable)
							bResult = TRUE;
						else
						{
							Register = 0x00000000;
							if(WritePhysicalMemory(Address + 0x000059a0, &Register, 4))
							{
								Register = 0x00000000;
								if(WritePhysicalMemory(Address + 0x000059a4, &Register, 4))
									bResult = TRUE;
							}
						}
					}
				}
			}
		}
	}
	return bResult;
}

BOOL GetCPUFrequencyNominal(DWORD* pMultiplier)
{
	BOOL bResult;
	DWORD MSR[2];
	bResult = FALSE;
	if(Rdmsr)
	{
		if(Rdmsr(0x00000648, &MSR[0], &MSR[1]))
		{
			*pMultiplier = MSR[0] & 0x000000ff;
			bResult = TRUE;
		}
	}
	return bResult;
}

BOOL GetCPUFrequency(DWORD* pMultiplier)
{
	BOOL bResult;
	DWORD MSR[2];
	bResult = FALSE;
	if(Rdmsr)
	{
		if(Rdmsr(0x00000198, &MSR[0], &MSR[1]))
		{
			*pMultiplier = (MSR[0] >> 8) & 0x000000ff;
			bResult = TRUE;
		}
	}
	return bResult;
}

BOOL SetCPUFrequency(DWORD Multiplier)
{
	BOOL bResult;
	DWORD MSR[2];
	bResult = FALSE;
	if(Rdmsr)
	{
		if(Rdmsr(0x00000199, &MSR[0], &MSR[1]))
		{
			if(Wrmsr)
			{
				MSR[0] = (MSR[0] & ~(0x000000ff << 8)) | ((Multiplier & 0x000000ff) << 8);
				bResult = Wrmsr(0x00000199, MSR[0], MSR[1]);
			}
		}
	}
	return bResult;
}

BOOL FixCPUFrequencyNominal()
{
	BOOL bResult;
	DWORD_PTR ProcessAffinity;
	DWORD_PTR SystemAffinity;
	DWORD i;
	DWORD Nominal;
	DWORD Timeout;
	BOOL bEnable;
	DWORD Current;
	bResult = FALSE;
	if(!GetProcessAffinityMask(GetCurrentProcess(), &ProcessAffinity, &SystemAffinity))
		SystemAffinity = (DWORD_PTR)-1;
	if(SetProcessAffinityMask(GetCurrentProcess(), SystemAffinity))
	{
		bResult = TRUE;
		for(i = 0; i < sizeof(DWORD_PTR) * 8; i++)
		{
			if(((DWORD_PTR)1 << i) & SystemAffinity)
			{
				if(SetThreadAffinityMask(GetCurrentThread(), (DWORD_PTR)1 << i))
				{
					if(GetCPUFrequencyNominal(&Nominal))
					{
						Timeout = timeGetTime();
						while(true)
						{
							EnableEIST(TRUE);
							SetCPUFrequency(Nominal);
							EnableEIST(FALSE);
							if(IsEnabledEIST(&bEnable))
							{
								if(!bEnable)
								{
									if(GetCPUFrequency(&Current))
									{
										if(Current == Nominal)
											break;
									}
								}
							}
							if(timeGetTime() - Timeout >= 1000)
							{
								bResult = FALSE;
								break;
							}
						}
					}
					else
						bResult = FALSE;
				}
				else
					bResult = FALSE;
			}
		}
		SetThreadAffinityMask(GetCurrentThread(), SystemAffinity);
	}
	return bResult;
}

BOOL RegisterTaskScheduler(LPCWSTR Name, LPCWSTR Xml)
{
	BOOL bResult;
	ITaskService* pTaskService;
	VARIANT Variant;
	BSTR Path;
	ITaskFolder* pTaskFolder;
	BSTR TaskPath;
	BSTR XmlText;
	IRegisteredTask* pRegisteredTask;
	bResult = FALSE;
	if(CoCreateInstance(CLSID_TaskScheduler, NULL, CLSCTX_INPROC_SERVER, IID_ITaskService, (LPVOID*)&pTaskService) == S_OK)
	{
		VariantInit(&Variant);
		if(pTaskService->Connect(Variant, Variant, Variant, Variant) == S_OK)
		{
			if(Path = SysAllocString(L"\\"))
			{
				if(pTaskService->GetFolder(Path, &pTaskFolder) == S_OK)
				{
					if(TaskPath = SysAllocString(Name))
					{
						if(XmlText = SysAllocString(Xml))
						{
							pRegisteredTask = NULL;
							if(pTaskFolder->RegisterTask(TaskPath, XmlText, TASK_CREATE_OR_UPDATE, Variant, Variant, TASK_LOGON_INTERACTIVE_TOKEN, Variant, &pRegisteredTask) == S_OK)
							{
								bResult = TRUE;
								pRegisteredTask->Release();
							}
							SysFreeString(XmlText);
						}
						SysFreeString(TaskPath);
					}
					pTaskFolder->Release();
				}
				SysFreeString(Path);
			}
		}
		pTaskService->Release();
	}
	return bResult;
}

BOOL UnregisterTaskScheduler(LPCWSTR Name)
{
	BOOL bResult;
	ITaskService* pTaskService;
	VARIANT Variant;
	BSTR Path;
	ITaskFolder* pTaskFolder;
	BSTR TaskPath;
	bResult = FALSE;
	if(CoCreateInstance(CLSID_TaskScheduler, NULL, CLSCTX_INPROC_SERVER, IID_ITaskService, (LPVOID*)&pTaskService) == S_OK)
	{
		VariantInit(&Variant);
		if(pTaskService->Connect(Variant, Variant, Variant, Variant) == S_OK)
		{
			if(Path = SysAllocString(L"\\"))
			{
				if(pTaskService->GetFolder(Path, &pTaskFolder) == S_OK)
				{
					if(TaskPath = SysAllocString(Name))
					{
						if(pTaskFolder->DeleteTask(TaskPath, 0) == S_OK)
							bResult = TRUE;
						SysFreeString(TaskPath);
					}
					pTaskFolder->Release();
				}
				SysFreeString(Path);
			}
		}
		pTaskService->Release();
	}
	return bResult;
}

BOOL RegisterStartup(LPCWSTR Name, LPCWSTR FileName)
{
	const WCHAR Xml[] =
		L"<?xml version=\"1.0\" encoding=\"UTF-16\"?>\r\n"
		L"<Task xmlns=\"http://schemas.microsoft.com/windows/2004/02/mit/task\">\r\n"
		L"  <Triggers>\r\n"
		L"    <LogonTrigger>\r\n"
		L"      <Enabled>true</Enabled>\r\n"
		L"    </LogonTrigger>\r\n"
		L"  </Triggers>\r\n"
		L"  <Principals>\r\n"
		L"    <Principal>\r\n"
		L"      <LogonType>InteractiveToken</LogonType>\r\n"
		L"      <RunLevel>HighestAvailable</RunLevel>\r\n"
		L"    </Principal>\r\n"
		L"  </Principals>\r\n"
		L"  <Settings>\r\n"
		L"    <DisallowStartIfOnBatteries>false</DisallowStartIfOnBatteries>\r\n"
		L"    <StopIfGoingOnBatteries>false</StopIfGoingOnBatteries>\r\n"
		L"    <AllowHardTerminate>true</AllowHardTerminate>\r\n"
		L"    <IdleSettings>\r\n"
		L"      <StopOnIdleEnd>false</StopOnIdleEnd>\r\n"
		L"    </IdleSettings>\r\n"
		L"    <Enabled>true</Enabled>\r\n"
		L"    <WakeToRun>false</WakeToRun>\r\n"
		L"    <ExecutionTimeLimit>PT0S</ExecutionTimeLimit>\r\n"
		L"  </Settings>\r\n"
		L"  <Actions>\r\n"
		L"    <Exec>\r\n"
		L"      <Command>\"%s\"</Command>\r\n"
		L"    </Exec>\r\n"
		L"  </Actions>\r\n"
		L"</Task>\r\n"
		;
	WCHAR Temp[4096];
	wsprintf(Temp, Xml, FileName);
	return RegisterTaskScheduler(Name, Temp);
}

BOOL SetStartup(BOOL bInstall)
{
	BOOL bResult;
	OSVERSIONINFO Version;
	TCHAR Temp[MAX_PATH + 4];
	HKEY hKey;
	bResult = FALSE;
	Version.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	if(GetVersionEx(&Version))
	{
		if(Version.dwMajorVersion >= 6)
		{
			UnregisterTaskScheduler(APPLICATION_NAME_OLD);
			if(bInstall)
			{
				if(RegisterStartup(APPLICATION_NAME, g_AppPath))
					bResult = TRUE;
			}
			else
			{
				if(UnregisterTaskScheduler(APPLICATION_NAME))
					bResult = TRUE;
			}
		}
		else
		{
			if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run"), 0, KEY_READ | KEY_WRITE, &hKey) == ERROR_SUCCESS)
			{
				RegDeleteValue(hKey, APPLICATION_NAME_OLD);
				if(bInstall)
				{
					_stprintf(Temp, _T("\"%s\""), g_AppPath);
					if(RegSetValueEx(hKey, APPLICATION_NAME, 0, REG_SZ, (BYTE*)&Temp, sizeof(TCHAR) * ((DWORD)_tcslen(Temp) + 1)) == ERROR_SUCCESS)
						bResult = TRUE;
				}
				else
				{
					if(RegDeleteValue(hKey, APPLICATION_NAME) == ERROR_SUCCESS)
						bResult = TRUE;
				}
				RegCloseKey(hKey);
			}
		}
	}
	return bResult;
}

void LoadSettings()
{
	HKEY hKey;
	HKEY hAppKey;
	DWORD Data;
	if(RegOpenKeyEx(HKEY_CURRENT_USER, _T("Software"), 0, KEY_READ | KEY_WRITE, &hKey) == ERROR_SUCCESS)
	{
		if(RegOpenKeyEx(hKey, APPLICATION_NAME, 0, KEY_READ | KEY_WRITE, &hAppKey) == ERROR_SUCCESS || RegOpenKeyEx(hKey, APPLICATION_NAME_OLD, 0, KEY_READ | KEY_WRITE, &hAppKey) == ERROR_SUCCESS)
		{
			Data = sizeof(DWORD);
			RegQueryValueEx(hAppKey, _T("Selection"), NULL, NULL, (BYTE*)&g_Selection, &Data);
			Data = sizeof(BOOL);
			RegQueryValueEx(hAppKey, _T("FreqNominal"), NULL, NULL, (BYTE*)&g_bFreqNominal, &Data);
			Data = sizeof(BOOL);
			RegQueryValueEx(hAppKey, _T("DisableEIST"), NULL, NULL, (BYTE*)&g_bDisableEIST, &Data);
			Data = sizeof(BOOL);
			RegQueryValueEx(hAppKey, _T("DisableIDA"), NULL, NULL, (BYTE*)&g_bDisableIDA, &Data);
			Data = sizeof(BOOL);
			RegQueryValueEx(hAppKey, _T("DisableC1E"), NULL, NULL, (BYTE*)&g_bDisableC1E, &Data);
			Data = sizeof(BOOL);
			RegQueryValueEx(hAppKey, _T("DisableBDPROCHOT"), NULL, NULL, (BYTE*)&g_bDisableBDPROCHOT, &Data);
			Data = sizeof(BOOL);
			RegQueryValueEx(hAppKey, _T("DisableRAPL"), NULL, NULL, (BYTE*)&g_bDisableRAPL, &Data);
			Data = sizeof(BOOL);
			RegQueryValueEx(hAppKey, _T("DisableMCH59A0"), NULL, NULL, (BYTE*)&g_bDisableMCH59A0, &Data);
			Data = sizeof(DWORD);
			RegQueryValueEx(hAppKey, _T("GPUFreq"), NULL, NULL, (BYTE*)&g_GPUFreq, &Data);
			Data = sizeof(DWORD);
			RegQueryValueEx(hAppKey, _T("TypeOverride"), NULL, NULL, (BYTE*)&g_TypeOverride, &Data);
			RegCloseKey(hAppKey);
		}
		RegCloseKey(hKey);
	}
}

void SaveSettings()
{
	HKEY hKey;
	HKEY hAppKey;
	if(RegOpenKeyEx(HKEY_CURRENT_USER, _T("Software"), 0, KEY_READ | KEY_WRITE, &hKey) == ERROR_SUCCESS)
	{
		RegRenameKey(hKey, APPLICATION_NAME_OLD, APPLICATION_NAME);
		RegDeleteTree(hKey, APPLICATION_NAME_OLD);
		if(RegCreateKeyEx(hKey, APPLICATION_NAME, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE, NULL, &hAppKey, NULL) == ERROR_SUCCESS)
		{
			RegSetValueEx(hAppKey, _T("Selection"), 0, REG_DWORD, (BYTE*)&g_Selection, sizeof(DWORD));
			RegSetValueEx(hAppKey, _T("FreqNominal"), 0, REG_DWORD, (BYTE*)&g_bFreqNominal, sizeof(BOOL));
			RegSetValueEx(hAppKey, _T("DisableEIST"), 0, REG_DWORD, (BYTE*)&g_bDisableEIST, sizeof(BOOL));
			RegSetValueEx(hAppKey, _T("DisableIDA"), 0, REG_DWORD, (BYTE*)&g_bDisableIDA, sizeof(BOOL));
			RegSetValueEx(hAppKey, _T("DisableC1E"), 0, REG_DWORD, (BYTE*)&g_bDisableC1E, sizeof(BOOL));
			RegSetValueEx(hAppKey, _T("DisableBDPROCHOT"), 0, REG_DWORD, (BYTE*)&g_bDisableBDPROCHOT, sizeof(BOOL));
			RegSetValueEx(hAppKey, _T("DisableRAPL"), 0, REG_DWORD, (BYTE*)&g_bDisableRAPL, sizeof(BOOL));
			RegSetValueEx(hAppKey, _T("DisableMCH59A0"), 0, REG_DWORD, (BYTE*)&g_bDisableMCH59A0, sizeof(BOOL));
			RegSetValueEx(hAppKey, _T("GPUFreq"), 0, REG_DWORD, (BYTE*)&g_GPUFreq, sizeof(DWORD));
			RegSetValueEx(hAppKey, _T("TypeOverride"), 0, REG_DWORD, (BYTE*)&g_TypeOverride, sizeof(DWORD));
			RegCloseKey(hAppKey);
		}
		RegCloseKey(hKey);
	}
}

void UpdateMenu()
{
	HMENU hMenu;
	MENUITEMINFO mii;
	hMenu = GetSubMenu(g_hMenu, 0);
	mii.cbSize = sizeof(MENUITEMINFO);
	mii.fMask = MIIM_STATE;
	GetMenuItemInfo(hMenu, ID_SELECTION0, FALSE, &mii);
	mii.fState &= ~MFS_DISABLED & ~MFS_CHECKED;
	SetMenuItemInfo(hMenu, ID_SELECTION0, FALSE, &mii);
	SetMenuItemInfo(hMenu, ID_SELECTION1, FALSE, &mii);
	SetMenuItemInfo(hMenu, ID_SELECTION2, FALSE, &mii);
	SetMenuItemInfo(hMenu, ID_SELECTION3, FALSE, &mii);
	SetMenuItemInfo(hMenu, ID_SELECTION4, FALSE, &mii);
	SetMenuItemInfo(hMenu, ID_FREQ_NOMINAL, FALSE, &mii);
	SetMenuItemInfo(hMenu, ID_DISABLE_EIST, FALSE, &mii);
	SetMenuItemInfo(hMenu, ID_DISABLE_IDA, FALSE, &mii);
	SetMenuItemInfo(hMenu, ID_DISABLE_C1E, FALSE, &mii);
	SetMenuItemInfo(hMenu, ID_DISABLE_BDPROCHOT, FALSE, &mii);
	SetMenuItemInfo(hMenu, ID_DISABLE_RAPL, FALSE, &mii);
	SetMenuItemInfo(hMenu, ID_DISABLE_MCH59A0, FALSE, &mii);
	SetMenuItemInfo(hMenu, ID_GPU_FREQ_RESET, FALSE, &mii);
	SetMenuItemInfo(hMenu, ID_GPU_FREQ_HIGHEST, FALSE, &mii);
	SetMenuItemInfo(hMenu, ID_GPU_FREQ_LOWEST, FALSE, &mii);
	SetMenuItemInfo(hMenu, ID_IDENTIFY, FALSE, &mii);
	mii.fState |= MFS_CHECKED;
	SetMenuItemInfo(hMenu, (UINT)(g_Selection + ID_SELECTION0), FALSE, &mii);
	if(g_bFreqNominal)
		SetMenuItemInfo(hMenu, ID_FREQ_NOMINAL, FALSE, &mii);
	if(g_bDisableEIST)
		SetMenuItemInfo(hMenu, ID_DISABLE_EIST, FALSE, &mii);
	if(g_bDisableIDA)
		SetMenuItemInfo(hMenu, ID_DISABLE_IDA, FALSE, &mii);
	if(g_bDisableC1E)
		SetMenuItemInfo(hMenu, ID_DISABLE_C1E, FALSE, &mii);
	if(g_bDisableBDPROCHOT)
		SetMenuItemInfo(hMenu, ID_DISABLE_BDPROCHOT, FALSE, &mii);
	if(g_bDisableRAPL)
		SetMenuItemInfo(hMenu, ID_DISABLE_RAPL, FALSE, &mii);
	if(g_bDisableMCH59A0)
		SetMenuItemInfo(hMenu, ID_DISABLE_MCH59A0, FALSE, &mii);
	SetMenuItemInfo(hMenu, (UINT)(g_GPUFreq + ID_GPU_FREQ_RESET), FALSE, &mii);
	if(g_TypeOverride != 0)
		SetMenuItemInfo(hMenu, ID_IDENTIFY, FALSE, &mii);
	if(!g_bSupportedGPU)
	{
		GetMenuItemInfo(hMenu, ID_SELECTION0, FALSE, &mii);
		mii.fState &= ~MFS_CHECKED;
		mii.fState |= MFS_DISABLED;
		SetMenuItemInfo(hMenu, ID_SELECTION0, FALSE, &mii);
		SetMenuItemInfo(hMenu, ID_SELECTION1, FALSE, &mii);
		SetMenuItemInfo(hMenu, ID_SELECTION2, FALSE, &mii);
		SetMenuItemInfo(hMenu, ID_SELECTION3, FALSE, &mii);
		SetMenuItemInfo(hMenu, ID_SELECTION4, FALSE, &mii);
		SetMenuItemInfo(hMenu, ID_GPU_FREQ_RESET, FALSE, &mii);
		SetMenuItemInfo(hMenu, ID_GPU_FREQ_HIGHEST, FALSE, &mii);
		SetMenuItemInfo(hMenu, ID_GPU_FREQ_LOWEST, FALSE, &mii);
	}
	mii.fMask = MIIM_STRING;
	mii.dwTypeData = g_GPUInfo1;
	SetMenuItemInfo(hMenu, ID_INFO1, FALSE, &mii);
	mii.dwTypeData = g_GPUInfo2;
	SetMenuItemInfo(hMenu, ID_INFO2, FALSE, &mii);
}

BOOL UpdateNotifyIcon()
{
	BOOL bResult;
	DWORD i;
	bResult = FALSE;
	i = 30;
	while(i > 0)
	{
		if(Shell_NotifyIcon(NIM_ADD, &g_Notify))
		{
			bResult = TRUE;
			break;
		}
		Shell_NotifyIcon(NIM_DELETE, &g_Notify);
		Sleep(1000);
		i--;
	}
	return bResult;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	TCHAR Temp[MAX_PATH];
	POINT Mouse;
	HMENU hMenu;
	DWORD Selection;
	DWORD GPUFreq;
	switch(uMsg)
	{
	case WM_CREATE:
		if(!Load())
		{
			MessageBox(hWnd, _T("Failed to load a DLL."), APPLICATION_NAME, MB_OK);
			DestroyWindow(hWnd);
			break;
		}
		_stprintf(Temp, _T("%s\\database.csv"), g_AppDir);
		if(!LoadDatabase(Temp))
		{
			MessageBox(hWnd, _T("Failed to load the database."), APPLICATION_NAME, MB_OK);
			DestroyWindow(hWnd);
			break;
		}
		g_hIcon = LoadIcon(g_hInstance, (LPCTSTR)IDI_ICON1);
		if(!g_hIcon)
		{
			MessageBox(hWnd, _T("Failed to load a resource."), APPLICATION_NAME, MB_OK);
			DestroyWindow(hWnd);
			break;
		}
		g_hMenu = LoadMenu(g_hInstance, (LPCTSTR)IDR_MENU1);
		if(!g_hMenu)
		{
			MessageBox(hWnd, _T("Failed to load a resource."), APPLICATION_NAME, MB_OK);
			DestroyWindow(hWnd);
			break;
		}
		ZeroMemory(&g_Notify, sizeof(NOTIFYICONDATA));
		g_Notify.cbSize = sizeof(NOTIFYICONDATA);
		g_Notify.uID = 0;
		g_Notify.hWnd = hWnd;
		g_Notify.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
		g_Notify.hIcon = g_hIcon;
		g_Notify.uCallbackMessage = WM_USER;
		_tcscpy(g_Notify.szTip, APPLICATION_NAME);
		if(!UpdateNotifyIcon())
		{
			MessageBox(hWnd, _T("Failed to initialize."), APPLICATION_NAME, MB_OK);
			DestroyWindow(hWnd);
			break;
		}
		LoadSettings();
		g_bSupportedGPU = IsSupportedGPU();
		GetGPUInfo(g_GPUInfo1, g_GPUInfo2);
		if(g_Selection > ID_SELECTION4 - ID_SELECTION0)
			g_Selection = 0;
		if(g_bSupportedGPU)
			ConfigureGPURegister(g_Selection);
		if(g_GPUFreq > ID_GPU_FREQ_LOWEST - ID_GPU_FREQ_RESET)
			g_GPUFreq = 0;
		if(g_bSupportedGPU)
			ConfigureGPURegister(g_GPUFreq + 5);
		if(g_bFreqNominal)
			FixCPUFrequencyNominal();
		if(g_bDisableEIST)
			EnableEIST(FALSE);
		if(g_bDisableIDA)
			EnableIDA(FALSE);
		if(g_bDisableC1E)
			EnableC1E(FALSE);
		if(g_bDisableBDPROCHOT)
			EnableBDPROCHOT(FALSE);
		if(g_bDisableRAPL)
			EnableRAPL(FALSE);
		if(g_bDisableMCH59A0)
			EnableMCH59A0(FALSE);
		UpdateMenu();
		g_TimerWaitCount = 0;
		if(!SetTimer(hWnd, 1, 100, NULL))
		{
			MessageBox(hWnd, _T("Failed to initialize."), APPLICATION_NAME, MB_OK);
			DestroyWindow(hWnd);
		}
		g_TaskbarCreated = RegisterWindowMessage(_T("TaskbarCreated"));
		g_hPowerNotify = RegisterPowerSettingNotification(hWnd, &GUID_LIDSWITCH_STATE_CHANGE, DEVICE_NOTIFY_WINDOW_HANDLE);
		break;
	case WM_DESTROY:
		UnregisterPowerSettingNotification(g_hPowerNotify);
		KillTimer(hWnd, 1);
		ReadPhysicalMemory(0, NULL, 0);
		Shell_NotifyIcon(NIM_DELETE, &g_Notify);
		Unload();
		UnloadDatabase();
		if(g_hIcon)
			DestroyIcon(g_hIcon);
		if(g_hMenu)
			DestroyMenu(g_hMenu);
		PostQuitMessage(0);
		break;
	case WM_DISPLAYCHANGE:
		if(g_bSupportedGPU)
			ConfigureGPURegister(g_Selection);
		break;
	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case ID_SAVE:
			SaveSettings();
			break;
		case ID_EXIT:
			DestroyWindow(hWnd);
			break;
		case ID_REGISTER:
			if(MessageBox(hWnd, _T("Do you want to run this program at the Windows startup?"), APPLICATION_NAME, MB_YESNO) == IDYES)
			{
				if(!SetStartup(TRUE))
					MessageBox(hWnd, _T("Failed to register to the startup."), APPLICATION_NAME, MB_OK);
			}
			break;
		case ID_UNREGISTER:
			if(MessageBox(hWnd, _T("Do you want to stop running this program at the Windows startup?"), APPLICATION_NAME, MB_YESNO) == IDYES)
			{
				if(!SetStartup(FALSE))
					MessageBox(hWnd, _T("Failed to unregister from the startup."), APPLICATION_NAME, MB_OK);
			}
			break;
		case ID_SELECTION0:
		case ID_SELECTION1:
		case ID_SELECTION2:
		case ID_SELECTION3:
		case ID_SELECTION4:
			Selection = LOWORD(wParam) - ID_SELECTION0;
			if(g_bSupportedGPU)
			{
				if(ConfigureGPURegister(Selection))
					g_Selection = Selection;
				else
					MessageBox(hWnd, _T("Failed to configure the registers."), APPLICATION_NAME, MB_OK);
			}
			UpdateMenu();
			break;
		case ID_FREQ_NOMINAL:
			if(MessageBox(hWnd, _T("Are you sure to change the configuration?\nChanges about power management may sometimes damage the device or make the system unstable."), APPLICATION_NAME, MB_YESNO) == IDYES)
			{
				g_bFreqNominal = (g_bFreqNominal ? FALSE : TRUE);
				if(g_bFreqNominal)
				{
					if(!FixCPUFrequencyNominal())
					{
						g_bFreqNominal = FALSE;
						MessageBox(hWnd, _T("Failed to configure the registers."), APPLICATION_NAME, MB_OK);
					}
				}
				else
				{
					if(!g_bDisableEIST)
						EnableEIST(TRUE);
				}
				UpdateMenu();
			}
			break;
		case ID_DISABLE_EIST:
			if(MessageBox(hWnd, _T("Are you sure to change the configuration?\nChanges about power management may sometimes damage the device or make the system unstable."), APPLICATION_NAME, MB_YESNO) == IDYES)
			{
				g_bDisableEIST = (g_bDisableEIST ? FALSE : TRUE);
				if(g_bDisableEIST)
				{
					if(!EnableEIST(FALSE))
					{
						g_bDisableEIST = FALSE;
						MessageBox(hWnd, _T("Failed to configure the registers."), APPLICATION_NAME, MB_OK);
					}
				}
				else
					EnableEIST(TRUE);
				UpdateMenu();
			}
			break;
		case ID_DISABLE_IDA:
			if(MessageBox(hWnd, _T("Are you sure to change the configuration?\nChanges about power management may sometimes damage the device or make the system unstable."), APPLICATION_NAME, MB_YESNO) == IDYES)
			{
				g_bDisableIDA = (g_bDisableIDA ? FALSE : TRUE);
				if(g_bDisableIDA)
				{
					if(!EnableIDA(FALSE))
					{
						g_bDisableIDA = FALSE;
						MessageBox(hWnd, _T("Failed to configure the registers."), APPLICATION_NAME, MB_OK);
					}
				}
				else
					EnableIDA(TRUE);
				UpdateMenu();
			}
			break;
		case ID_DISABLE_C1E:
			if(MessageBox(hWnd, _T("Are you sure to change the configuration?\nChanges about power management may sometimes damage the device or make the system unstable."), APPLICATION_NAME, MB_YESNO) == IDYES)
			{
				g_bDisableC1E = (g_bDisableC1E ? FALSE : TRUE);
				if(g_bDisableC1E)
				{
					if(!EnableC1E(FALSE))
					{
						g_bDisableC1E = FALSE;
						MessageBox(hWnd, _T("Failed to configure the registers."), APPLICATION_NAME, MB_OK);
					}
				}
				else
					EnableC1E(TRUE);
				UpdateMenu();
			}
			break;
		case ID_DISABLE_BDPROCHOT:
			if(MessageBox(hWnd, _T("Are you sure to change the configuration?\nChanges about power management may sometimes damage the device or make the system unstable."), APPLICATION_NAME, MB_YESNO) == IDYES)
			{
				g_bDisableBDPROCHOT = (g_bDisableBDPROCHOT ? FALSE : TRUE);
				if(g_bDisableBDPROCHOT)
				{
					if(!EnableBDPROCHOT(FALSE))
					{
						g_bDisableBDPROCHOT = FALSE;
						MessageBox(hWnd, _T("Failed to configure the registers."), APPLICATION_NAME, MB_OK);
					}
				}
				else
					EnableBDPROCHOT(TRUE);
				UpdateMenu();
			}
			break;
		case ID_DISABLE_RAPL:
			if(MessageBox(hWnd, _T("Are you sure to change the configuration?\nChanges about power management may sometimes damage the device or make the system unstable."), APPLICATION_NAME, MB_YESNO) == IDYES)
			{
				g_bDisableRAPL = (g_bDisableRAPL ? FALSE : TRUE);
				if(g_bDisableRAPL)
				{
					if(!EnableRAPL(FALSE))
					{
						g_bDisableRAPL = FALSE;
						MessageBox(hWnd, _T("Failed to configure the registers."), APPLICATION_NAME, MB_OK);
					}
				}
				else
					EnableRAPL(TRUE);
				UpdateMenu();
			}
			break;
		case ID_DISABLE_MCH59A0:
			if(MessageBox(hWnd, _T("Are you sure to change the configuration?\nChanges about power management may sometimes damage the device or make the system unstable."), APPLICATION_NAME, MB_YESNO) == IDYES)
			{
				g_bDisableMCH59A0 = (g_bDisableMCH59A0 ? FALSE : TRUE);
				if(g_bDisableMCH59A0)
				{
					if(!EnableMCH59A0(FALSE))
					{
						g_bDisableMCH59A0 = FALSE;
						MessageBox(hWnd, _T("Failed to configure the registers."), APPLICATION_NAME, MB_OK);
					}
				}
				else
					EnableMCH59A0(TRUE);
				UpdateMenu();
			}
			break;
		case ID_GPU_FREQ_RESET:
		case ID_GPU_FREQ_HIGHEST:
		case ID_GPU_FREQ_LOWEST:
			GPUFreq = LOWORD(wParam) - ID_GPU_FREQ_RESET;
			if(g_bSupportedGPU)
			{
				if(ConfigureGPURegister(GPUFreq + 5))
					g_GPUFreq = GPUFreq;
				else
					MessageBox(hWnd, _T("Failed to configure the registers."), APPLICATION_NAME, MB_OK);
			}
			UpdateMenu();
			break;
		case ID_IDENTIFY:
			if(IdentifyGPUByQuestions())
			{
				g_bSupportedGPU = IsSupportedGPU();
				UpdateMenu();
			}
			break;
		}
		break;
	case WM_TIMER:
		switch(wParam)
		{
		case 1:
			if(g_GPUFreq != 0)
			{
				if(g_bSupportedGPU)
					ConfigureGPURegister(g_GPUFreq + 5);
			}
			if(g_TimerWaitCount > 0)
			{
				g_TimerWaitCount--;
				if(g_TimerWaitCount == 0)
				{
					if(g_bSupportedGPU)
						ConfigureGPURegister(g_Selection);
					if(g_bFreqNominal)
						FixCPUFrequencyNominal();
					if(g_bDisableEIST)
						EnableEIST(FALSE);
					if(g_bDisableIDA)
						EnableIDA(FALSE);
					if(g_bDisableC1E)
						EnableC1E(FALSE);
					if(g_bDisableBDPROCHOT)
						EnableBDPROCHOT(FALSE);
					if(g_bDisableRAPL)
						EnableRAPL(FALSE);
					if(g_bDisableMCH59A0)
						EnableMCH59A0(FALSE);
				}
			}
			break;
		}
		break;
	case WM_POWERBROADCAST:
		g_TimerWaitCount = 5;
		return TRUE;
		break;
	case WM_DEVICECHANGE:
		g_TimerWaitCount = 5;
		break;
	case WM_USER:
		switch(lParam)
		{
		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:
			SetForegroundWindow(hWnd);
			GetCursorPos(&Mouse);
			hMenu = GetSubMenu(g_hMenu, 0);
			TrackPopupMenu(hMenu, TPM_LEFTALIGN, Mouse.x, Mouse.y, 0, hWnd, NULL);
			PostMessage(hWnd, WM_NULL, 0, 0);
			break;
		}
		break;
	default:
		if(g_TaskbarCreated != 0 && uMsg == g_TaskbarCreated)
		{
			if(!UpdateNotifyIcon())
			{
				MessageBox(hWnd, _T("Failed to initialize."), APPLICATION_NAME, MB_OK);
				DestroyWindow(hWnd);
				break;
			}
		}
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
		break;
	}
	return 0;
}

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
	WNDCLASSEX Class;
	MSG msg;
	g_hInstance = hInstance;
	SetAppDir();
	ZeroMemory(&Class, sizeof(WNDCLASSEX));
	Class.cbSize = sizeof(WNDCLASSEX);
	Class.style = CS_CLASSDC;
	Class.lpfnWndProc = WndProc;
	Class.hInstance = g_hInstance;
	Class.hCursor = LoadCursor(NULL, IDC_ARROW);
	Class.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	Class.lpszClassName = APPLICATION_NAME;
	RegisterClassEx(&Class);
	g_hWnd = CreateWindowEx(0, Class.lpszClassName, APPLICATION_NAME, 0, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, NULL, NULL);
	if(!g_hWnd)
		return 0;
	while(true)
	{
		if(PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
		{
			if(!GetMessage(&msg, NULL, 0, 0))
				break;
			DispatchMessage(&msg);
		}
		else
			WaitMessage();
	}
	return (int)msg.wParam;
}

