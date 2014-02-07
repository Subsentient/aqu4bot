/*aqu4, daughter of pr0t0, half-sister to Johnsbot!
 * Copyright 2014 Subsentient, all rights reserved.*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>

#include "aqu4.h"

struct _ServerInfo ServerInfo;
struct ChannelTree *Channels;

void IRC_Loop(void)
{ /*Most of the action is triggered here.*/
	char MessageBuf[2048];
	
	while (1)
	{
		if (!Net_Read(MessageBuf, sizeof MessageBuf))
		{ /*No command should ever call Net_Read() besides us and the connecting stuff that comes before us.*/
			puts("\033[31mCONNECTION LOST\033[0m");
			close(SocketDescriptor);
			exit(1);
		}
		
		if (!strncmp(MessageBuf, "PING ", strlen("PING "))) IRC_Pong(MessageBuf); /*Respond to pings.*/
		
		switch (IRC_GetMessageType(MessageBuf))
		{
			char Nick[1024], Ident[1024], Mask[1024];

			case IMSG_PRIVMSG:
			{
				char MessageData[2048], *TC = NULL, Channel[1024];
				unsigned long Inc = 0;
								
				IRC_BreakdownNick(MessageBuf, Nick, Ident, Mask);
				IRC_GetMessageData(MessageBuf, MessageData);
				
				if (strcmp(Nick, ServerInfo.Nick) != 0) Log_WriteMsg(MessageBuf, IMSG_PRIVMSG);
				
				for (; MessageData[Inc] != ' ' && MessageData[Inc] && Inc < sizeof Channel - 1; ++Inc)
				{
					Channel[Inc] = MessageData[Inc];
				}
				Channel[Inc] = '\0';
				
				TC = strchr(MessageData, ' ') + 1;
				
				if (*TC == ':') ++TC;
				
				if (*TC == 0x01)
				{ /*IRC escape code thingy...*/
					
					if (!strcmp(TC, "\01VERSION\01"))
					{
						IRC_Notice(Nick, "\01VERSION aqu4bot " BOT_VERSION "\01");
					}
					else if (!strncmp(TC, "\01PING", strlen("\01PING")))
					{
						IRC_Notice(Nick, TC);
					}
				}
				else
				{
					/*I don't feel like including time.h*/
					time_t time(time_t*);
										
					CMD_ProcessCommand(MessageBuf);
					
					if (strcmp(Nick, ServerInfo.Nick) != 0)
					{
						CMD_UpdateSeenDB(time(NULL), Nick, Channel, TC);
					}
				}
				break;
			}
			case IMSG_NOTICE:
				IRC_BreakdownNick(MessageBuf, Nick, Ident, Mask);
				
				if (strcmp(Nick, ServerInfo.Nick) != 0) Log_WriteMsg(MessageBuf, IMSG_NOTICE);
				
				break;				
			case IMSG_INVITE:
			{
				IRC_BreakdownNick(MessageBuf, Nick, Ident, Mask);
				
				if (Auth_IsAdmin(Nick, Ident, Mask, NULL))
				{
					const char *TWorker = MessageBuf;
					TWorker = strchr(TWorker, '#');
					
					if (!TWorker)
					{
						IRC_Message(Nick, "There is something wrong, and although you are an admin,"
									"your invite request is either malformed or it's my fault.");
						break;
					}
					
					IRC_Message(Nick, "Coming.");
					
					if (IRC_JoinChannel(TWorker)) IRC_AddChannelToTree(TWorker);
				}
				else
				{
					IRC_Message(Nick, "I'm sorry, I can only accept invite requests from my admins.");
				}
				break;
			}
			case IMSG_KICK:
			{
				char InBuf[2048], *Worker = InBuf;
				
				IRC_GetMessageData(MessageBuf, InBuf);
				
				Log_WriteMsg(MessageBuf, IMSG_KICK);
				
				if (!(Worker = strchr(InBuf, ' '))) break;
				
				*Worker = '\0';
				
				IRC_DelChannelFromTree(*InBuf == ':' ? InBuf + 1 : InBuf);
				break;
			}
			case IMSG_QUIT:
			{				
				IRC_BreakdownNick(MessageBuf, Nick, Ident, Mask);
				
				if (!strcmp(Nick, ServerInfo.Nick))
				{
					IRC_Quit(NULL);
					IRC_ShutdownChannelTree();
					Auth_ShutdownAdmin();
					CMD_SaveSeenDB();
					exit(0);
				}
				break;
			}
			case IMSG_JOIN:
			{
				
				IRC_BreakdownNick(MessageBuf, Nick, Ident, Mask);
				
				Log_WriteMsg(MessageBuf, IMSG_JOIN);
				while (CMD_ReadTellDB(Nick));
				break;
			}
			case IMSG_PART:
				IRC_BreakdownNick(MessageBuf, Nick, Ident, Mask);

				Log_WriteMsg(MessageBuf, IMSG_PART);
				break;
			case IMSG_NICK:
			{
				char NewNick[1024];

				IRC_GetMessageData(MessageBuf, NewNick);
				
				while (CMD_ReadTellDB(*NewNick == ':' ? NewNick + 1 : NewNick));
			}
			default:
				break;
		}
	}
}

