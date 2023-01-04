#ifndef _UDP_H_
#define _UDP_H_

#include<stdbool.h>

void Udp_setup(void);
// void Udp_ReceiveMessagesFromClient(void);
int Udp_GetSocketDescriptor(void);
void Udp_CloseConnection(void);
void Udp_establishConnection(void);
void *Udp_threadCommandHandling(void* buffer);
bool Udp_signifyCloseOfProgram(void);

#endif