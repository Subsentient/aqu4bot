/*aqu4, daughter of pr0t0, half-sister to Johnsbot!
 * Copyright 2014 Subsentient, all rights reserved.*/

/**This is a particularly small file.**/

#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#include "aqu4.h"

Bool Logging;
Bool LogPMs;

Bool Log_WriteMsg(const char *InStream, MessageType MType)
{
	FILE *Descriptor = NULL;
	char Filename[1024], OutBuf[2048], Origin[2048], Message[2048];
	char Nick[1024], Ident[1024], Mask[1024], *Worker = Origin;
	time_t Time = time(NULL);
	struct tm TimeStruct;
	char TimeString[1024];
	struct stat DirStat;
	
	if (!Logging) return true;
	
	if (!IRC_BreakdownNick(InStream, Nick, Ident, Mask) || !*Nick || !*Ident || !*Mask) return true;
		
	IRC_GetMessageData(InStream, Origin);
	
	if (*Origin != '#' && !LogPMs) return true;
	
	if (MType != IMSG_JOIN && MType != IMSG_PART)
	{
		Worker = strchr(Worker, ' ');
		*Worker++ = '\0';
		
		while (*Worker == ' ' ) ++Worker;
		
		if (*Worker == ':') ++Worker;
		
		strncpy(Message, Worker, sizeof Message - 1);
		Message[sizeof Message - 1] = '\0';
	}
	
	if (MType == IMSG_PART || MType == IMSG_JOIN) /*I really doubt I'll ever see that on a JOIN.*/
	{
		if ((Worker = strstr(Origin, " :"))) *Worker = '\0';
	}
	else if (MType == IMSG_KICK)
	{
		if ((Worker = strstr(Message, " :"))) *Worker = '\0';
	}
	
	if (stat("logs", &DirStat) != 0)
	{
		if (mkdir("logs", 0755) != 0) return false;
	}
	
	snprintf(Filename, sizeof Filename, "logs/%s.txt", *Origin == '#' ? Origin : Nick);
	
	gmtime_r(&Time, &TimeStruct);
	strftime(TimeString, sizeof TimeString, "[%F %T UTC]", &TimeStruct);

	switch (MType)
	{		
		case IMSG_PRIVMSG:
			if (!strncmp(Message, "\01ACTION", strlen("\01ACTION")))
			{
				char *Temp =  strchr(Message + 1, '\01');
				
				if (Temp) *Temp = '\0';
				
				snprintf(OutBuf, sizeof OutBuf, "%s **%s %s**\n", TimeString, Nick, Message + strlen("\01ACTION "));
			}
			else snprintf(OutBuf, sizeof OutBuf, "%s %s: %s\n", TimeString, Nick, Message);
			break;
		case IMSG_NOTICE:
			snprintf(OutBuf, sizeof OutBuf, "%s %s (notice): %s\n", TimeString, Nick, Message);
			break;
		case IMSG_JOIN:
			snprintf(OutBuf, sizeof OutBuf, "%s <%s joined %s>\n", TimeString, Nick, Origin);
			break;
		case IMSG_PART:
			snprintf(OutBuf, sizeof OutBuf, "%s <%s left %s>\n", TimeString, Nick, Origin);
			break;
		case IMSG_KICK:
			snprintf(OutBuf, sizeof OutBuf, "%s <%s was kicked from %s by %s>\n", TimeString, Message, Origin, Nick);
			break;
		default:
			return false;
	}
	
	if (!(Descriptor = fopen(Filename, "a")))
	{
		return false;
	}
	
	fwrite(OutBuf, 1, strlen(OutBuf), Descriptor);
	fclose(Descriptor);
	
	return true;
}
