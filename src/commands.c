/*
aqu4, daughter of pr0t0, half-sister to Johnsbot! Copyright 2014 Subsentient.

This file is part of aqu4bot.
aqu4bot is public domain software.
See the file UNLICENSE.TXT for more information.
*/
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>
#include <sys/stat.h>
#include "substrings/substrings.h"
#include "aqu4.h"

char GlobalCmdPrefix[sizeof ((struct ChannelTree*)0)->CmdPrefix] = "$";
enum ArgMode { NOARG, OPTARG, REQARG };
enum HPerms { ANY, ADMIN, OWNER };

struct StickySpec
{
	unsigned ID;
	time_t Time;
	char Owner[128];
	char Data[2048];
};

static struct UserModeSpec
{
	char Nick[128];
	char Ident[128];
	char Mask[128];
	char Mode[128];
	char Target[384];
	char Channel[128];
	
	struct UserModeSpec *Next;
	struct UserModeSpec *Prev;
} *UserModeRoot;

struct
{
	char CmdName[64];
	char HelpString[512];
	enum ArgMode AM;
	enum HPerms P;
} CmdHelpList[] = 
		{
			{ "burrito", "Chucks a nasty, rotten burrito at someone.", REQARG, ANY },
			{ "beer", "Gives someone a cold, Samuel Adams beer.", REQARG, ANY },
			{ "godzilla", "Turns me into godzilla for a couple moments.", NOARG, OWNER },
			{ "wz", "Shows games in either the Warzone 2100 or Warzone 2100 Legacy game lobby."
				" Passing 'legacy' as an argument chooses the Legacy server, otherwise it's the wz2100.net server.", OPTARG, ANY },
			{ "guessinggame", "A simple number-guessing game where you guess from one to ten. "
				"The first guess starts the game.", REQARG, ANY },
			{ "sr", "A goofy command that returns whatever text you give it backwards.", REQARG, ANY },
#ifndef WIN
			{ "time", "Displays the current time in a specified timezone, or UTC if omitted or not found. "
				"After the timezone, you can specify strftime()-style syntax for custom output.", OPTARG, ANY },
#else
			{ "time", "Displays the current time. You may specify strftime()-style syntax for custom output, if desired. "
				"If output format is specified, the timezone is the local time of the bot. Otherwise it's UTC. "
				"Note that %r will not work, because this bot is built for Windows and Windows does not support this.",
				OPTARG, ANY },
#endif /*WIN*/

#ifndef NO_LIBCURL
			{ "title", "Displays the title of an http webpage. Domains must be prefixed with http:// or have no prefix, and https is unsupported.",
				REQARG, ANY },
			{ "ddg", "Searches DuckDuckGo for three search results. "
				"Many things return blank due to the limitations of DuckDuckGo's API. "
				"You can find the API that the results come from at \"http://api.duckduckgo.com/\".", REQARG, ANY },
#endif /*NO_LIBCURL*/
			{ "coin", "Flips a coin on the question you provide, giving heads or tails as the result.", REQARG, ANY },
			{ "seen", "Used to get information about the last time I have seen a nickname speak.", REQARG, ANY },
			{ "tell", "Used to tell someone a message the next time they enter a channel or speak.", REQARG, ANY },
			{ "sticky", "Used to save a sticky note. sticky save saves it, sticky read <number> reads it, sticky delete <number> "
				"deletes it, but only if it's your sticky. For admins, sticky reset deletes all stickies. "
				"sticky list lists the names of the owners and times of creation for all stickies.", REQARG, ANY },
			{ "replace", "Used to modify a quote by yourself or another user. Syntax is 'replace nick old^new'. "
				"You may omit 'new' to delete from a string, but not 'old'.", REQARG, ANY },
			{ "whoami", "Tells you your full nickname, along with whether or not you're a bot owner/admin.", NOARG, ANY },
			{ "tail", "Reads the end of the log for the channel or user specified as the first argument. "
				"The number of lines is optionally specified by the second argument, but the default is 10.", REQARG, ADMIN },
			{ "msg", "Sends a message to a nick/channel.", REQARG, ADMIN },
			{ "memsg", "Sends a message to a nick/channel in /me format.", REQARG, ADMIN },
			{ "noticemsg", "Sends a message as a notice.", REQARG, ADMIN },
			{ "chanctl", "Used for administrating channels. I must be OP in the channel for this to be useful. "
				"See chanctl help for a list of subcommands and more.", REQARG, ADMIN },
			{ "join", "Joins the specified channel(s). You can also specify a command prefix for the channels with a comma, e.g. 'join #derp,@'. "
				"You must be at least admin for this. ", REQARG, ADMIN },
			{ "part", "Leaves the specified channel(s). You must be at least admin for this."
				" If no argument is specified and you are already in a channel, "
				"it leaves the channel the command is issued from.", OPTARG, ADMIN },
			{ "cycle", "Leaves and then rejoins the specified channel."
				"If already in a channel, the channel can be omitted to cycle that channel."
				"You can also pass 'fresh' after or in place of the channel name to reset the channel"
				"to aqu4bot defaults, ignoring the config file.", OPTARG, ADMIN },
			{ "debug", "Subcommands include 'listchannels' to print all channels this bot is in, "
				"'dumpchanneldb' to list known users and their masks for either all channels, or you may specify a channel,"
				"and 'togglecontrolcodes', to toggle whether or not color and bold is permitted to appear in this bot's messages.", 
				REQARG, ADMIN },
			{ "nickchange", "Changes my nickname to the selected nick. "
				"Make sure the new nick is not taken before issuing this.", REQARG, OWNER },
			{ "admin", "Used to temporarily grant/remove bot admins. Cannot add owners. Changes must be made to"
				" the configuration file to make added or deleted admins permanent. The subcommands are add/del, both "
				"of which require a valid vhost, and list, which takes no argument.", REQARG, OWNER },
			{ "blacklist", "Used by admins to blacklist users from using me. 'blacklist set' is used to create a blacklist, "
				"'blacklist unset' is used to unset them, and 'blacklist list' lists known blacklistings. "
				"'set' and 'unset' require a valid vhost as an argument.", REQARG, ADMIN },
			{ "netwrite", "Writes raw data to the IRC network. For example, 'PRIVMSG derp :Hee hee' "
				"will send a private message to derp via raw IRC protocol.", REQARG, OWNER },
			{ "quit", "Tells me to shut down and disconnect. If an argument is given, I use it "
				"as my quit message.", OPTARG, OWNER },
			{ "restart", "Tells me to restart myself.", NOARG, OWNER },
			{ "help", "Prints info about me. If you specify an argument, then I'll give help for that command.", OPTARG, ANY },
			{ "commands", "Prints the list of commands I know.", NOARG, ANY },
			{ { '\0' } } /*Terminator.*/
		};

static struct _RandomGame
{
	bool Set;
	unsigned int Num : 4;
} RandGame;


static struct _SeenDB
{
	time_t Time;
	char Nick[128];
	char Channel[128];
	char LastMessage[2048];
	
	struct _SeenDB *Next;
	struct _SeenDB *Prev;
} *SeenRoot;

static void CMD_ChanCTL(const char *Message, const char *CmdStream, const char *SendTo);
static struct _SeenDB *CMD_SeenDBLookup(const char *const Nick);
static bool CMD_ListStickies(const char *SendTo);
static bool CMD_StickyDB(unsigned StickyID, void *OutSpec_, bool JustDelete);

