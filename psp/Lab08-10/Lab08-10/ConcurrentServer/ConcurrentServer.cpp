#include "stdafx.h"
#include "Winsock2.h"
#include "Errors.h"
#include <string>
#include <list>
#include <time.h>
#include <iostream>
#include <Shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")

#define AS_SQUIRT 10
using namespace std;

SOCKET sS;
int serverPort;
char dllName[50];
char namedPipeName[10];

volatile long connectionCount = 0;
volatile long deniedConnections = 0;
volatile long successConnections = 0;
volatile long activeConnections = 0;

HANDLE hAcceptServer, hConsolePipe, hGarbageCleaner, hDispatchServer, hResponseServer;
HANDLE hClientConnectedEvent = CreateEvent(NULL, FALSE, FALSE, L"ClientConnected");

DWORD WINAPI AcceptServer(LPVOID pPrm);
DWORD WINAPI ConsolePipe(LPVOID pPrm);
DWORD WINAPI GarbageCleaner(LPVOID pPrm);
DWORD WINAPI DispatchServer(LPVOID pPrm);
DWORD WINAPI ResponseServer(LPVOID pPrm);

CRITICAL_SECTION scListContact;

enum TalkersCommand 
{
	START, STOP, EXIT, STATISTICS, WAIT, SHUTDOWN, GETCOMMAND, LOAD_LIB, UNLOAD_LIB, CALL_FUNCTION
};
volatile TalkersCommand  previousCommand = GETCOMMAND;

struct Contact
{
	enum TE 
	{
		EMPTY,
		ACCEPT,
		CONTACT
	}    type;
	enum ST 
	{
		WORK,
		ABORT,
		TIMEOUT,
		FINISH
	}      sthread;

	SOCKET      s;
	SOCKADDR_IN prms;
	int         lprms;
	HANDLE      hthread;
	HANDLE      htimer;
	HANDLE		serverHThread;

	char msg[50];
	char srvname[15];

	Contact(TE t = EMPTY, const char* namesrv = "")
	{
		ZeroMemory(&prms, sizeof(SOCKADDR_IN));
		lprms = sizeof(SOCKADDR_IN);
		type = t;
		strcpy(srvname, namesrv);
		msg[0] = 0x00;
	};

	void SetST(ST sth, const char* m = "")
	{
		sthread = sth;
		strcpy(msg, m);
	}
};
typedef list<Contact> ListContact;

ListContact contacts;

void CALLBACK ASWTimer(LPVOID Prm, DWORD, DWORD) {
	Contact* contact = (Contact*)(Prm);
	printf("timer is over");
	TerminateThread(contact->serverHThread, NULL);
	send(contact->s, "TimeOUT", strlen("TimeOUT") + 1, NULL);
	EnterCriticalSection(&scListContact);
	CancelWaitableTimer(contact->htimer);
	contact->type = contact->EMPTY;
	contact->sthread = contact->TIMEOUT;
	LeaveCriticalSection(&scListContact);
}

bool  GetRequestFromClient(char* name, short port, SOCKADDR_IN* from, int* flen);

bool AcceptCycle(int squirt) {
	bool rc = false;
	Contact c(Contact::ACCEPT, "EchoServer");
	while (squirt-- > 0 && !rc) {
		if ((c.s = accept(sS, (sockaddr*)&c.prms, &c.lprms)) == INVALID_SOCKET) {
			if (WSAGetLastError() != WSAEWOULDBLOCK)
				throw  SetErrorMsgText("error accept: ", WSAGetLastError());
		}
		else {
			rc = true;
			EnterCriticalSection(&scListContact);
			contacts.push_front(c);
			auto i = contacts.begin();
			LeaveCriticalSection(&scListContact);
			puts("contact connected");
			InterlockedIncrement(&connectionCount);
			i->htimer = CreateWaitableTimer(0, FALSE, 0);
			LARGE_INTEGER Li;
			int seconds = 60;
			Li.QuadPart = -(10000000 * seconds);
			SetWaitableTimer(i->htimer, &Li, 0, ASWTimer, (LPVOID) & (*i), FALSE);
		}
	}
	return rc;
};

