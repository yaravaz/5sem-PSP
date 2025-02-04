#include "stdafx.h"

//#define MAILSLOT_NAME L"//./mailslot/Box"
//#define MAILSLOT_NAME L"//Marta/mailslot/Box"
//#define MAILSLOT_NAME L"//DESKTOP-9MC22NJ/mailslot/Box"
#define MAILSLOT_NAME L"//*/mailslot/Box"

int main()
{
	setlocale(LC_ALL, "rus");
	cout << "Client" << endl;

	HANDLE hM;
	DWORD wb; 
	char obuf[300];
	try
	{
		if ((hM = CreateFile(MAILSLOT_NAME, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL)) == INVALID_HANDLE_VALUE)
			throw SetErrorMsgText("error CreateFile: ", GetLastError());

		for (int i = 0; i < 10; i++)
		{
			string obufstr = "Hello from Maislot-client " + to_string(i + 1);
			strcpy_s(obuf, obufstr.c_str());
			if (!WriteFile(hM, obuf, sizeof(obuf)+1, &wb, NULL))//для локального использовать strlen(obuf) + 1
				throw SetErrorMsgText("error WriteFile: ", GetLastError());
		}

		strcpy_s(obuf,  "STOP");
		if (!WriteFile(hM, obuf, strlen(obuf) + 1, &wb, NULL))
			throw SetErrorMsgText("error WriteFile: ", GetLastError());
		/*if (!WriteFile(hM, "", 0, &wb, NULL))
            throw SetErrorMsgText("error WriteFile: ", GetLastError());*/
		CloseHandle(hM);
	}
	catch(string err)
	{ 
		cout << err << endl;
	}
}