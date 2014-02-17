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
#include <unistd.h>
#include <string.h>
#include <signal.h>

#include "aqu4.h"

Bool ShowOutput;
int _argc;
char **_argv;

static void SigHandler(int Signal)
{
	switch (Signal)
	{
		case SIGINT:
			puts("Caught SIGINT, shutting down.");
			IRC_Quit(NULL);
			IRC_ShutdownChannelTree();
			Auth_ShutdownAdmin();
			CMD_SaveSeenDB();
			Auth_ShutdownBlacklist();
			exit(0);
			break;
		default:
			break;
	}
}

int main(int argc, char **argv)
{
	int Inc = 1;
	Bool Background = false;
	
	_argc = argc;
	_argv = argv;
	
	signal(SIGINT, SigHandler);

	for (; Inc < argc; ++Inc)
	{
		if (!strcmp(argv[Inc], "--verbose"))
		{
			ShowOutput = true;
		}
		else if (!strcmp(argv[Inc], "--background"))
		{
			Background = true;
		}
		else
		{
			fprintf(stderr, "Bad command line argument \"%s\".\n", argv[Inc]);
			exit(1);
		}
	}
		
	puts("\033[36maqu4bot " BOT_VERSION " starting up.\033[0m\n");
	
	if (Background)
	{
		pid_t NewPID;
		
		puts("Backgrounding.");
		
		if ((NewPID = fork()) == -1)
		{
			fprintf(stderr, "Failed to fork()!");
			exit(1);
		}
		else if (NewPID > 0)
		{
			signal(SIGCHLD, SIG_IGN);
			exit(0);
		}
		else
		{
			setsid();
			freopen("stdout.log", "a", stdout);
			freopen("stderr.log", "a", stderr);
		}
	}
	
	if (!Config_ReadConfig())
	{
		return 1;
	}

	if (!IRC_Connect())
	{
		return 1;
	}
	
	/*Load the seen command data.*/
	CMD_LoadSeenDB();
	
	/*Load the blacklist data.*/
	Auth_BlacklistLoad();
	
	/*The main IRC loop.*/
	IRC_Loop();
	
	return 0;
}