void CMD_ProcessCommand(const char *InStream_)
{ /*Every good program needs at least one huge, unreadable,
	monolithic function that does wayyyy too much.*/
	char Nick[128], Ident[128], Mask[128], Target[128];
	char CommandID[128], Argument[4096];
	int Inc = 0;
	const char *SendTo = NULL;
	const char *InStream = InStream_;
	bool IsAdmin = false, BotOwner = false;
	char NickBasedPrefix[128], CmdPrefix[sizeof GlobalCmdPrefix] = { '\0' };
	struct ChannelTree *CWorker = Channels;
	
	snprintf(NickBasedPrefix, sizeof NickBasedPrefix, "%s:", ServerInfo.Nick);
	
	/*Get the nick info from this user.*/
	if (!IRC_BreakdownNick(InStream, Nick, Ident, Mask)) return;
	
	if (!strcmp(Nick, ServerInfo.Nick)) return; /*Don't let us order ourselves.*/
	
	/*Check authorization.*/
	IsAdmin = Auth_IsAdmin(Nick, Ident, Mask, &BotOwner);
	
	while (CMD_ReadTellDB(Nick));
	
	/*Skip past PRIVMSG.*/
	InStream = strstr(InStream, "PRIVMSG ") + (sizeof "PRIVMSG " - 1);


	//Get the target.
	SubStrings.CopyUntilC(Target, sizeof Target, &InStream, " ", true);
	
	
	/*Determine if our reply goes to a channel or via PM, depending on who said it.*/
	SendTo = Target[0] == '#' ? Target : Nick;
	
	/*Skip past all spaces preceding our new command.*/
	while (*InStream == ' ') ++InStream;
	
	/*Commands.*/
	if (*InStream == ':') ++InStream;
	
	/*Figure out what prefix we need to use. Copy in the default in case nothing is set.*/
	SubStrings.Copy(CmdPrefix, GlobalCmdPrefix, sizeof CmdPrefix);
	
	if (*Target == '#')
	{ /*If it's a channel, we might have a channel-specific prefix set.*/
		char TempTarget[sizeof Target];
		
		//Need to lower case it for the comparison.
		SubStrings.Copy(TempTarget, Target, sizeof TempTarget);
		SubStrings.ASCII.LowerS(TempTarget);
		
		for (; CWorker; CWorker = CWorker->Next)
		{
			if (!strcmp(CWorker->Channel, TempTarget))
			{
				if (*CWorker->CmdPrefix)
				{
					SubStrings.Copy(CmdPrefix, CWorker->CmdPrefix, sizeof CmdPrefix);
				}
				break;
			}
		}
	}
		
	
	if (*CmdPrefix && !strncmp(InStream, CmdPrefix, strlen(CmdPrefix)))
	{ /*The prefix matches one of ours.*/
		InStream += strlen(CmdPrefix);
	}
	else if (!strncmp(InStream, NickBasedPrefix, strlen(NickBasedPrefix)))
	{ /*Someone is sending us a command via our name rather than the prefix.*/
		InStream += strlen(NickBasedPrefix);
		
		while (*InStream == ' ' || *InStream == '\t') ++InStream;
		
		/*Handle the joke commands people send to us via nick.*/
		if (!strcmp(InStream, "hi") || !strcmp(InStream, "hello"))
		{
			IRC_Message(SendTo, "Hiyah.");
			return;
		}
		else if (*InStream == '\0')
		{
			IRC_Message(SendTo, "Umm, can I help you?");
			return;
		}
	}
	else if (*Target == '#')
	{ /*Auto-URL-title-read, if enabled for channel. Must be explicitly enabled.*/
		struct ChannelTree *LookupChannel = NULL;
		
		for (Inc = 0; Target[Inc] != '\0' && Inc < sizeof Target - 1; ++Inc)
		{ /*Lowercase the channel.*/
			Target[Inc] = tolower(Target[Inc]);
		}
		
		LookupChannel = IRC_GetChannelFromDB(Target); /*look up the channel.*/
		
		if (!LookupChannel) return; /*Channel not found.*/
		
		/*We have libcurl so check for a link.*/
		if (LookupChannel->AutoLinkTitle &&
		(!strncmp(InStream, "http://", sizeof "http://" - 1) || strstr(InStream, " http://") ||
		!strncmp(InStream, "https://", sizeof "https://" - 1) || strstr(InStream, " https://")))
		{ /*Starts with http.*/
			
			if (strncmp(InStream, "http://", sizeof "http://" - 1) != 0 &&
				strncmp(InStream, "https://", sizeof "https://" - 1) != 0)
			{ /*Not located at the beginning.*/
				const char *const OldInStream = InStream;
				
				InStream = strstr(InStream, " http://"); /*http*/
				if (!InStream) InStream = strstr(OldInStream, " https://"); /*https*/
				++InStream; /*skip past the space at front.*/
			}
			
			const char *Temp = InStream; //Don't want CopyUntilC() to modify InStream.

			/*Copy in this link to the argument and jump to title command.*/			
			SubStrings.CopyUntilC(Argument, sizeof Argument, &Temp, " ", true);


#ifndef NO_LIBCURL	
			goto TitleCommand; /*Jump into title code. Suck my goto.*/
#else
			IRC_Message(SendTo, "Auto-URL-title invoked, but this aqu4bot was compiled without libcurl.");
#endif
		}
		
		return;
	}

	const char *OIS = InStream;
	
	SubStrings.CopyUntilC(CommandID, sizeof CommandID, &InStream, "\t ", true);
	
	if (!InStream)
	{ //No argument.
		InStream = OIS;
		*Argument = '\0';
	}
	else
	{
		Inc = SubStrings.Copy(Argument, InStream, sizeof Argument);
		
		/*Get rid of trailing spaces.*/
		if (Inc > 0)
		{
			for (--Inc; Argument[Inc] == ' ' && Inc >= 0; --Inc) Argument[Inc] = '\0';
		}
	}
		
	/**		Start processing commands!		**/
	if (!strcmp(CommandID, "help"))
	{
		char TmpBuf[2048];
		const char *ArgRequired[3] = { "", " <optional_arg(s)>", " <required_arg(s)>" };
		const char *PermStrings[3] = { "", "\02ADMINS:\02 ", "\02OWNERS:\02 " };
		
		if (*Argument == '\0')
		{
			IRC_Message(SendTo, "Hi, I'm aqu4bot, version \"" BOT_VERSION
						"\", running on " BOT_OS ". "
						"I'm a bot written in pure C by Subsentient [" ROOT_URL "]. "
						"My source code can be found at \"" SOURCECODE_URL "\". "
						"Try the 'commands' command to get a list of what I can do, "
						"or try 'help cmd' where 'cmd' is a particular command.");
		}
		else
		{
			for (Inc = 0; *CmdHelpList[Inc].CmdName != '\0'; ++Inc)
			{
				if (!strcmp(Argument, CmdHelpList[Inc].CmdName))
				{
					snprintf(TmpBuf, sizeof TmpBuf, "%s[%s%s%s]: %s", PermStrings[CmdHelpList[Inc].P],
							CmdPrefix, CmdHelpList[Inc].CmdName, ArgRequired[CmdHelpList[Inc].AM],
							CmdHelpList[Inc].HelpString);
					IRC_Message(SendTo, TmpBuf);
					break;
				}
			}
			
			if (*CmdHelpList[Inc].CmdName == '\0')
			{
				IRC_Message(SendTo, "No help found for that command. Does it exist?"
							" Try an empty help command for a list of commands.");
			}
		}
		return;
	}
	else if (!strcmp(CommandID, "debug"))
	{
		char Subcommand[sizeof Argument];
		char Subargs[sizeof Argument], *TW = NULL;
		
		/*Get the subcommand.*/
		SubStrings.Copy(Subcommand, Argument, sizeof Subcommand);
		if ((TW = strpbrk(Subcommand, " \t"))) *TW = '\0'; /*Don't copy in any arguments into the subcommand, it's confusing.*/
		
		/*Get any arguments to a subcommand.*/		
		if (!(TW = SubStrings.Line.WhitespaceJump(Argument)))
		{ /*No argument.*/
			*Subargs = '\0';
		}
		else
		{
			SubStrings.Copy(Subargs, TW, sizeof Subargs);
		}
			
		
		
		/*Process said debug commands.*/
		if (!strcmp(Subcommand, "dumpchanneldb"))
		{
			struct ChannelTree *Worker = Channels;
			struct _UserList *UWorker = NULL;
			char OutBuf[1024];
			
			if (!BotOwner)
			{
				IRC_Message(SendTo, "You must be an owner for that.");
				return;
			}
			
			IRC_Message(SendTo, "Dumping channel database contents.");
			
			for (; Worker; Worker = Worker->Next)
			{
				if (!*Subargs || !strcmp(Subargs, Worker->Channel))
				{
					for (UWorker = Worker->UserList; UWorker; UWorker = UWorker->Next)
					{
						if (UWorker->FullUser)
						{
							snprintf(OutBuf, sizeof OutBuf, "%s: %s!%s@%s", Worker->Channel, UWorker->Nick, UWorker->Ident, UWorker->Mask);
						}
						else
						{
							snprintf(OutBuf, sizeof OutBuf, "%s: %s", Worker->Channel, UWorker->Nick);
						}
						
						IRC_Message(SendTo, OutBuf);
					}
				}
			}
		
			IRC_Message(SendTo, "End of list.");
		}
		else if (!strcmp(Subcommand, "listchannels"))
		{
			struct ChannelTree *Worker = Channels;
			int TInc = 1;
			char OutBuf[sizeof Worker->Channel] = { '\0' };
			
			if (!IsAdmin)
			{
				IRC_Message(SendTo, "You aren't authorized to see that.");
				return;
			}
			
			IRC_Message(SendTo, "List of channels I'm in:");
			
			for (; Worker; Worker = Worker->Next)
			{
				snprintf(OutBuf + strlen(OutBuf), sizeof OutBuf - strlen(OutBuf), "%s, ", Worker->Channel);
				
				if (TInc == 4 || !Worker->Next)
				{
					OutBuf[strlen(OutBuf) - 2] = '\0';
					TInc = 1;
					IRC_Message(SendTo, OutBuf);
					*OutBuf = '\0';
				}
				else ++TInc;
			}
			
			IRC_Message(SendTo, "End of list.");
		}
		else if (!strcmp(Subcommand, "togglecontrolcodes"))
		{
			if (!BotOwner)
			{
				IRC_Message(SendTo, "You aren't authorized to do that. Only owners.");
				return;
			}
			
			NoControlCodes = !NoControlCodes;
			
			IRC_Message(SendTo, NoControlCodes ? "Control codes disabled." : "Control codes enabled.");
			return;
		}
		else
		{
			IRC_Message(SendTo, "Bad debug subcommand.");
			return;
		}
		
		return;
	}
	else if (!strcmp(CommandID, "coin"))
	{ /*flip a coin.*/
		int Magic = 0;
		unsigned YesNo = false;
		char TmpBuf[1024];
		const char *CoinPhrases[10] = 
						{ "flips a bubblegum covered penny",
							"flips a shiney Soviet palladium coin",
							"flips a special edition USA quarter",
							"flips a canadian nickle found under the stove",
							"flips a Mexican peso",
							"flips a washer that could pass as a quarter",
							"flips a chocolate coin bought at the convenience store",
							"flips a big heavy round sewer lid", 
							"flips a dime found on the floor",
							"flips a 3V buttoncell battery that looks like a nickel",
						};
						
		if (!*Argument)
		{
			IRC_Message(SendTo, "I need something to ask the coin.");
			return;
		}
	
		snprintf(TmpBuf, sizeof TmpBuf, "\001ACTION %s on \"%s\"\001", CoinPhrases[rand() / (RAND_MAX / 10 + 1)], Argument);
		IRC_Message(SendTo, TmpBuf);
		
		/*Gerbil magic.*/
		for (Inc = 0; Argument[Inc] != '\0'; ++Inc)
		{
			Magic += Argument[Inc];
		}
		
		/*Get a rand and add our gerbil magic.*/
		YesNo = ((unsigned)rand() + Magic) / (RAND_MAX / 2 + 1); /*boolean value.*/
		
		snprintf(TmpBuf, sizeof TmpBuf, "I got %s.", YesNo ? "heads" : "tails");
		
		IRC_Message(SendTo, TmpBuf);
		return;
	}	
	else if (!strcmp(CommandID, "tail"))
	{
		char InBuf[16384], *Worker = Argument;
		int NumLines = 10;
		char LineBuf[1024];
		char TailChannel[128];
		
		if (!IsAdmin)
		{
			IRC_Message(SendTo, "You must be an admin to do that.");
			return;
		}
		
		if (!*Argument)
		{
			IRC_Message(SendTo, "You must provide a channel or user name.");
			return;
		}
		
		
		SubStrings.CopyUntilC(TailChannel, sizeof TailChannel, (void*)&Worker, " ", true);
		SubStrings.ASCII.LowerS(TailChannel);
		
		if (Worker != NULL)
		{ //No line count specified.
			char NumBuf[16];
			
			SubStrings.Copy(NumBuf, Worker, sizeof NumBuf);
			
			NumLines = atoi(NumBuf);
		}
		
		//Now get the info.
		if (!Log_TailLog(TailChannel, NumLines, InBuf, sizeof InBuf))
		{
			IRC_Message(SendTo, "Unable to tail the log. Most likely the channel/user isn't logged.");
			return;
		}
		
		Worker = InBuf;
		
		//Iteratively send the requested log lines.
		while (SubStrings.Line.GetLine(LineBuf, sizeof LineBuf, (const char**)&Worker))
		{			
			IRC_Message(SendTo, LineBuf);
		}
			
		
		IRC_Message(SendTo, "End of tail.");
		return;
	}
	else if (!strcmp(CommandID, "blacklist"))
	{
		char Subcommand[32], *Worker = Argument;
		
		if (!Auth_IsAdmin(Nick, Ident, Mask, NULL))
		{
			IRC_Message(SendTo, "You must be an admin to use that command.");
			return;
		}
		
		if (!*Argument)
		{
			IRC_Message(SendTo, "I need a subcommand. See 'help blacklist'.");
			return;
		}
		
		//Get the subcommand.
		SubStrings.CopyUntilC(Subcommand, sizeof Subcommand, (const char**)&Worker, " ", true);
		
		if (!strcmp(Subcommand, "set") || !strcmp(Subcommand, "unset"))
		{
				
			char NNick[128], NIdent[128], NMask[128];
			
			if (!Worker)
			{
				IRC_Message(SendTo, "This command requires both a subcommand and an argument to the subcommand.");
				return;
			}
			
			while (*Worker == ' ') ++Worker;
			
			if (!IRC_BreakdownNick(Worker, NNick, NIdent, NMask))
			{
				IRC_Message(SendTo, "That doesn't seem to be a valid vhost.");
				return;
			}
			
			if (!strcmp(Subcommand, "unset"))
			{
				if (Auth_BlacklistDel(NNick, NIdent, NMask))
				{
					IRC_Message(SendTo, "Unblacklisting successful.");
					return;
				}
				else
				{
					IRC_Message(SendTo, "Unable to delete blacklist, it probably doesn't exist.");
					return;
				}
			}
			else
			{
				if (Auth_BlacklistAdd(NNick, NIdent, NMask))
				{
					IRC_Message(SendTo, "Blacklisting successful.");
					return;
				}
				else
				{
					IRC_Message(SendTo, "Unable to add blacklist!");
					return;
				}
			}
		}
		else if (!strcmp(Subcommand, "list"))
		{
			Auth_BlacklistSendList(SendTo);
		}
		else
		{
			IRC_Message(SendTo, "Invalid subcommand. Valid are set, unset, and list.");
			return;
		}
	}
	else if (!strcmp(CommandID, "commands"))
	{
		char *CommandList = NULL;
		const char *PermStars[3] = { "", "\0038*\003", "\0034**\003" };
		
		if (*Argument)
		{
			IRC_Message(SendTo, "This command takes no argument.");
			return;
		}
		
		IRC_Message(SendTo, "Commands with 1 star = admins only, 2 stars = owners only. \02Commands available\02:");
		
		/*Count number of commands.*/
		for (Inc = 0; *CmdHelpList[Inc].CmdName != '\0'; ++Inc);
		
		CommandList = malloc((sizeof(CmdHelpList->CmdName) + 10) * Inc + 1);
		*CommandList = '\0';
		
		for (Inc = 0; *CmdHelpList[Inc].CmdName != '\0'; ++Inc)
		{
			strcat(CommandList, CmdHelpList[Inc].CmdName);
			strcat(CommandList, PermStars[CmdHelpList[Inc].P]);
			strcat(CommandList, ", ");
		}
		
		CommandList[strlen(CommandList) - 2] = '\0';
		
		IRC_Message(SendTo, CommandList);
		free(CommandList);
		return;
	}
	else if (!strcmp(CommandID, "burrito"))
	{
		char OutBuf[1024];
		
		if (*Argument == '\0') IRC_Message(SendTo, "The burrito command requires an argument.");
		else
		{
			snprintf(OutBuf, sizeof OutBuf, "\01ACTION chucks a nasty, rotten burrito at %s\01",
					!strcmp(Argument, ServerInfo.Nick) ? Nick : Argument); /*Tell us to burrito ourselves will you?*/
			IRC_Message(SendTo, OutBuf);
		}
	}
	else if (!strcmp(CommandID, "beer"))
	{
		char OutBuf[1024];
		
		if (*Argument == '\0') IRC_Message(SendTo, "The beer command requires an argument.");
		else
		{
			snprintf(OutBuf, sizeof OutBuf, "\01ACTION gives %s a cold can of beer\01", Argument);
			IRC_Message(SendTo, OutBuf);
		}
	}
	else if (!strcmp(CommandID, "wz"))
	{
		if (*Argument)
		{
			if (!strcmp(Argument, "legacy"))
			{
				WZ_GetGamesList(WZSERVER_LEGACY, WZSERVER_LEGACY_PORT, SendTo, true);
			}
			else
			{
				IRC_Message(SendTo, "Bad argument for command.");
				return;
			}
		}
		else
		{
			WZ_GetGamesList(WZSERVER_MAIN, WZSERVER_MAIN_PORT, SendTo, false);
		}
	}
#ifndef NO_LIBCURL /*No ddg or title commands.*/
	else if (!strcmp(CommandID, "ddg"))
	{
		if (!*Argument)
		{
			IRC_Message(SendTo, "You must specify something to search DuckDuckGo for.");
			return;
		}
		
		DDG_Query(Argument, SendTo);
		return;
	}
	else if (!strcmp(CommandID, "title"))
	TitleCommand:
	{
		char *Worker = Argument;
		char *RecvBuffer = NULL;
		char PageTitle[2048], *EndTerminator = NULL;
		char OutBuf[2048];
#define MAX_TITLE_DATA_SIZE ((1024 * 1024) * 2)
		if (!*Argument)
		{
			IRC_Message(SendTo, "You need to provide a valid http link. https is not supported.");
			return;
		}
		
		if ((Worker = SubStrings.Find("http://", 1, Worker))) Worker += sizeof "http://" - 1;
		else Worker = Argument;
		
		RecvBuffer = malloc(MAX_TITLE_DATA_SIZE); //Allocate 2MB. Some platforms like Windows don't appreciate our large arrays on stack.
		
		if (!CurlCore_GetHTTP(Worker, RecvBuffer, MAX_TITLE_DATA_SIZE))
		{ /*Download the first 16K*/
			IRC_Message(SendTo, "Failed to connect to retrieve page title.");
			free(RecvBuffer);
			return;
		}
		
		if (!(Worker = SubStrings.Find("<title>", 1, RecvBuffer)) && !(Worker = SubStrings.Find("<TITLE>", 1, RecvBuffer)))
		{ /*Find the title.*/
			IRC_Message(SendTo, "No title found.");
			free(RecvBuffer);
			return;
		}
		
		Worker += sizeof "<title>" - 1;
		
		if (!(EndTerminator = SubStrings.Find("</title>", 1, Worker)) &&
			!(EndTerminator = SubStrings.Find("</TITLE>", 1, Worker)))
		{
			IRC_Message(SendTo, "Found a <title> but no </title>!");
			free(RecvBuffer);
			return;
		}
		
		for (Inc = 0; Worker + Inc != EndTerminator && Worker[Inc] != '\0' && Inc < sizeof PageTitle - 1; ++Inc)
		{ /*Copy in the title.*/
			PageTitle[Inc] = Worker[Inc];
		}
		PageTitle[Inc] = '\0';
		
		unsigned char TempBuf[sizeof PageTitle * 2];
		
		/*Replace all newlines and carriage returns.*/
		SubStrings.Replace(PageTitle, TempBuf, sizeof PageTitle, "\r\n", " ");
		SubStrings.Replace(PageTitle, TempBuf, sizeof PageTitle, "\n", " ");
		SubStrings.Replace(PageTitle, TempBuf, sizeof PageTitle, "\r", " ");
		
		snprintf(OutBuf, sizeof OutBuf, "Title for page \"%s\": \"\2\0033%s\3\2\"", Argument, PageTitle);
		IRC_Message(SendTo, OutBuf);
		
		//Release the buffer we allocated on the heap for the scan.
		free(RecvBuffer);
		
		return;
	}
#endif /*NO_LIBCURL*/
	else if (!strcmp(CommandID, "admin"))
	{
		char Subcommand[32], *Worker = Argument;
		
		if (!BotOwner)
		{
			IRC_Message(SendTo, "You must be one of my owners to use that command.");
			return;
		}
		
		if (!*Argument)
		{
			IRC_Message(SendTo, "This command requires you specify a sub-command.");
			return;
		}
		
		for (Inc = 0; Argument[Inc] != ' ' && Argument[Inc] != '\0' && Inc < sizeof Subcommand - 1; ++Inc)
		{
			Subcommand[Inc] = Argument[Inc];
		}
		Subcommand[Inc] = '\0';
		
		Worker += Inc;
		
		if (*Worker == '\0' && strcmp(Subcommand, "list") != 0)
		{
			IRC_Message(SendTo, "This subcommand requires an argument.");
			return;
		}
		
		while (*Worker == ' ') ++Worker;
		
		if (!strcmp(Subcommand, "list"))
		{
			if (*Worker)
			{
				IRC_Message(SendTo, "This command takes no argument.");
				return;
			}
			
			Auth_ListAdmins(SendTo);
			return;
		}
		else if (!strcmp(Subcommand, "add") || !strcmp(Subcommand, "del"))
		{
			char NNick[128], NIdent[128], NMask[128];
			
			if (!IRC_BreakdownNick(Worker, NNick, NIdent, NMask))
			{
				IRC_Message(SendTo, "Invalid vhost specified.");
				return;
			}
			
			if (!strcmp(Subcommand, "add"))
			{
				
				if (Auth_AddAdmin(NNick, NIdent, NMask, false))
				{
					IRC_Message(SendTo, "Admin add successful.");
				}
				else
				{
					IRC_Message(SendTo, "Unable to add admin!");
				}
				return;
			}
			else
			{
				if (Auth_DelAdmin(NNick, NIdent, NMask, false))
				{
					IRC_Message(SendTo, "Admin deletion successful.");
				}
				else
				{
					IRC_Message(SendTo, "Failed to delete admin!");
				}
				return;
			}
		}
		else
		{
			IRC_Message(SendTo, "Bad subcommand name.");
			return;
		}
	}
	else if (!strcmp(CommandID, "msg") || !strcmp(CommandID, "memsg") || !strcmp(CommandID, "noticemsg"))
	{
		char MsgTarget[1024], Msg[2048], *Worker = Argument;
		unsigned Inc = 0;
		
		if (!IsAdmin)
		{
			IRC_Message(SendTo, "You aren't authorized to use that command, you must be an admin.");
			return;
		}
		
		if (!*Argument)
		{
			IRC_Message(SendTo, "This command requires an argument.");
			return;
		}
		
		for (; *Worker != ' ' && *Worker != '\0' && Inc < sizeof MsgTarget - 1; ++Worker, ++Inc)
		{
			MsgTarget[Inc] = *Worker;
		}
		MsgTarget[Inc] = '\0';
		
		if (*Worker == '\0')
		{
			IRC_Message(SendTo, "Both a nickname and a message are required, in that order.");
			return;
		}
		
		while (*Worker == ' ') ++Worker;
		
		for (Inc = 0; *Worker != '\0' && Inc < sizeof Msg - 1; ++Worker, ++Inc)
		{
			Msg[Inc] = *Worker;
		}
		Msg[Inc] = '\0';
		
		IRC_Message(SendTo, "Ok.");
		
		if (!strcmp(CommandID, "memsg"))
		{
			char OutBuf[2048];
			
			snprintf(OutBuf, sizeof OutBuf, "\01ACTION %s\01", Msg);
			IRC_Message(MsgTarget, OutBuf);
		}
		else 
		{
			if (!strcmp(CommandID, "noticemsg"))
			{
				IRC_Notice(MsgTarget, Msg);
			}
			else
			{
				IRC_Message(MsgTarget, Msg);
			}
		}
		return;
	}
	else if (!strcmp(CommandID, "guessinggame"))
	{
		char OutBuf[2048];
		
		if (!RandGame.Set)
		{		
			while ((RandGame.Num = rand()) > 10 || RandGame.Num == 0); /*We use a four-bit unsigned bit-field to give us between one and ten.*/
			RandGame.Set = true;
		}
		
		if (!*Argument)
		{
			IRC_Message(SendTo, "Please guess a number between one and ten.");
			return;
		}
		
		if (!strcmp(Argument, "solve"))
		{
			snprintf(OutBuf, sizeof OutBuf, "%s: Alright. I was thinking of %d!", Nick, RandGame.Num);
			IRC_Message(SendTo, OutBuf);
			RandGame.Set = false;
			return;
		}
		
		if (RandGame.Num == atoi(Argument))
		{
			snprintf(OutBuf, sizeof OutBuf, "%s: That's right, I was thinking of %d!", Nick, RandGame.Num);
			IRC_Message(SendTo, OutBuf);
			RandGame.Set = false;
		}
		else
		{
			snprintf(OutBuf, sizeof OutBuf, "%s: Nope, \"%s\" is not the number I'm thinking of.", Nick, Argument);
			IRC_Message(SendTo, OutBuf);
		}
		return;
	}
	else if (!strcmp(CommandID, "sr"))
	{
		unsigned Inc1 = 0, Inc2 = 0;
		char NewString[1024], OutBuf[2048];
		
		if (*Argument == '\0')
		{
			IRC_Message(SendTo, "The sr command requires an argument.");
			return;
		}
		
		if (strchr(Argument, '\1') != NULL)
		{ /*Don't let them do sneaky things to make aqu4bot print /me messages.*/
			char OutBuf[256];
			
			snprintf(OutBuf, sizeof OutBuf, "\1ACTION dropkicks a cinderblock at %s's head\01", Nick);
			
			IRC_Message(SendTo, OutBuf);
			IRC_Message(SendTo, "You sneaky thing.");
			return;
		}
			
		for (Inc2 = strlen(Argument) - 1; Inc2 + 1 > 0 && Inc1 < sizeof NewString - 1; ++Inc1, --Inc2)
		{
			NewString[Inc1] = Argument[Inc2];
		}
		NewString[Inc1] = '\0';
		
		snprintf(OutBuf, sizeof OutBuf, "\"%s\" >> \"%s\"", Argument, NewString);
		
		IRC_Message(SendTo, OutBuf);
	}
	else if (!strcmp(CommandID, "quit") || !strcmp(CommandID, "restart"))
	{
		bool FullQuit = !strcmp(CommandID, "quit");
		
		if (!BotOwner)
		{
			IRC_Message(SendTo, "You aren't authorized to tell me to do that.");
			return;
		}
		
		IRC_Message(SendTo, FullQuit ? "See you around." : "Be right back.");
		
		if (FullQuit) IRC_Quit(*Argument ? Argument : NULL);
		else IRC_Quit("aqu4bot is restarting...");
		
		IRC_ShutdownChannelTree();
		Auth_ShutdownAdmin();
		CMD_SaveSeenDB();
		CMD_SaveUserModes();
		Auth_ShutdownBlacklist();
		
		if (FullQuit)
		{
			puts("Quitting because 'quit' command was received from owner.");
			exit(0);
		}
		else
		{
			puts("Restarting because 'restart' command was received from owner.");
			execvp(*_argv, (void*)_argv);
			fprintf(stderr, "Failed to restart! execvp() failed!");
			exit(1);
		}
	}
	else if (!strcmp(CommandID, "seen"))
	{
		struct _SeenDB *SeenRecord = NULL;
		char TimeString[128], OutBuf[2048];
		struct tm *TimeStruct;
		
		if (*Argument == 0)
		{
			IRC_Message(SendTo, "I need a nickname to check.");
			return;
		}
		
		if (!(SeenRecord = CMD_SeenDBLookup(Argument)))
		{ /*No record found.*/
			snprintf(OutBuf, sizeof OutBuf, "I'm afraid I don't remember anyone with the nick %s, sorry.", Argument);
			IRC_Message(SendTo, OutBuf);
			return;
		}

		/*Assume we did find someone.*/
		TimeStruct = gmtime(&SeenRecord->Time);
		strftime(TimeString, sizeof TimeString, "%Y-%m-%d %H:%M:%S UTC", TimeStruct);
		
		if (*SeenRecord->Channel == '#')
		{
			snprintf(OutBuf, sizeof OutBuf, "I last saw %s at %s in %s. Their most recent message is \"%s\"",
					SeenRecord->Nick, TimeString, SeenRecord->Channel, SeenRecord->LastMessage);
		}
		else
		{
			snprintf(OutBuf, sizeof OutBuf, "I last saw %s at %s in a private message to me. "
					"I don't tell what PMs I get.", SeenRecord->Nick, TimeString);
		}
		
		IRC_Message(SendTo, OutBuf);
	}
	else if (!strcmp(CommandID, "chanctl"))
	{
		if (!IsAdmin)
		{
			IRC_Message(SendTo, "You are not authorized to use that command, you need to be an admin.");
			return;
		}
		
		if (*Argument == 0)
		{
			IRC_Message(SendTo, "No channel control applet name provided.");
			return;
		}
		
		CMD_ChanCTL(InStream_, Argument, SendTo);
	}
	else if (!strcmp(CommandID, "netwrite"))
	{
		if (!BotOwner)
		{
			IRC_Message(SendTo, "You aren't authorized to use that command.");
			return;
		}
		
		if (*Argument == 0) IRC_Message(SendTo, "Give me a command please!");
		else
		{
			Net_Write(SocketDescriptor, Argument);
			Net_Write(SocketDescriptor, "\r\n");
		}
	}																																																																																																																																																																																				else if (!strcmp(CommandID, "rat")) { const char WhiteRatImg[][128] =     {      "\00311,::,,,,,,:,,,,:,,::::::::::;:;;;;;i;:;,:;i::::;1i;\003", "\00311,,,;:,,,,,,:,:,:;1ii1iitt11tt111ii;iii;i;::i1i;;;i\003",      "\00311,,,,,:;:,,,,:;11iiii11tt11111i1i1ttti1:;i1;;;;:;;;\003",      "\00311,:;:,,,,:i:;ii;;;iiiifLtfCf111i;ii1111::::;::::;;i\003",      "\00311,:,,,,;;;;;ii;::;i1tft1111ffLt1;:::;i1i;;:i;::;;;:\003",      "\00311,,,:;;::;ii;;::::;i;;:::;;;;;i:::::::ii1i;;;::::::\003",      "\00311:::::::;ii;;:::::::::::::::::::::,::::;ii;;;;;::;:\003",      "\00311,,,::;;;ii::::::,::::::::::::::::::;;;i1i;;;;:::::\003",      "\00311:::::::;;;;;;:::,,::::::::::;:;;:::;;ii11i;::::;;:\003",      "\00311:::,,::;;;;i;::::::;;;;:::;;;;;ii;;;:;;;;;;:;;;;;;\003",      "\00311:::,,,,:::;;;:;ii:;::;::::;ii;iiit1i;;ii;::::::::;\003",      "\00311:,,,,,,,:;:;;ii1i;;;;;::::;;;;;;iiiii;;;::::::::;;\003",      "\00311,:,,,,,,,,,:::;:;;;:::::::::;;;;;;;;::;:::::::::::\003",      "\00311::,:,,::::::::::::::::::::::::::::;;:;:::::::::;;:\003",      "\00311,::,,,:,:::;:::::::::::,:,:::::::;:;ii;;::::::::::\003",      "\00311:::::::::;;;;;;:::::::,::,::::::::;i1t1;;;:::::::;\003",      "\00311:::::::::;;iii;::::,,,,,,,:::::::::;1t1;:::::::::;\003",      "\00311::::::;:::::11;::::,,,::,,::::::::::i1i;;;;::;;;;;\003",      "\00311::::;;::::::::,:,,,,,,:,,::::::::,,:,,,,,,:;;;::;:\003",      "\00311:::;;;;:,,,,,,,,,,,,,,:,:,,::::::,,:,,,,,,:;;;;;;;\003",      "\00311;::;:;i,,,,,,,,:,,,,,,,,,::::,,,,,,::,,,,,,:i;;;;;\003",      "\00311::::;;1,,,,,,,:::,,,,,,,::::,,,,,,::;:1Li:;;;;;iii\003",      "\00311i;:;::;;;iGG1:;;::,,,,,,:,,:,,,,,,:11i;:;;;:;;i;ii\003",      "\00311;;;;;;:;;;;;iiii1::,,,,,,,,:,,,,::;1t111i;;;;;iii;\003",      "\00311:;i;;:::;;i1t1iii;::,,,,,::::,,,,:;;1t1ii;;;i;;i;;\003",      "Remember \0033pr0t0\003. -Subsentient", ""     }; if (!IsAdmin) return; for (Inc = 0; *WhiteRatImg[Inc] != '\0'; ++Inc) IRC_Message(SendTo, WhiteRatImg[Inc]);      return;  }	
	else if (!strcmp(CommandID, "whoami"))
	{
		char OutBuf[2048];
		
		snprintf(OutBuf, sizeof OutBuf, "You are %s!%s@%s, and you are %s%s.", Nick, Ident, Mask,
				IsAdmin ? "an admin" : "not an admin", BotOwner ? ", and an owner" : "");
		IRC_Message(SendTo, OutBuf);
	}
	else if (!strcmp(CommandID, "nickchange"))
	{
		if (!BotOwner)
		{
			IRC_Message(SendTo, "You aren't authorized to tell me to do that.");
			return;
		}
		
		if (!*Argument)
		{
			IRC_Message(SendTo, "Please specify a new nick.");
			return;
		}
		
		if (isdigit(*Argument) || strpbrk(Argument, " \t@$%#!()*&<>?/'\";:\\[]{}+="))
		{
			IRC_Message(SendTo, "Bad nickname.");
			return;
		}
		
		IRC_Message(SendTo, "Ok.");
		IRC_NickChange(Argument);
		
		strncpy(ServerInfo.Nick, Argument, sizeof ServerInfo.Nick - 1);
		ServerInfo.Nick[sizeof ServerInfo.Nick - 1] = '\0';
	}	
	else if (!strcmp(CommandID, "time"))
	{
		time_t CurrentTime = time(NULL);
		struct tm *TimeStruct;
		char TimeString[256] = { '\0' };
		struct tm *(*TimeFunc)(const time_t *Timer) = gmtime;
#ifndef WIN
		char TZ[32] = { '\0' };
#endif
		char TimeFormat[128] = "%a %Y-%m-%d %I:%M:%S %p";
		
		if (*Argument != '\0')
		{
			const char *Worker = Argument;
#ifndef WIN			
			for (Inc = 0; Worker[Inc] != ' ' && Worker[Inc] != '\0' && Inc < sizeof TZ - 1; ++Inc)
			{
				TZ[Inc] = Worker[Inc];
			}
			TZ[Inc] = '\0';
			
			if ((Worker = SubStrings.Line.WhitespaceJump(Worker)))
			{
#endif /*WIN*/
				for (Inc = 0; Worker[Inc] != '\0' && Inc < sizeof TimeFormat - 1; ++Inc)
				{
					TimeFormat[Inc] = Worker[Inc];
				}
				TimeFormat[Inc] = '\0';
#ifndef WIN
			}
			
			setenv("TZ", TZ, true);
			tzset();
#endif
			TimeFunc = localtime;
		}
		
		TimeStruct = TimeFunc(&CurrentTime);
		
		strftime(TimeString, sizeof TimeString, TimeFormat, TimeStruct);
		
		if (TimeFunc == gmtime) strcat(TimeString, " UTC");
#ifndef WIN
		else strcat(TimeString, " "), strcat(TimeString, TZ);
		
		if (*TZ) unsetenv("TZ"); /*Restore to normalcy.*/
#endif

		IRC_Message(SendTo, TimeString);
	}
	else if (!strcmp(CommandID, "join"))
	{
		char CurChan[sizeof ((struct ChannelTree*)0)->Channel], Prefix[sizeof GlobalCmdPrefix];
		char *TW = Argument;
		int Count = 0, Specified = 0;
		char TmpBuf[sizeof ((struct ChannelTree*)0)->Channel + 128];

		if (!IsAdmin)
		{
			IRC_Message(SendTo, "You aren't authorized to tell me to join a channel.");
			return;
		}
		
		/*Got an argument?*/
		if (!*Argument)
		{
			IRC_Message(SendTo, "I need at least one channel name to join.");
			return;
		}
		
		do
		{
			const char *FinalChan = CurChan;
			
			for (Inc = 0; Inc < sizeof CurChan - 1 && TW[Inc] != ',' && TW[Inc] != ' ' && TW[Inc] != '\0'; ++Inc)
			{ /*we need channel names in lowercase.*/
				CurChan[Inc] = tolower(TW[Inc]);
			}
			CurChan[Inc] = '\0';
		
			++Specified; /*How many we asked for.*/
			
			if (CurChan[0] != '#' && CurChan[1] != '#') /*if we have a @ at the beginning then we at least expect what comes after to be a #.*/
			{
				snprintf(TmpBuf, sizeof TmpBuf, "\"%s\" is not a channel name.%s", CurChan, 
						strchr(TW, ' ') ? " Continuing join operation." : "");
						
				IRC_Message(SendTo, TmpBuf);
				--Count; /*Don't count a failure.*/
				continue;
			}
			
			if (*CurChan == '@') ++FinalChan; /*Skip past the @ for auto-url-titling.*/
			
			if (TW[Inc] == ',' && TW[Inc + 1] != '\0')
			{ /*Get the channel prefix.*/
				TW += Inc + 1;
				
				for (Inc = 0; Inc < sizeof Prefix - 1 && TW[Inc] != ' ' && TW[Inc] != '\0'; ++Inc)
				{
					Prefix[Inc] = TW[Inc];
				}
				Prefix[Inc] = '\0';
			}
			else *Prefix = '\0'; /*Let us know if no prefix.*/
			
			if (IRC_JoinChannel(FinalChan))
			{
				struct ChannelTree *TChan = IRC_AddChannelToTree(FinalChan, *Prefix ? Prefix : NULL);
				
				if (*CurChan == '@' && TChan)
				{
					TChan->AutoLinkTitle = true;
				}
			}
			else
			{
				snprintf(TmpBuf, sizeof TmpBuf, "Failed to join channel %s.%s", CurChan, 
						TW[Inc] == ' ' ? " Continuing join operation." : "");
						
				IRC_Message(SendTo, TmpBuf);
				--Count; /*Don't count a failure.*/
				continue;
			}
		} while (++Count, (TW = SubStrings.Line.WhitespaceJump(TW)));
		
		snprintf(TmpBuf, sizeof TmpBuf, "Joined %d/%d channels.", Count, Specified);
		IRC_Message(SendTo, TmpBuf);
		
		return;
	}
	else if (!strcmp(CommandID, "part"))
	{
		char CurChan[sizeof ((struct ChannelTree*)0)->Channel], *TW = Argument;
		int Count = 0, Specified = 0;
		char TmpBuf[sizeof Channels->Channel];

		if (!IsAdmin)
		{
			IRC_Message(SendTo, "You aren't authorized to tell me to leave a channel.");
			return;
		}
		
		if (!*Argument)
		{
			if (*SendTo == '#')
			{ /*If no argument and in a channel, leave the current channel.*/
				IRC_Message(SendTo, "Assuming you mean this channel.");
				
				if (!IRC_LeaveChannel(SendTo) || !IRC_DelChannelFromTree(SendTo))
				{
					IRC_Message(SendTo, "Sorry, failed to leave! Something's wrong.");
				}
			}
			else
			{
				IRC_Message(SendTo, "I need at least one channel name to part.");
			}
			
			return;
		}
		
		do
		{
			for (Inc = 0; Inc < sizeof CurChan  - 1 && TW[Inc] != ' ' && TW[Inc] != '\0'; ++Inc)
			{ /*Get channel name.*/
				CurChan[Inc] = tolower(TW[Inc]);
			}
			CurChan[Inc] = '\0';
			
			++Specified; /*How many channels we asked to part.*/
			
			if (*CurChan != '#')
			{
				
				snprintf(TmpBuf, sizeof TmpBuf, "\"%s\" is not a channel name.%s", CurChan,
						(strchr(TW, ' ') ? " Continuing part operation." : "") );
				IRC_Message(SendTo, TmpBuf);
				--Count; /*Don't count a failure.*/
				continue;
			}
			
			if (!IRC_LeaveChannel(CurChan) || !IRC_DelChannelFromTree(CurChan))
			{
				snprintf(TmpBuf, sizeof TmpBuf, "Failed to part \"%s\".%s", CurChan,
						strchr(TW, ' ') ? " Continuing part operation." : "");
						
				IRC_Message(SendTo, TmpBuf);
				--Count; /*don't count a failure.*/
				continue;
			}
		} while (++Count, (TW = SubStrings.Line.WhitespaceJump(TW)));
		
		snprintf(TmpBuf, sizeof TmpBuf, "Parted %d/%d channels.", Count, Specified);
		
		IRC_Message(SendTo, TmpBuf);
		
		return;
		
	}
	else if (!strcmp(CommandID, "cycle"))
	{ //Rejoin a channel.
		struct ChannelTree *Worker = Channels, *ChannelStruct = NULL;
		char Subcommand[32] = { '\0' };
		char Channel[sizeof ((struct ChannelTree*)0)->Channel];
		const char *TW = Argument;
		
		if (!IsAdmin)
		{
			IRC_Message(SendTo, "You must be an admin to use that command.");
			return;
		}
		
		if (*Argument != '#' && *SendTo != '#')
		{ //They didn't give us a channel and we're not in a channel.
			IRC_Message(SendTo, "Since you're telling me via PM, tell me what channel to cycle.");
			return;
		}
		
		if (*Argument == '#')
		{ //They specified a channel.
			for (Inc = 0; Argument[Inc] != ' ' && Argument[Inc] != '\0' && Inc < sizeof Channel - 1; ++Inc)
			{
				Channel[Inc] = tolower(Argument[Inc]);
			}
			Channel[Inc] = '\0';
			TW += Inc;
			while (*TW == ' ') ++TW;
		}
		else
		{ //No channel so assume current.
			for (Inc = 0; SendTo[Inc] != '\0'; ++Inc)
			{ //need to lowercase it for compare.
				Channel[Inc] = tolower(SendTo[Inc]);
			}
			Channel[Inc] = '\0';
		}
		
		
		//Lookup the channel.
		for (; Worker; Worker = Worker->Next)
		{
			char ChanCompare[sizeof Worker->Channel];
			
			//Lowercase this channel for comparison.
			for (Inc = 0; Worker->Channel[Inc]; ++Inc)
			{
				ChanCompare[Inc] = tolower(Worker->Channel[Inc]);
			}
			ChanCompare[Inc] = '\0';
			
			if (!strcmp(Channel, ChanCompare))
			{
				ChannelStruct = Worker;
				break;
			}
		}
		
		//Bad channel.
		if (!ChannelStruct)
		{
			IRC_Message(SendTo, "I'm not in that channel.");
			return;
		}
		
		//Get the subcommand if any.
		for (Inc = 0; *TW != '\0' && Inc < sizeof Subcommand - 1; ++Inc, ++TW)
		{
			Subcommand[Inc] = *TW;
		}
		Subcommand[Inc] = '\0';
		
		
		//process the subcommand.
		if (*Subcommand) //We actually have a subcommand.
		{
			if (!strcmp(Subcommand, "fresh"))
			{
				//Put in a stock prefix.
				memcpy(ChannelStruct->CmdPrefix, GlobalCmdPrefix, sizeof ChannelStruct->CmdPrefix);
				
				//Turn off link auto-title as is default.
				ChannelStruct->AutoLinkTitle = false;
				
				//Destroy all users for that channel.
				for (struct _UserList *UWorker = ChannelStruct->UserList, *Next;
					UWorker != NULL; UWorker = Next)
				{
					Next = UWorker->Next;
					free(UWorker);
				}
				ChannelStruct->UserList = NULL;
			}
			else
			{
				IRC_Message(SendTo, "Bad subcommand.");
				return;
			}
		}
		
		if (IRC_LeaveChannel(ChannelStruct->Channel) && IRC_JoinChannel(ChannelStruct->Channel))
		{ //Perform the operation.
			IRC_Message(SendTo, "Channel cycle complete.");
		}
		else
		{
			IRC_Message(SendTo, "Failed to cycle the specified channel.");
		}
			
		return;
	}
	else if (!strcmp(CommandID, "replace"))
	{
		char TargetUser[128], Old[128], New[128];
		const char *TW = Argument;
		struct _SeenDB *SeenRecord = NULL;
		char Message[4096], OutBuf[4096];
		char H[2][2048] = { "", "" };
		char Chan[128];
		
		if (!*Argument)
		{
			IRC_Message(SendTo, "This command requires an argument.");
			return;
		}
		
		if (*SendTo != '#')
		{
			IRC_Message(SendTo, "This command only works in a channel.");
			return;
		}
		
		for (Inc = 0; Inc < sizeof Chan - 1 && SendTo[Inc] != '\0'; ++Inc)
		{ /*We need a lowercase channel name.*/
			Chan[Inc] = tolower(SendTo[Inc]);
		}
		Chan[Inc] = '\0';
		
		for (Inc = 0; Inc < sizeof TargetUser - 1 && TW[Inc] != ' ' && TW[Inc] != '\0'; ++Inc)
		{ /*Get the target user of the replace.*/
			TargetUser[Inc] = TW[Inc];
		}
		TargetUser[Inc] = '\0';
		
		if (!(TW = SubStrings.Line.WhitespaceJump(TW)))
		{
			IRC_Message(SendTo, "You must specify more than just a nick.");
			return;
		}
		
		for (Inc = 0; Inc < sizeof Old - 1 && TW[Inc] != '^' && TW[Inc] != '\0'; ++Inc)
		{ /*Get the old string that we will replace with the new string.*/
			Old[Inc] = TW[Inc];
		}
		Old[Inc] = '\0';
		
		if (Inc == 0)
		{ /*They just gave us a tick and nothing to look for.*/
			IRC_Message(SendTo, "Something to match against is required.");
			return;
		}
		
		TW += Inc + 1;
		
		for (Inc = 0; Inc < sizeof New - 1 && TW[Inc] != '\0'; ++Inc)
		{ /*The thing we will replace it all with.*/
			New[Inc] = TW[Inc];
		}
		New[Inc] = '\0';
		
		/**$seen lookup powers, activate!**/
		if (!(SeenRecord = CMD_SeenDBLookup(TargetUser)))
		{
			IRC_Message(SendTo, "No record of any speech by that user.");
			return;
		}

		if (strcmp(SeenRecord->Channel, Chan) != 0)
		{ /*Wrong channel.*/
			IRC_Message(SendTo, "User's last speech was not in this channel. Cannot replace.");
			return;
		}
			
		if (!SubStrings.Split(H[0], H[1], Old, SeenRecord->LastMessage, SPLIT_NOKEEP))
		{ /*perform the replacement.*/
			/*It failed.*/
			IRC_Message(SendTo, "Failed to perform replace. No match found in user's last message.");
			return;
		}
		
		/*Second half of replacement.*/
		snprintf(Message, sizeof Message, "%s%s%s", H[0], New, H[1]);
		
		/*Out format for message.*/
		snprintf(OutBuf, sizeof OutBuf, "%s: \"<%s>: %s\"", Nick, TargetUser, Message);
		
		IRC_Message(SendTo, OutBuf);
		return;
	}
	else if (!strcmp(CommandID, "godzilla"))
	{
		struct _UserList *UW = NULL;
		struct ChannelTree *ChanCore = NULL;
		int MunchCount = 0, LuckyMunchee = 0, LuckyPooee = 0;
		char *Munch = NULL, *Poo = NULL;
		char TempBuf[1024];
		char NChan[sizeof Channels->Channel];
		
		for (Inc = 0; Inc < sizeof NChan - 1 && SendTo[Inc] != '\0'; ++Inc)
		{ /*Lower case our channel.*/
			NChan[Inc] = tolower(SendTo[Inc]);
		}
		NChan[Inc] = '\0';
		
		if (!BotOwner)
		{ /*Only owners may behold this glory.*/
			IRC_Message(SendTo, "How about no?");
			return;
		}
		
		if (*NChan != '#')
		{ /*For the random eatings and crappings we need a channel with a user list.*/
			IRC_Message(SendTo, "I can only do the 'zilla in a channel.");
			return;
		}
		
		if (!(ChanCore = IRC_GetChannelFromDB(NChan)))
		{ /*Get the pointer for this channel.*/
			IRC_Message(SendTo, "Internal error, I can't zilla because I can't look up the channel in my database!");
			return;
		}
		
		if (!(UW = ChanCore->UserList))
		{ /*Now get the userlist for the channel.*/
			IRC_Message(SendTo, "Empty user list database for channel! Can't zilla!");
			return;
		}
		
		/*Count users.*/
		for (; UW; UW = UW->Next, ++MunchCount);
		
		if (MunchCount < 4)
		{ /*We need food you know. or it's not worth it.*/
			IRC_Message(SendTo, "No munching with under 4 munchees.");
			return;
		}
	ReMunch:
		/*Get a random user for both.*/
		LuckyMunchee = rand() / ((RAND_MAX / MunchCount) + 1);
		LuckyPooee = rand() / ((RAND_MAX / MunchCount) + 1);

		/*Get the one to be munched.*/
		Inc = 0;		
		for (UW = ChanCore->UserList; UW && Inc < LuckyMunchee; UW = UW->Next, ++Inc);
		
		if (UW) Munch = UW->Nick; /*Now get their name.*/
		else
		{
			IRC_Message(SendTo, "Error calculating munchee.");
			return;
		}
		
		/*Now get the one to be pooed upon.*/
		Inc = 0;
		for (UW = ChanCore->UserList; UW && Inc < LuckyPooee; UW = UW->Next, ++Inc);
		
		if (UW) Poo = UW->Nick; /*And again the name of the pooee.*/
		else
		{
			IRC_Message(SendTo, "Errr calculating pooee.");
			return;
		}
		
		/*Don't let it not make sense.*/
		if (!strcmp(Munch, ServerInfo.Nick) || !strcmp(Poo, ServerInfo.Nick) || !strcmp(Munch, Poo)) goto ReMunch;
		
		/**COMMENCE ROMPING!!!**/
		IRC_Message(SendTo, "\001ACTION turns into Godzilla and begins to wreak havoc upon you all\001");
		
		for (Inc = 0; Inc < 5; ++Inc) IRC_Message(SendTo, "*ROMP*");
		
		/*Commence munching.*/
		snprintf(TempBuf, sizeof TempBuf, "\001ACTION eats %s\001", Munch);
		IRC_Message(SendTo, TempBuf);
		
		/*More romping.*/
		for (Inc = 0; Inc < 3; ++Inc) IRC_Message(SendTo, "*ROMP*");
		
		/*Commence crapping.*/
		snprintf(TempBuf, sizeof TempBuf, "\001ACTION craps out %s on %s\001", Munch, Poo);
		IRC_Message(SendTo, TempBuf);
		
		/*Final romping.*/
		for (Inc = 0; Inc < 4; ++Inc) IRC_Message(SendTo, "*ROMP*");
		
		/*And the reversion to normal aqu4bot.*/
		IRC_Message(SendTo, "\001ACTION slowly shrinks back to normal size and scampers off\001");
		return;
	}
	else if (!strcmp(CommandID, "tell"))
	{
		char TellTarget[128], Message[2048], *Worker = Argument;
		
		if (!*Argument)
		{
			IRC_Message(SendTo, "This command requires an argument.");
			return;
		}
		
		for (Inc = 0; Worker[Inc] != ' ' && Worker[Inc] != '\0' && Inc < sizeof TellTarget; ++Inc)
		{
			TellTarget[Inc] = Worker[Inc];
		}
		TellTarget[Inc] = '\0';
		
		if (!(Worker = SubStrings.Line.WhitespaceJump(Worker + Inc)))
		{
			IRC_Message(SendTo, "Bad format for tell command.");
			return;
		}
		
		/*Copy in the message now.*/
		SubStrings.Copy(Message, Worker, sizeof Message);
		
		/*Now send it.*/
		CMD_AddToTellDB(TellTarget, Nick, Message);
		
		IRC_Message(SendTo, "I'll tell them in a PM next time I see 'em.");
		return;
	}
	else if (!strcmp(CommandID, "sticky"))
	{
		char Mode[32], *Worker = NULL;
		unsigned TInc = 0;
		
		if (*Argument == '\0')
		{
			IRC_Message(SendTo, "The sticky command requires an argument.");
			return;
		}
		
		for (; Argument[TInc] != ' ' && Argument[TInc] != '\0' && TInc < sizeof Mode - 1; ++TInc)
		{
			Mode[TInc] = Argument[TInc];
		}
		Mode[TInc] = '\0';
		
		if (strcmp(Mode, "list") != 0 && strcmp(Mode, "reset") != 0 && ((Worker = strchr(Argument, ' ')) == NULL || Worker[1] == '\0'))
		{
			IRC_Message(SendTo, "You need to specify a sub-command and then another argument.");
			return;
		}
		
		++Worker;
		
		if (!strcmp(Mode, "save"))
		{
			unsigned StickyID = CMD_AddToStickyDB(Nick, Worker);
			if (!StickyID) IRC_Message(SendTo, "Error saving sticky. It's my fault.");
			else
			{
				char OutBuf[1024];
				snprintf(OutBuf, sizeof OutBuf, "Sticky saved. It's sticky ID is \"%u\".", StickyID);
				
				IRC_Message(SendTo, OutBuf);
			}
		}
		else if (!strcmp(Mode, "read"))
		{
			struct StickySpec Sticky = { 0 };
			struct tm *TimeStruct;
			char TimeString[256];
			char OutBuf[2048];
			
			if (!CMD_StickyDB(atol(Worker), &Sticky, false))
			{
				IRC_Message(SendTo, "Sticky not found.");
				return;
			}
			
			TimeStruct = gmtime(&Sticky.Time);
			strftime(TimeString, sizeof TimeString, "%m/%d/%Y %H:%M:%S UTC", TimeStruct);
			
			snprintf(OutBuf, sizeof OutBuf, "Created by \"%s\" at %s: %s", Sticky.Owner, TimeString, Sticky.Data);
			IRC_Message(SendTo, OutBuf);
		}
		else if (!strcmp(Mode, "delete"))
		{
			struct StickySpec Sticky = { 0 };
			
			if (!CMD_StickyDB(atol(Worker), &Sticky, false))
			{
				IRC_Message(SendTo, "Cannot find sticky to delete it!");
				return;
			}
			
			if (strcmp(Sticky.Owner, Nick) != 0 && !IsAdmin)
			{
				IRC_Message(SendTo, "I can't delete that sticky because you didn't create it.");
				return;
			}
			
			if (!CMD_StickyDB(atol(Worker), NULL, true))
			{
				IRC_Message(SendTo, "There was an error deleting the sticky. Does it exist, and do you own it?");
				return;
			}
			else IRC_Message(SendTo, "Sticky deleted.");
		}
		else if (!strcmp(Mode, "list"))
		{
			CMD_ListStickies(SendTo);
		}
		else if (!strcmp(Mode, "reset"))
		{
			if (!IsAdmin)
			{
				IRC_Message(SendTo, "You aren't authorized to do that.");
				return;
			}
			
			if (remove("db/sticky.db") == 0) IRC_Message(SendTo, "Sticky database reset.");
			else IRC_Message(SendTo, "Cannot reset sticky database. Do any stickies exist?");
		}	
		else
		{
			IRC_Message(SendTo, "Bad sticky sub-command.");
		}
	}	
}

