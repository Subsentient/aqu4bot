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
#include <signal.h>

#include "aqu4.h"

Bool ShowOutput;


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
	
	signal(SIGINT, SigHandler);
	
	if (argc == 2)
	{
		if (!strcmp(argv[1], "--verbose"))
		{
			ShowOutput = true;
		}
	}
		
	puts("\033[36maqu4bot " BOT_VERSION " starting up.\033[0m\n");
	
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