void openSocket() {
	SOCKADDR_IN serv;
	sockaddr_in clnt;
	u_long nonblk = 1;

	if ((sS = socket(AF_INET, SOCK_STREAM, NULL)) == INVALID_SOCKET)
		throw  SetErrorMsgText("error socket: ", WSAGetLastError());

	int lclnt = sizeof(clnt);
	serv.sin_family = AF_INET;
	serv.sin_port = htons(serverPort);
	serv.sin_addr.s_addr = INADDR_ANY;

	if (bind(sS, (LPSOCKADDR)&serv, sizeof(serv)) == SOCKET_ERROR)
		throw  SetErrorMsgText("error bind: ", WSAGetLastError());
	if (listen(sS, SOMAXCONN) == SOCKET_ERROR)
		throw  SetErrorMsgText("error listen: ", WSAGetLastError());
	if (ioctlsocket(sS, FIONBIO, &nonblk) == SOCKET_ERROR)
		throw SetErrorMsgText("error ioctlsocket: ", WSAGetLastError());
}

void closeSocket() {
	if (closesocket(sS) == SOCKET_ERROR)
		throw  SetErrorMsgText("error closesocket: ", WSAGetLastError());
}

void CommandsCycle(TalkersCommand& cmd) {
	int squirt = 0;
	while (cmd != EXIT) {
		switch (cmd) {
		case START: 
			cmd = GETCOMMAND;
			if (previousCommand != START) {
				squirt = AS_SQUIRT;
				puts("START");
				openSocket();
				previousCommand = START;
			}
			else 
				puts("START was already used");
			break;
		case STOP:  
			cmd = GETCOMMAND;
			if (previousCommand != STOP) {
				squirt = 0;
				puts("STOP");
				closeSocket();
				previousCommand = STOP;
			}
			else 
				puts("STOP was already used");
			break;
		case WAIT:  
			cmd = GETCOMMAND;
			squirt = 0;
			puts("WAITing for other clients");
			closeSocket();
			while (contacts.size() != 0);
			printf("contacts: %d\n", contacts.size());
			cmd = START;
			previousCommand = WAIT;
			puts("socket is open");
			break;
		case SHUTDOWN:
			squirt = 0;
			puts("SHUTDOWN");
			closeSocket();
			while (contacts.size() != 0);
			printf("contacts: %d\n", contacts.size());
			cmd = EXIT;
			break;
		case GETCOMMAND:  
			cmd = GETCOMMAND;
			break;
		};
		if (cmd != STOP) {

			if (AcceptCycle(squirt)){
				cmd = GETCOMMAND;
				SetEvent(hClientConnectedEvent);
			}
			else SleepEx(0, TRUE);
		}
	};
};

DWORD WINAPI AcceptServer(LPVOID pPrm) {
	DWORD rc = 0;
	WSADATA wsaData;
	try {
		if (WSAStartup(MAKEWORD(2, 0), &wsaData) != 0)
			throw  SetErrorMsgText("error wsastartup: ", WSAGetLastError());

		CommandsCycle(*((TalkersCommand*)pPrm));

		if (WSACleanup() == SOCKET_ERROR)
			throw SetErrorMsgText("error cleanup: ", WSAGetLastError());
	}
	catch (string errorMsgText){
		printf("\n%s\n", errorMsgText.c_str());
	}
	puts("shutdown acceptServer");
	ExitThread(rc);
}

