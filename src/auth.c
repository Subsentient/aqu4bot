/*
aqu4, daughter of pr0t0, half-sister to Johnsbot! Copyright 2014 Subsentient.

This file is part of aqu4bot.
aqu4bot is public domain software.
See the file UNLICENSE.TXT for more information.
*/
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include "aqu4.h"
#include "substrings/substrings.h"

struct AuthTree *AdminAuths;

static struct Blacklist
{
	char Nick[128];
	char Ident[128];
	char Mask[128];
	
	struct Blacklist *Next;
	struct Blacklist *Prev;
} *BLCore;

bool Auth_AddAdmin(const char *Nick, const char *Ident, const char *Mask, bool BotOwner)
{
	struct AuthTree *Worker = AdminAuths;
	
	if (Auth_IsAdmin(Nick, Ident, Mask, NULL)) return false;
	
	if (!AdminAuths)
	{
		Worker = AdminAuths = (struct AuthTree*)malloc(sizeof(struct AuthTree));
		memset(AdminAuths, 0, sizeof(struct AuthTree));
		
	}
	else
	{
		while (Worker->Next != NULL) Worker = Worker->Next;
		
		Worker->Next = (struct AuthTree*)malloc(sizeof(struct AuthTree));
		memset(Worker->Next, 0, sizeof(struct AuthTree));
		Worker->Next->Prev = Worker;
		
		Worker = Worker->Next;
	}
	
	snprintf(Worker->Nick, sizeof Worker->Nick, "%s", Nick);
	snprintf(Worker->Ident, sizeof Worker->Ident, "%s", Ident);
	snprintf(Worker->Mask, sizeof Worker->Mask, "%s", Mask);
	
	if (BotOwner) Worker->BotOwner = BotOwner;
	
	return true;
}

bool Auth_DelAdmin(const char *Nick, const char *Ident, const char *Mask, bool OwnersToo)
{
	struct AuthTree *Worker = AdminAuths;
	bool Success = false;
	
	if (!Nick && !Ident && !Mask) return false;
	
	for (; Worker; Worker = Worker->Next)
	{
		if (!strcmp(Worker->Nick, Nick) && !strcmp(Worker->Ident, Ident) && !strcmp(Worker->Mask, Mask))
		{
			if (Worker->BotOwner && !OwnersToo) continue;
			
			if (Worker == AdminAuths)
			{
				if (Worker->Next)
				{
					AdminAuths = Worker->Next;
					AdminAuths->Prev = NULL;
					free(Worker);
				}
				else
				{
					free(AdminAuths);
					AdminAuths = NULL;
				}
			}
			else
			{
				if (Worker->Next) Worker->Next->Prev = Worker->Prev;
				Worker->Prev->Next = Worker->Next;
				free(Worker);
			}
			Success = true;
		}
	}
	return Success;
}

void Auth_ListAdmins(const char *SendTo)
{
	struct AuthTree *Worker = AdminAuths;
	char OutBuf[2048];
	int Count = 1;
	
	IRC_Message(SendTo, "List of admins:");
	
	for (; Worker; Worker = Worker->Next, ++Count)
	{
		snprintf(OutBuf, sizeof OutBuf, "[%d] (%s) %s!%s@%s", Count, 
				Worker->BotOwner ? "\0034OWNER\003" : "\0038ADMIN\003", *Worker->Nick ? Worker->Nick : "*",
				*Worker->Ident ? Worker->Ident : "*", *Worker->Mask ? Worker->Mask : "*");
		IRC_Message(SendTo, OutBuf);
	}
	
	IRC_Message(SendTo, "End of list.");
}

void Auth_ShutdownAdmin(void)
{
	struct AuthTree *Worker = AdminAuths, *Del = NULL;
	
	for (; Worker; Worker = Del)
	{
		Del = Worker->Next;
		free(Worker);
	}
	
	AdminAuths = NULL;
}

bool Auth_IsAdmin(const char *Nick, const char *Ident, const char *Mask, bool *BotOwner)
{
	struct AuthTree *Worker = AdminAuths;
	
	if (!AdminAuths) return false;
	
	for (; Worker; Worker = Worker->Next)
	{	
		if ((Worker->Nick[0] == '*' || !strcmp(Nick, Worker->Nick)) &&
			(Worker->Ident[0] == '*' ||!strcmp(Ident, Worker->Ident)) &&
			(Worker->Mask[0] == '*' || !strcmp(Mask, Worker->Mask)))
		{
			if (BotOwner) *BotOwner = Worker->BotOwner;
			return true;
		}
	}
		
	return false;		
}

/*Beyond this point, stuff for blacklisting.*/


bool Auth_BlacklistDel(const char *Nick, const char *Ident, const char *Mask)
{
	struct Blacklist *Worker = BLCore;
	bool Success = false;
	
	if (!BLCore) return false;
	
	for (; Worker; Worker = Worker->Next)
	{
		if (!strcmp(Nick, Worker->Nick) &&
			!strcmp(Ident, Worker->Ident) &&
			!strcmp(Mask, Worker->Mask))
		{
			if (Worker == BLCore)
			{
				if (Worker->Next)
				{
					Worker->Next->Prev = NULL;
					BLCore = Worker->Next;
					free(Worker);
				}
				else
				{
					free(BLCore);
					BLCore = NULL;
				}
			}
			else
			{
				if (Worker->Next) Worker->Next->Prev = Worker->Prev;
				Worker->Prev->Next = Worker->Next;
				free(Worker);
			}
			Success = true;
		}
	}
	
	Auth_BlacklistSave();
	return Success;
}

