/*
aqu4, daughter of pr0t0, half-sister to Johnsbot! Copyright 2014 Subsentient.

This file is part of aqu4bot.
aqu4bot is public domain software.
See the file UNLICENSE.TXT for more information.
*/

#define BOT_VERSION "baking"
#define CONFIG_FILE "aqu4bot.conf"

#define IRC_CODE_OK 1
#define IRC_CODE_NICKTAKEN 433
#define IRC_CODE_BADPASSWORD 465

#define WZSERVER_MAIN "lobby.wz2100.net"
#define WZSERVER_MAIN_PORT 9990

#define WZSERVER_LEGACY "universe2.us"
#define WZSERVER_LEGACY_PORT 9990

#define ROOT_URL "http://universe2.us/"
#define SOURCECODE_URL "http://github.com/Subsentient/aqu4bot/"

#ifndef BOT_OS
#define BOT_OS "UNIX"
#endif

#include <stdbool.h>

/*The commands that are found via IRC.*/
typedef enum
{
	IMSG_INVALID = -1,
	IMSG_UNKNOWN,
	IMSG_PRIVMSG,
	IMSG_NOTICE,
	IMSG_MODE,
	IMSG_JOIN,
	IMSG_PART,
	IMSG_PING,
	IMSG_PONG,
	IMSG_QUIT,
	IMSG_KICK,
	IMSG_KILL,
	IMSG_NICK,
	IMSG_INVITE,
	IMSG_TOPIC,
	IMSG_TOPICORIGIN,
	IMSG_NAMES,
	IMSG_WHO
} MessageType;

struct ChannelTree
{
	char Channel[128];
	char Topic[2048];
	char TopicSetter[128];
	char CmdPrefix[128];
	unsigned long TopicSetTime;
	bool AutoLinkTitle; /*Automatically get link titles?*/
	bool ExcludeFromLogs; //Do not log this channel at all.
	
	struct _UserList
	{
		char Nick[128];
		char Ident[128];
		char Mask[128];
		bool FullUser;
		
		struct _UserList *Next;
		struct _UserList *Prev;
	} *UserList;
	
	struct ChannelTree *Next;
	struct ChannelTree *Prev;
};

struct _ServerInfo
{
	char Hostname[128];
	char Nick[128];
	char Ident[128];
	char RealName[128];
	char NickservPwd[256];
	char NickservNick[128];
	char ServerPassword[256];
	unsigned short PortNum;
	bool SetBotmode;
};

struct AuthTree
{
	char Nick[128];
	char Ident[128];
	char Mask[128];
	bool BotOwner;
	
	struct AuthTree *Next;
	struct AuthTree *Prev;
};

struct _CmdList
{
	char CmdName[64];
	char HelpString[512];
	enum ArgMode { NOARG, OPTARG, REQARG } AM;
	enum HPerms { ANY, ADMIN, OWNER } P;
	bool DisableCommand;
};

typedef enum { BLACK = 30, RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, WHITE, ENDCOLOR = 0 } ConsoleColor;

/*main.c*/
extern void Bot_SetTextColor(ConsoleColor Color);
extern bool Main_SaveSocket(const char *OkMessageTarget);

/*config.c*/
extern bool Config_ReadConfig(void);
extern bool Config_LoadBrain(void);
extern bool Config_DumpBrain(void);

/*netcore.c*/
extern bool Net_Connect(const char *InHost, unsigned short PortNum, int *SocketDescriptor_);
extern bool Net_Write(int SockDescriptor, const char *InMsg);
extern bool Net_Read(int SockDescriptor, void *OutStream_, unsigned MaxLength, bool TextStream);
extern bool Net_Disconnect(int SockDescriptor);

