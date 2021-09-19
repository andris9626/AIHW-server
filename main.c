#include <stdio.h>
#include <time.h>
#include <math.h>
#include "server.h"
#include "ws2tcpip.h"

#define STEERING_AMPLITUDE_DEG       60u
#define STEERING_FREQUENCY_HZ        1.2f
#define STEERING_FREQUENCY_ANGULAR   2 * 3.14 * STEERING_FREQUENCY_HZ
#define CONV_KMPH_TO_CMPS(kmph)      (speed_t)round((float)(kmph) * 27.78f)
#define CONV_CMPS_TO_KMPH(cmps)      (speed_t)round((float)(cmps) * 0.036f)
#define CONV_DEG_TO_MIRAD(deg)       (swa_t)round((float)(deg) * 17.453f)
#define CONV_MIRAD_TO_DEG(mirad)     (swa_t)round((float)(mirad) * 0.0573f)
#define CONV_CLOCK_TO_S(clock)       (float)(((double)(clock) / (double)CLOCKS_PER_SEC))
#define CONV_CLOCK_TO_MS(clock)      (float)(((double)(clock) / (double)CLOCKS_PER_SEC) * 1000.f)

static inline void simulateSteering(server_t* server, clock_t currTime)
{	
	server->currSwa = (uint16_t)(CONV_DEG_TO_MIRAD(STEERING_AMPLITUDE_DEG) *
		sin(STEERING_FREQUENCY_ANGULAR * CONV_CLOCK_TO_S(currTime)));
}

int main(int argc, char* argv[])
{
	server_t server = {
	.selfDriveEngaged = 0u,
	.speedRef = 0,
	.swaRef = 0,
	.currSpeed = 0,
	.currSwa = 0
	};
	clock_t currTime = 0u;
	clock_t prevTime = 0u;
	clock_t printTime = 0u;
	clock_t selfDriveUpdateTime = 0u;
	SOCKET sock = INVALID_SOCKET;
	WSADATA wsaData;
	uint8_t buffer[LONGEST_MSG_SIZE_SERVER];

	/* Set expected/initial sequence cntrs */
	seqCntrs_t seqCntrs = {
		.carStateReqCntr = 1u,
		.carStateCntr = 1u,
		.engangeReqCntr = 1u,
		.setRefCntr = 1u
	};


	if (2u == argc)
	{
		server.speedRef = CONV_KMPH_TO_CMPS((uint16_t)atoi(argv[1]));
		printf("Speed reference is %d km/h = %d cm/s\n", atoi(argv[1]), server.speedRef);
	}
	else
	{
		printf("Error, invalid number of arguments\n");
	}

	initWsa(&wsaData);
	initServer(&sock);

	while (1)
	{
		currTime = clock();

		/* Update speed and SWA */
		if (10u <= (uint32_t)CONV_CLOCK_TO_MS(currTime - prevTime))
		{
			prevTime = currTime;
			updateSpeed(&server);
			if (FALSE == server.selfDriveEngaged)
			{
				simulateSteering(&server, currTime);
			}
			else
			{
				updateSwa(&server);
			}			
		}

		/* Printing info to console */
		if (1000u <= (uint32_t)CONV_CLOCK_TO_MS(currTime - printTime))
		{
			printTime = currTime;			
			printf("Current speed: %d km/h = %d cm/s, Current SWA: %d deg, time: %.1f ms\n",
				CONV_CMPS_TO_KMPH(server.currSpeed),
				server.currSpeed,
				CONV_MIRAD_TO_DEG(server.currSwa),
				CONV_CLOCK_TO_MS(currTime));
		}

		//receiveMsgs(buffer, &sock);
		handleMsgsServer(buffer, &server, sock, &seqCntrs, &selfDriveUpdateTime);

		currTime = clock();

		/* Quit self-drive if reference has not been updated fot longer than 5 sec */
		if (TRUE == server.selfDriveEngaged && (5000u <= (uint32_t)CONV_CLOCK_TO_MS(currTime - selfDriveUpdateTime)))
		{
			server.selfDriveEngaged = FALSE;
			printf("Self drive off due to time-out! curr time: %lu selfDriveUpdateTime: %lu\n", currTime, selfDriveUpdateTime);
			perror("Self drive off due to time-out!");
		}

	}

	closesocket(sock);
	WSACleanup();

	return 0;
}