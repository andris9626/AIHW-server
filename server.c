#include "server.h"
#include "ws2tcpip.h"
#include <stdio.h> //to remove!!!

inline void updateSpeed(server_t* server)
{
	if (server->speedRef > server->currSpeed)
	{
		server->currSpeed = server->currSpeed + SPEED_CHANGE_PER_10MS;
	}
	else if (server->speedRef < server->currSpeed)
	{
		server->currSpeed = server->currSpeed - SPEED_CHANGE_PER_10MS;
	}
	else
	{
		/* They are equal, do nothing */
	}
}

inline void updateSwa(server_t* server)
{
	if (server->swaRef > server->currSwa)
	{
		server->currSwa = server->currSwa + SPEED_CHANGE_PER_10MS;
	}
	else if (server->swaRef < server->currSwa)
	{
		server->currSwa = server->currSwa - SPEED_CHANGE_PER_10MS;
	}
	else
	{
		/* They are equal, do nothing */
	}
}

void initWsa(WSADATA* wsaData)
{
	int res = WSAStartup(MAKEWORD(2, 2), wsaData);

	if (res < 0)
	{
		perror("Error initializing WSA");
		exit(EXIT_FAILURE);
	}
}

void initServer(SOCKET* sock)
{
	/* Create UDP socket */
	*sock = socket(AF_INET, SOCK_DGRAM, 0);

#ifdef _WIN32
	DWORD dw = 10;
	setsockopt(*sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&dw, sizeof(dw));
#else
	struct timeval read_timeout;
	read_timeout.tv_sec = 0;
	read_timeout.tv_usec = 1000;
	setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (char*)&read_timeout, sizeof(struct timeval));
#endif
	

	/* Create a server hint structure for the server */
	struct sockaddr_in serverHint;
	serverHint.sin_addr.S_un.S_addr = ADDR_ANY; // Use any IP address available on the machine
	serverHint.sin_family = AF_INET; // Address format is IPv4
	serverHint.sin_port = htons(54000); // Convert from little to big endian

	/* Try and bind the socket to the IP and port */
	if (bind(*sock, (const struct sockaddr*)&serverHint, sizeof(serverHint)) == SOCKET_ERROR)
	{
		return;
	}

	struct sockaddr_in client; // Use to hold the client information (port / ip address)
	int clientLength = sizeof(client); // The size of the client information
}

void receiveMsgs(uint8_t* buffer, SOCKET sock, struct sockaddr_in* clientAddr)
{
	int clientLength = sizeof(*clientAddr); // The size of the client information
	char clientIp[256];

	ZeroMemory(clientAddr, clientLength); // Clear the client structure
	//ZeroMemory(buffer, 1024); // Clear the receive buffer

	/* Waiting for messages (with time-out) */
	int bytesIn = recvfrom(sock, buffer, LONGEST_MSG_SIZE_SERVER, 0, (struct sockaddr*)clientAddr, &clientLength);
	if (bytesIn == SOCKET_ERROR)
	{
		return;
	}

	 // Create enough space to convert the address byte array
	ZeroMemory(clientIp, 256); // to string of characters

	// Convert from byte array to chars
	inet_ntop(AF_INET, &(clientAddr->sin_addr), clientIp, 256);

	//printf("Msg received from %s: %s (0x%02X)\n", clientIp, buffer, buffer[0]);
}