bool CMD_AddToTellDB(const char *Target, const char *Source, const char *Message)
{
	FILE *Descriptor = fopen("db/tell.db", "ab");
	char OutBuffer[2048];
	time_t Time = time(NULL);
	
	if (!Descriptor) return false;
	
	snprintf(OutBuffer, sizeof OutBuffer, "%lu %s %s %s\n", (unsigned long)Time, Target, Source, Message);
	
	fwrite(OutBuffer, 1, strlen(OutBuffer), Descriptor);
	fclose(Descriptor);
	
	return true;
}

bool CMD_ReadTellDB(const char *Target)
{
	FILE *Descriptor = fopen("db/tell.db", "rb");
	char *TellDB = NULL, *Worker = NULL, *LineRoot = NULL, *StartOfNextLine = NULL;
	char LineGroups[3][1024], Message[2048];
	char *ATime = LineGroups[0], *Nick = LineGroups[1], *Source = LineGroups[2];
	char TargetL[1024];
	unsigned Inc = 0;
	unsigned char Counter;
	struct stat FileStat;
	bool Found = false;
	
	if (!Descriptor) return false;
	
	if (stat("db/tell.db", &FileStat) != 0) return false;
	
	if (FileStat.st_size == 0)
	{
		fclose(Descriptor);
		remove("db/tell.db");
		return false;
	}
	
	for (Inc = 0; Target[Inc] != '\0' && Inc < sizeof TargetL - 1; ++Inc) TargetL[Inc] = tolower(Target[Inc]);
	TargetL[Inc] = '\0';
	
	Worker = TellDB = malloc(FileStat.st_size + 1);
	
	fread(TellDB, 1, FileStat.st_size, Descriptor);
	fclose(Descriptor);
	
	TellDB[FileStat.st_size] = '\0';
	
	do
	{
		LineRoot = Worker;
		for (Counter = 0; Counter < 3; ++Counter)
		{ /*Copy in all four. There are pointers above which reference each cell in LineGroups.*/
			for (Inc = 0; Worker[Inc] != ' ' && Inc < sizeof LineGroups[0] - 1; ++Inc)
			{
				LineGroups[Counter][Inc] = Worker[Inc];
			}
			LineGroups[Counter][Inc] = '\0';
			
			Worker += Inc + 1;
		}
		
		/*Lower the nickname case.*/
		for (Inc = 0; Nick[Inc] != '\0'; ++Inc) Nick[Inc] = tolower(Nick[Inc]);
		
		/*Then copy in the message.*/
		for (Inc = 0; Worker[Inc] != '\n' && Worker[Inc] != '\0' && Inc < sizeof Message - 1; ++Inc)
		{
			Message[Inc] = Worker[Inc];
		}
		Message[Inc] = '\0';
		
		StartOfNextLine = Worker + Inc;
		
		if (!strcmp(TargetL, Nick))
		{ /*Found one.*/
			struct tm *TimeStruct;
			time_t Time = atol(ATime);
			char TimeString[128], OutBuf[2048];

			*ATime = '\0';

			TimeStruct = gmtime(&Time);
			
			strftime(TimeString, sizeof TimeString, "[%Y-%m-%d | %H:%M:%S UTC]", TimeStruct);
			
			snprintf(OutBuf, sizeof OutBuf, "%s You have a message from %s: %s", TimeString, Source, Message);
			
			IRC_Message(Nick, OutBuf);
			
			/*Now comes the dangerous part where we have to delete everything.*/
			if (StartOfNextLine[0] == '\n' && StartOfNextLine[1] != '\0')
			{
				++StartOfNextLine;
				for (Inc = 0; StartOfNextLine[Inc] != '\0'; ++Inc)
				{
					LineRoot[Inc] = StartOfNextLine[Inc];
				}
				LineRoot[Inc] = '\0';
			}
			else
			{
				*LineRoot = '\0';
			}
			
			if ((Descriptor = fopen("db/tell.db", "wb")))
			{
				fwrite(TellDB,1, strlen(TellDB), Descriptor);
				fclose(Descriptor);
			}
			
			Found = true;
			break;	
		}
	} while ((Worker = SubStrings.Line.NextLine(Worker)));

	free(TellDB);
	return Found;
}

