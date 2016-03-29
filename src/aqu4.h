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

#ifndef __cplusplus
#include <stdbool.h>
#else
extern "C"
{
#endif
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

struct _UserList
{
	char Nick[128];
	char Ident[128];
	char Mask[128];
	bool FullUser;
	
	struct _UserList *Next;
	struct _UserList *Prev;
};
struct ChannelTree
{
	char Channel[128];
	char Topic[2048];
	char TopicSetter[128];
	char CmdPrefix[128];
	unsigned long TopicSetTime;
	bool AutoLinkTitle; /*Automatically get link titles?*/
	bool ExcludeFromLogs; //Do not log this channel at all.
	
	struct _UserList *UserList;
	
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

enum ArgMode { NOARG, OPTARG, REQARG };
enum HPerms { ANY, ADMIN, OWNER };
struct _CmdList
{
	char CmdName[64];
	char HelpString[512];
	enum ArgMode AM;
	enum HPerms P;
	bool DisableCommand;
};

typedef enum { BLACK = 30, RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, WHITE, ENDCOLOR = 0 } ConsoleColor;

/*main.cpp*/
void Bot_SetTextColor(ConsoleColor Color);
bool Main_SaveSocket(const char *OkMessageTarget);

/*config.cpp*/
bool Config_ReadConfig(void);
bool Config_LoadBrain(void);
bool Config_DumpBrain(void);

/*netcore.cpp*/
bool Net_Connect(const char *InHost, unsigned short PortNum, int *SocketDescriptor_);
bool Net_Write(int SockDescriptor, const char *InMsg);
bool Net_Read(int SockDescriptor, void *OutStream_, unsigned MaxLength, bool TextStream);
bool Net_Disconnect(int SockDescriptor);

/*irc.cpp*/
bool IRC_Connect(void);
void IRC_Loop(void);
bool IRC_Quit(const char *QuitMSG);
bool IRC_JoinChannel(const char *Channel);
bool IRC_LeaveChannel(const char *Channel);
bool IRC_Message(const char *Target, const char *Message);
bool IRC_Notice(const char *Target, const char *Notice);
bool IRC_NickChange(const char *Nick);
void IRC_Pong(const char *Param);
struct ChannelTree *IRC_AddChannelToTree(const char *const Channel, const char *const Prefix);
bool IRC_DelChannelFromTree(const char *Channel);
void IRC_ShutdownChannelTree(void);
MessageType IRC_GetMessageType(const char *InStream_);
bool IRC_GetMessageData(const char *Message, char *OutData);
bool IRC_BreakdownNick(const char *Message, char *NickOut, char *IdentOut, char *MaskOut);
bool IRC_GetStatusCode(const char *Message, int *OutNumber);
bool IRC_AddUserToChannel(const char *Channel, const char *Nick, const char *Ident, const char *Mask, bool FullUser);
bool IRC_DelUserFromChannel(const char *Channel, const char *Nick);
void IRC_CompleteChannelUser(const char *const Nick, const char *const Ident, const char *const Mask);
bool IRC_DelUserFromChannelP(struct ChannelTree *const Channel, const char *const Nick);
bool IRC_UserInChannel(const char *Channel, const char *Nick);
bool IRC_UserInChannelP(const struct ChannelTree *Channel, const char *Nick);
void IRC_ShutdownChannelUsers(struct ChannelTree *Channel);
struct ChannelTree *IRC_GetChannelFromDB(const char *const Channel);
bool IRC_StripControlCodes(char *Stream);
struct _UserList *IRC_GetUserInChannel(const char *const ChannelName, const char *Nick_);

/*commands.cpp*/
void CMD_ProcessCommand(const char *InStream);
bool CMD_AddToTellDB(const char *Target, const char *Source, const char *Message);
bool CMD_ReadTellDB(const char *Target);
unsigned CMD_AddToStickyDB(const char *Owner, const char *Sticky, bool Private);
void CMD_UpdateSeenDB(long Time, const char *Nick, const char *Channel, const char *LastMessage);
bool CMD_SaveSeenDB(void);
void CMD_LoadSeenDB(void);
void CMD_AddUserMode(const char *Nick, const char *Ident, const char *Mask, const char *Mode,
							const char *Target, const char *Channel);
bool CMD_DelUserMode(const char *Nick, const char *Ident, const char *Mask, const char *Mode, const char *Target, const char *Channel);
bool CMD_LoadUserModes(void);
bool CMD_SaveUserModes(void);
void CMD_ListUserModes(const char *SendTo);
void CMD_ProcessUserModes(const char *Nick, const char *Ident, const char *Mask, const char *Channel);

/*auth.cpp*/
bool Auth_AddAdmin(const char *Nick, const char *Ident, const char *Mask, bool BotOwner);
bool Auth_DelAdmin(const char *Nick, const char *Ident, const char *Mask, bool OwnersToo);
bool Auth_IsAdmin(const char *Nick, const char *Ident, const char *Mask, bool *BotOwner);
void Auth_ListAdmins(const char *SendTo);
void Auth_ShutdownAdmin(void);
bool Auth_IsBlacklisted(const char *Nick, const char *Ident, const char *Mask);
bool Auth_BlacklistDel(const char *Nick, const char *Ident, const char *Mask);
bool Auth_BlacklistAdd(const char *Nick, const char *Ident, const char *Mask);
void Auth_ShutdownBlacklist(void);
void Auth_BlacklistLoad(void);
bool Auth_BlacklistSave(void);
void Auth_BlacklistSendList(const char *SendTo);

/*logging.cpp*/
bool Log_WriteMsg(const char *InStream, MessageType MType);
bool Log_CoreWrite(const char *InStream, const char *FileTitle);
bool Log_TailLog(const char *const ChannelOrNick, const int NumLinesToOut, char *const OutStream, const int Capacity);

/*wz.cpp*/
bool WZ_GetGamesList(const char *Server, unsigned short Port, const char *SendTo, bool WZLegacy);

/*ddg.cpp*/
bool DDG_Query(const char *Search, const char *SendTo);

/*curlcore.cpp*/
bool CurlCore_GetHTTP(const char *const URL, void *const OutStream, const unsigned MaxOutBytes);

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

#ifdef __cplusplus
}
#endif
