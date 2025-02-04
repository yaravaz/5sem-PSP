#include "stdafx.h"

#define PIPE_NAME L"//./pipe/Tube"

int main()
{
    setlocale(LC_ALL, "Rus");
    cout << "Client" << endl;

    DWORD ps;

    try
    {
        char buf[50], rbuf[50];
        string wbuf = "Hello from Client ";
        strcpy_s(buf, wbuf.c_str());

        if (CallNamedPipe(PIPE_NAME, buf, wbuf.length() + 1, rbuf, sizeof(rbuf), &ps, NULL)) {
            cout << rbuf << endl;
        }
        else {
            throw SetPipeError("read-write error:", GetLastError());
        }
    }
    catch (string error)
    {
        cout << error << endl;
    }

}