//TalkersCommand set_param(char* param) {
//	if (!strcmp(param, "start"))      return START;
//	if (!strcmp(param, "stop"))		  return STOP;
//	if (!strcmp(param, "exit"))		  return EXIT;
//	if (!strcmp(param, "wait"))		  return WAIT;
//	if (!strcmp(param, "shutdown"))	  return SHUTDOWN;
//	if (!strcmp(param, "statistics")) return STATISTICS;
//	if (!strcmp(param, "getcommand")) return GETCOMMAND;
//
//	char* prm = param;
//	char* command = strtok(prm, " "); 
//	if (command != NULL) {
//		if (!strcmp(command, "load_lib")) {
//			return LOAD_LIB;
//		}
//		if (!strcmp(command, "unload_lib")) {
//			return UNLOAD_LIB;
//		}
//	}
//}
TalkersCommand set_param(const char* param) {
	char* copiedParam = _strdup(param);
	if (!strcmp(copiedParam, "start"))      return START;
	if (!strcmp(copiedParam, "stop"))       return STOP;
	if (!strcmp(copiedParam, "exit"))       return EXIT;
	if (!strcmp(copiedParam, "wait"))       return WAIT;
	if (!strcmp(copiedParam, "shutdown"))   return SHUTDOWN;
	if (!strcmp(copiedParam, "statistics"))  return STATISTICS;
	if (!strcmp(copiedParam, "getcommand"))  return GETCOMMAND;
	if (!strcmp(copiedParam, "call_function"))  return CALL_FUNCTION;

	char* command = strtok(copiedParam, " ");
	if (command != NULL) {
		if (!strcmp(command, "load_lib")) {
			free(copiedParam);
			return LOAD_LIB;
		}
		if (!strcmp(command, "unload_lib")) {
			free(copiedParam);
			return UNLOAD_LIB;
		}
	}
	free(copiedParam);
}

typedef void* (*FUNCTION)(char*, LPVOID);
FUNCTION ts;

volatile bool is_load_library = false;
list<HMODULE> list_of_dlls;
list<FUNCTION> list_of_functions; 
typedef int (*FUNC)(int, int);

HMODULE st;
SOCKET sSUDP;

//DWORD WINAPI ConsolePipe(LPVOID pPrm) {
//	DWORD rc = 0;
//	char rbuf[100];
//	DWORD dwRead, dwWrite;
//	HANDLE hPipe;
//	try {
//		char namedPipeConnectionString[50];
//		sprintf(namedPipeConnectionString, "\\\\.\\pipe\\%s", namedPipeName);
//
//		SECURITY_ATTRIBUTES sa;
//		SECURITY_DESCRIPTOR sd;
//
//		InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
//		SetSecurityDescriptorDacl(&sd, TRUE, NULL, FALSE);
//
//		sa.nLength = sizeof(SECURITY_ATTRIBUTES);
//		sa.lpSecurityDescriptor = &sd;
//		sa.bInheritHandle = FALSE;
//
//		if ((hPipe = CreateNamedPipeA(namedPipeConnectionString, PIPE_ACCESS_DUPLEX, PIPE_TYPE_MESSAGE | PIPE_WAIT, 1, NULL, NULL, INFINITE, &sa)) == INVALID_HANDLE_VALUE)
//			throw SetPipeError("error create: ", GetLastError());
//		if (!ConnectNamedPipe(hPipe, NULL))
//			throw SetPipeError("error connect: ", GetLastError());
//		TalkersCommand& param = *((TalkersCommand*)pPrm);
//
//		while (param != EXIT) {
//			puts("connecting to the pipe...");
//			ConnectNamedPipe(hPipe, NULL);
//			while (ReadFile(hPipe, rbuf, sizeof(rbuf), &dwRead, NULL)) {
//				printf("console message:  %s\n", rbuf);
//				if (sizeof(rbuf));
//				param = set_param(rbuf);
//				if (param == LOAD_LIB)
//				{
//					is_load_library = true;
//					EnterCriticalSection(&scListContact);
//					list_of_dlls.push_front(LoadLibraryA(strstr(rbuf, "Win")));
//					list_of_functions.push_front((FUNCTION)GetProcAddress(list_of_dlls.front(), "SSS"));
//					LeaveCriticalSection(&scListContact);
//				}
//				else if (param == UNLOAD_LIB)
//				{
//					is_load_library = false;
//					EnterCriticalSection(&scListContact);
//					list_of_dlls.pop_front();
//					list_of_functions.pop_front();
//					LeaveCriticalSection(&scListContact);
//				}
//				if (param == STATISTICS) {
//					char sendStastistics[200];
//					sprintf(sendStastistics, "\nstatistics:\n"\
//						"connectings: %d\n" \
//						"denided:     %d\n" \
//						"successed:   %d\n" \
//						"active:	  %d\n" \
//						"", connectionCount, deniedConnections, successConnections, contacts.size());
//					WriteFile(hPipe, sendStastistics, sizeof(sendStastistics), &dwWrite, NULL);
//				}
//
//				if (param != STATISTICS)
//					WriteFile(hPipe, rbuf, strlen(rbuf) + 1, &dwWrite, NULL);
//				if (param == EXIT || param == SHUTDOWN) break;
//			}
//			DisconnectNamedPipe(hPipe);
//			if (param == EXIT || param == SHUTDOWN) break;
//		}
//	}
//	catch (string ErrorPipeText) {
//		printf("\n%s\n", ErrorPipeText.c_str());
//		return -1;
//	}
//	CloseHandle(hPipe);
//	puts("shutdown ConsolePipe");
//	ExitThread(rc);
//}