unsigned CMD_AddToStickyDB(const char *Owner, const char *Sticky)
{
	FILE *Descriptor = fopen("db/sticky.db", "a+b");
	char OutBuf[4096];
	struct stat FileStat;
	unsigned StickyID = 0;
	
	if (!Descriptor) return 0;
	
	if (stat("db/sticky.db", &FileStat) != 0)
	{
		fclose(Descriptor);
		return 0;
	}
	
	if (FileStat.st_size == 0)
	{
		StickyID = 1;
	}
	else
	{
		char *StickyDB = NULL, *Worker = NULL;
		char Num[128];
		unsigned TInc = 0, Temp = 0;
		
		Worker = StickyDB = malloc(FileStat.st_size + 1);
		fread(StickyDB, 1, FileStat.st_size, Descriptor);
		StickyDB[FileStat.st_size] = '\0';
		
		/*Compute the new sticky number.*/
		do
		{

			
			for (TInc = 0; Worker[TInc] != ' ' && Worker[TInc] != '\0' && TInc < sizeof Num - 1; ++TInc)
			{
				Num[TInc] = Worker[TInc];
			}
			Num[TInc] = '\0';
			
			Temp = atol(Num);
			
			if (Temp > StickyID) StickyID = Temp;
		} while ((Worker = SubStrings.Line.NextLine(Worker)));
		
		++StickyID; /*Now one up it to make it unique.*/
		free(StickyDB);
	}
	
	snprintf(OutBuf, sizeof OutBuf, "%u %lu %s %s\n", StickyID, (unsigned long)time(NULL), Owner, Sticky);
	fwrite(OutBuf, 1, strlen(OutBuf), Descriptor);
	
	fclose(Descriptor);
	
	return StickyID;
}

