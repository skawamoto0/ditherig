#include "stdafx.h"
#include "APIServer.h"

struct APISERVERCONTEXT
{
	BYTE SecretKey[32];
	APIServerCallback Callback;
	HANDLE hPipe;
	HANDLE hThread;
};

BOOL GetHMACSHA256(void* pOut, const void* pKey, DWORD KeySize, const void* pMessage, DWORD MessageSize)
{
	BOOL bResult;
	BCRYPT_ALG_HANDLE Alg;
	BCRYPT_HASH_HANDLE Hash;
	bResult = FALSE;
	if(BCRYPT_SUCCESS(BCryptOpenAlgorithmProvider(&Alg, BCRYPT_SHA256_ALGORITHM, NULL, BCRYPT_ALG_HANDLE_HMAC_FLAG)))
	{
		if(BCRYPT_SUCCESS(BCryptCreateHash(Alg, &Hash, NULL, 0, (PUCHAR)pKey, KeySize, 0)))
		{
			if(BCRYPT_SUCCESS(BCryptHashData(Hash, (PUCHAR)pMessage, MessageSize, 0)))
			{
				if(BCRYPT_SUCCESS(BCryptFinishHash(Hash, (PUCHAR)pOut, 32, 0)))
					bResult = TRUE;
			}
			BCryptDestroyHash(Hash);
		}
		BCryptCloseAlgorithmProvider(Alg, 0);
	}
	return bResult;
}

DWORD CALLBACK APIServerThreadProc(LPVOID lpParameter)
{
	APISERVERCONTEXT* pContext;
	ULARGE_INTEGER LastTime;
	APISERVERMESSAGE Data;
	DWORD Size;
	ULARGE_INTEGER RecievedTime;
	BYTE* pBuffer;
	BYTE Digest[32];
	pContext = (APISERVERCONTEXT*)lpParameter;
	GetSystemTimeAsFileTime((LPFILETIME)&LastTime);
	while(true)
	{
		if(!ConnectNamedPipe(pContext->hPipe, NULL))
		{
			if(GetLastError() != ERROR_PIPE_CONNECTED)
				break;
		}
		if(ReadFile(pContext->hPipe, &Data, (DWORD)sizeof(APISERVERMESSAGE), &Size, NULL))
		{
			GetSystemTimeAsFileTime((LPFILETIME)&RecievedTime);
			if(RecievedTime.QuadPart - LastTime.QuadPart >= 10000000ull && RecievedTime.QuadPart >= Data.Timestamp.QuadPart && RecievedTime.QuadPart - Data.Timestamp.QuadPart < 10000000ull)
			{
				if(Data.Length < 65536)
				{
					pBuffer = new BYTE[sizeof(APISERVERMESSAGE) + Data.Length];
					if(pBuffer)
					{
						memcpy(pBuffer, &Data, sizeof(APISERVERMESSAGE));
						if(ReadFile(pContext->hPipe, pBuffer + sizeof(APISERVERMESSAGE), Data.Length, &Size, NULL))
						{
							if(GetHMACSHA256(&Digest, &pContext->SecretKey, 32, pBuffer + 32, (DWORD)sizeof(APISERVERMESSAGE) - 32 + Data.Length))
							{
								if(memcmp(&Digest, &Data.Digest, 32) == 0)
								{
									LastTime = RecievedTime;
									pContext->Callback((APISERVERMESSAGE*)pBuffer, pContext->hPipe);
								}
							}
						}
						delete[]pBuffer;
					}
				}
			}
		}
		DisconnectNamedPipe(pContext->hPipe);
	}
	return 0;
}

