#include "stdafx.h"

#define PIPE_NAME L"//./pipe/Tube"

int main()
{
    setlocale(LC_ALL, "Rus");
    cout << "Client" << endl;

    HANDLE hPipe;
    DWORD ps, state = PIPE_READMODE_MESSAGE;

    try
    {
        if ((hPipe = CreateFile(PIPE_NAME, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, NULL, NULL)) == INVALID_HANDLE_VALUE)
            throw SetPipeError("create file error: ", GetLastError());

        if (!SetNamedPipeHandleState(hPipe, &state, NULL, NULL))
            throw SetPipeError("dynamic settings error: ", GetLastError());

        int messageCount = 100;
        char buf[50], rbuf[50];
        string zero = "";
        for (int i = 1; i <= messageCount; i++)
        {
            string wbuf = "Hello from Client " + to_string(i);
            strcpy_s(buf, wbuf.c_str());

            if (TransactNamedPipe(hPipe, buf, sizeof(buf), rbuf, sizeof(rbuf), &ps, NULL)) {
                cout << rbuf << endl;
            }
            else {
                throw SetPipeError("read-write error:", GetLastError());
            }
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
