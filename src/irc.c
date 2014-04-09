/*
aqu4, daughter of pr0t0, half-sister to Johnsbot! Copyright 2014 Subsentient.

This file is part of aqu4bot.
aqu4bot is public domain software.
See the file UNLICENSE.TXT for more information.
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>

#include "substrings/substrings.h"
#include "aqu4.h"

struct _ServerInfo ServerInfo;
struct ChannelTree *Channels;

void IRC_Loop(void)
{ /*Most of the action is triggered here.*/
	char MessageBuf[2048];
	
	while (1)
	{
		if (!Net_Read(SocketDescriptor, MessageBuf, sizeof MessageBuf, true))
		{ /*No command should ever call Net_Read() besides us and the connecting stuff that comes before us.*/
			int MaxTry = 0;
			
			Bot_SetTextColor(RED);
			puts("CONNECTION LOST");
			Bot_SetTextColor(ENDCOLOR);
			
			do
			{
				if (SocketDescriptor) close(SocketDescriptor);
				SocketDescriptor = 0;

				Bot_SetTextColor(GREEN);
				puts("Attempting to reconnect...");
				Bot_SetTextColor(ENDCOLOR);
				
				if (IRC_Connect())
				{
					Bot_SetTextColor(GREEN);
					printf("\nConnection reestablished after %d attempts.\n", MaxTry + 1);
					Bot_SetTextColor(ENDCOLOR);
					break;
				}
				else
				{
					Bot_SetTextColor(RED);
					printf("\nReconnect attempt %d failed.\n", MaxTry + 1);
					Bot_SetTextColor(ENDCOLOR);
				}
			} while (++MaxTry, MaxTry < 3);
			
			if (MaxTry == 3)
			{
				Bot_SetTextColor(RED);
				puts("** After three attempts, cannot reconnect to IRC server! Shutting down. **");
				Bot_SetTextColor(ENDCOLOR);
				
				if (SocketDescriptor) close(SocketDescriptor);
				exit(1);
			}
		}
		
		if (!strncmp(MessageBuf, "PING ", strlen("PING "))) IRC_Pong(MessageBuf); /*Respond to pings.*/
		
		switch (IRC_GetMessageType(MessageBuf))
		{
			char Nick[128], Ident[128], Mask[128];

			case IMSG_PRIVMSG:
			{
				char MessageData[2048], *TC = NULL, Channel[128];
				unsigned long Inc = 0;
								
				if (!IRC_BreakdownNick(MessageBuf, Nick, Ident, Mask)) continue;
				
				IRC_GetMessageData(MessageBuf, MessageData);
				
				for (; MessageData[Inc] != ' ' && MessageData[Inc] && Inc < sizeof Channel - 1; ++Inc)
				{
					Channel[Inc] = MessageData[Inc];
				}
				Channel[Inc] = '\0';
				
				IRC_CompleteChannelUser(Channel, Nick, Ident, Mask); /*In case we only have the nick available.*/
				
				if (Auth_IsBlacklisted(Nick, Ident, Mask)) continue; /*Says "you are not going to be listened to bud.*/
				
				if (strcmp(Nick, ServerInfo.Nick) != 0) Log_WriteMsg(MessageBuf, IMSG_PRIVMSG);
								
				TC = strchr(MessageData, ' ') + 1;
				
				if (*TC == ':') ++TC;
				
				if (*TC == 0x01)
				{ /*IRC escape code thingy...*/
					
					if (!strcmp(TC, "\01VERSION\01"))
					{
						IRC_Notice(Nick, "\01VERSION aqu4bot " BOT_VERSION ", compiled " __DATE__ " " __TIME__ "\01");
					}
					else if (!strncmp(TC, "\01PING", strlen("\01PING")))
					{
						IRC_Notice(Nick, TC);
					}
				}
				else
				{										
					CMD_ProcessCommand(MessageBuf);
					
					if (strcmp(Nick, ServerInfo.Nick) != 0)
					{
						CMD_UpdateSeenDB(time(NULL), Nick, Channel, TC);
					}
				}
				break;
			}
			case IMSG_NOTICE:
				if (!IRC_BreakdownNick(MessageBuf, Nick, Ident, Mask)) continue;
				
				if (Auth_IsBlacklisted(Nick, Ident, Mask)) continue;
				
				if (strcmp(Nick, ServerInfo.Nick) != 0) Log_WriteMsg(MessageBuf, IMSG_NOTICE);
				
				break;				
			case IMSG_INVITE:
			{
				if (!IRC_BreakdownNick(MessageBuf, Nick, Ident, Mask)) continue;
				
				if (Auth_IsBlacklisted(Nick, Ident, Mask)) continue;
				
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
					
					if (IRC_JoinChannel(TWorker)) IRC_AddChannelToTree(TWorker, NULL);
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
				struct ChannelTree *Worker = Channels;
				
				
				if (!IRC_BreakdownNick(MessageBuf, Nick, Ident, Mask)) continue;
				
				if (!strcmp(Nick, ServerInfo.Nick))
				{
					IRC_Quit(NULL);
					IRC_ShutdownChannelTree();
					Auth_ShutdownAdmin();
					CMD_SaveSeenDB();
					CMD_SaveUserModes();
					Auth_ShutdownBlacklist();
					exit(0);
				}
				
				Log_WriteMsg(MessageBuf, IMSG_QUIT);
				
				if (!Channels) continue;
				
				for (; Worker; Worker = Worker->Next)
				{
					IRC_DelUserFromChannelP(Worker, Nick);
				}
				
				break;
			}
			case IMSG_JOIN:
			{
				const char *Search = SubStrings.CFind('#', 1, MessageBuf);
				
				if (!IRC_BreakdownNick(MessageBuf, Nick, Ident, Mask)) continue;
				
				IRC_AddUserToChannel(Search, Nick, Ident, Mask, true);
				
				Log_WriteMsg(MessageBuf, IMSG_JOIN);
				
				if (!Search) printf("Unable to process modes for join by %s!%s@%s to bad channel.", Nick, Ident, Mask);
				else
				{
					CMD_ProcessUserModes(Nick, Ident, Mask, Search);
				}
				
				while (CMD_ReadTellDB(Nick));
				break;
			}
			case IMSG_PART:
			{
				char *Search = SubStrings.CFind('#', 1, MessageBuf), *Two = Search;
				
				if ((Two = SubStrings.CFind(' ', 1, Two))) *Two = '\0';
				
				if (!IRC_BreakdownNick(MessageBuf, Nick, Ident, Mask)) continue;

				IRC_DelUserFromChannel(Search, Nick);
				
				Log_WriteMsg(MessageBuf, IMSG_PART);
				break;
			}
			case IMSG_NICK:
			{
				char NewNick[128];
				struct ChannelTree *Worker = Channels;
				
				IRC_GetMessageData(MessageBuf, NewNick);
				
				Log_WriteMsg(MessageBuf, IMSG_NICK);
				
				while (CMD_ReadTellDB(*NewNick == ':' ? NewNick + 1 : NewNick));
				
				for (; Worker; Worker = Worker->Next)
				{
					if (IRC_UserInChannelP(Worker, Nick))
					{
						IRC_DelUserFromChannelP(Worker, Nick);
						IRC_AddUserToChannel(Worker->Channel, Nick, Ident, Mask, true);
					}
				}
				
				break;
			}
			case IMSG_MODE:
				Log_WriteMsg(MessageBuf, IMSG_MODE);
				break;
			case IMSG_TOPIC:
			{
				char TChannel[128], Topic[1024];
				char *TWorker = SubStrings.CFind('#', 1, MessageBuf);
				struct ChannelTree *Worker = Channels;
				unsigned long Inc = 0;
				
				if (!TWorker) continue;
				
				for (; TWorker[Inc] != ' ' && TWorker[Inc] != '\0' && Inc < sizeof TChannel - 1; ++Inc)
				{
					TChannel[Inc] = TWorker[Inc];
				}
				TChannel[Inc] = '\0';
				
				if (!(TWorker = SubStrings.Line.WhitespaceJump(TWorker))) continue;
				
				if (*TWorker == ':') ++TWorker;
				
				SubStrings.Copy(Topic, TWorker, sizeof Topic);
				
				for (; Worker; Worker = Worker->Next)
				{
					if (!strcmp(TChannel, Worker->Channel))
					{
						SubStrings.Copy(Worker->Topic, Topic, sizeof Worker->Topic);
						break;
					}
				}
				
				Log_WriteMsg(MessageBuf, IMSG_TOPIC);
				break;
			}
			case IMSG_NAMES:
			{
				char *NamesList = SubStrings.CFind('#', 1, MessageBuf);
				char Channel[128], Nick[128];
				unsigned long Inc = 0;
				
				if (!NamesList) continue;
				
				for (; NamesList[Inc] != ' ' && NamesList[Inc] != '\0'; ++Inc)
				{
					Channel[Inc] = NamesList[Inc];
				}
				Channel[Inc] = '\0';
				
				if (!(NamesList = SubStrings.Line.WhitespaceJump(NamesList))) continue;
				
				if (*NamesList == ':') ++NamesList;
				
				do
				{
					/*Some characters are used for OP and such.*/
					if (*NamesList == '!' || *NamesList == '@' || *NamesList == '#'
						|| *NamesList == '$' || *NamesList == '%' || *NamesList == '^'
						|| *NamesList == '&' || *NamesList == '*' || *NamesList == '+'
						|| *NamesList == '=' || *NamesList == '-' || *NamesList == '('
						|| *NamesList == ')' || *NamesList == '?') ++NamesList;
						
					for (Inc = 0; NamesList[Inc] != ' ' && NamesList[Inc] != '\0' && Inc < sizeof Nick - 1; ++Inc)
					{ /*We don't have full data on them yet, so just record nick.*/
						Nick[Inc] = NamesList[Inc];
					}
					Nick[Inc] = '\0';
					
					IRC_AddUserToChannel(Channel, Nick, NULL, NULL, false);
				} while ((NamesList = SubStrings.Line.WhitespaceJump(NamesList)));
				
				break;
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
	
	if (!Net_Connect(ServerInfo.Hostname, ServerInfo.PortNum, &SocketDescriptor)) goto Error;
	
	snprintf(UserString, sizeof UserString, "USER %s 8 * :%s\r\n", ServerInfo.Ident, ServerInfo.RealName);
	snprintf(NickString, sizeof NickString, "NICK %s\r\n", ServerInfo.Nick);
	
	Net_Write(SocketDescriptor, UserString);
	Net_Write(SocketDescriptor, NickString);
	
	/*Check if the server likes us.*/
	while (!ServerLikesUs)
	{
		if (!Net_Read(SocketDescriptor, MessageBuf, sizeof MessageBuf, true)) goto Error;
		
		/*Some servers ping right after you connect. It's horrible, but true.*/
		if (!strncmp("PING ", MessageBuf,  sizeof("PING ") - 1))
		{
			*strchr(MessageBuf, 'I') = 'O';
			Net_Write(SocketDescriptor, MessageBuf);
			Net_Write(SocketDescriptor, "\r\n");
			continue;
		}
		
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
		
		Net_Write(SocketDescriptor, OutBuf);
		
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
	if (!SocketDescriptor) return false;

	if (QuitMSG)
	{
		char OutBuf[2048];
		
		snprintf(OutBuf, sizeof OutBuf, "QUIT :%s\r\n", QuitMSG);
		Net_Write(SocketDescriptor, OutBuf);
	}
	else Net_Write(SocketDescriptor, "QUIT :aqu4bot " BOT_VERSION " shutting down.\r\n");
	
	if (Net_Disconnect(SocketDescriptor))
	{
		SocketDescriptor = 0;
		return true;
	}
	
	return false;
}

void IRC_CompleteChannelUser(const char *const Channel, const char *const Nick, const char *const Ident, const char *const Mask)
{
	struct ChannelTree *Worker = Channels;
	
	if (!Channels) return;
	
	for (; Worker; Worker = Worker->Next)
	{
		if (!strcmp(Channel, Worker->Channel))
		{
			struct _UserList *UWorker = Worker->UserList;
			
			for (; UWorker; UWorker = UWorker->Next)
			{
				if (!strcmp(Nick, UWorker->Nick) && !UWorker->FullUser)
				{
					SubStrings.Copy(UWorker->Ident, Ident, sizeof UWorker->Ident);
					SubStrings.Copy(UWorker->Mask, Mask, sizeof UWorker->Mask);
					UWorker->FullUser = true;
				}
			}
		}
	}
}

Bool IRC_AddUserToChannel(const char *const Channel, const char *const Nick, const char *const Ident, const char *const Mask, Bool FullUser)
{
	struct ChannelTree *Worker = Channels;
	
	for (; Worker; Worker = Worker->Next)
	{
		if (!strcmp(Channel, Worker->Channel))
		{
			struct _UserList *UWorker = Worker->UserList;
	
			if (!Worker->UserList)
			{
				UWorker = Worker->UserList = malloc(sizeof(struct _UserList));
				memset(Worker->UserList, 0, sizeof(struct _UserList));
			}
			else
			{
				struct _UserList *TW = UWorker;
				
				/*Check if it already exists.*/
				for (; TW; TW = TW->Next)
				{
					if (!strcmp(Nick, TW->Nick) && (!FullUser || !TW->FullUser || (!strcmp(Ident, TW->Ident) && !strcmp(Mask, TW->Mask))))
					{
						return true; /*No need to worry anybody, it'd be the same result if it wasn't already here.*/
					}
				}
				
				while (UWorker->Next) UWorker = UWorker->Next;
				
				UWorker->Next = malloc(sizeof(struct _UserList));
				memset(UWorker->Next, 0, sizeof(struct _UserList));
				UWorker->Next->Prev = UWorker;
				
				UWorker = UWorker->Next;
			}
			
			SubStrings.Copy(UWorker->Nick, Nick, sizeof UWorker->Nick);

			if ((UWorker->FullUser = FullUser))
			{
				SubStrings.Copy(UWorker->Ident, Ident, sizeof UWorker->Ident);
				SubStrings.Copy(UWorker->Mask, Mask, sizeof UWorker->Mask);
			}
			return true;
		}
	}
	
	return false;
}
	
Bool IRC_DelUserFromChannel(const char *const Channel, const char *const Nick)
{
	struct ChannelTree *Worker = Channels;
	
	if (!Channels) return false;
	
	for (; Worker; Worker = Worker->Next)
	{
		if (!strcmp(Channel, Worker->Channel))
		{
			struct _UserList *UWorker = Worker->UserList;
			
			for (; UWorker; UWorker = UWorker->Next)
			{
				if (!strcmp(UWorker->Nick, Nick))
				{
					if (UWorker == Worker->UserList)
					{
						if (UWorker->Next)
						{
							UWorker->Next->Prev = NULL;
							Worker->UserList = UWorker->Next;
							free(UWorker);
							return true;
						}
						else
						{
							Worker->UserList = NULL;
							free(UWorker);
							return true;
						}
					}
					else
					{
						if (UWorker->Next) UWorker->Next->Prev = UWorker->Prev;
						UWorker->Prev->Next = UWorker->Next;
						free(UWorker);
						return true;
					}
				}
			}
		}
	}
	
	return false;
}

Bool IRC_DelUserFromChannelP(struct ChannelTree *const Channel, const char *const Nick)
{
	struct _UserList *UWorker = Channel->UserList;
			
	for (; UWorker; UWorker = UWorker->Next)
	{
		if (!strcmp(UWorker->Nick, Nick))
		{
			if (UWorker == Channel->UserList)
			{
				if (UWorker->Next)
				{
					UWorker->Next->Prev = NULL;
					Channel->UserList = UWorker->Next;
					free(UWorker);
					return true;
				}
				else
				{
					Channel->UserList = NULL;
					free(UWorker);
					return true;
				}
			}
			else
			{
				if (UWorker->Next) UWorker->Next->Prev = UWorker->Prev;
				UWorker->Prev->Next = UWorker->Next;
				free(UWorker);
				return true;
			}
		}
	}
	
	return false;
}

void IRC_ShutdownChannelUsers(struct ChannelTree *const Channel)
{
	struct _UserList *UWorker = Channel->UserList, *Del;
	
	for (; UWorker; UWorker = Del)
	{
		Del = UWorker->Next;
		free(UWorker);
	}
	Channel->UserList = NULL;
}

Bool IRC_UserInChannel(const char *const Channel, const char *const Nick)
{
	struct ChannelTree *Worker = Channels;
	
	for (; Worker; Worker = Worker->Next)
	{
		if (!strcmp(Worker->Channel, Channel))
		{
			struct _UserList *UWorker = Worker->UserList;
			
			for (; UWorker; UWorker = UWorker->Next)
			{
				if (!strcmp(Nick, UWorker->Nick))
				{
					return true;
				}
			}
		}
	}
	
	return false;
}

Bool IRC_UserInChannelP(const struct ChannelTree *const Channel, const char *const Nick)
{
	struct _UserList *UWorker = Channel->UserList;
	
	for (; UWorker; UWorker = UWorker->Next)
	{
		if (!strcmp(Nick, UWorker->Nick))
		{
			return true;
		}
	}
	
	return false;
}

void IRC_AddChannelToTree(const char *const Channel, const char *const Prefix)
{
	struct ChannelTree *Worker = Channels;

	for (; Worker; Worker = Worker->Next)
	{
		if (!strcmp(Channel, Worker->Channel)) return;
	}
	Worker = Channels;
	
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
	
	if (Prefix)
	{
		strncpy(Worker->CmdPrefix, Prefix, sizeof Worker->CmdPrefix - 1);
		Worker->CmdPrefix[sizeof Worker->CmdPrefix - 1] = '\0';
	}
}

Bool IRC_DelChannelFromTree(const char *Channel)
{
	struct ChannelTree *Worker = Channels;

	if (!Channels) return true;
	
	for (; Worker; Worker = Worker->Next)
	{
		if (!strcmp(Channel, Worker->Channel))
		{
			IRC_ShutdownChannelUsers(Worker);
			
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
		IRC_ShutdownChannelUsers(Worker);
		free(Worker);
	}
	
	Channels = NULL;
}

Bool IRC_JoinChannel(const char *Channel)
{
	char ChanString[2048];
	
	snprintf(ChanString, sizeof ChanString, "JOIN %s\r\n", Channel);
	
	return Net_Write(SocketDescriptor, ChanString);
}
	
Bool IRC_LeaveChannel(const char *Channel)
{
	char ChanString[2048];
	
	snprintf(ChanString, sizeof ChanString, "PART %s\r\n", Channel);
	
	return Net_Write(SocketDescriptor, ChanString);
}

Bool IRC_Message(const char *Target, const char *Message)
{
	char OutString[2048];
	
	snprintf(OutString, sizeof OutString, "PRIVMSG %s :%s\r\n", Target, Message);
	return Net_Write(SocketDescriptor, OutString);
}

Bool IRC_Notice(const char *Target, const char *Notice)
{
	char OutString[2048];
	snprintf(OutString, sizeof OutString, "NOTICE %s :%s\r\n", Target, Notice);
	return Net_Write(SocketDescriptor, OutString);
}

Bool IRC_NickChange(const char *Nick)
{
	char OutString[2048];
	
	snprintf(OutString, sizeof OutString, "NICK %s\r\n", Nick);
	return Net_Write(SocketDescriptor, OutString);
}

MessageType IRC_GetMessageType(const char *InStream_)
{
	const char *InStream = InStream_;
	char Command[32];
	unsigned long Inc = 0;
	
	if (InStream[0] != ':') return IMSG_INVALID;
	
	if ((InStream = strchr(InStream, ' ')) == NULL) return IMSG_INVALID;
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
	else if (!strcmp(Command, "332") || !strcmp(Command, "TOPIC")) return IMSG_TOPIC;
	else if (!strcmp(Command, "333")) return IMSG_TOPICORIGIN;
	else if (!strcmp(Command, "353") || !strcmp(Command, "NAMES")) return IMSG_NAMES;
	else return IMSG_UNKNOWN;
}

Bool IRC_GetMessageData(const char *Message, char *OutData)
{
	const char *Worker = Message;
	
	if (!(Worker = strchr(Worker, ' '))) return false;
	++Worker;
	
	if (!(Worker = strchr(Worker, ' '))) return false;
	++Worker;
	
	if (*Worker == ':') ++Worker; /*This might cause problems somewhere, but I hope not. Saves work fiddling with server specific stuff.*/
		
	strcpy(OutData, Worker);
	return true;
}

Bool IRC_BreakdownNick(const char *Message, char *NickOut, char *IdentOut, char *MaskOut)
{
	char ComplexNick[128], *Worker = ComplexNick;
	unsigned long Inc = 0;
	
	for (; Message[Inc] != ' ' && Message[Inc] != '\0'; ++Inc)
	{
		ComplexNick[Inc] = Message[Inc];
	}
	ComplexNick[Inc] = '\0';
	
	if (*Worker == ':') ++Worker;
	
	for (Inc = 0; Worker[Inc] != '!' &&  Worker[Inc] != ' ' && Worker[Inc] != '\0'; ++Inc)
	{
		NickOut[Inc] = Worker[Inc];
	}
	NickOut[Inc] = '\0';
	
	if (Worker[Inc] != '!')
	{
		*NickOut = 0;
		*IdentOut = 0;
		*MaskOut = 0;
		return false;
	}
	
	Worker += Inc + 1;
	
	if (*Worker == '~') ++Worker;
	
	for (Inc = 0; Worker[Inc] != '@' && Worker[Inc] != ' ' && Worker[Inc] != '\0'; ++Inc)
	{
		IdentOut[Inc] = Worker[Inc];
	}
	IdentOut[Inc] = '\0';
	
	if (Worker[Inc] != '@')
	{
		*NickOut = 0;
		*IdentOut = 0;
		*MaskOut = 0;
		return false;
	}
	
	Worker += Inc + 1;
	
	for (Inc = 0; Worker[Inc] != ' ' && Worker[Inc] != '\0'; ++Inc)
	{
		MaskOut[Inc] = Worker[Inc];
	}
	MaskOut[Inc] = '\0';
	
	return true;
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
	
	snprintf(OutBuf, sizeof OutBuf, "PONG%s\r\n", Param + strlen("PING"));
	Net_Write(SocketDescriptor, OutBuf);
}
