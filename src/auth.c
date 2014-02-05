/*aqu4, daughter of pr0t0, half-sister to Johnsbot!
 * Copyright 2014 Subsentient, all rights reserved.*/
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "aqu4.h"

struct AuthTree *AdminAuths = NULL;

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
