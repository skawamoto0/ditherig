typedef BOOL (__stdcall* WinRing0_InitializeOls)();
typedef void (__stdcall* WinRing0_DeinitializeOls)();
typedef BOOL (__stdcall* WinRing0_Rdmsr)(DWORD, DWORD*, DWORD*);
typedef BOOL (__stdcall* WinRing0_Wrmsr)(DWORD, DWORD, DWORD);
typedef BOOL (__stdcall* WinRing0_ReadPciConfigWordEx)(DWORD, DWORD, PWORD);
typedef BOOL (__stdcall* WinRing0_ReadPciConfigDwordEx)(DWORD, DWORD, PDWORD);
typedef DWORD (__stdcall* WinRing0_FindPciDeviceById)(WORD, WORD, BYTE);
typedef DWORD (__stdcall* WinRing0_FindPciDeviceByClass)(BYTE, BYTE, BYTE, BYTE);

extern WinRing0_InitializeOls InitializeOls;
extern WinRing0_DeinitializeOls DeinitializeOls;
extern WinRing0_Rdmsr Rdmsr;
extern WinRing0_Wrmsr Wrmsr;
extern WinRing0_ReadPciConfigWordEx ReadPciConfigWordEx;
extern WinRing0_ReadPciConfigDwordEx ReadPciConfigDwordEx;
extern WinRing0_FindPciDeviceById FindPciDeviceById;
extern WinRing0_FindPciDeviceByClass FindPciDeviceByClass;

BOOL LoadWinRing0(LPCTSTR FileName);
void UnloadWinRing0();