DWORD WINAPI ConsolePipe(LPVOID pPrm) {
	DWORD rc = 0;
	char rbuf[100];
	DWORD dwRead, dwWrite;
	HANDLE hPipe;

	try {
		char namedPipeConnectionString[50];
		sprintf(namedPipeConnectionString, "\\\\.\\pipe\\%s", namedPipeName);

		SECURITY_ATTRIBUTES sa;
		SECURITY_DESCRIPTOR sd;

		InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
		SetSecurityDescriptorDacl(&sd, TRUE, NULL, FALSE);

		sa.nLength = sizeof(SECURITY_ATTRIBUTES);
		sa.lpSecurityDescriptor = &sd;
		sa.bInheritHandle = FALSE;

		hPipe = CreateNamedPipeA(namedPipeConnectionString, PIPE_ACCESS_DUPLEX, PIPE_TYPE_MESSAGE | PIPE_WAIT, 1, NULL, NULL, INFINITE, &sa);
		if (hPipe == INVALID_HANDLE_VALUE) {
			throw SetPipeError("Error creating pipe: ", GetLastError());
		}

		while (true) {
			
			if (!ConnectNamedPipe(hPipe, NULL)) {
				throw SetPipeError("Error connecting to pipe: ", GetLastError());
			}

			puts("Waiting for a connection to the pipe...");

			TalkersCommand& param = *((TalkersCommand*)pPrm);
			while (ReadFile(hPipe, rbuf, sizeof(rbuf), &dwRead, NULL)) {
				rbuf[dwRead] = '\0';
				printf("Console message: %s\n", rbuf);
				param = set_param(rbuf);
				
				if (param == LOAD_LIB) {
					is_load_library = true;
					EnterCriticalSection(&scListContact);
					char* libraryName = strstr(rbuf, " ");
					if (libraryName != NULL) {
						libraryName++;
						HMODULE hModule = LoadLibraryA(libraryName); 
						if (hModule) {
							list_of_dlls.push_front(hModule);
							list_of_functions.push_front((FUNCTION)GetProcAddress(hModule, "sum"));
						}
						else printf("Failed to load library: %s\n", libraryName);
					}
					else printf("Library name not found in the input: %s\n", rbuf);
					LeaveCriticalSection(&scListContact);

					char resultMsg[50];
					snprintf(resultMsg, sizeof(resultMsg), "Library %s was loaded\n", libraryName);

					if (!WriteFile(hPipe, resultMsg, strlen(resultMsg) + 1, &dwWrite, NULL)) {
						printf("Failed to write to pipe: %d\n", GetLastError());
					}
					
				}
				else if (param == UNLOAD_LIB) {
					is_load_library = false;
					EnterCriticalSection(&scListContact);

					char* libraryName = strstr(rbuf, " ");
					if (libraryName != NULL) {
						libraryName++;
						libraryName[strcspn(libraryName, "\n")] = 0;

						bool found = false;
						for (auto it = list_of_dlls.begin(); it != list_of_dlls.end(); ++it) {
							char moduleFileName[MAX_PATH];
							GetModuleFileNameA(*it, moduleFileName, sizeof(moduleFileName));

							char* fileName = PathFindFileNameA(moduleFileName);
							char nameWithoutExt[MAX_PATH];
							_splitpath_s(fileName, NULL, 0, NULL, 0, nameWithoutExt, MAX_PATH, NULL, 0);

							if (strcmp(nameWithoutExt, libraryName) == 0) {
								FreeLibrary(*it);

								auto functionIt = list_of_functions.begin();
								std::advance(functionIt, std::distance(list_of_dlls.begin(), it)); 
								list_of_dlls.erase(it);
								list_of_functions.erase(functionIt);

								found = true;

								char resultMsg[50];
								snprintf(resultMsg, sizeof(resultMsg), "Library %s was unloaded\n", libraryName);
								WriteFile(hPipe, resultMsg, strlen(resultMsg) + 1, &dwWrite, NULL);
								break;
							}
						}

						if (!found) {
							char resultMsg[50];
							snprintf(resultMsg, sizeof(resultMsg), "Library %s not found.\n", libraryName);
							WriteFile(hPipe, resultMsg, strlen(resultMsg) + 1, &dwWrite, NULL);
						}
					}
					else {
						char resultMsg[50];
						snprintf(resultMsg, sizeof(resultMsg), "Library name not found in the input.\n");
						WriteFile(hPipe, resultMsg, strlen(resultMsg) + 1, &dwWrite, NULL);
					}

					LeaveCriticalSection(&scListContact);
				}
				else if (param == CALL_FUNCTION) {
					if (is_load_library) {
						FUNCTION funcPtr = list_of_functions.front();
						FUNC func = (FUNC)funcPtr;
						int result = func(5, 10);

						char resultMsg[50];
						snprintf(resultMsg, sizeof(resultMsg), "Result of function call: %d\n", result);
						if (!WriteFile(hPipe, resultMsg, strlen(resultMsg) + 1, &dwWrite, NULL)) {
							printf("Failed to write to pipe: %d\n", GetLastError());
						}
					}
					else {
						const char* errorMsg = "Error: Library is not loaded.\n";
						WriteFile(hPipe, errorMsg, strlen(errorMsg) + 1, &dwWrite, NULL);
					}
				}
				else if (param == STATISTICS) {
					char sendStatistics[200];
					sprintf(sendStatistics, "\nStatistics:\n"\
						"Connections: %d\n" \
						"Denied: %d\n" \
						"Succeeded: %d\n" \
						"Active: %d\n",
						connectionCount, deniedConnections, successConnections, contacts.size());
					WriteFile(hPipe, sendStatistics, strlen(sendStatistics) + 1, &dwWrite, NULL);
				}
				else {
					WriteFile(hPipe, rbuf, strlen(rbuf) + 1, &dwWrite, NULL);
				}

				if (param == EXIT || param == SHUTDOWN) {
					break;
				}
			}
			DisconnectNamedPipe(hPipe);
			if (param == EXIT || param == SHUTDOWN) break;
		}
	}
	catch (const string& ErrorPipeText) {
		printf("\n%s\n", ErrorPipeText.c_str());
		rc = -1;
	}

	if (hPipe != INVALID_HANDLE_VALUE) {
		CloseHandle(hPipe);
	}
	puts("Shutdown ConsolePipe");
	ExitThread(rc);
}

