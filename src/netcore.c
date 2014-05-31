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
#include <fcntl.h>

#include "substrings/substrings.h"
#include "aqu4.h"

int SocketDescriptor;
unsigned short SendDelay = 8; /*Ten is one second.*/

Bool Net_Connect(const char *InHost, unsigned short PortNum, int *SocketDescriptor_, Bool Nonblock)
{

	char *FailMsg = "Failed to establish a connection to the server:";
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
	
	if (Nonblock)
	{
		int Counter = 0;

		/*Set nonblocking for connect.*/
		fcntl(*SocketDescriptor_, F_SETFL, O_NONBLOCK);
		
		/*Try 50 times.*/
		for (; connect(*SocketDescriptor_, (void*)&SocketStruct, sizeof SocketStruct) != 0 && Counter < 50; ++Counter)
		{
			usleep(100000);
		}
		
		if (Counter == 50)
		{
			fprintf(stderr, "Failed to connect to server \"%s\".\n", InHost);
			*SocketDescriptor_ = 0;
			return false;
		}
		/*Set back to blocking.*/
		fcntl(*SocketDescriptor_, F_SETFL, (fcntl(*SocketDescriptor_, F_GETFL) & ~O_NONBLOCK) );
	}
	else
	{
		if (connect(*SocketDescriptor_, (void*)&SocketStruct, sizeof SocketStruct) != 0)
		{
			
			fprintf(stderr, "Failed to connect to server \"%s\".\n", InHost);
			*SocketDescriptor_ = 0;
			return false;
		}
	}
	
	return true;
}

Bool Net_Write(int SockDescriptor, const char *InMsg)
{
	size_t StringSize = strlen(InMsg);
	unsigned long Transferred = 0, TotalTransferred = 0;

	do
	{
#ifdef WIN
		Transferred = send(SockDescriptor, InMsg, (StringSize - TotalTransferred), 0);
#else
		Transferred = send(SockDescriptor, InMsg, (StringSize - TotalTransferred), MSG_NOSIGNAL);
#endif		
		if (Transferred == -1) /*This is ugly I know, but it's converted implicitly, so shut up.*/
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

Bool Net_Read(int SockDescriptor, void *OutStream_, unsigned long MaxLength, Bool TextStream)
{
	int Status = 0;
	unsigned char Byte = 0;
	unsigned char *OutStream = OutStream_;
	unsigned long Inc = 0;
	
	*OutStream = '\0';
	
	do
	{
		Status = recv(SockDescriptor, (void*)&Byte, 1, 0);
		if (TextStream && Byte == '\n') break;
		
		*OutStream++ = Byte;
	} while (++Inc, Status > 0 && Inc < (TextStream ? MaxLength - 1 : MaxLength));
	
	if (TextStream)
	{
		*OutStream = '\0';
		if (*(OutStream - 1) == '\r') *(OutStream - 1) = '\0';
	}
	
	if (Status == -1) return false;
	
	if (ShowOutput) puts(OutStream_), fflush(stdout);
	
	return true;
}

Bool Net_Disconnect(int SockDescriptor)
{
	if (!SockDescriptor) return false;
	
#ifdef WIN
	return !closesocket(SockDescriptor);
#else
	return !close(SockDescriptor);
#endif
}

Bool Net_GetHTTP(const char *Hostname, const char *Filename, unsigned long MaxData, char *OutStream, int Attempts)
{ /*Retrieve a text-based HTTP page.*/
#define HTTP_GET_1 "GET %s HTTP/1.0\r\n" /*We use 1.0 because 1.1 allows for the connection to stay open. That's bad for us.*/
#define HTTP_GET_2 "Host: %s\r\n\r\n"
#define HTTP_PORT 80

	int SDesc = 0;
	char OutCommand[2][128];
	char *PageData = NULL, *Worker = NULL;
	
	do
	{
		if (Net_Connect(Hostname, HTTP_PORT, &SDesc, true)) break;
	} while (--Attempts > 0);
	
	if (Attempts == 0) return false;
	
	snprintf(OutCommand[0], sizeof OutCommand[0], HTTP_GET_1, Filename);
	snprintf(OutCommand[1], sizeof OutCommand[1], HTTP_GET_2, Hostname);
	
	PageData = calloc(MaxData, 1); /*We need it zeroed for a null terminator.*/
	
	if (!Net_Write(SDesc, OutCommand[0]) || !Net_Write(SDesc, OutCommand[1]))
	{
		free(PageData);
		return false;
	}
	
	if (!Net_Read(SDesc, PageData, MaxData, false))
	{
		free(PageData);
		return false;
	}
	
	if (!(Worker = SubStrings.Find("\r\n\r\n", 1, PageData)))
	{ /*Skip past the page header.*/
		free(PageData);
		return false;
	}
	
	Worker += sizeof "\r\n\r\n" - 1;
	
	/*Output the page.*/
	SubStrings.Copy(OutStream, Worker, MaxData);
	free(PageData);
	
	if (!Net_Disconnect(SDesc))
	{
		return false;
	}
	
	return true;
}