void handleMsgsServer(uint8_t* recBuff, server_t* server, SOCKET sock, seqCntrs_t* seqCntrs, clock_t* selfDriveUpdateTime)
{
	uint8_t msgId = MSG_ID_INVALID;
	uint8_t seqCntr = 0u;
	uint8_t sendBuff[LONGEST_MSG_SIZE_SERVER];
	struct sockaddr_in clientAddr;

	receiveMsgs(recBuff, sock, &clientAddr);
	
	msgId = recBuff[0u];
	seqCntr = recBuff[1u];

	/* Check if sequence counter is valid */
	if (0u == seqCntr)
	{
		/* Invalid sequence counter */
		//printf("Invalid sequence counter has been received!");
		//perror("Invalid sequence counter has been received!");
	}

	//memset(&sendBuff, 0u, sizeof(sendBuff));

	switch (msgId)
	{
	case MSG_ID_CAR_STATE_REQ:

		/* Check if sequence counter is as expected */
		if (seqCntr != seqCntrs->carStateReqCntr)
		{
			printf("Car state request sequence counter error! (received: %u expected: %u)", seqCntr, seqCntrs->carStateReqCntr);
			perror("Car state request sequence counter error!");

			/* Set sequence counter to actual value to keep up with client */
			seqCntrs->carStateReqCntr = seqCntr;
		}
		/*carState_t msg = {
			.msgId = MSG_ID_CAR_STATE_REQ,
			.sequCntr = seqCntrs->carStateReqCntr,
			.engaged = (uint8_t)server->selfDriveEngaged,
			.carSpeed = (uint16_t)server->currSpeed,
			.swa = (uint16_t)server->currSwa
		};
		memcpy(sendBuff, &msg, sizeof(carState_t));*/
		sendBuff[0u] = MSG_ID_CAR_STATE;
		sendBuff[1u] = seqCntrs->carStateCntr;
		sendBuff[2u] = server->selfDriveEngaged;
		sendBuff[3u] = (uint8_t)(server->currSpeed >> 8u);
		sendBuff[4u] = (uint8_t)(server->currSpeed);
		sendBuff[5u] = (uint8_t)(server->currSwa >> 8u);
		sendBuff[6u] = (uint8_t)(server->currSwa);

		if (sendto(sock, sendBuff, sizeof(sendBuff), 0, (const struct sockaddr*)&clientAddr, sizeof(clientAddr)) >= 0)
		{
			/* Update cntr if buffer was sent */
			updateSeqCntr(&seqCntrs->carStateCntr);
			//printf("msg sent!\n");
		}
		else
		{
			printf("%d", WSAGetLastError());
		}

		/* Update expected counter value */
		updateSeqCntr(&seqCntrs->carStateReqCntr);
			//printf("speed: %d (0x%02X)\n", server->currSpeed, server->currSpeed);
			//printf("swa: %d (0x%02X)\n", server->currSwa, server->currSwa);

		break;
	case MSG_ID_ENGAGE_REQ:

		/* Check if sequence counter is as expected */
		if (seqCntr != seqCntrs->engangeReqCntr)
		{
			printf("Engage self-drive request sequence counter error! (received: %u expected: %u)", seqCntr, seqCntrs->engangeReqCntr);
			perror("Engage self-drive request sequence counter error!");

			/* Set sequence counter to actual value to keep up with client */
			seqCntrs->engangeReqCntr = seqCntr;
		}

		if (TRUE == recBuff[2u])
		{
			/* Engage self-drive*/
			server->selfDriveEngaged = TRUE;

			/* Save time to start timer and to avoid quitting too early */
			*selfDriveUpdateTime = clock();

			printf("Self-drive engaged! %lu\n", *selfDriveUpdateTime);
		}
		else if (FALSE == recBuff[2u])
		{
			server->selfDriveEngaged = FALSE;
			printf("Self-drive is off! engage: 0x%02X\n", recBuff[2u]);
		}
		else
		{
			/* Invalid value was received */
			printf("Invalid value was received: 0x%02X\n", recBuff[2u]);
		}	

		/* Update cntr value */
		updateSeqCntr(&seqCntrs->engangeReqCntr);

		break;
	case MSG_ID_SET_REF:

		/* Save time for time-out */
		*selfDriveUpdateTime = clock();

		/* Check if sequence counter is as expected */
		if (seqCntr != seqCntrs->setRefCntr)
		{
			printf("Set reference sequence counter error! (received: %u expected: %u)", seqCntr, seqCntrs->setRefCntr);
			perror("Set reference request sequence counter error!");

			/* Set sequence counter to actual value to keep up with client */
			seqCntrs->setRefCntr = seqCntr;
		}

		/* Check if self-drive is engaged */
		if (TRUE == server->selfDriveEngaged)
		{
			int16_t newSpeed = ((recBuff[2u] << 8u) | recBuff[3u]);
			int16_t newSwa = ((recBuff[4u] << 8u) | recBuff[5u]);
			/* Print references to console if they are not the same as the previously set ones */
			if (server->speedRef != newSpeed || server->swaRef != newSwa)
			{
				printf("New speed ref: %d (0x%02X)\n", newSpeed, newSpeed);
				printf("New swa ref: %d (0x%02X)\n", newSwa, newSwa);
			}

			/* Set new references */
			server->speedRef = ((recBuff[2u] << 8u) | recBuff[3u]);
			server->swaRef = ((recBuff[4u] << 8u) | recBuff[5u]);
		}
		else
		{
			/* Self-drive is off */
			printf("References could not be set (self-drive is not engaged!\n");
		}

		/* Update cntr value */
		updateSeqCntr(&seqCntrs->setRefCntr);

		break;
	default:
		//printf("Invalid msgId: 0x%02X (%d), buffer[1]: %d\n", msgId, msgId, recBuff[1u]);
		break;
	}

	ZeroMemory(recBuff, LONGEST_MSG_SIZE_SERVER);
	//printf("buffer cleared!");
}

void updateSeqCntr(uint8_t* cntr)
{
	(*cntr)++;

	/* Avoid invalid counter value */
	if (0u == *cntr)
	{
		*cntr = 1u;
	}
}