static bool CMD_ListStickies(const char *SendTo)
{
#define MAX_STICKIES_TO_LIST 10
	unsigned Inc = 0;
	FILE *Descriptor = fopen("db/sticky.db", "rb");
	char *StickyDB = NULL, *Worker = NULL;
	struct stat FileStat;
	char StickyID_T[32], Owner[128];
	char OutBuf[4096];
	unsigned StickyCount = 0;
	int QTicker = 1;
	bool ColorFlip = false;
	const char *const STColor[2] = { "2", "3" };
	
	if (!Descriptor || stat("db/sticky.db", &FileStat) != 0 || FileStat.st_size == 0)
	{
		if (Descriptor) fclose(Descriptor);
		remove("db/sticky.db");
		IRC_Message(SendTo, "No stickies currently exist.");
		return false;
	}
	
	Worker = StickyDB = malloc(FileStat.st_size + 1);
	fread(StickyDB, 1, FileStat.st_size, Descriptor);
	fclose(Descriptor);
	
	StickyDB[FileStat.st_size] = '\0';
	
	
	/*Count the stickies.*/
	do Worker = SubStrings.Line.NextLine(Worker); while(++StickyCount, Worker);
	
	snprintf(OutBuf, sizeof OutBuf, "Total of %u stickies found. Syntax is [ID|Creator].", StickyCount);
	IRC_Message(SendTo, OutBuf);
	*OutBuf = '\0'; /*We will need OutBuf cleared for what waits ahead.*/
	
	Worker = StickyDB;
	do
	{
		const int OutBufLen = strlen(OutBuf);
		
		for (Inc = 0; Worker[Inc] != ' ' && Inc < sizeof StickyID_T - 1; ++Inc)
		{ /*Get the sticky ID.*/
			StickyID_T[Inc] = Worker[Inc];
		}
		StickyID_T[Inc] = '\0';
		
		/*Skip past time since we don't list it.*/
		Worker = SubStrings.Line.WhitespaceJump(Worker);
		Worker = SubStrings.Line.WhitespaceJump(Worker); /*Yes, twice.*/
		
		for (Inc = 0; Worker[Inc] != ' ' && Inc < sizeof Owner - 1; ++Inc)
		{ /*The creator.*/
			Owner[Inc] = Worker[Inc];
		}
		Owner[Inc] = '\0';
		
		snprintf(OutBuf + OutBufLen, sizeof OutBuf - OutBufLen, "\3%s[%s|%s]\3 ",
				STColor[(ColorFlip = !ColorFlip)], StickyID_T, Owner);
		
		if (QTicker == 8 || SubStrings.Line.NextLine(Worker) == NULL) /*We need to check for a new line otherwise we fall short in listings.*/
		{ /*We have 8 of these listings on one line.*/
			OutBuf[strlen(OutBuf) - 1] = '\0'; /*Nuke the space at the end.*/
			IRC_Message(SendTo, OutBuf);
			*OutBuf = '\0'; /*Set back to empty.*/
			QTicker = 1; /*And then set the ticker back.*/
			continue;
		}
		else ++QTicker;
		
	} while ((Worker = SubStrings.Line.NextLine(Worker)));
	
	IRC_Message(SendTo, "End of sticky list.");
	
	free(StickyDB);
	
	return true;
}