Bool IRC_Connect(void)
{
	char UserString[2048], NickString[2048], MessageBuf[2048];
	struct ChannelTree *Worker = Channels;
	Bool ServerLikesUs = false;
	int Code = 0;
	
	printf("Connecting to \"%s:%hu\"... ", ServerInfo.Hostname, ServerInfo.PortNum), fflush(stdout);
	
	if (!Net_Connect(ServerInfo.Hostname, ServerInfo.PortNum)) goto Error;
	
	snprintf(UserString, sizeof UserString, "USER %s 8 * :%s\r\n", ServerInfo.Ident, ServerInfo.RealName);
	snprintf(NickString, sizeof NickString, "NICK %s\r\n", ServerInfo.Nick);
	
	Net_Write(UserString);
	Net_Write(NickString);
	
	/*Check if the server likes us.*/
	while (!ServerLikesUs)
	{
		if (!Net_Read(MessageBuf, sizeof MessageBuf)) goto Error;
		
		IRC_GetStatusCode(MessageBuf, &Code);
		
		if (Code == 0) continue;
		
		switch (Code)
		{
			case IRC_CODE_OK:
				ServerLikesUs = true;
				break;
			case IRC_CODE_NICKTAKEN:
				fprintf(stderr, "Nickname taken. Shutting down.\n");
				close(SocketDescriptor);
				exit(1);
				break;
			default:
				break;
		}
	}
	
	puts("Connected.");
	
	if (ServerInfo.SetBotmode)
	{
		char OutBuf[2048];
		
		printf("Setting +B on ourselves...");
		
		snprintf(OutBuf, sizeof OutBuf, "MODE %s +B\r\n", ServerInfo.Nick);
		
		Net_Write(OutBuf);
		
		puts(" Done.");
	}
	
	if (ServerInfo.NickservPwd[0] != '\0')
	{
		char NickservString[2048];
		
		printf("Authenticating with NickServ...");
		snprintf(NickservString, 2048, "identify %s", ServerInfo.NickservPwd);
		IRC_Message("NickServ", NickservString);
		puts(" Done.");
	}
	
	if (Channels)
	{
		printf("Joining channels..."), fflush(stdout);
		
		for (; Worker != NULL; Worker = Worker->Next)
		{
			IRC_JoinChannel(Worker->Channel);
			printf(" %s", Worker->Channel), fflush(stdout);
		}
		
		putc('\n', stdout);
	}
	
	puts("\nEverything's up, ready to go.\n----"), fflush(NULL);
	
	return true;
Error:
	fprintf(stderr, "Failed\n");
	return false;
}

Bool IRC_Quit(const char *QuitMSG)
{
	if (SocketDescriptor)
	{
		if (QuitMSG)
		{
			char OutBuf[2048];
			
			snprintf(OutBuf, sizeof OutBuf, "QUIT :%s\r\n", QuitMSG);
			Net_Write(OutBuf);
		}
		else Net_Write("QUIT :aqu4bot " BOT_VERSION " shutting down.\r\n");
	}
	
	return Net_Disconnect();
}

void IRC_AddChannelToTree(const char *Channel)
{
	struct ChannelTree *Worker = Channels;

	if (Channels == NULL)
	{
		Worker = Channels = malloc(sizeof (struct ChannelTree));
		memset(Channels, 0, sizeof(struct ChannelTree));
	}
	else
	{
		while (Worker->Next != NULL) Worker = Worker->Next;
		
		Worker->Next = malloc(sizeof(struct ChannelTree));
		memset(Worker->Next, 0, sizeof(struct ChannelTree));
		Worker->Next->Prev = Worker;
		
		Worker = Worker->Next;
	}
		
	strncpy(Worker->Channel, Channel, sizeof(Worker->Channel) - 1);
	Worker->Channel[sizeof(Worker->Channel) - 1] = '\0';
}

Bool IRC_DelChannelFromTree(const char *Channel)
{
	struct ChannelTree *Worker = Channels;

	if (!Channels) return true;
	
	for (; Worker; Worker = Worker->Next)
	{
		if (!strcmp(Channel, Worker->Channel))
		{
			if (Worker->Prev == NULL)
			{
				if (Worker->Next)
				{
					Worker->Next->Prev = NULL;
					Channels = Worker->Next;
					free(Worker);
					return true;
				}
				else
				{
					free(Worker);
					Channels = NULL;
					return true;
				}
			}
			else
			{
				if (Worker->Next) Worker->Next->Prev = Worker->Prev;
				Worker->Prev->Next = Worker->Next;
				free(Worker);
				return true;
			}
		}
	}
	
	return false;
}

void IRC_ShutdownChannelTree(void)
{
	struct ChannelTree *Worker = Channels, *Del = NULL;
	
	for (; Worker; Worker = Del)
	{
		Del = Worker->Next;
		free(Worker);
	}
	
	Channels = NULL;
}

