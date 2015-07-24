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
#include <time.h>
#include <ctype.h>
#include <sys/stat.h>
#ifdef WIN
#include <winsock2.h>
#endif
#include "aqu4.h"
#include "substrings/substrings.h"

bool ShowOutput;
int _argc;
char **_argv;

static void SigHandler(int Signal)
{
	switch (Signal)
	{
		case SIGTERM:
		case SIGINT:
			puts(Signal == SIGINT ? "Caught SIGINT, shutting down." : "Caught SIGTERM, shutting down.");
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

#ifndef NOSOCKETINHERIT
bool Main_SaveSocket(const char *OkMessageTarget)
{ //Writes the socket to config.
	FILE *Descriptor = fopen("aqu4bot.socket", "wb");
	
	if (!Descriptor) return false;
	
	//Write the descriptor.
	fwrite(&SocketDescriptor, 1, sizeof(int), Descriptor);
	
	//Write the target we'll tell restart successful to after we restart.
	fwrite(OkMessageTarget, 1, strlen(OkMessageTarget), Descriptor);
	
	fclose(Descriptor);
	return true;
}

static bool Main_ResumeFromSocket(void)
{ //Resumes our session from a socket.
	FILE *Descriptor = fopen("aqu4bot.socket", "rb");
	char OkMessageTarget[128];
	
	if (!Descriptor) return false;
	
	//Read in the socket.
	fread(&SocketDescriptor, 1, sizeof(int), Descriptor);
	
	//Read in the OK message target.
	int Char, Inc = 0;
	while ((Char = fgetc(Descriptor)) != EOF)
	{
		OkMessageTarget[Inc++] = Char;
	}
	OkMessageTarget[Inc] = '\0';
	
	//Close the descriptor.
	fclose(Descriptor);
	
	
	//Delete the file.
	remove("aqu4bot.socket");
	
	IRC_Message(OkMessageTarget, "New aqu4bot instance loaded. Rebuilding list of users in channels. Please wait...");
	
	char OutBuf[1024];
	//Now we need to send a names request to every channel to repopulate the user lists.
	for (struct ChannelTree *Worker = Channels; Worker; Worker = Worker->Next)
	{
		snprintf(OutBuf, sizeof OutBuf, "WHO %s\r\n", Worker->Channel);
		Net_Write(SocketDescriptor, OutBuf);
	}
	
	//Now send our I'm ok mommy speech.
	IRC_Message(OkMessageTarget, "Restart completed.");
	return true;
}

#endif //NOSOCKETINHERIT
static bool Main_GenConfig(void)
{
	FILE *Descriptor = NULL;
	char LineBuf[4096], CurChan[128], *Worker = LineBuf;
	char FileBuf[16384] = { '\0' }; /*16K max config size for generator. Should be ridiculously more than enough.*/
	register unsigned Inc = 0;
	char Nick[128], Ident[128], Mask[128];
	char OutBuf[sizeof LineBuf + 128], OutPath[256];
	struct stat FileStat;
	
	puts("Generating a config file as per your request.\n"
		"I'll need some information from you now.\n");
	
	printf("What is the hostname of the IRC server? e.g. irc.myserver.net\n--> ");
	
	fgets(LineBuf, sizeof LineBuf, stdin); LineBuf[strlen(LineBuf) - 1] = '\0';
	
	snprintf(OutBuf, sizeof OutBuf, "Hostname=%s\n", LineBuf);
	SubStrings.Cat(FileBuf, OutBuf, sizeof FileBuf);
	
	printf("What is the port number of the IRC server?\n--> ");
	
	fgets(LineBuf, sizeof LineBuf, stdin); LineBuf[strlen(LineBuf) - 1] = '\0';
	snprintf(OutBuf, sizeof OutBuf, "PortNum=%s\n", LineBuf);
	SubStrings.Cat(FileBuf, OutBuf, sizeof FileBuf);
	
	printf("What nickname do you want to give the bot?\n--> ");
	
	fgets(LineBuf, sizeof LineBuf, stdin); LineBuf[strlen(LineBuf) - 1] = '\0';
	snprintf(OutBuf, sizeof OutBuf, "Nick=%s\n", LineBuf);
	SubStrings.Cat(FileBuf, OutBuf, sizeof FileBuf);
	
	printf("What ident do you want to give the bot?\n--> ");
	
	fgets(LineBuf, sizeof LineBuf, stdin); LineBuf[strlen(LineBuf) - 1] = '\0';
	snprintf(OutBuf, sizeof OutBuf, "Ident=%s\n", LineBuf);
	SubStrings.Cat(FileBuf, OutBuf, sizeof FileBuf);
	
	printf("What do you want the 'real name' of the bot to be?\n--> ");
	
	fgets(LineBuf, sizeof LineBuf, stdin); LineBuf[strlen(LineBuf) - 1] = '\0';
	snprintf(OutBuf, sizeof OutBuf, "RealName=%s\n", LineBuf);
	SubStrings.Cat(FileBuf, OutBuf, sizeof FileBuf);
	
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
			
			snprintf(OutBuf, sizeof OutBuf, "Channel=%s\n", CurChan);
			SubStrings.Cat(FileBuf, OutBuf, sizeof FileBuf);
			printf("Added channel %s\n", CurChan);			
		} while ((Worker = SubStrings.Line.WhitespaceJump(Worker)));
	}
	
LLogging:
	printf("Do you want to enable logging? Please enter y or n.\n--> ");
	
	fgets(LineBuf, sizeof LineBuf, stdin); LineBuf[strlen(LineBuf) - 1] = '\0';
	
	switch (tolower(*LineBuf))
	{
		case 'y':
			snprintf(OutBuf, sizeof OutBuf, "Logging=true\n");
			SubStrings.Cat(FileBuf, OutBuf, sizeof FileBuf);
		LLogPMs:
			printf("Do you want to log private messages sent to the bot? Please enter y or n.\n--> ");
			
			fgets(LineBuf, sizeof LineBuf, stdin); LineBuf[strlen(LineBuf) - 1] = '\0';
			
			switch (tolower(*LineBuf))
			{
				case 'y':
					snprintf(OutBuf, sizeof OutBuf, "LogPMs=true\n");
					SubStrings.Cat(FileBuf, OutBuf, sizeof FileBuf);
					break;
				case 'n':
					snprintf(OutBuf, sizeof OutBuf, "LogPMs=false\n");
					SubStrings.Cat(FileBuf, OutBuf, sizeof FileBuf);
					break;
				default:
					putchar('\n');
					goto LLogPMs;
					break;
			}
			
			break;
		case 'n':
			snprintf(OutBuf, sizeof OutBuf, "Logging=false\n");
			SubStrings.Cat(FileBuf, OutBuf, sizeof FileBuf);
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

	snprintf(OutBuf, sizeof OutBuf, "BotOwner=%s!%s@%s\n", Nick, Ident, Mask);
	SubStrings.Cat(FileBuf, OutBuf, sizeof FileBuf);
	
	printf("A config file has been generated, but now we need a place to save it.\n"
		"Enter a filename for the new config file. aqu4bot.conf is what it should be\n"
		"named if you intend to use this config file.\n--> ");
	
	fgets(OutPath, sizeof OutPath, stdin); OutPath[strlen(OutPath) - 1] = '\0';
	
	
	if (stat(OutPath, &FileStat) == 0)
	{
		Bot_SetTextColor(YELLOW);
		printf("WARNING: ");
		Bot_SetTextColor(ENDCOLOR);
		
		printf("The config file \"%s\" already exists! Overwrite it?\n"
			"Please enter y or n\n--> ", OutPath);
		
		fgets(LineBuf, sizeof LineBuf, stdin); LineBuf[strlen(LineBuf) - 1] = '\0';
		
		if (tolower(*LineBuf) != 'y')
		{
			puts("Aborting, not overwriting the config file.");
			return false;
		}
	}
	
	Bot_SetTextColor(YELLOW);
	printf("Writing config file to \"%s\"...\n", OutPath);
	Bot_SetTextColor(ENDCOLOR);
	
	if (!(Descriptor = fopen(OutPath, "w")))
	{
		Bot_SetTextColor(RED);
		puts("\nFailed to save config file!");
		Bot_SetTextColor(ENDCOLOR);
		
		puts("Make sure it's a valid location,\nand that you have permission to write there.");
		return false;
	}
	
	/*Add a bit of help at the top of the file.*/
	fprintf(Descriptor, "#aqu4bot config file generated with --genconfig option.\n\n"
			"#Other options include:\n"
			"#NickservPwd=Nickserv Password\n"
			"#Prefix=$ (The global command prefix)\n"
			"#SendDelay=8 (tenths of a second to wait between sending messages)\n"
			"#SetBotmode=true/false (some networks request you set a special mode for your IRC bots.)\n"
			"#Admin=nick!ident@mask (Add a less privileged admin who can administrate channels etc,\n"
			"#but not administrate the bot itself.)\n"
			"#NoControlCodes=true/false (some networks will not allow you bold/etc.\n");
	fwrite(FileBuf, 1, strlen(FileBuf), Descriptor);
	
	fflush(NULL); fclose(Descriptor);
	
	Bot_SetTextColor(GREEN);
	puts("Done!");
	Bot_SetTextColor(ENDCOLOR);
	
	return true;
}

int main(int argc, char **argv)
{
	int Inc = 1;
	struct stat DirStat;
#ifndef WIN
	bool Background = false;
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
	
	/*Seed rand()*/
	srand(time(NULL));

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
			puts("Current options are --genconfig, --botrootdir=, and --verbose.");
#else
			puts("Current options are --genconfig, --botrootdir=, --verbose, and --background.");
#endif
			exit(0);
		}
		else if (SubStrings.StartsWith("--botrootdir=", argv[Inc]))
		{
			const char *const Path = argv[Inc] + strlen("--botrootdir=");
			
			if (!*Path)
			{
				fprintf(stderr, "No root directory for bot specified.\n");
			}
			
			printf("Bot root directory specified to be \"%s\".\n", Path);
			
			struct stat Derp;
			
			//If it exists, then cd to it. We don't check for errors because of bot restarts.
			if (stat(Path, &Derp) == 0) chdir(Path);
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
			FILE *Descriptor = fopen("aqu4bot.pid", "wb");
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


	if (stat("db", &DirStat) != 0)
	{
		printf("Database directory 'db' does not exist. Creating it.\n");
#ifdef WIN
		if (mkdir("db") != 0)
#else
		if (mkdir("db", 0755) != 0)
#endif
		{
			fprintf(stderr, "Unable to create database directory 'db'! Cannot continue!\n");
			exit(1);
		}
	}
	
	if (Config_LoadBrain()) //Restore if we are resuming from a restart.
	{
		fputs("Resuming session from brain.resume.\n", stderr);
	}

#ifndef NOSOCKETINHERIT
	if (Main_ResumeFromSocket())
	{
		fputs("Loaded saved socket descriptor. Resuming session.\n", stderr);
	}
	else
#endif
	if (!IRC_Connect()) return 1;
	
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