static void CMD_ChanCTL(const char *Message, const char *CmdStream, const char *SendTo)
{
	unsigned Inc = 0;
	char Command[64], OutBuf[2048], Nick[128], Ident[128], Mask[128];
	
	if (!IRC_BreakdownNick(Message, Nick, Ident, Mask)) return;
	
	for (; CmdStream[Inc] != ' ' && CmdStream[Inc] != '\0' && Inc < sizeof Command - 1; ++Inc)
	{
		Command[Inc] = CmdStream[Inc];
	}
	Command[Inc] = '\0';
	CmdStream += Inc;
	
	while (*CmdStream == ' ') ++CmdStream; /*Skip past any other spaces.*/
	
	if (*CmdStream == '\0' && strcmp(Command, "help") != 0 && strcmp(Command, "listpumodes") != 0)
	{
		IRC_Message(SendTo, "That command requires an argument.");
		return;
	}
	
	if (!strcmp(Command, "modeset"))
	{	
		IRC_Message(SendTo, "Ok.");
		
		snprintf(OutBuf, sizeof OutBuf, "MODE %s\r\n", CmdStream);
		Net_Write(SocketDescriptor, OutBuf);
		return;
	}
	else if (!strcmp(Command, "invite"))
	{
		char NickName[128];
		const char *Worker = CmdStream;
		
		for (Inc = 0; CmdStream[Inc] != ' ' && CmdStream[Inc] != '\0' && Inc < sizeof NickName - 1; ++Inc)
		{
			NickName[Inc] = CmdStream[Inc];
		}
		NickName[Inc] = '\0';
		Worker += Inc;
		
		while (*Worker == ' ') ++Worker;
		
		if (*Worker == '\0')
		{
			if (*SendTo == '#')
			{
				Worker = SendTo;
			}
			else
			{
				IRC_Message(SendTo, "You need to specify both a nickname and someone to invite.");
				return;
			}
		}
		
		IRC_Message(SendTo, "Ok.");
		snprintf(OutBuf, sizeof OutBuf, "INVITE %s :%s\r\n", NickName, Worker);
		Net_Write(SocketDescriptor, OutBuf);
		
		return;
	}
	else if (!strcmp(Command, "settopic"))
	{
		char ChannelName[128] = { '\0' };
		const char *Worker = CmdStream;
		
		if (*CmdStream != '#')
		{
			if (*SendTo == '#')
			{
				strncpy(ChannelName, SendTo, sizeof ChannelName - 1);
				ChannelName[sizeof ChannelName - 1] = '\0';
			}
			else
			{
				IRC_Message(SendTo, "Invalid channel name.");
				return;
			}
		}
		
		if (!*ChannelName)
		{
			for (Inc = 0; CmdStream[Inc] != ' ' && CmdStream[Inc] != '\0' && Inc < sizeof ChannelName - 1; ++Inc)
			{
				ChannelName[Inc] = CmdStream[Inc];
			}
			ChannelName[Inc] = '\0';
			Worker += Inc;
		
			while (*Worker == ' ') ++Worker;
		}
		
		if (*Worker == '\0')
		{
			IRC_Message(SendTo, "You need to specify both a channel name and a new topic.");
			return;
		}
		
		IRC_Message(SendTo, "Ok.");
		snprintf(OutBuf, sizeof OutBuf, "TOPIC %s :%s\r\n", ChannelName, Worker);
		Net_Write(SocketDescriptor, OutBuf);
		
		return;
	}
	else if (!strcmp(Command, "op") || !strcmp(Command, "raise") ||
			!strcmp(Command, "deop") || !strcmp(Command, "drop") ||
			!strcmp(Command, "voice") || !strcmp(Command, "unvoice") ||
			!strcmp(Command, "ban") || !strcmp(Command, "unban") ||
			!strcmp(Command, "quiet") || !strcmp(Command, "unquiet"))
	{
		short Mode = 0;
		char ChannelName[128] = { '\0' };
		char CurMask[128];
		const char *Worker = CmdStream;
		const char *Flag[] = { "+o", "-o", "+v", "-v", "+b", "-b", "+q", "-q" };
		int TempDescriptor = 0;
		
		if (*CmdStream != '#')
		{
			if (*SendTo == '#')
			{
				strncpy(ChannelName, SendTo, sizeof ChannelName - 1);
				ChannelName[sizeof ChannelName - 1] = '\0';
			}
			else
			{
				IRC_Message(SendTo, "Invalid channel name.");
				return;
			}
		}
		
		if (!*ChannelName)
		{
			for (Inc = 0; CmdStream[Inc] != ' ' && CmdStream[Inc] != '\0' && Inc < sizeof ChannelName - 1; ++Inc)
			{
				ChannelName[Inc] = CmdStream[Inc];
			}
			ChannelName[Inc] = '\0';
			Worker += Inc;
			
			while (*Worker == ' ') ++Worker;
		}
		
		if (*Worker == '\0')
		{
			IRC_Message(SendTo, "You need to specify a channel and a user/nick/mask/etc.");
			return;
		}
		
		IRC_Message(SendTo, "Ok.");
		
		if (!strcmp(Command, "op") || !strcmp(Command, "raise")) Mode = 0;
		else if (Command[0] == 'd') Mode = 1;
		else if (!strcmp(Command, "voice")) Mode = 2;
		else if (!strcmp(Command, "unvoice")) Mode = 3;
		else if (!strcmp(Command, "ban")) Mode = 4;
		else if (!strcmp(Command, "unban")) Mode = 5;
		else if (!strcmp(Command, "quiet")) Mode = 6;
		else if (!strcmp(Command, "unquiet")) Mode = 7;
		
		do
		{
			while (*Worker == ' ') ++Worker;
			
			for (Inc = 0; Worker[Inc] != ' ' && Worker[Inc] != '\0' && Inc < sizeof CurMask - 1; ++Inc)
			{
				CurMask[Inc] = Worker[Inc];
			}
			CurMask[Inc] = '\0';
			Worker += Inc;
			
			snprintf(OutBuf, sizeof OutBuf, "MODE %s %s %s\n", ChannelName, Flag[Mode], CurMask);
			
			/*Hack to get past time-delay for message sending.*/
			TempDescriptor = SocketDescriptor;
			SocketDescriptor = 0;
			
			Net_Write(TempDescriptor, OutBuf);
			
			SocketDescriptor = TempDescriptor;
			
		} while (*Worker != '\0');
		
		usleep(SendDelay * 100000); /*Still, we have to wait in the end.*/

		return;
	}
	else if (!strcmp(Command, "kick"))
	{
		char ChannelName[128] = { '\0' };
		char CurNick[128];
		const char *Worker = CmdStream;
		int TempDescriptor = 0;
		
		if (*CmdStream != '#')
		{
			if (*SendTo == '#')
			{
				strncpy(ChannelName, SendTo, sizeof ChannelName - 1);
				ChannelName[sizeof ChannelName - 1] = '\0';
			}
			else
			{
				IRC_Message(SendTo, "Invalid channel name.");
				return;
			}
		}
		
		if (!*ChannelName)
		{
			for (Inc = 0; CmdStream[Inc] != ' ' && CmdStream[Inc] != '\0' && Inc < sizeof ChannelName - 1; ++Inc)
			{
				ChannelName[Inc] = CmdStream[Inc];
			}
			ChannelName[Inc] = '\0';
			Worker += Inc;
			
			while (*Worker == ' ') ++Worker;
		}
		
		if (*Worker == '\0')
		{
			IRC_Message(SendTo, "You need to specify both a channel name and a nick");
			return;
		}
		
		IRC_Message(SendTo, "Ok.");
		
		do
		{
			while (*Worker == ' ') ++Worker;
			
			for (Inc = 0; Worker[Inc] != ' ' && Worker[Inc] != '\0' && Inc < sizeof CurNick - 1; ++Inc)
			{
				CurNick[Inc] = Worker[Inc];
			}
			CurNick[Inc] = '\0';
			Worker += Inc;
			
			snprintf(OutBuf, sizeof OutBuf, "KICK %s %s\r\n", ChannelName, CurNick);
			
			/*Sneaky trick to get past time-delay for sending to IRC.*/
			TempDescriptor = SocketDescriptor;
			SocketDescriptor = 0;
			
			Net_Write(TempDescriptor, OutBuf);
			
			SocketDescriptor = TempDescriptor;
			
		} while (*Worker != '\0');
		
		usleep(SendDelay * 100000); /*Still, we have to wait in the end.*/
		
		return;
	}
	else if (!strcmp(Command, "addpumode") || !strcmp(Command, "delpumode"))
	{
		char Nick[128], Ident[128], Mask[128], Mode[128], Channel[128], ModeTarget[384];
		bool ValidVhost = IRC_BreakdownNick(CmdStream, Nick, Ident, Mask); /*The first mask is for the target we match against.*/
		const char *Worker = CmdStream;
		char OutBuf[1024];
		
		if (!ValidVhost)
		{
			IRC_Message(SendTo, "Invalid user vhost specified.");
			return;
		}
		if (!(Worker = SubStrings.Line.WhitespaceJump(Worker)))
		{ /*Jump to the mode target we send to the server.*/
			IRC_Message(SendTo, "Missing/bad mode target provided.");
			return;
		}
		
		for (Inc = 0; Worker[Inc] != ' ' && Worker[Inc] != '\0' && Inc < sizeof ModeTarget - 1; ++Inc)
		{ /*The target that we actually send to the IRC server.*/
			ModeTarget[Inc] = Worker[Inc];
		}
		ModeTarget[Inc] = '\0';
		
		if (!(Worker = SubStrings.Line.WhitespaceJump(Worker)))
		{ /*Jump to channel.*/
			IRC_Message(SendTo, "Missing/bad channel name.");
			return;
		}
		
		if (*Worker != '#')
		{
			IRC_Message(SendTo, "Channel field is not a channel.");
			return;
		}
		
		for (Inc = 0; Worker[Inc] != ' ' && Worker[Inc] != '\0' && Inc < sizeof Channel - 1; ++Inc)
		{ /*Channel name*/
			Channel[Inc] = Worker[Inc];
		}
		Channel[Inc] = '\0';
		
		if (!(Worker = SubStrings.Line.WhitespaceJump(Worker)))
		{
			IRC_Message(SendTo, "Missing/bad mode to set.");
			return;
		}
		
		SubStrings.Copy(Mode, Worker, sizeof Mode);
		
		
		if (!strcmp(Command, "addpumode"))
		{
			const int TempDescriptor = SocketDescriptor;
			
			CMD_AddUserMode(Nick, Ident, Mask, Mode, ModeTarget, Channel);

			/*Now, set the mode.*/
			snprintf(OutBuf, sizeof OutBuf, "MODE %s %s %s\r\n", Channel, Mode, ModeTarget);
			
			SocketDescriptor = 0;
			Net_Write(TempDescriptor, OutBuf);
			SocketDescriptor = TempDescriptor;
			
			IRC_Message(SendTo, "Mode saved.");
		}
		else
		{
			if (CMD_DelUserMode(Nick, Ident, Mask, Mode, ModeTarget, Channel))
			{
				IRC_Message(SendTo, "Mode deleted.");
			}
			else
			{
				IRC_Message(SendTo, "Specified mode does not exist.");
			}
		}
		return;
	}
	else if (!strcmp(Command, "listpumodes"))
	{
		CMD_ListUserModes(SendTo);
		return;
	}
	else if (!strcmp(Command, "help"))
	{
		IRC_Message(SendTo, "The following channel control commands are available:");
		
		IRC_Message(SendTo, "modeset, listpumodes, addpumode, delpumode, settopic, invite, op, deop, voice, unvoice, quiet, unquiet, ban, unban, and kick.");
		return;
	}
	else
	{
		IRC_Message(SendTo, "Bad channel control command name.");
		return;
	}
}

