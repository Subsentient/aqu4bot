/*
aqu4, daughter of pr0t0, half-sister to Johnsbot! Copyright 2014 Subsentient.

This file is part of aqu4bot.
aqu4bot is public domain software.
See the file UNLICENSE.TXT for more information.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#ifdef WIN
#include <winsock2.h>
#endif
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
			CMD_SaveUserModes();
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
	struct stat DirStat;
#ifndef WIN
	Bool Background = false;
#endif

#ifdef WIN
	WSADATA WSAData;

    if (WSAStartup(MAKEWORD(1,1), &WSAData) != 0)
    {
        fprintf(stderr, "Unable to initialize WinSock2!");
        exit(1);
    }
#endif

	_argc = argc;
	_argv = argv;
	
	signal(SIGINT, SigHandler);

	for (; Inc < argc; ++Inc)
	{
		if (!strcmp(argv[Inc], "--verbose"))
		{
			ShowOutput = true;
		}
#ifndef WIN
		else if (!strcmp(argv[Inc], "--background"))
		{
			Background = true;
		}
#endif
		else
		{
			fprintf(stderr, "Bad command line argument \"%s\".\n", argv[Inc]);
			exit(1);
		}
	}
		
	puts("\033[36maqu4bot " BOT_VERSION " (" BOT_OS ") starting up.\033[0m\n"
		"Compiled " __DATE__ " " __TIME__ "\n");

#ifdef WIN
	if (stat("db", &DirStat) != 0 && mkdir("db") != 0)
#else
	if (stat("db", &DirStat) != 0 && mkdir("db", 0755) != 0)
#endif
	{
		fprintf(stderr, "Unable to create database directory 'db'! Cannot continue!\n");
		exit(1);
	}
#ifndef WIN
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
			FILE *Descriptor = fopen("aqu4bot.pid", "w");
			setsid();
			freopen("stdout.log", "a", stdout);
			freopen("stderr.log", "a", stderr);
			
			if (Descriptor)
			{
				fprintf(Descriptor, "%lu", (unsigned long)getpid());
				fclose(Descriptor);
			}
		}
	}
#endif
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
	
	/*Load user modes.*/
	CMD_LoadUserModes();
	
	/*The main IRC loop.*/
	IRC_Loop();
	
	return 0;
}
