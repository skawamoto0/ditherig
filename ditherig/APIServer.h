#pragma pack(push, 1)
struct APISERVERMESSAGE
{
	BYTE Digest[32];
	ULARGE_INTEGER Timestamp;
	DWORD Length;
};
#pragma pack(pop)

typedef BOOL(__stdcall* APIServerCallback)(APISERVERMESSAGE*, HANDLE);

BOOL CreateSecretKey(void* pSecretKey, void* pIdentifier, DWORD Length);
void* StartAPIServer(LPCTSTR Endpoint, const void* pSecretKey, APIServerCallback Callback);
BOOL StopAPIServer(void* pHandle);