bool Auth_IsBlacklisted(const char *Nick, const char *Ident, const char *Mask)
{
	struct Blacklist *Worker = BLCore;
	
	if (!Nick || !Ident || !Mask) return false;
	
	for (; Worker; Worker = Worker->Next)
	{
		if ((Worker->Nick[0] == '*' || !strcmp(Worker->Nick, Nick)) &&
			(Worker->Ident[0] == '*' || !strcmp(Worker->Ident, Ident)) &&
			(Worker->Mask[0] == '*' || !strcmp(Worker->Mask, Mask)))
		{
			return true;
		}
	}
	
	return false;
}

bool Auth_BlacklistAdd(const char *Nick, const char *Ident, const char *Mask)
{
	struct Blacklist *Worker = BLCore;
	
	if (Auth_IsAdmin(Nick, Ident, Mask, NULL)) return false;
	
	if (Auth_IsBlacklisted(Nick, Ident, Mask)) return false;
	
	if (!BLCore)
	{
		Worker = BLCore = (struct Blacklist*)malloc(sizeof(struct Blacklist));
		memset(BLCore, 0, sizeof(struct Blacklist));
	}
	else
	{
		while (Worker->Next) Worker = Worker->Next;
		
		Worker->Next = (struct Blacklist*)malloc(sizeof(struct Blacklist));
		memset(Worker->Next, 0, sizeof(struct Blacklist));
		Worker->Next->Prev = Worker;
		Worker = Worker->Next;
	}
	

	strncpy(Worker->Nick, Nick, sizeof Worker->Nick - 1);
	Worker->Nick[sizeof Worker->Nick - 1] = '\0';

	strncpy(Worker->Ident, Ident, sizeof Worker->Ident - 1);
	Worker->Ident[sizeof Worker->Ident - 1] = '\0';

	strncpy(Worker->Mask, Mask, sizeof Worker->Mask - 1);
	Worker->Mask[sizeof Worker->Mask - 1] = '\0';


	Auth_BlacklistSave();
	return true;
}

void Auth_BlacklistLoad(void)
{
	FILE *Descriptor = fopen("db/blacklist.db", "rb");
	char *BlacklistDB = NULL, *Worker = NULL;
	char CurLine[2048], Nick[128], Ident[128], Mask[128];
	struct stat FileStat;
	unsigned Inc = 0;
	
	if (!Descriptor) return;
	
	if (stat("db/blacklist.db", &FileStat) != 0) return;
	
	Worker = BlacklistDB = (char*)malloc(FileStat.st_size + 1);
	
	fread(BlacklistDB, 1, FileStat.st_size, Descriptor);
	fclose(Descriptor);
	BlacklistDB[FileStat.st_size] = '\0';
	
	do
	{
		for (Inc = 0; Worker[Inc] != '\n' && Worker[Inc] != '\0' && Inc < sizeof CurLine - 1; ++Inc)
		{
			CurLine[Inc] = Worker[Inc];
		}
		CurLine[Inc] = '\0';
		
		if (!IRC_BreakdownNick(CurLine, Nick, Ident, Mask))
		{
			fprintf(stderr, "Bad blacklist in db/blacklist.db");
			continue;
		}
		
		if (!Auth_BlacklistAdd(Nick, Ident, Mask))
		{
			fprintf(stderr, "Unable to add blacklist %s!%s@%s", Nick, Ident, Mask);
			continue;
		}
	} while ((Worker = SubStrings.Line.NextLine(Worker)));
	
	free(BlacklistDB);
}

bool Auth_BlacklistSave(void)
{
	FILE *Descriptor = NULL;
	struct Blacklist *Worker = BLCore;
	char OutBuf[2048];
	
	if (!BLCore)
	{
		remove("db/blacklist.db");
		return true;
	}
	
	if (!(Descriptor = fopen("db/blacklist.db", "wb"))) return false;
	
	for (; Worker != NULL; Worker = Worker->Next)
	{
		snprintf(OutBuf, sizeof OutBuf, "%s!%s@%s\n", Worker->Nick, Worker->Ident, Worker->Mask);
		fwrite(OutBuf, 1, strlen(OutBuf), Descriptor);
	}
	
	fclose(Descriptor);
	
	return true;
}
	
void Auth_BlacklistSendList(const char *SendTo)
{
	char OutBuf[2048];
	struct Blacklist *Worker = BLCore;
	int Count = 0;
	
	if (!BLCore)
	{
		IRC_Message(SendTo, "No blacklisted vhosts exist.");
		return;
	}
	
	for (; Worker; Worker = Worker->Next) ++Count;
	
	snprintf(OutBuf, sizeof OutBuf, "%d blacklisted vhosts found:", Count);
	
	IRC_Message(SendTo, OutBuf);
	
	for (Count = 1, Worker = BLCore; Worker; Worker = Worker->Next, ++Count)
	{
		snprintf(OutBuf, sizeof OutBuf, "[%d] %s!%s@%s", Count, Worker->Nick, Worker->Ident, Worker->Mask);
		IRC_Message(SendTo, OutBuf);
	}
	
	IRC_Message(SendTo, "End of blacklist list.");
}
	
void Auth_ShutdownBlacklist(void)
{
	struct Blacklist *Worker = BLCore, *Del;
	
	for (; Worker; Worker = Del)
	{
		Del = Worker->Next;
		free(Worker);
	}

	BLCore = NULL;
}
