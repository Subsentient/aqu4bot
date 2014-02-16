/*
aqu4, daughter of pr0t0, half-sister to Johnsbot! Copyright 2014 Subsentient.

This file is part of aqu4bot.

aqu4bot is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

aqu4bot is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with aqu4bot.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "aqu4.h"

int SocketDescriptor;
unsigned short SendDelay = 8; /*Ten is one second.*/

Bool Net_Connect(const char *InHost, unsigned short PortNum, int *SocketDescriptor_)
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
	
	if (connect(*SocketDescriptor_, (void*)&SocketStruct, sizeof SocketStruct) != 0)
	{
		
		fprintf(stderr, "Failed to connect to server \"%s\".\n", InHost);
		*SocketDescriptor_ = 0;
		return false;
	}

	return true;
}

Bool Net_Write(int SockDescriptor, const char *InMsg)
{
	size_t StringSize = strlen(InMsg);
	unsigned long Transferred = 0, TotalTransferred = 0;

	do
	{
		Transferred = send(SockDescriptor, InMsg, (StringSize - TotalTransferred), MSG_NOSIGNAL);
		
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
		Status = recv(SockDescriptor, &Byte, 1, 0);
		
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
	
	return !close(SockDescriptor);
}