Bool IRC_JoinChannel(const char *Channel)
{
	char ChanString[2048];
	
	snprintf(ChanString, sizeof ChanString, "JOIN %s\r\n", Channel);
	
	return Net_Write(ChanString);
}
	
Bool IRC_LeaveChannel(const char *Channel)
{
	char ChanString[2048];
	
	snprintf(ChanString, sizeof ChanString, "PART %s\r\n", Channel);
	
	return Net_Write(ChanString);
}

Bool IRC_Message(const char *Target, const char *Message)
{
	char OutString[2048];
	
	snprintf(OutString, sizeof OutString, "PRIVMSG %s :%s\r\n", Target, Message);
	return Net_Write(OutString);
}

Bool IRC_Notice(const char *Target, const char *Notice)
{
	char OutString[2048];
	snprintf(OutString, sizeof OutString, "NOTICE %s :%s\r\n", Target, Notice);
	return Net_Write(OutString);
}

Bool IRC_NickChange(const char *Nick)
{
	char OutString[2048];
	
	snprintf(OutString, sizeof OutString, "NICK %s\r\n", Nick);
	return Net_Write(OutString);
}

MessageType IRC_GetMessageType(const char *InStream_)
{
	const char *InStream = InStream_;
	char Command[32];
	unsigned long Inc = 0;
	
	if (InStream[0] != ':') return IMSG_INVALID;
	
	if ((InStream = strstr(InStream, " ")) == NULL) return IMSG_INVALID;
	++InStream;
	
	for (; InStream[Inc] != ' '  && InStream[Inc] != '\0' && Inc < sizeof Command - 1; ++Inc)
	{ /*Copy in the command.*/
		Command[Inc] = InStream[Inc];
	}
	Command[Inc] = '\0';
	
	/*Time for the comparison.*/
	if (!strcmp(Command, "PRIVMSG")) return IMSG_PRIVMSG;
	else if (!strcmp(Command, "NOTICE")) return IMSG_NOTICE;
	else if (!strcmp(Command, "MODE")) return IMSG_MODE;
	else if (!strcmp(Command, "JOIN")) return IMSG_JOIN;
	else if (!strcmp(Command, "PART")) return IMSG_PART;
	else if (!strcmp(Command, "PING")) return IMSG_PING;
	else if (!strcmp(Command, "PONG")) return IMSG_PONG;
	else if (!strcmp(Command, "NICK")) return IMSG_NICK;
	else if (!strcmp(Command, "QUIT")) return IMSG_QUIT;
	else if (!strcmp(Command, "KICK")) return IMSG_KICK;
	else if (!strcmp(Command, "KILL")) return IMSG_KILL;
	else if (!strcmp(Command, "INVITE")) return IMSG_INVITE;
	else return IMSG_UNKNOWN;
}

Bool IRC_GetMessageData(const char *Message, char *OutData)
{
	const char *Worker = Message;
	
	if (!(Worker = strchr(Worker, ' '))) return false;
	++Worker;
	
	if (!(Worker = strchr(Worker, ' '))) return false;
	++Worker;
		
	strcpy(OutData, Worker);
	return true;
}

void IRC_BreakdownNick(const char *Message, char *NickOut, char *IdentOut, char *MaskOut)
{
	char ComplexNick[1024], *Worker = ComplexNick;
	unsigned long Inc = 0;
	
	for (; Message[Inc] != ' ' && Message[Inc] != '\0'; ++Inc)
	{
		ComplexNick[Inc] = Message[Inc];
	}
	ComplexNick[Inc] = '\0';
	
	if (*Worker == ':') ++Worker;
	
	for (Inc = 0; Worker[Inc] != '!'; ++Inc)
	{
		NickOut[Inc] = Worker[Inc];
	}
	NickOut[Inc] = '\0';
	
	Worker += Inc + 1;
	
	if (*Worker == '~') ++Worker;
	
	for (Inc = 0; Worker[Inc] != '@'; ++Inc)
	{
		IdentOut[Inc] = Worker[Inc];
	}
	IdentOut[Inc] = '\0';
	
	Worker += Inc + 1;
	
	for (Inc = 0; Worker[Inc] != ' ' && Worker[Inc] != '\0'; ++Inc)
	{
		MaskOut[Inc] = Worker[Inc];
	}
	MaskOut[Inc] = '\0';
}

Bool IRC_GetStatusCode(const char *Message, int *OutNumber)
{ /*Returns true if we get a status code.*/
	unsigned long Inc = 0;
	char Num[64];
	
	if (!(Message = strchr(Message, ' '))) return false;
	++Message;
	
	for (; Message[Inc] != ' ' && Message[Inc] != '\0' && Inc < sizeof Num - 1; ++Inc)
	{
		if (!isdigit(Message[Inc])) return false;
		Num[Inc] = Message[Inc];
	}
	Num[Inc] = '\0';
	
	*OutNumber = atoi(Num);
	return true;
}

void IRC_Pong(const char *Param)
{
	char OutBuf[2048];
	
	snprintf(OutBuf, 2048, "PONG%s\r\n", Param + strlen("PING"));
	Net_Write(OutBuf);
}
