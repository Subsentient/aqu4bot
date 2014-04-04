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
#include <ctype.h>
#include <sys/stat.h>
#ifdef WIN
#include <winsock2.h>
#endif
#include "aqu4.h"
#include "substrings/substrings.h"

Bool ShowOutput;
int _argc;
char **_argv;

static void SigHandler(int Signal)
{
	switch (Signal)
	{
		case SIGTERM:
			puts("Caught SIGTERM, shutting down.");
			IRC_Quit(NULL);
			IRC_ShutdownChannelTree();
			Auth_ShutdownAdmin();
			CMD_SaveSeenDB();
			CMD_SaveUserModes();
			Auth_ShutdownBlacklist();
			exit(0);
			break;
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

static Bool Main_GenConfig(void)
{
	FILE *Descriptor = fopen("aqu4bot-generated.conf", "wb"); /*Write in binary to keep Windows from meddling.*/
	char LineBuf[4096], CurChan[128], *Worker = LineBuf;
	register unsigned long Inc = 0;
	char Nick[128], Ident[128], Mask[128];
	if (!Descriptor)
	{
		fprintf(stderr, "I can't open aqu4bot.conf for writing in the current directory!");
		return false;
	}
	
	puts("Generating a config file as per your request.\n"
		"I'll need some information from you now.\n");
	
	printf("What is the hostname of the IRC server? e.g. irc.myserver.net\n--> ");
	
	fgets(LineBuf, sizeof LineBuf, stdin); LineBuf[strlen(LineBuf) - 1] = '\0';
	
	fprintf(Descriptor, "Hostname=%s\n", LineBuf);
	
	printf("What is the port number of the IRC server?\n--> ");
	
	fgets(LineBuf, sizeof LineBuf, stdin); LineBuf[strlen(LineBuf) - 1] = '\0';
	fprintf(Descriptor, "PortNum=%s\n", LineBuf);
	
	printf("What nickname do you want to give the bot?\n--> ");
	
	fgets(LineBuf, sizeof LineBuf, stdin); LineBuf[strlen(LineBuf) - 1] = '\0';
	fprintf(Descriptor, "Nick=%s\n", LineBuf);
	
	printf("What ident do you want to give the bot?\n--> ");
	
	fgets(LineBuf, sizeof LineBuf, stdin); LineBuf[strlen(LineBuf) - 1] = '\0';
	fprintf(Descriptor, "Ident=%s\n", LineBuf);
	
	printf("What do you want the 'real name' of the bot to be?\n--> ");
	
	fgets(LineBuf, sizeof LineBuf, stdin); LineBuf[strlen(LineBuf) - 1] = '\0';
	fprintf(Descriptor, "RealName=%s\n", LineBuf);
	
	printf("Please enter a list of channels separated by spaces.\n"
			"Put a comma followed by a prefix to specify a command prefix for that channel.\n--> ");
			
	fgets(LineBuf, sizeof LineBuf, stdin); LineBuf[strlen(LineBuf) - 1] = '\0';
	
	if (*LineBuf)
	{
		do
		{
			for (Inc = 0; Worker[Inc] != ' ' && Worker[Inc] != '\0' && Inc < sizeof CurChan - 1; ++Inc)
			{
				CurChan[Inc] = Worker[Inc];
			}
			CurChan[Inc] = '\0';
			
			fprintf(Descriptor, "Channel=%s\n", CurChan);
			printf("Added channel %s\n", CurChan);			
		} while ((Worker = SubStrings.Line.WhitespaceJump(Worker)));
	}
	
LLogging:
	printf("Do you want to enable logging? Please enter y or n.\n--> ");
	
	*LineBuf = getchar();
	if (*LineBuf == '\n') *LineBuf = getchar();
	getchar();
	
	switch (tolower(*LineBuf))
	{
		case 'y':
			fprintf(Descriptor, "Logging=true\n");
		LLogPMs:
			printf("Do you want to log private messages sent to the bot? Please enter y or n.\n--> ");
			
			*LineBuf = getchar();
			if (*LineBuf == '\n') *LineBuf = getchar();
			getchar();
			switch (tolower(*LineBuf))
			{
				case 'y':
					fprintf(Descriptor, "LogPMs=true\n");
					break;
				case 'n':
					fprintf(Descriptor, "LogPMs=false\n");
					break;
				default:
					putchar('\n');
					goto LLogPMs;
					break;
			}
			
			break;
		case 'n':
			fprintf(Descriptor, "Logging=false\n");
			break;
		default:
			goto LLogging;
			break;
	}
OwnerGet:
	printf("Enter a user's vhost to be bot 'owner', which is of the format 'nick!ident@ip_or_hostmask'.\n"
			"Wildcard '*' is accepted if it denotes an entire field as wild.\n--> ");
	
	fgets(LineBuf, sizeof LineBuf, stdin); LineBuf[strlen(LineBuf) - 1] = '\0';
	
	if (!IRC_BreakdownNick(LineBuf, Nick, Ident, Mask))
	{
		puts("That's not a valid vhost. Try again.");
		goto OwnerGet;
	
	
	}

	fprintf(Descriptor, "BotOwner=%s!%s@%s\n", Nick, Ident, Mask);
	
	puts("\nWell, you now have a config file at aqu4bot-generated.conf.\n"
		"Rename it to aqu4bot.conf to use it as your config file.\n"
		"Want me to do that for you?");
	Bot_SetTextColor(RED);
	printf("WARNING: ");
	Bot_SetTextColor(ENDCOLOR);
	printf("This will overwrite your existing\n"
		"config file, if you have one.\n--> ");
	
	*LineBuf = getchar();
	if (*LineBuf == '\n') *LineBuf = getchar();
	getchar();
	
	if (tolower(*LineBuf) == 'y')
	{
		remove(CONFIG_FILE);
		rename("aqu4bot-generated.conf", CONFIG_FILE);
		puts("File renamed.");
	}
	
	fclose(Descriptor);
	
	return true;

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
    { /*Initialize winsock*/
        fprintf(stderr, "Unable to initialize WinSock2!");
        exit(1);
    }
    
    /*Our dialog box for questions.*/
    
    if (stat("aqu4bot.conf", &DirStat) != 0)
    {
		int MBReturn;
		const char *WelcomeMsg = "Welcome to aqu4bot " BOT_VERSION " for Windows!\n"
								"I can't find your " CONFIG_FILE " file, so would\n"
								"you like to use the config file generator to create\n"
								"a basic usable configuration?";
		MBReturn = MessageBox(NULL, WelcomeMsg, "Welcome to aqu4bot " BOT_VERSION " for Windows!",
				MB_ICONINFORMATION | MB_YESNO | MB_DEFBUTTON2);
		
		if (MBReturn == IDYES)
		{
			Main_GenConfig();
			puts("Press any key to exit.");
			getchar();
			exit(0);
		}
		else
		{
			MessageBox(NULL, "Not launching the config generator.\nExiting because there is no config file.",
					"aqu4bot", MB_ICONINFORMATION | MB_OK);
			exit(0);
		}
	}
#endif

	_argc = argc;
	_argv = argv;
	
	signal(SIGINT, SigHandler);
	signal(SIGTERM, SigHandler);

	for (; Inc < argc; ++Inc)
	{
		if (!strcmp(argv[Inc], "--verbose"))
		{
			ShowOutput = true;
		}
		else if (!strcmp(argv[Inc], "--genconfig"))
		{ /*Config file builder.*/
			exit(!Main_GenConfig());
		}	
#ifndef WIN
		else if (!strcmp(argv[Inc], "--background"))
		{
			Background = true;
		}
#endif
		else if (!strcmp(argv[Inc], "--help"))
		{
#ifdef WIN
			puts("Current options are --genconfig and --verbose.");
#else
			puts("Current options are --genconfig, --verbose, and --background.");
#endif
			exit(0);
		}
		else
		{
			fprintf(stderr, "Bad command line argument \"%s\".\n", argv[Inc]);
			exit(1);
		}
	}
	
	signal(SIGINT, SigHandler);

#ifndef WIN
	if (getuid() == 0)
	{ /*Running as root is bad.*/
		fprintf(stderr, "WARNING: It is strongly recommended that you NOT run aqu4bot as root, for security reasons!\n\n");
	}
#endif

	Bot_SetTextColor(CYAN);
	puts("aqu4bot " BOT_VERSION " (" BOT_OS ") starting up.\n"
		"Compiled " __DATE__ " " __TIME__ "\n");
	Bot_SetTextColor(ENDCOLOR);
	
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


void Bot_SetTextColor(ConsoleColor Color)
{
#ifndef WIN
	printf("\033[%dm", Color);
#else
	BOOL (WINAPI *DoSetConsoleTextAttribute)(HANDLE, WORD) = NULL;
	HANDLE WDescriptor = GetStdHandle(STD_OUTPUT_HANDLE);
	static HMODULE kernel32 = (HMODULE)0xffffffff;
	WORD WinColor = 0;
	
	if (kernel32 == 0)
	{
		return;
	}
	else if(kernel32 == (HMODULE)0xffffffff)
	{
		kernel32 = LoadLibrary("kernel32.dll");
		
		if (kernel32 == 0)
		{
			return;
		}
	}
	
	/*Not supported.*/
	if (!(DoSetConsoleTextAttribute = (BOOL (WINAPI *)(HANDLE, WORD))GetProcAddress(kernel32, "SetConsoleTextAttribute"))) return;
	
	
	switch (Color)
	{
		case BLACK:
			WinColor = 0;
			break;
		case BLUE:
			WinColor = 1;
			break;
		case GREEN:
			WinColor = 2;
			break;
		case CYAN:
			WinColor = 3;
			break;
		case RED:
			WinColor = 4;
			break;
		case MAGENTA:
			WinColor = 5;
			break;
		case YELLOW:
			WinColor = 6;
			break;
		default: /*Everything else just becomes white.*/
			WinColor = 7;
			break;
	}
			
	DoSetConsoleTextAttribute(WDescriptor, WinColor);
#endif
}
