typedef void (__stdcall* InpOut32_Out32)(short, short);
typedef short (__stdcall* InpOut32_Inp32)(short);
typedef BOOL (__stdcall* InpOut32_IsInpOutDriverOpen)();
typedef BOOL (__stdcall* InpOut32_IsXP64Bit)();
typedef UCHAR (__stdcall* InpOut32_DlPortReadPortUchar)(USHORT);
typedef void (__stdcall* InpOut32_DlPortWritePortUchar)(USHORT, UCHAR);
typedef USHORT (__stdcall* InpOut32_DlPortReadPortUshort)(USHORT);
typedef void (__stdcall* InpOut32_DlPortWritePortUshort)(USHORT, USHORT);
typedef ULONG (__stdcall* InpOut32_DlPortReadPortUlong)(ULONG);
typedef void (__stdcall* InpOut32_DlPortWritePortUlong)(ULONG, ULONG);
typedef PBYTE (__stdcall* InpOut32_MapPhysToLin)(PBYTE, DWORD, HANDLE*);
typedef BOOL (__stdcall* InpOut32_UnmapPhysicalMemory)(HANDLE, PBYTE);
typedef BOOL (__stdcall* InpOut32_GetPhysLong)(PBYTE, PDWORD);
typedef BOOL (__stdcall* InpOut32_SetPhysLong)(PBYTE, DWORD);

extern InpOut32_Out32 Out32;
extern InpOut32_Inp32 Inp32;
extern InpOut32_IsInpOutDriverOpen IsInpOutDriverOpen;
extern InpOut32_IsXP64Bit IsXP64Bit;
extern InpOut32_DlPortReadPortUchar DlPortReadPortUchar;
extern InpOut32_DlPortWritePortUchar DlPortWritePortUchar;
extern InpOut32_DlPortReadPortUshort DlPortReadPortUshort;
extern InpOut32_DlPortWritePortUshort DlPortWritePortUshort;
extern InpOut32_DlPortReadPortUlong DlPortReadPortUlong;
extern InpOut32_DlPortWritePortUlong DlPortWritePortUlong;
extern InpOut32_MapPhysToLin MapPhysToLin;
extern InpOut32_UnmapPhysicalMemory UnmapPhysicalMemory;
extern InpOut32_GetPhysLong GetPhysLong;
extern InpOut32_SetPhysLong SetPhysLong;

BOOL LoadInpOut32(LPCTSTR FileName);
void UnloadInpOut32();

