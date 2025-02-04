#include "stdafx.h"

char* get_command_by_id(int id)
{
	switch (id)
	{
	case 1: return (char*)"start";
	case 2: return (char*)"stop";
	case 3: return (char*)"wait";
	case 4: return (char*)"statistics";
	case 5: return (char*)"exit";
	case 6: return (char*)"shutdown";
	case 7: return (char*)"load_lib";
	case 8: return (char*)"unload_lib";
	case 9: return (char*)"call_function";
	default: return (char*)"error";
	}
}
int _tmain(int argc, _TCHAR* argv[])
{
	setlocale(LC_ALL, "");
	char rbuf[200];

	DWORD dwRead;
	DWORD dwWrite;
	HANDLE hPipe;
	try
	{
		if ((hPipe = CreateFile(
			L"\\\\.\\pipe\\Tube",
			//L"\\\\Marta\\pipe\\Tube",
			GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL, OPEN_EXISTING, NULL,
			NULL)) == INVALID_HANDLE_VALUE)
			throw  SetPipeError("createfile:", GetLastError());
	}
	catch (string ErrorPipeText)
	{
		printf("\n%s\n", ErrorPipeText.c_str());
		return -1;
	}
	char wbuf[40] = "start";
	char ebuf[40] = "error";
	while (true) 
	{
		char param[20];
		int serverSendCommand = 0;
		puts("сhoose: start ~ 1, stop ~ 2, wait ~ 3, statistic ~ 4, exit ~ 5, shutdown ~ 6, load_lib ~ 7, unload_lib ~ 8, call_function ~ 9");
		scanf("%d", &serverSendCommand);
		strcpy(wbuf, get_command_by_id(serverSendCommand));
		system("cls");
		if (serverSendCommand == 7 || serverSendCommand == 8) {
			printf("Enter the library name: ");
			scanf("%s", param);
			strcat(wbuf, " ");   
			strcat(wbuf, param); 
		}

		WriteFile(hPipe, wbuf, sizeof(wbuf), &dwWrite, NULL);

		printf("send:  %s\n", wbuf);
		if (serverSendCommand == 6)
			break;
		ReadFile(hPipe, rbuf, sizeof(rbuf), &dwRead, NULL);
		if (!strcmp(wbuf, "error")) 
		{
			printf("send: nocmd\n");
		}
		else
			printf("receive: %s\n", rbuf);
	}
	CloseHandle(hPipe);
	system("pause");
	return 0;
}