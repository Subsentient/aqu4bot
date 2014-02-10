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
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include "aqu4.h"

struct AuthTree *AdminAuths;

static struct Blacklist
{
	char Nick[1024];
	char Ident[1024];
	char Mask[1024];
	
	struct Blacklist *Next;
	struct Blacklist *Prev;
} *BLCore;

Bool Auth_AddAdmin(const char *Nick, const char *Ident, const char *Mask, Bool BotOwner)
{
	struct AuthTree *Worker = AdminAuths;
	
	if (!Nick && !Ident && !Mask) return false;
	
	if (!AdminAuths)
	{
		Worker = AdminAuths = malloc(sizeof(struct AuthTree));
		memset(AdminAuths, 0, sizeof(struct AuthTree));
		
	}
	else
	{
		while (Worker->Next != NULL) Worker = Worker->Next;
		
		Worker->Next = malloc(sizeof(struct AuthTree));
		memset(Worker->Next, 0, sizeof(struct AuthTree));
		Worker->Next->Prev = Worker;
		
		Worker = Worker->Next;
	}
	
	if (Nick) snprintf(Worker->Nick, sizeof Worker->Nick, "%s", Nick);
	if (Ident) snprintf(Worker->Ident, sizeof Worker->Ident, "%s", Ident);
	if (Mask) snprintf(Worker->Mask, sizeof Worker->Mask, "%s", Mask);
	
	if (BotOwner) Worker->BotOwner = BotOwner;
	
	return true;
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

Bool Auth_IsAdmin(const char *Nick, const char *Ident, const char *Mask, Bool *BotOwner)
{
	struct AuthTree *Worker = AdminAuths;
	
	if (!AdminAuths) return false;
	
	if (!Nick || !Ident || !Mask) return false;
	
	for (; Worker; Worker = Worker->Next)
	{	
		if ((!*Worker->Nick || !strcmp(Nick, Worker->Nick)) &&
			(!*Worker->Ident || !strcmp(Ident, Worker->Ident)) &&
			(!*Worker->Mask || !strcmp(Mask, Worker->Mask)))
		{
			if (BotOwner) *BotOwner = Worker->BotOwner;
			return true;
		}
	}
		
	return false;		
}

/*Beyond this point, stuff for blacklisting.*/


Bool Auth_BlacklistDel(const char *Nick, const char *Ident, const char *Mask)
{
	struct Blacklist *Worker = BLCore;
	Bool Success = false;
	
	if (!BLCore) return false;
	
	for (; Worker; Worker = Worker->Next)
	{
		if ((!Nick || !*Worker->Nick || !strcmp(Nick, Worker->Nick)) &&
			(!Ident || !*Worker->Ident || !strcmp(Ident, Worker->Ident)) &&
			(!Mask || !*Worker->Mask || !strcmp(Mask, Worker->Mask)))
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

Bool Auth_IsBlacklisted(const char *Nick, const char *Ident, const char *Mask)
{
	struct Blacklist *Worker = BLCore;
	
	if (!Nick || !Ident || !Mask) return false;
	
	for (; Worker; Worker = Worker->Next)
	{
		if ((!*Worker->Nick || !strcmp(Worker->Nick, Nick)) &&
			(!*Worker->Ident || !strcmp(Worker->Ident, Ident)) &&
			(!*Worker->Mask || !strcmp(Worker->Mask, Mask)))
		{
			return true;
		}
	}
	
	return false;
}

Bool Auth_BlacklistAdd(const char *Nick, const char *Ident, const char *Mask)
{
	struct Blacklist *Worker = BLCore;
	
	if (!Nick && !Ident && !Mask) return false;
	
	if (Auth_IsBlacklisted(Nick, Ident, Mask)) return true;
	
	if (!BLCore)
	{
		Worker = BLCore = malloc(sizeof(struct Blacklist));
		memset(BLCore, 0, sizeof(struct Blacklist));
	}
	else
	{
		while (Worker->Next) Worker = Worker->Next;
		
		Worker->Next = malloc(sizeof(struct Blacklist));
		memset(Worker->Next, 0, sizeof(struct Blacklist));
		Worker->Next->Prev = Worker;
		Worker = Worker->Next;
	}
	
	if (Nick)
	{
		strncpy(Worker->Nick, Nick, sizeof Worker->Nick - 1);
		Worker->Nick[sizeof Worker->Nick - 1] = '\0';
	}
	
	if (Ident)
	{
		strncpy(Worker->Ident, Ident, sizeof Worker->Ident - 1);
		Worker->Ident[sizeof Worker->Ident - 1] = '\0';
	}
	
	if (Mask)
	{
		strncpy(Worker->Mask, Mask, sizeof Worker->Mask - 1);
		Worker->Mask[sizeof Worker->Mask - 1] = '\0';
	}
	
	Auth_BlacklistSave();
	return true;
}

void Auth_BlacklistLoad(void)
{
	FILE *Descriptor = fopen("blacklist.db", "r");
	char *BlacklistDB = NULL, *Worker = NULL;
	char CurLine[2048], Nick[1024], Ident[1024], Mask[1024];
	struct stat FileStat;
	unsigned long Inc = 0;
	
	if (!Descriptor) return;
	
	if (stat("blacklist.db", &FileStat) != 0) return;
	
	Worker = BlacklistDB = malloc(FileStat.st_size + 1);
	
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
			fprintf(stderr, "Bad blacklist in blacklist.db");
			continue;
		}
		
		if (!Auth_BlacklistAdd(*Nick == '*' ? NULL : Nick, *Ident == '*' ? NULL : Ident, *Mask == '*' ? NULL : Mask))
		{
			fprintf(stderr, "Unable to add blacklist %s!%s@%s", Nick, Ident, Mask);
			continue;
		}
	} while ((Worker = NextLine(Worker)));
	
	free(BlacklistDB);
}

Bool Auth_BlacklistSave(void)
{
	FILE *Descriptor = NULL;
	struct Blacklist *Worker = BLCore;
	char OutBuf[2048];
	
	if (!BLCore)
	{
		remove("blacklist.db");
		return true;
	}
	
	if (!(Descriptor = fopen("blacklist.db", "w"))) return false;
	
	for (; Worker != NULL; Worker = Worker->Next)
	{
		snprintf(OutBuf, sizeof OutBuf, "%s!%s@%s\n", *Worker->Nick ? Worker->Nick : "*",
				*Worker->Ident ? Worker->Ident : "*", *Worker->Mask ? Worker->Mask : "*");
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
		snprintf(OutBuf, sizeof OutBuf, "[%d] %s!%s@%s", Count, *Worker->Nick ? Worker->Nick : "*",
				*Worker->Ident ? Worker->Ident : "*", *Worker->Mask ? Worker->Mask : "*");
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
