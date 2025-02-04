#pragma once
#include "Errors.h"
#include <time.h>
#include <string>
struct Contact {
	enum TE {
		EMPTY,
		ACCEPT,
		CONTACT
	}   type;
	enum ST {
		WORK,
		ABORT,
		TIMEOUT,
		FINISH
	}   sthread;

	SOCKET      s;
	SOCKADDR_IN prms;
	int         lprms;
	HANDLE      hthread;
	HANDLE      htimer;
	HANDLE		serverHThread;

	char msg[50];
	char srvname[15];

	Contact(TE t = EMPTY, const char* namesrv = "") {
		memset(&prms, 0, sizeof(SOCKADDR_IN));
		lprms = sizeof(SOCKADDR_IN);
		type = t;
		strcpy(srvname, namesrv);
		msg[0] = 0;
	};

	void SetST(ST sth, const char* m = "") {
		sthread = sth;
		strcpy(msg, m);
	}
};

string runServer;

const string currentDateTime() {
	time_t now = time(0);
	tm tstruct;
	char buf[80];
	tstruct = *localtime(&now);
	strftime(buf, sizeof(buf), "%Y-%m-%d.%X", &tstruct);
	return buf;
}

void CALLBACK ASStartMessage(DWORD p) {
	char *hostName = new char(4);
	gethostname(hostName, sizeof(hostName));

	printf("used server: %s  \nstart time:  %s\n", runServer.c_str(), currentDateTime().c_str());
}

void CALLBACK ASFinishMessage(DWORD p)
{
	printf("finish time: %s\n", currentDateTime().c_str());
}

void QueueUserAPCWrapper(PAPCFUNC functionName, Contact *contact) 
{
	QueueUserAPC(functionName, contact->hthread, 0);
}

void sendMsgToClient(Contact* contact)
{
	if (send(contact->s, contact->msg, sizeof(contact->msg), NULL) == SOCKET_ERROR)
		throw  SetErrorMsgText("send:", WSAGetLastError());
}
DWORD WINAPI EchoServer(LPVOID pPrm)
{
	DWORD rc = 0;
	Contact *contact = (Contact*)(pPrm);
	u_long nonblk = 0;
	try
	{
		runServer = "EchoServer";
		QueueUserAPCWrapper((PAPCFUNC)ASStartMessage, contact);
		int libuf;
		contact->sthread = contact->WORK;
		contact->type = contact->CONTACT;

		if (ioctlsocket(contact->s, FIONBIO, &nonblk) == SOCKET_ERROR)
			throw SetErrorMsgText("ioctlsocket:", WSAGetLastError());

		while (true) 
		{

			if ((libuf = recv(contact->s, contact->msg, sizeof(contact->msg), NULL)) == SOCKET_ERROR)
				throw  SetErrorMsgText("recv:", WSAGetLastError());

			sendMsgToClient(contact);
			if (atoi(contact->msg) == 0) break;
		}
	}
	catch (...)
	{

		puts("error in EchoServer");
		contact->sthread = contact->ABORT;
		contact->type = contact->EMPTY;
		rc = contact->sthread;

		QueueUserAPCWrapper((PAPCFUNC)ASFinishMessage, contact);
		CancelWaitableTimer(contact->htimer);
		ExitThread(rc);

	}

	contact->type = contact->ACCEPT;

	nonblk = 1;
	if (ioctlsocket(contact->s, FIONBIO, &nonblk) == SOCKET_ERROR)
		throw SetErrorMsgText("ioctlsocket:", WSAGetLastError());

	QueueUserAPCWrapper((PAPCFUNC)ASFinishMessage, contact);
	CancelWaitableTimer(contact->htimer);
	ExitThread(rc);
}

DWORD WINAPI TimeServer(LPVOID pPrm)
{
	DWORD rc = 0;
	Contact *contact = (Contact*)(pPrm);
	runServer = "TimeServer";
	QueueUserAPCWrapper((PAPCFUNC)ASStartMessage, contact);
	strcpy(contact->msg, currentDateTime().c_str());
	sendMsgToClient(contact);

	contact->type = contact->ACCEPT;

	QueueUserAPCWrapper((PAPCFUNC)ASFinishMessage, contact);
	CancelWaitableTimer(contact->htimer);
	ExitThread(rc);
}

DWORD WINAPI RandomServer(LPVOID pPrm)
{
	DWORD rc = 0;
	Contact *contact = (Contact*)(pPrm);
	runServer = "Random";
	QueueUserAPCWrapper((PAPCFUNC)ASStartMessage, contact);
	srand(time(NULL));
	int randNumber;
	randNumber = rand() % 100 + 1;
	sprintf(contact->msg, "%d", randNumber);
	sendMsgToClient(contact);


	contact->type = contact->ACCEPT;

	QueueUserAPCWrapper((PAPCFUNC)ASFinishMessage, contact);
	CancelWaitableTimer(contact->htimer);
	ExitThread(rc);
}