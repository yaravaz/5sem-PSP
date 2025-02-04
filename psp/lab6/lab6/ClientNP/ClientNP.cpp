#include "stdafx.h"

//#define PIPE_NAME L"//./pipe/Tube"
#define PIPE_NAME L"//Marta/pipe/Tube"

int main()
{
    setlocale(LC_ALL, "Rus");
    cout << "Client" << endl;

    HANDLE hPipe;
    DWORD ps;

    try
    {
        if ((hPipe = CreateFile(PIPE_NAME, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, NULL, NULL)) == INVALID_HANDLE_VALUE)
            throw SetPipeError("create file error: ", GetLastError());

        int messageCount = 100;
        char rbuf[50];
        string zero = "";
        for (int i = 1; i <= messageCount; i++)
        {
            string wbuf = "Hello from Client " + to_string(i);

            if (!WriteFile(hPipe, wbuf.c_str(), wbuf.length() + 1, &ps, NULL))
                throw SetPipeError("write file error: ", GetLastError());

            if (ReadFile(hPipe, rbuf, sizeof(rbuf), &ps, NULL))
                cout << rbuf << endl;
            else
                throw SetPipeError("read file error: ", GetLastError());
        }

        if (!WriteFile(hPipe, zero.c_str(), zero.length() + 1, &ps, NULL))
            throw SetPipeError("write file error: ", GetLastError());

        CloseHandle(hPipe);
    }
    catch (string error)
    {
        cout << error << endl;
    }

}
