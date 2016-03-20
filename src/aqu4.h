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

/*main.c*/
namespace Main
{
	void SetTextColor(ConsoleColor Color);
	bool SaveSocket(const char *OkMessageTarget);
}
/*config.c*/
namespace Config
{
	bool ReadConfig(void);
	bool LoadBrain(void);
	bool DumpBrain(void);
}
/*netcore.c*/
namespace Net
{
	bool Connect(const char *InHost, unsigned short PortNum, int *SocketDescriptor_);
	bool Write(int SockDescriptor, const char *InMsg);
	bool Read(int SockDescriptor, void *OutStream_, unsigned MaxLength, bool TextStream);
	bool Disconnect(int SockDescriptor);
}
/*irc.c*/
namespace IRC
{
	bool Connect(void);
	void Loop(void);
	bool Quit(const char *QuitMSG);
	bool JoinChannel(const char *Channel);
	bool LeaveChannel(const char *Channel);
	bool Message(const char *Target, const char *Message);
	bool Notice(const char *Target, const char *Notice);
	bool NickChange(const char *Nick);
	void Pong(const char *Param);
	struct ChannelTree *AddChannelToTree(const char *const Channel, const char *const Prefix);
	bool DelChannelFromTree(const char *Channel);
	void ShutdownChannelTree(void);
	MessageType GetMessageType(const char *InStream_);
	bool GetMessageData(const char *Message, char *OutData);
	bool BreakdownNick(const char *Message, char *NickOut, char *IdentOut, char *MaskOut);
	bool GetStatusCode(const char *Message, int *OutNumber);
	bool AddUserToChannel(const char *Channel, const char *Nick, const char *Ident, const char *Mask, bool FullUser);
	bool DelUserFromChannel(const char *Channel, const char *Nick);
	void CompleteChannelUser(const char *const Nick, const char *const Ident, const char *const Mask);
	bool DelUserFromChannelP(struct ChannelTree *const Channel, const char *const Nick);
	bool UserInChannel(const char *Channel, const char *Nick);
	bool UserInChannelP(const struct ChannelTree *Channel, const char *Nick);
	void ShutdownChannelUsers(struct ChannelTree *Channel);
	struct ChannelTree *GetChannelFromDB(const char *const Channel);
	bool StripControlCodes(char *Stream);
	struct _UserList *GetUserInChannel(const char *const ChannelName, const char *Nick_);
}
/*commands.c*/
namespace CMD
{
	void ProcessCommand(const char *InStream);
	bool AddToTellDB(const char *Target, const char *Source, const char *Message);
	bool ReadTellDB(const char *Target);
	unsigned AddToStickyDB(const char *Owner, const char *Sticky, bool Private);
	void UpdateSeenDB(long Time, const char *Nick, const char *Channel, const char *LastMessage);
	bool SaveSeenDB(void);
	void LoadSeenDB(void);
	void AddUserMode(const char *Nick, const char *Ident, const char *Mask, const char *Mode,
								const char *Target, const char *Channel);
	bool DelUserMode(const char *Nick, const char *Ident, const char *Mask, const char *Mode, const char *Target, const char *Channel);
	bool LoadUserModes(void);
	bool SaveUserModes(void);
	void ListUserModes(const char *SendTo);
	void ProcessUserModes(const char *Nick, const char *Ident, const char *Mask, const char *Channel);
}

/*auth.c*/
namespace Auth
{
	bool AddAdmin(const char *Nick, const char *Ident, const char *Mask, bool BotOwner);
	bool DelAdmin(const char *Nick, const char *Ident, const char *Mask, bool OwnersToo);
	bool IsAdmin(const char *Nick, const char *Ident, const char *Mask, bool *BotOwner);
	void ListAdmins(const char *SendTo);
	void ShutdownAdmin(void);
	bool IsBlacklisted(const char *Nick, const char *Ident, const char *Mask);
	bool BlacklistDel(const char *Nick, const char *Ident, const char *Mask);
	bool BlacklistAdd(const char *Nick, const char *Ident, const char *Mask);
	void ShutdownBlacklist(void);
	void BlacklistLoad(void);
	bool BlacklistSave(void);
	void BlacklistSendList(const char *SendTo);
}
/*logging.c*/
namespace Log
{
	bool WriteMsg(const char *InStream, MessageType MType);
	bool CoreWrite(const char *InStream, const char *FileTitle);
	bool TailLog(const char *const ChannelOrNick, const int NumLinesToOut, char *const OutStream, const int Capacity);
}
/*wz.c*/
namespace WZ
{
	bool GetGamesList(const char *Server, unsigned short Port, const char *SendTo, bool WZLegacy);
}
/*ddg.c*/
namespace DDG
{
	bool Query(const char *Search, const char *SendTo);
}

/*curlcore.c*/
namespace CurlCore
{
	bool GetHTTP(const char *const URL, void *const OutStream, const unsigned MaxOutBytes);
}
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