BOOL CreateSecretKey(void* pSecretKey, void* pIdentifier, DWORD Length)
{
	BOOL bResult;
	BYTE Secret[32];
	TCHAR Dir[MAX_PATH];
	TCHAR Path[MAX_PATH];
	HANDLE hFile;
	FILETIME Time;
	HKEY hKey;
	DWORD Data;
	WCHAR Value[39];
	CLSID Id;
	bResult = FALSE;
	ZeroMemory(&Secret, 32);
	if(GetSystemDirectory(Dir, MAX_PATH))
	{
		_stprintf(Path, _T("%s\\config\\DRIVERS"), Dir);
		hFile = CreateFile(Path, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
		if(hFile != INVALID_HANDLE_VALUE)
		{
			if(GetFileTime(hFile, &Time, NULL, NULL))
				memcpy(&Secret[0], &Time, 8);
			CloseHandle(hFile);
		}
		_stprintf(Path, _T("%s\\config\\BCD-Template"), Dir);
		hFile = CreateFile(Path, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
		if(hFile != INVALID_HANDLE_VALUE)
		{
			if(GetFileTime(hFile, &Time, NULL, NULL))
				memcpy(&Secret[8], &Time, 8);
			CloseHandle(hFile);
		}
	}
	if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("BCD00000000\\Objects\\{9dea862c-5cdd-4e70-acc1-f32b344d4795}\\Elements\\23000003"), 0, KEY_READ, &hKey) == ERROR_SUCCESS)
	{
		Data = sizeof(WCHAR) * 39;
		if(RegQueryValueEx(hKey, _T("Element"), NULL, NULL, (BYTE*)&Value, &Data) == ERROR_SUCCESS)
		{
			if(CLSIDFromString(Value, &Id) == NOERROR)
				memcpy(&Secret[16], &Id, 16);
		}
		RegCloseKey(hKey);
	}
	if(GetHMACSHA256(pSecretKey, &Secret, 32, pIdentifier, Length))
		bResult = TRUE;
	return bResult;
}

void* StartAPIServer(LPCTSTR Endpoint, const void* pSecretKey, APIServerCallback Callback)
{
	void* pResult;
	APISERVERCONTEXT* pContext;
	SECURITY_DESCRIPTOR sd;
	SECURITY_ATTRIBUTES sa;
	pResult = NULL;
	pContext = new APISERVERCONTEXT;
	if(pContext)
	{
		memcpy(&pContext->SecretKey, pSecretKey, 32);
		pContext->Callback = Callback;
		if(InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION))
		{
			if(SetSecurityDescriptorDacl(&sd, TRUE, NULL, FALSE))
			{
				ZeroMemory(&sa, sizeof(SECURITY_ATTRIBUTES));
				sa.nLength = sizeof(SECURITY_ATTRIBUTES);
				sa.lpSecurityDescriptor = &sd;
				sa.bInheritHandle = FALSE;
				pContext->hPipe = CreateNamedPipe(Endpoint, PIPE_ACCESS_DUPLEX | FILE_FLAG_FIRST_PIPE_INSTANCE, PIPE_WAIT | PIPE_READMODE_BYTE | PIPE_TYPE_BYTE, 1, 4096, 4096, 0, &sa);
				if(pContext->hPipe != INVALID_HANDLE_VALUE)
				{
					if(pContext->hThread = CreateThread(NULL, 0, APIServerThreadProc, pContext, 0, NULL))
						pResult = pContext;
				}
			}
		}
		if(!pResult)
			StopAPIServer(pContext);
	}
	return pResult;
}

BOOL StopAPIServer(void* pHandle)
{
	BOOL bResult;
	APISERVERCONTEXT* pContext;
	bResult = FALSE;
	pContext = (APISERVERCONTEXT*)pHandle;
	if(pContext->hPipe != INVALID_HANDLE_VALUE)
	{
		if(pContext->hThread)
		{
			while(WaitForSingleObject(pContext->hThread, 1) != WAIT_OBJECT_0)
				CancelIoEx(pContext->hPipe, NULL);
			CloseHandle(pContext->hThread);
			bResult = TRUE;
		}
		CloseHandle(pContext->hPipe);
	}
	delete pContext;
	return bResult;
}

