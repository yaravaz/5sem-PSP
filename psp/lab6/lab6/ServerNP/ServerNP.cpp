#include "stdafx.h"

#define PIPE_NAME L"//./pipe/Tube"

int main()
{
	setlocale(LC_ALL, "Rus");
	cout << "Server" << endl;

	HANDLE hPipe;
	DWORD ps;

	try
	{
		SECURITY_ATTRIBUTES sa;
		SECURITY_DESCRIPTOR sd;

		InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
		SetSecurityDescriptorDacl(&sd, TRUE, NULL, FALSE);

		sa.nLength = sizeof(SECURITY_ATTRIBUTES);
		sa.lpSecurityDescriptor = &sd;
		sa.bInheritHandle = FALSE;

		if ((hPipe = CreateNamedPipe(PIPE_NAME, PIPE_ACCESS_DUPLEX, PIPE_TYPE_MESSAGE | PIPE_WAIT, 1, NULL, NULL, NMPWAIT_USE_DEFAULT_WAIT, &sa)) == INVALID_HANDLE_VALUE)
			throw SetPipeError("create error: ", GetLastError());

		while (true) {
			cout << "Waiting..." << endl;

			if (!ConnectNamedPipe(hPipe, NULL)) 
				throw SetPipeError("connect error: ", GetLastError());

			char rbuf[50];

			while (true) {
				if (!ReadFile(hPipe, rbuf, sizeof(rbuf), &ps, NULL))
				{
					throw SetPipeError("read file error: ", GetLastError());
				}

				if (strlen(rbuf) == 0) break;

				cout << rbuf << endl;

				if (!WriteFile(hPipe, rbuf, sizeof(rbuf), &ps, NULL))
					throw "write file error: ";
			}

			if (!DisconnectNamedPipe(hPipe))
				throw SetPipeError("disconnect error: ", GetLastError());
			else break;
		}
		if (!CloseHandle(hPipe))
			throw SetPipeError("close error: ", GetLastError());

	}
	catch (string error)
	{
		cout << error << endl;
	}

}