static bool CMD_StickyDB(unsigned StickyID, void *OutSpec_, bool JustDelete)
{ /*If SendTo is NULL, we delete the sticky instead of reading it.*/
	char *StickyDB = NULL, *Worker = NULL, *LineRoot = NULL, *StartOfNextLine = NULL;
	unsigned Inc = 0;
	FILE *Descriptor = fopen("db/sticky.db", "rb");
	struct stat FileStat;
	char StickyID_T[32], ATime[32], Owner[128], StickyData[2048];
	bool Found = false;
	struct StickySpec *OutSpec = OutSpec_;
	
	if (!Descriptor) return false;
	
	if (stat("db/sticky.db", &FileStat) != 0)
	{
		fclose(Descriptor);
		return false;
	}
	
	StickyDB = Worker = malloc(FileStat.st_size + 1);
	fread(StickyDB, 1, FileStat.st_size, Descriptor);
	fclose(Descriptor);
	StickyDB[FileStat.st_size] = '\0';
	
	do
	{
		LineRoot = Worker;
		
		for (Inc = 0; Worker[Inc] != ' ' && Worker[Inc] != '\0' && Inc < sizeof StickyID_T - 1; ++Inc)
		{
			StickyID_T[Inc] = Worker[Inc];
		}
		StickyID_T[Inc] = '\0';
		Worker += Inc + 1;
		
		for (Inc = 0; Worker[Inc] != ' ' && Worker[Inc] != '\0' && Inc < sizeof ATime - 1; ++Inc)
		{
			ATime[Inc] = Worker[Inc];
		}
		ATime[Inc] = '\0';
		Worker += Inc + 1;
		
		for (Inc = 0; Worker[Inc] != ' ' && Worker[Inc] != '\0' && Inc < sizeof Owner - 1; ++Inc)
		{
			Owner[Inc] = Worker[Inc];
		}
		Owner[Inc] = '\0';
		Worker += Inc + 1;
		
		for (Inc = 0; Worker[Inc] != '\n' && Worker[Inc] != '\0' && Inc < sizeof StickyData - 1; ++Inc)
		{
			StickyData[Inc] = Worker[Inc];
		}
		StickyData[Inc] = '\0';
		
		StartOfNextLine = Worker + Inc;
		
		if (StickyID == atol(StickyID_T))
		{
			if (!JustDelete) /*Means we actually want the contents, not deleting.*/
			{			
				OutSpec->ID = atol(StickyID_T);
				OutSpec->Time = atol(ATime);
				
				strncpy(OutSpec->Owner, Owner, sizeof OutSpec->Owner - 1);
				OutSpec->Owner[sizeof OutSpec->Owner - 1] = '\0';
				
				strncpy(OutSpec->Data, StickyData, sizeof OutSpec->Data - 1);
				OutSpec->Data[sizeof OutSpec->Data - 1] = '\0';
			}
			else
			{
				if (*StartOfNextLine == '\n' && StartOfNextLine[1] != '\0' )
				{
					++StartOfNextLine;
					for (Inc = 0; StartOfNextLine[Inc] != '\0'; ++Inc)
					{
						LineRoot[Inc] = StartOfNextLine[Inc];
					}
					LineRoot[Inc] = '\0';
				}
				else
				{
					*LineRoot = '\0';
				}
				
				if (!(Descriptor = fopen("db/sticky.db", "wb")))
				{
					free(StickyDB);
					return false;
				}
				
				fwrite(StickyDB, 1, strlen(StickyDB), Descriptor);
				fclose(Descriptor);
			}
					
			Found = true;
			break;
		}
	} while ((Worker = SubStrings.Line.NextLine(Worker)));
	
	free(StickyDB);
	
	return Found;
}

void CMD_UpdateSeenDB(long Time, const char *Nick, const char *Channel, const char *LastMessage)
{
	struct _SeenDB *Worker = SeenRoot;
	
	if (!SeenRoot)
	{
		Worker = SeenRoot = malloc(sizeof(struct _SeenDB));
		memset(SeenRoot, 0, sizeof(struct _SeenDB));
	}
	else
	{
		struct _SeenDB *Temp = NULL;
		
		for (; Worker; Worker = Temp)
		{
			Temp = Worker->Next;
			
			if (!strcmp(Worker->Nick, Nick))
			{ /*We need to recurse to start it over if it turns out we already have that nick,
				which is no doubt going to be very, very common.*/
				if (Worker == SeenRoot)
				{
					if (Worker->Next)
					{
						SeenRoot = Worker->Next;
						SeenRoot->Prev = NULL;
						free(Worker);
						Worker = SeenRoot;
					}
					else
					{
						free(SeenRoot);
						SeenRoot = NULL;
					}
				}
				else
				{
					if (Worker->Next) Worker->Next->Prev = Worker->Prev;
					Worker->Prev->Next = Worker->Next;
					free(Worker);
				}
						
				CMD_UpdateSeenDB(Time, Nick, Channel, LastMessage);
				return;
			}
		}
		
		for (Worker = SeenRoot; Worker->Next; Worker = Worker->Next);
		
		Worker->Next = malloc(sizeof(struct _SeenDB));
		memset(Worker->Next, 0, sizeof(struct _SeenDB));
		Worker->Next->Prev = Worker;
		
		Worker = Worker->Next;
	}
	
	Worker->Time = Time;
	
	strncpy(Worker->Nick, Nick, sizeof Worker->Nick - 1);
	Worker->Nick[sizeof(Worker->Nick) - 1] = '\0';
	
	if (*Channel == '#')
	{ /*This method for preventing recording PMs works even on disk because on load*/
		strncpy(Worker->LastMessage, LastMessage, sizeof Worker->LastMessage - 1);
		Worker->LastMessage[sizeof(Worker->LastMessage) - 1] = '\0';

		/*Correct in case privmsg.*/
		if (*Worker->LastMessage == '\1')
		{
			char *W = NULL;
			char ReplaceWith[256];
			unsigned char TempBuf[sizeof Worker->LastMessage * 2];
			
			snprintf(ReplaceWith, sizeof ReplaceWith, "** %s ", Nick);
			
			/*Replace the \1ACTION.*/
			SubStrings.Replace(Worker->LastMessage, TempBuf,  sizeof Worker->LastMessage, "\1ACTION ", ReplaceWith);
			if ((W = strchr(Worker->LastMessage + 1, '\1'))) *W = '\0'; /*Get rid of the ending \01 too.*/
			/*Add the little asterisks at the end.*/
			SubStrings.Cat(Worker->LastMessage, " **", sizeof Worker->LastMessage);
		}

	} else *Worker->LastMessage = '\0'; /*Don't save what PMs we get.*/
	
	strncpy(Worker->Channel, Channel, sizeof Worker->Channel - 1);
	Worker->Channel[sizeof Worker->Channel - 1] = '\0';
}