DWORD WINAPI GarbageCleaner(LPVOID pPrm) {
	DWORD rc = 0;
	while (*((TalkersCommand*)pPrm) != EXIT) {
		if (contacts.size() != 0) {
			for (auto i = contacts.begin(); i != contacts.end();) {
				if (i->type == i->EMPTY) {
					EnterCriticalSection(&scListContact);
					if (i->sthread == i->FINISH)
						InterlockedIncrement(&successConnections);
					if (i->sthread == i->ABORT || i->sthread == i->TIMEOUT)
						InterlockedIncrement(&deniedConnections);
					i = contacts.erase(i);
					LeaveCriticalSection(&scListContact);
				}
				else ++i;
			}
		}
	}
	puts("shutdown garbageCleaner");
	ExitThread(rc);
}

DWORD WINAPI DispatchServer(LPVOID pPrm){
	DWORD rc = 0;
	TalkersCommand& command = *(TalkersCommand*)pPrm;
	while (command != EXIT){
		if (command == STOP) continue;

		WaitForSingleObject(hClientConnectedEvent, INFINITE);
		ResetEvent(hClientConnectedEvent);

		while (true) {
			for (auto i = contacts.begin(); i != contacts.end(); i++) {
				if (i->type == i->ACCEPT) {

					char serviceType[10];
					if (recv(i->s, serviceType, sizeof(serviceType), NULL) < 1)
						continue;
					printf("command: %s\n", serviceType);
					strcpy(i->msg, serviceType);

					if (!strcmp(i->msg, "close")) {
						if ((send(i->s, "echo: close", strlen("echo: close") + 1, NULL)) == SOCKET_ERROR)
							throw  SetErrorMsgText("error send: ", WSAGetLastError());
						CancelWaitableTimer(i->htimer);
						i->sthread = i->FINISH;
						i->type = i->EMPTY;
						continue;
					}
					if (strcmp(i->msg, "echo") && strcmp(i->msg, "time") && strcmp(i->msg, "rand")) {
						if ((send(i->s, "ErrorInquiry", strlen("ErrorInquiry") + 1, NULL)) == SOCKET_ERROR)
							throw  SetErrorMsgText("error send: ", WSAGetLastError());
						CancelWaitableTimer(i->htimer);
						i->sthread = i->ABORT;
						i->type = i->EMPTY;
					}
					else {
						i->type = i->CONTACT;
						i->hthread = hAcceptServer;
						i->serverHThread = ts(serviceType, (LPVOID) & (*i));
						CancelWaitableTimer(i->htimer);
						i->htimer = CreateWaitableTimer(0, FALSE, 0);
						LARGE_INTEGER Li;
						int seconds = 60;
						Li.QuadPart = -(10000000 * seconds);
						SetWaitableTimer(i->htimer, &Li, 0, ASWTimer, (LPVOID) & (*i), FALSE);
						SleepEx(0, TRUE);
					}
				}
			}
			Sleep(200);
		}
	}
	puts("shutdown dispatchServer");
	ExitThread(rc);
}

