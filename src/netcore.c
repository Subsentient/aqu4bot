/*aqu4, daughter of pr0t0, half-sister to Johnsbot!
 * Copyright 2014 Subsentient, all rights reserved.*/

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "aqu4.h"

int SocketDescriptor;
unsigned short SendDelay = 8; /*Ten is one second.*/

Bool Net_Connect(char *InHost, unsigned short PortNum)
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
	
	if ((SocketDescriptor = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		SocketDescriptor = 0;
		perror(FailMsg);
		return false;
	}
	
	memcpy(&SocketStruct.sin_addr, HostnameStruct->h_addr_list[0], HostnameStruct->h_length);
	
	SocketStruct.sin_family = AF_INET;
	SocketStruct.sin_port = htons(PortNum);
	
	if (connect(SocketDescriptor, (struct sockaddr *)&SocketStruct, sizeof SocketStruct) != 0)
	{
		
		fprintf(stderr, "Failed to connect to server \"%s\".\n", InHost);
		SocketDescriptor = 0;
		return false;
	}

	return true;
}

Bool Net_Write(const char *InMsg)
{
	size_t StringSize = strlen(InMsg);
	unsigned long Transferred = 0, TotalTransferred = 0;

	do
	{
		Transferred = send(SocketDescriptor, InMsg, (StringSize - TotalTransferred), MSG_NOSIGNAL);
		
		if (Transferred == -1) /*This is ugly I know, but it's converted implicitly, so shut up.*/
		{
			return false;
		}
		
		TotalTransferred += Transferred;
	} while (StringSize > TotalTransferred);
	
	if (ShowOutput) printf("%s", InMsg), fflush(stdout);
	
	usleep(SendDelay * 100000); /*We wait a moment so we don't get kicked for flooding.*/
	
	return true;
}

Bool Net_Read(char *OutStream_, unsigned long MaxLength)
{
	int Status = 0;
	char Byte = 0;
	char *OutStream = OutStream_;
	unsigned long Inc = 0;
	
	*OutStream = '\0';
	
	do
	{
		Status = recv(SocketDescriptor, &Byte, 1, 0);
		
		if (Byte == '\n') break;
		
		*OutStream++ = Byte;
	} while (++Inc, Status > 0 && Inc < MaxLength - 1);
	
	*OutStream = '\0';
	
	if (*(OutStream - 1) == '\r') *(OutStream - 1) = '\0'; /*Remove carriage returns.*/
	
	if (Status == -1) return false;
	
	if (ShowOutput) puts(OutStream_), fflush(stdout);
	
	return true;
}

Bool Net_Disconnect(void)
{
	if (!SocketDescriptor) return false;
	
	return !close(SocketDescriptor);
}
