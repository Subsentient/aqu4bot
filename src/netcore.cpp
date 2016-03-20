/*
aqu4, daughter of pr0t0, half-sister to Johnsbot! Copyright 2014 Subsentient.

This file is part of aqu4bot.
aqu4bot is public domain software.
See the file UNLICENSE.TXT for more information.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#ifdef WIN
#include <winsock2.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#endif

#include "substrings/substrings.h"
#include "aqu4.h"

int SocketDescriptor;
unsigned short SendDelay = 8; /*Ten is one second.*/

bool Net::Connect(const char *InHost, unsigned short PortNum, int *SocketDescriptor_)
{

	const char *FailMsg = "Failed to establish a connection to the server:";
	struct sockaddr_in SocketStruct;
	struct hostent *HostnameStruct;
	
	memset(&SocketStruct, 0, sizeof(SocketStruct));
	
	if ((HostnameStruct = gethostbyname(InHost)) == NULL)
	{
		
		fprintf(stderr, "Failed to resolve hostname \"%s\".", InHost);
		
		return 0;
	}
	
	if ((*SocketDescriptor_ = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		SocketDescriptor = 0;
		perror(FailMsg);
		return false;
	}
	
	memcpy(&SocketStruct.sin_addr, HostnameStruct->h_addr_list[0], HostnameStruct->h_length);
	
	SocketStruct.sin_family = AF_INET;
	SocketStruct.sin_port = htons(PortNum);
	
	if (connect(*SocketDescriptor_, (const struct sockaddr*)&SocketStruct, sizeof SocketStruct) != 0)
	{
		
		fprintf(stderr, "Failed to connect to server \"%s\".\n", InHost);
		*SocketDescriptor_ = 0;
		return false;
	}

	return true;
}

bool Net::Write(int SockDescriptor, const char *InMsg)
{
	size_t StringSize = strlen(InMsg);
	unsigned Transferred = 0, TotalTransferred = 0;

	do
	{
		Transferred = send(SockDescriptor, InMsg + TotalTransferred, (StringSize - TotalTransferred), 0);
		if (Transferred == (unsigned)-1)
		{
			return false;
		}
		
		TotalTransferred += Transferred;
	} while (StringSize > TotalTransferred);
	
	if (ShowOutput) printf("%s", InMsg), fflush(stdout);
	
	/*Means this is IRC.*/
	if (SocketDescriptor == SockDescriptor) usleep(SendDelay * 100000); /*We wait a moment so we don't get kicked for flooding.*/
	
	return true;
}

bool Net::Read(int SockDescriptor, void *OutStream_, unsigned MaxLength, bool TextStream)
{
	int Status = 0;
	unsigned char Byte = 0;
	unsigned char *OutStream = (unsigned char*)OutStream_;
	unsigned Inc = 0;
	
	*OutStream = '\0';
	
	do
	{
		Status = recv(SockDescriptor, (char*)&Byte, 1, 0);
		if (TextStream && Byte == '\n') break;
		
		*OutStream++ = Byte;
	} while (++Inc, Status > 0 && Inc < (TextStream ? MaxLength - 1 : MaxLength));
	
	if (TextStream)
	{
		*OutStream = '\0';
		if (*(OutStream - 1) == '\r') *(OutStream - 1) = '\0';
	}
	
	if (Status == -1) return false;
	
	if (ShowOutput) puts((const char*)OutStream_), fflush(stdout);
	
	return true;
}

bool Net::Disconnect(int SockDescriptor)
{
	if (!SockDescriptor) return false;
	
#ifdef WIN
	return !closesocket(SockDescriptor);
#else
	return !close(SockDescriptor);
#endif
}
