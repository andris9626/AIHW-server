#ifndef SERVER_H
#define SERVER_H

#include <stdint.h>
#if defined(_W64) || defined(_WIN32)
#include <WinSock2.h>
#else
#include <sys/socket.h>
#endif

#include "msg.h"
#include <time.h>

#define SPEED_CHANGE_PER_10MS   1u
#define SWA_CHANGE_PER_10MS     7u

typedef uint8_t bool;
typedef int16_t speed_t;
typedef int16_t swa_t;

typedef struct server_t
{
	bool selfDriveEngaged; // bool
	speed_t speedRef; // cm/s
	swa_t swaRef; // mrad
	speed_t currSpeed; // cm/s
	swa_t currSwa; // mrad
}server_t;

extern inline void updateSpeed(server_t*);

extern inline void updateSwa(server_t*);

extern inline void initWsa(WSADATA*);

extern void initServer(SOCKET*);

extern void receiveMsgs(uint8_t*, SOCKET sock, struct sockaddr_in*);

extern void handleMsgsServer(uint8_t*, server_t*, SOCKET, seqCntrs_t*, clock_t*);

extern inline void updateSeqCntr(uint8_t*);

#endif // !SERVER_H

