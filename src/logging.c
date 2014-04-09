/*
aqu4, daughter of pr0t0, half-sister to Johnsbot! Copyright 2014 Subsentient.

This file is part of aqu4bot.
aqu4bot is public domain software.
See the file UNLICENSE.TXT for more information.
*/

/**This is a particularly small file.**/

#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include "substrings/substrings.h"
#include "aqu4.h"

Bool Logging;
Bool LogPMs;
static Bool NoLogToConsole;

/*Prototypes.*/
static Bool Log_TopicLog(const char *InStream);
static Bool Log_ModeLog(const char *InStream);

Bool Log_CoreWrite(const char *InStream, const char *FileTitle_)
{
	char TimeString[128];
	time_t Time = time(NULL);
	struct tm *TimeStruct;
	char OutBuf[1024];	
	char FileTitle[256];
	unsigned long Inc = 0;
	
	SubStrings.Copy(FileTitle, FileTitle_, sizeof FileTitle);
	
	for (; FileTitle[Inc] != '\0'; ++Inc) FileTitle[Inc] = tolower(FileTitle[Inc]);
	
	TimeStruct = gmtime(&Time);
	strftime(TimeString, sizeof TimeString, "[%Y-%m-%d %H:%M:%S UTC]", TimeStruct);
	
	snprintf(OutBuf, sizeof OutBuf, "%s %s\n", TimeString, InStream);
	
	if (Logging && (*FileTitle == '#' || LogPMs))
	{
		FILE *Descriptor = NULL;
		char FileName[256] = "logs/";
		struct stat DirStat;
		
		if (stat("logs", &DirStat) != 0)
		{
#ifdef WIN
			if (mkdir("logs") != 0) return false;
#else
			if (mkdir("logs", 0755) != 0) return false;
#endif
		}
		
		SubStrings.Cat(FileName, FileTitle, sizeof FileName);
		SubStrings.Cat(FileName, ".txt", sizeof FileName);
		
		if (!(Descriptor = fopen(FileName, "a"))) return false;
		
		fwrite(OutBuf, 1, SubStrings.Length(OutBuf), Descriptor);
		fflush(Descriptor);
		fclose(Descriptor);
	}
	
	if (!ShowOutput && !NoLogToConsole) /*Don't spit dual copies everywhere if we're in verbose.*/
	{
		OutBuf[SubStrings.Length(OutBuf) - 1] = '\0'; /*Delete newline.*/
		puts(OutBuf);
	}
	
	return true;
}


static Bool Log_ModeLog(const char *InStream)
{
	char Nick[128], Channel[128], Mode[256];
	char OutBuf[1024];
	const char *Worker = InStream + 1;
	unsigned long Inc = 0;
	
	for (; Worker[Inc] != '!' && Worker[Inc] != ' ' && Worker[Inc] != '\0' && Inc < sizeof Nick - 1; ++Inc)
	{
		Nick[Inc] = Worker[Inc];
	}
	Nick[Inc] = '\0';
	
	/*Do not log for ourselves.*/
	if (SubStrings.Compare(Nick, ServerInfo.Nick)) return true;
	
	if (!(Worker = SubStrings.Line.WhitespaceJump(Worker))) return false;
	if (!(Worker = SubStrings.Line.WhitespaceJump(Worker))) return false; /*Twice for the MODE command.*/
	
	for (Inc = 0; Worker[Inc] != ' ' && Worker[Inc] != '\0' && Inc < sizeof Channel - 1; ++Inc)
	{
		Channel[Inc] = Worker[Inc];
	}
	Channel[Inc] = '\0';
	
	if (!(Worker = SubStrings.Line.WhitespaceJump(Worker))) return false;
	
	if (*Worker == ':') ++Worker;
	
	for (Inc = 0; Worker[Inc] != '\0' && Inc < sizeof Mode - 1; ++Inc)
	{
		Mode[Inc] = Worker[Inc];
	}
	Mode[Inc] = '\0';
	
	snprintf(OutBuf, sizeof OutBuf, "<%s sets mode %s on %s>", Nick, Mode, Channel);
	Log_CoreWrite(OutBuf, Channel);
	
	return true;
}