bool PutAnswerToClient(char* name, sockaddr* to, int* lto) {

	char msg[] = "message to client";
	if ((sendto(sSUDP, msg, sizeof(msg) + 1, NULL, to, *lto)) == SOCKET_ERROR)
		throw  SetErrorMsgText("error sendto: ", WSAGetLastError());
	return false;
}

bool GetRequestFromClient(char* name, short port, SOCKADDR_IN* from, int* flen) {
	SOCKADDR_IN clnt;
	int lc = sizeof(clnt);
	ZeroMemory(&clnt, lc);
	char ibuf[500];
	int  lb = 0;
	int optval = 1;
	int TimeOut = 10;
	setsockopt(sSUDP, SOL_SOCKET, SO_BROADCAST, (char*)&optval, sizeof(int));
	setsockopt(sSUDP, SOL_SOCKET, SO_RCVTIMEO, (char*)&TimeOut, sizeof(TimeOut));
	while (true) 
	{
		if ((lb = recvfrom(sSUDP, ibuf, sizeof(ibuf), NULL, (sockaddr*)&clnt, &lc)) == SOCKET_ERROR) return false;
		ibuf[lb] = '\0';
		cout << endl << ibuf << endl;
		if (!strcmp(name, ibuf)) 
		{
			*from = clnt;
			*flen = lc;
			return true;
		}
		puts("\nThis name isn't suitable");
	}

}