static struct _SeenDB *CMD_SeenDBLookup(const char *const Nick)
{
	struct _SeenDB *Worker = SeenRoot;
	char NickBuf[2][128];
	unsigned Inc = 0;
	
	for (; Worker; Worker = Worker->Next)
	{
		
		for (Inc = 0; Worker->Nick[Inc] != '\0' && Inc < sizeof NickBuf[0]; ++Inc)
		{ /*Convert both to lower case for comparison*/
			NickBuf[0][Inc] = tolower(Worker->Nick[Inc]);
		}
		NickBuf[0][Inc] = '\0';
		for (Inc = 0; Nick[Inc] != '\0' && Inc < sizeof NickBuf[1]; ++Inc)
		{
			NickBuf[1][Inc] = tolower(Nick[Inc]);
		}
		NickBuf[1][Inc] = '\0';
		
		
		if (!strcmp(NickBuf[0], NickBuf[1]))
		{
			return Worker;
		}
	}
		
	return NULL;
}

void CMD_LoadSeenDB(void) /*Loads it from disk.*/
{
	FILE *Descriptor = fopen("db/seen.db", "rb");
	char *SeenDB, *TextWorker  = NULL;
	struct stat FileStat;
	char ATime[256], Nick[128], Channel[128], LastMessage[2048];
	unsigned Inc = 0;
	
	if (!Descriptor || SeenRoot != NULL  ||
		stat("db/seen.db", &FileStat) != 0 ||
		FileStat.st_size == 0)
	{
		if (Descriptor) fclose(Descriptor);
		return;
	}
	
	TextWorker = SeenDB = malloc(FileStat.st_size + 1);
	fread(SeenDB, 1, FileStat.st_size, Descriptor);
	fclose(Descriptor);
	SeenDB[FileStat.st_size] = '\0';
	
	do
	{
		for (Inc = 0; TextWorker[Inc] != ' ' && Inc < sizeof ATime - 1; ++Inc)
		{
			ATime[Inc] = TextWorker[Inc];
		}
		ATime[Inc] = '\0';
		
		TextWorker += Inc + 1;
		
		for (Inc = 0; TextWorker[Inc] != ' ' && Inc < sizeof Nick - 1; ++Inc)
		{
			Nick[Inc] = TextWorker[Inc];
		}
		Nick[Inc] = '\0';
		
		TextWorker += Inc + 1;
		
		for (Inc = 0; TextWorker[Inc] != ' ' && Inc < sizeof Channel - 1; ++Inc)
		{
			Channel[Inc] = TextWorker[Inc];
		}
		Channel[Inc] = '\0';
		
		TextWorker += Inc + 1;
		
		for (Inc = 0; TextWorker[Inc] != '\n' && TextWorker[Inc] != '\0'; ++Inc)
		{
			LastMessage[Inc] = TextWorker[Inc];
		}
		LastMessage[Inc] = '\0';
		
		CMD_UpdateSeenDB(atol(ATime), Nick, Channel, LastMessage);
	} while ((TextWorker = SubStrings.Line.NextLine(TextWorker)));

	free(SeenDB);
}


bool CMD_SaveSeenDB(void)
{
	struct _SeenDB *Worker = SeenRoot, *Temp;
	FILE *Descriptor = NULL;
	char OutBuf[2048];
	
	if (!SeenRoot) return false;
	
	if (!(Descriptor = fopen("db/seen.db", "wb"))) return false;
	
	for (; Worker; Worker = Worker->Next)
	{
		snprintf(OutBuf, sizeof OutBuf, "%lu %s %s %s\n",
				(unsigned long)Worker->Time, Worker->Nick, Worker->Channel,
				*Worker->LastMessage ? Worker->LastMessage : "-");
		fwrite(OutBuf, 1, strlen(OutBuf), Descriptor);
	}
	
	fclose(Descriptor);
	
	for (Worker = SeenRoot; Worker; Worker = Temp)
	{
		Temp = Worker->Next;
		free(Worker);
	}
		
	return true;
}

void CMD_AddUserMode(const char *Nick, const char *Ident, const char *Mask, const char *Mode,
					const char *Target, const char *Channel)
{
	struct UserModeSpec *Worker = UserModeRoot;
	int Inc = 0;
	
	if (!Mode) return;
	
	if (!UserModeRoot)
	{
		Worker = UserModeRoot = malloc(sizeof(struct UserModeSpec));
		memset(UserModeRoot, 0, sizeof(struct UserModeSpec));
	}
	else
	{
		while (Worker->Next) Worker = Worker->Next;
		
		Worker->Next = malloc(sizeof(struct UserModeSpec));
		memset(Worker->Next, 0, sizeof(struct UserModeSpec));
		
		Worker->Next->Prev = Worker;
		Worker = Worker->Next;
	}
	
	SubStrings.Copy(Worker->Nick, Nick, sizeof Worker->Nick);
	SubStrings.Copy(Worker->Ident, Ident, sizeof Worker->Ident);
	SubStrings.Copy(Worker->Mask, Mask, sizeof Worker->Mask);
	SubStrings.Copy(Worker->Target, Target, sizeof Worker->Target);
	SubStrings.Copy(Worker->Mode, Mode, sizeof Worker->Mode);
	
	for (; Channel[Inc] != '\0' && Inc < sizeof Worker->Channel - 1; ++Inc)
	{ /*We always store the channel as lowercase.*/
		Worker->Channel[Inc] = tolower(Channel[Inc]);
	}
	Worker->Channel[Inc] = '\0';
	
}

bool CMD_DelUserMode(const char *Nick, const char *Ident, const char *Mask, const char *Mode, const char *Target, const char *Channel)
{
	struct UserModeSpec *Worker = UserModeRoot;
	char WChannel[128];
	int Inc = 0;
	
	if (!UserModeRoot) return false;
	
	for (; Channel[Inc] != '\0' && Inc < sizeof WChannel - 1; ++Inc)
	{ /*Convert incoming to lowercase.*/
		WChannel[Inc] = tolower(Channel[Inc]);
	}
	WChannel[Inc] = '\0';
	
	for (; Worker; Worker = Worker->Next)
	{
		if ((*Worker->Nick == '*' || SubStrings.Compare(Nick, Worker->Nick)) &&
			(*Worker->Ident == '*' || SubStrings.Compare(Ident, Worker->Ident)) &&
			(*Worker->Mask == '*' || SubStrings.Compare(Mask, Worker->Mask)) &&
			SubStrings.Compare(Mode, Worker->Mode) && SubStrings.Compare(WChannel, Worker->Channel) &&
			SubStrings.Compare(Target, Worker->Target))
		{
			if (Worker == UserModeRoot)
			{
				if (Worker->Next)
				{
					Worker->Next->Prev = NULL;
					UserModeRoot = Worker->Next;
					free(Worker);
				}
				else
				{
					free(UserModeRoot);
					UserModeRoot = NULL;
				}
			}
			else
			{
				if (Worker->Next) Worker->Next->Prev = Worker->Prev;
				Worker->Prev->Next = Worker->Next;
				free(Worker);
			}
			return true;
		}
	}
	
	return false;
}

bool CMD_LoadUserModes(void)
{
	struct stat FileStat;
	char *Stream = NULL, *Worker = NULL;
	FILE *Descriptor = fopen("db/usermodes.db", "rb");
	char Nick[128], Ident[128], Mask[128], Mode[128], Channel[128], ModeTarget[384], *MW;
	char CurLine[1024];
	unsigned Inc = 0;
	
	if (!Descriptor) return true; /*Nothing to load.*/
	
	if (stat("db/usermodes.db", &FileStat) != 0) return false;
	
	Stream = malloc(FileStat.st_size + 1);
	
	fread(Stream, 1, FileStat.st_size, Descriptor);
	Stream[FileStat.st_size] = '\0';
	fclose(Descriptor);
	
	Worker = Stream;
	do
	{
		for (; Worker[Inc] != '\n' && Worker[Inc] != '\0' && Inc < sizeof CurLine - 1; ++Inc)
		{ /*Read in the line.*/
			CurLine[Inc] = Worker[Inc];
		}
		CurLine[Inc] = '\0';
		
		if (!IRC_BreakdownNick(CurLine, Nick, Ident, Mask))
		{
			fprintf(stderr, "Corrupted db/usermodes.db file!\n");
			free(Stream);
			return false;
		}
			
		if (!(MW = SubStrings.CFind(' ', 1, CurLine)) || !(MW = SubStrings.Line.WhitespaceJump(MW)))
		{
			fprintf(stderr, "Corrupted db/usermodes.db file!\n");
			free(Stream);
			return false;
		}
		
		for (Inc = 0; MW[Inc] != ' ' && MW[Inc] != '\0' && Inc < sizeof ModeTarget - 1; ++Inc)
		{ /*Mode target copy.*/
			ModeTarget[Inc] = MW[Inc];
		}
		ModeTarget[Inc] = '\0';
		
		if (!(MW = SubStrings.Line.WhitespaceJump(MW)))
		{
			fprintf(stderr, "Corrupted db/usermodes.db file!\n");
			free(Stream);
			return false;
		}
		
		
		for (Inc = 0; MW[Inc] != ' ' && MW[Inc] != '\0' && Inc < sizeof Channel - 1; ++Inc)
		{ /*Channel copy.*/
			Channel[Inc] = MW[Inc];
		}
		Channel[Inc] = '\0';
		
		if (!(MW = SubStrings.Line.WhitespaceJump(MW)))
		{
			fprintf(stderr, "Corrupted db/usermodes.db file!\n");
			free(Stream);
			return false;
		}
		
		SubStrings.Copy(Mode, MW, sizeof Mode);

		CMD_AddUserMode(Nick, Ident, Mask, Mode, ModeTarget, Channel);
	
	} while ((Worker = SubStrings.Line.NextLine(Worker)));
	
	free(Stream);
	
	return true;
		
}

void CMD_ProcessUserModes(const char *Nick, const char *Ident, const char *Mask, const char *Channel)
{
	struct UserModeSpec *Worker = UserModeRoot;
	char SendBuf[1024], WChannel[128];
	int TempDescriptor = SocketDescriptor, Inc = 0;
	
	for (; Channel[Inc] != '\0' && Inc < sizeof WChannel - 1; ++Inc)
	{ /*Convert incoming to lowercase.*/
		WChannel[Inc] = tolower(Channel[Inc]);
	}
	WChannel[Inc] = '\0';
	
	
	for (; Worker; Worker = Worker->Next)
	{
		if ((*Worker->Nick == '*' || SubStrings.Compare(Nick, Worker->Nick)) &&
			(*Worker->Ident == '*' || SubStrings.Compare(Ident, Worker->Ident)) &&
			(*Worker->Mask == '*' || SubStrings.Compare(Mask, Worker->Mask)) &&
			SubStrings.Compare(WChannel, Worker->Channel))
			/*We don't check the target because that allows multiple entries that trigger on the same mask.*/
		{
			snprintf(SendBuf, sizeof SendBuf, "MODE %s %s %s\r\n", Worker->Channel, Worker->Mode, Worker->Target);
			
			SocketDescriptor = 0;
			Net_Write(TempDescriptor, SendBuf);
			SocketDescriptor = TempDescriptor;
		}
	}
}

bool CMD_SaveUserModes(void)
{
	FILE *Descriptor = fopen("db/usermodes.db", "wb");
	struct UserModeSpec *Worker = UserModeRoot, *Del;
	
	if (!Descriptor) return false;
	
	if (!UserModeRoot)
	{
		fclose(Descriptor);
		remove("db/usermodes.db");
		return true;
	}
	
	for (; Worker; Worker = Del)
	{
		Del = Worker->Next;
		
		fprintf(Descriptor, "%s!%s@%s %s %s %s\n", Worker->Nick, Worker->Ident, Worker->Mask, Worker->Target, Worker->Channel, Worker->Mode);
		
		free(Worker);
	}
	
	UserModeRoot = NULL;
	
	fflush(Descriptor);
	fclose(Descriptor);
	return true;
}

void CMD_ListUserModes(const char *SendTo)
{
	struct UserModeSpec *Worker = UserModeRoot;
	char OutBuf[1024];
	int Inc = 0;
	
	if (!UserModeRoot)
	{
		IRC_Message(SendTo, "No user modes are saved.");
		return;
	}
	
	IRC_Message(SendTo, "List of user modes currently saved:");
	
	for (; Worker; Worker = Worker->Next, ++Inc)
	{
		snprintf(OutBuf, sizeof OutBuf, "[%d] \002Activating mask:\002 %s!%s@%s | \002Target:\002 %s | \002Channel:\002 %s | \002Mode:\002 %s",
				Inc, Worker->Nick, Worker->Ident, Worker->Mask, Worker->Target, Worker->Channel, Worker->Mode);

		
		IRC_Message(SendTo, OutBuf);
	}
	
	IRC_Message(SendTo, "End of list.");
}