static Bool Log_TopicLog(const char *InStream)
{
	char Channel[128], Topic[1024], OutBuf[1024];
	char *Worker = SubStrings.CFind('#', 1, InStream);
	unsigned long Inc = 0;
	
	if (!Worker) return false;
	
	for (; Worker[Inc] != ' ' && Worker[Inc] != '\0' && Inc < sizeof Channel - 1; ++Inc)
	{
		Channel[Inc] = Worker[Inc];
	}
	Channel[Inc] = '\0';
	
	if (!(Worker = SubStrings.Line.WhitespaceJump(Worker))) return false;
	
	if (*Worker == ':') ++Worker;
	
	for (Inc = 0; Worker[Inc] != '\0' && Inc < sizeof Topic - 1; ++Inc)
	{
		Topic[Inc] = Worker[Inc];
	}
	Topic[Inc] = '\0';
	
	snprintf(OutBuf, sizeof OutBuf, "<Topic for %s is \"%s\">", Channel, Topic);
	
	Log_CoreWrite(OutBuf, Channel);
	
	return true;
}
	
	
Bool Log_WriteMsg(const char *InStream, MessageType MType)
{
	char OutBuf[2048], Origin[2048], Message[2048];
	char Nick[128], Ident[128], Mask[128], *Worker = Origin;
	
	if (MType == IMSG_TOPIC) return Log_TopicLog(InStream);
	else if (MType == IMSG_MODE) return Log_ModeLog(InStream);
	
	if (!IRC_BreakdownNick(InStream, Nick, Ident, Mask) || !*Nick || !*Ident || !*Mask) return true;
		
	IRC_GetMessageData(InStream, Origin);
	
	
	if (MType != IMSG_JOIN && MType != IMSG_QUIT && MType != IMSG_NICK && MType != IMSG_PART)
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

	switch (MType)
	{		
		case IMSG_PRIVMSG:
			if (!strncmp(Message, "\01ACTION", strlen("\01ACTION")))
			{
				char *Temp =  strchr(Message + 1, '\01');
				
				if (Temp) *Temp = '\0';
				
				snprintf(OutBuf, sizeof OutBuf, "(%s) **%s %s**",
						*Origin == '#' ? Origin : Nick, Nick, Message + strlen("\01ACTION "));
			}
			else snprintf(OutBuf, sizeof OutBuf, "(%s) %s: %s", *Origin == '#' ? Origin : Nick, Nick, Message);
			break;
		case IMSG_NOTICE:
			snprintf(OutBuf, sizeof OutBuf, "(%s) %s (notice): %s", *Origin == '#' ? Origin : Nick, Nick, Message);
			break;
		case IMSG_JOIN:
			snprintf(OutBuf, sizeof OutBuf, "<%s joined %s>", Nick, Origin);
			break;
		case IMSG_PART:
			snprintf(OutBuf, sizeof OutBuf, "<%s left %s>", Nick, Origin);
			break;
		case IMSG_KICK:
			snprintf(OutBuf, sizeof OutBuf, "<%s was kicked from %s by %s>", Message, Origin, Nick);
			break;
		case IMSG_NICK:
		case IMSG_QUIT:
		{
			struct ChannelTree *Worker = Channels;
			
			if (MType == IMSG_QUIT)
			{
				snprintf(OutBuf, sizeof OutBuf, "<%s has quit: %s>", Nick, *Origin == ':' ? Origin + 1 : Origin);
			}
			else
			{
				snprintf(OutBuf, sizeof OutBuf, "<%s is now known as %s>", Nick, Origin);
			}
			
			if (!Channels)
			{
				Log_CoreWrite(OutBuf, Nick);
				return true;
			}
			
			for (; Worker; Worker = Worker->Next)
			{
				if (IRC_UserInChannelP(Worker, Nick))
				{
					Log_CoreWrite(OutBuf, Worker->Channel);
					NoLogToConsole = true;
				}
			}
			
			NoLogToConsole = false;
			return true;
		}
		default:
			return false;
	}

	Log_CoreWrite(OutBuf, *Origin == '#' ? Origin : Nick);
	
	return true;
}