DWORD WINAPI ResponseServer(LPVOID pPrm) {
	DWORD rc = 0;
	WSADATA wsaData;
	SOCKADDR_IN serv;
	u_long nonblk = 1;
	if (WSAStartup(MAKEWORD(2, 0), &wsaData) != 0)
		throw  SetErrorMsgText("error wsastartup: ", WSAGetLastError());
	if ((sSUDP = socket(AF_INET, SOCK_DGRAM, NULL)) == INVALID_SOCKET)
		throw  SetErrorMsgText("error socket:", WSAGetLastError());
	serv.sin_family = AF_INET;
	serv.sin_port = htons(2000);
	//serv.sin_addr.s_addr = INADDR_ANY	;
	serv.sin_addr.s_addr = inet_addr("127.0.0.2");

	if (bind(sSUDP, (LPSOCKADDR)&serv, sizeof(serv)) == SOCKET_ERROR)
		throw  SetErrorMsgText("error bind: ", WSAGetLastError());
	
	if (ioctlsocket(sS, FIONBIO, &nonblk) == SOCKET_ERROR)
		throw SetErrorMsgText("error ioctlsocket: ", WSAGetLastError());
	

	SOCKADDR_IN some_server;
	int serverSize = sizeof(some_server);

	SOCKADDR_IN from;
	int lc = sizeof(from);
	ZeroMemory(&from, lc);
	int numberOfClients = 0;
	while (*(TalkersCommand*)pPrm != EXIT) {
		try {
			if (GetRequestFromClient((char*)"Hello", serverPort, &from, &lc)) {
				printf("Connected Client: %d, port: %d, address: %s\n", ++numberOfClients, htons(from.sin_port), inet_ntoa(from.sin_addr));
				PutAnswerToClient((char*)"Hello", (sockaddr*)&from, &lc);
			}
		}
		catch (string errorMsgText) {
			printf("\n%s", errorMsgText.c_str());
		}
	}
	if (closesocket(sSUDP) == SOCKET_ERROR)
		throw  SetErrorMsgText("error closesocket: ", WSAGetLastError());
	if (WSACleanup() == SOCKET_ERROR)
		throw  SetErrorMsgText("error cleanup: ", WSAGetLastError());
	ExitThread(rc);
}


int main(int argc, char* argv[]) {

	if (argc == 2) {
		serverPort = atoi(argv[1]);
		strcpy(dllName, "ServiceLibrary.dll");
		strcpy(namedPipeName, "Tube");
	}
	else if (argc == 3) {
		serverPort = atoi(argv[1]);
		strcpy(dllName, argv[2]);
		strcpy(namedPipeName, "Tube");
	}
	else if (argc == 4) {
		serverPort = atoi(argv[1]);
		strcpy(dllName, argv[2]);
		strcpy(namedPipeName, argv[3]);
	}
	else {
		serverPort = 2000;
		strcpy(dllName, "ServiceLibrary.dll");
		strcpy(namedPipeName, "Tube");
	}

	st = LoadLibraryA(dllName);
	ts = (HANDLE(*)(char*, LPVOID))GetProcAddress(st, "SSS");

	volatile TalkersCommand  cmd = START;

	InitializeCriticalSection(&scListContact);

	hAcceptServer = CreateThread(NULL, NULL, AcceptServer, (LPVOID)&cmd, NULL, NULL);
	hConsolePipe = CreateThread(NULL, NULL, ConsolePipe, (LPVOID)&cmd, NULL, NULL);
	hGarbageCleaner = CreateThread(NULL, NULL, GarbageCleaner, (LPVOID)&cmd, NULL, NULL);
	hDispatchServer = CreateThread(NULL, NULL, DispatchServer, (LPVOID)&cmd, NULL, NULL);
	hResponseServer = CreateThread(NULL, NULL, ResponseServer, (LPVOID)&cmd, NULL, NULL);


	SetThreadPriority(hGarbageCleaner, THREAD_PRIORITY_BELOW_NORMAL);
	SetThreadPriority(hDispatchServer, THREAD_PRIORITY_NORMAL);
	SetThreadPriority(hConsolePipe, THREAD_PRIORITY_NORMAL);
	SetThreadPriority(hResponseServer, THREAD_PRIORITY_NORMAL);
	SetThreadPriority(hAcceptServer, THREAD_PRIORITY_HIGHEST);


	WaitForSingleObject(hAcceptServer, INFINITE);
	CloseHandle(hAcceptServer);
	WaitForSingleObject(hConsolePipe, INFINITE);
	CloseHandle(hConsolePipe);
	WaitForSingleObject(hGarbageCleaner, INFINITE);
	CloseHandle(hGarbageCleaner);

	TerminateThread(hDispatchServer, 0);
	puts("shutdown dispatchServer");
	TerminateThread(hResponseServer, 0);
	puts("shutdown responseServer");
	CloseHandle(hDispatchServer);
	CloseHandle(hResponseServer);

	DeleteCriticalSection(&scListContact);
	FreeLibrary(st);
	return 0;
};


