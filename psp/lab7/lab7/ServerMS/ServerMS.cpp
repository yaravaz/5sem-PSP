#include "stdafx.h"

#define MAILSLOT_NAME L"//./mailslot/Box"

int main()
{
    setlocale(LC_ALL, "rus");
    cout << "Server" << endl;

    HANDLE hM;
    DWORD rb;
    char rbuf[500];
    DWORD time_waiting = 180000;
    bool flag = true;
    clock_t start, end;
    try
    {
        if ((hM = CreateMailslot(MAILSLOT_NAME, 500, time_waiting, NULL)) == INVALID_HANDLE_VALUE)
            throw SetErrorMsgText("error CreateMailslot: ", GetLastError());

        start = clock();

        while (true)
        {

            if (ReadFile(hM, rbuf, sizeof(rbuf), &rb, NULL) && strlen(rbuf) != 0)
            {
                rbuf[rb] = '\0';
                cout << rbuf << endl;
                if (flag)
                {
                    start = clock();
                    flag = false;
                }
            }
            else {
                end = clock();
                double elapsed_time = (double)(end - start) / CLOCKS_PER_SEC;

                if (elapsed_time >= time_waiting / 1000)
                {
                    cout << "Server stopped waiting after 3 minutes" << endl;
                    break;
                }

                throw SetErrorMsgText("error ReadFile: ", GetLastError());
            }

            if (strcmp(rbuf, "STOP") == 0)
            {
                end = clock();
                cout << ((double)(end - start) / CLK_TCK) << " sec" << endl;
                flag = true;
            }
        }

        CloseHandle(hM);
    }
    catch (string err)
    {
        cout << err << endl;
    }
}