/*irc.c*/
extern bool IRC_Connect(void);
extern void IRC_Loop(void);
extern bool IRC_Quit(const char *QuitMSG);
extern bool IRC_JoinChannel(const char *Channel);
extern bool IRC_LeaveChannel(const char *Channel);
extern bool IRC_Message(const char *Target, const char *Message);
extern bool IRC_Notice(const char *Target, const char *Notice);
extern bool IRC_NickChange(const char *Nick);
extern void IRC_Pong(const char *Param);
extern struct ChannelTree *IRC_AddChannelToTree(const char *const Channel, const char *const Prefix);
extern bool IRC_DelChannelFromTree(const char *Channel);
extern void IRC_ShutdownChannelTree(void);
extern MessageType IRC_GetMessageType(const char *InStream_);
extern bool IRC_GetMessageData(const char *Message, char *OutData);
extern bool IRC_BreakdownNick(const char *Message, char *NickOut, char *IdentOut, char *MaskOut);
extern bool IRC_GetStatusCode(const char *Message, int *OutNumber);
extern bool IRC_AddUserToChannel(const char *Channel, const char *Nick, const char *Ident, const char *Mask, bool FullUser);
extern bool IRC_DelUserFromChannel(const char *Channel, const char *Nick);
extern void IRC_CompleteChannelUser(const char *const Nick, const char *const Ident, const char *const Mask);
extern bool IRC_DelUserFromChannelP(struct ChannelTree *const Channel, const char *const Nick);
extern bool IRC_UserInChannel(const char *Channel, const char *Nick);
extern bool IRC_UserInChannelP(const struct ChannelTree *Channel, const char *Nick);
extern void IRC_ShutdownChannelUsers(struct ChannelTree *Channel);
extern struct ChannelTree *IRC_GetChannelFromDB(const char *const Channel);
extern bool IRC_StripControlCodes(char *Stream);
extern struct _UserList *IRC_GetUserInChannel(const char *const ChannelName, const char *Nick_);

/*commands.c*/
extern void CMD_ProcessCommand(const char *InStream);
extern bool CMD_AddToTellDB(const char *Target, const char *Source, const char *Message);
extern bool CMD_ReadTellDB(const char *Target);
extern unsigned CMD_AddToStickyDB(const char *Owner, const char *Sticky, bool Private);
extern void CMD_UpdateSeenDB(long Time, const char *Nick, const char *Channel, const char *LastMessage);
extern bool CMD_SaveSeenDB(void);
extern void CMD_LoadSeenDB(void);
extern void CMD_AddUserMode(const char *Nick, const char *Ident, const char *Mask, const char *Mode,
							const char *Target, const char *Channel);
extern bool CMD_DelUserMode(const char *Nick, const char *Ident, const char *Mask, const char *Mode, const char *Target, const char *Channel);
extern bool CMD_LoadUserModes(void);
extern bool CMD_SaveUserModes(void);
extern void CMD_ListUserModes(const char *SendTo);
extern void CMD_ProcessUserModes(const char *Nick, const char *Ident, const char *Mask, const char *Channel);

/*auth.c*/
extern bool Auth_AddAdmin(const char *Nick, const char *Ident, const char *Mask, bool BotOwner);
extern bool Auth_DelAdmin(const char *Nick, const char *Ident, const char *Mask, bool OwnersToo);
extern bool Auth_IsAdmin(const char *Nick, const char *Ident, const char *Mask, bool *BotOwner);
extern void Auth_ListAdmins(const char *SendTo);
extern void Auth_ShutdownAdmin(void);
extern bool Auth_IsBlacklisted(const char *Nick, const char *Ident, const char *Mask);
extern bool Auth_BlacklistDel(const char *Nick, const char *Ident, const char *Mask);
extern bool Auth_BlacklistAdd(const char *Nick, const char *Ident, const char *Mask);
extern void Auth_ShutdownBlacklist(void);
extern void Auth_BlacklistLoad(void);
extern bool Auth_BlacklistSave(void);
extern void Auth_BlacklistSendList(const char *SendTo);

/*logging.c*/
extern bool Log_WriteMsg(const char *InStream, MessageType MType);
extern bool Log_CoreWrite(const char *InStream, const char *FileTitle);
extern bool Log_TailLog(const char *const ChannelOrNick, const int NumLinesToOut, char *const OutStream, const int Capacity);

/*wz.c*/
extern bool WZ_GetGamesList(const char *Server, unsigned short Port, const char *SendTo, bool WZLegacy);

/*ddg.c*/
extern bool DDG_Query(const char *Search, const char *SendTo);

/*curlcore.c*/
extern bool CurlCore_GetHTTP(const char *const URL, void *const OutStream, const unsigned MaxOutBytes);

/*Various globals.*/
extern int SocketDescriptor;
extern struct _ServerInfo ServerInfo;
extern struct ChannelTree *Channels;
extern char GlobalCmdPrefix[sizeof ((struct ChannelTree*)0)->CmdPrefix];
extern unsigned short SendDelay;
extern bool ShowOutput;
extern bool Logging;
extern bool LogPMs;
extern int _argc;
extern char **_argv;
extern struct AuthTree *AdminAuths;
extern bool NoControlCodes;
extern struct _CmdList CmdList[];
extern char HelpGreeting[1024];
extern bool NEXUSCompat;
extern char BotRootDir[4096];
