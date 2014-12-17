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

#define WZSERVER_LEGACY "warzonelegacy.org"
#define WZSERVER_LEGACY_PORT 9990

#define ROOT_URL "http://universe2.us/"
#define SOURCECODE_URL "http://github.com/Subsentient/aqu4bot/"

#ifndef BOT_OS
#define BOT_OS "UNIX"
#endif

/*This is how we do it.*/
typedef signed char Bool;
enum { false, true };

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
	IMSG_NAMES
} MessageType;

struct ChannelTree
{
	char Channel[128];
	char Topic[2048];
	char TopicSetter[128];
	char CmdPrefix[128];
	unsigned long TopicSetTime;
	Bool AutoLinkTitle; /*Automatically get link titles?*/
	
	struct _UserList
	{
		char Nick[128];
		char Ident[128];
		char Mask[128];
		Bool FullUser;
		
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
	char ServerPassword[256];
	unsigned short PortNum;
	Bool SetBotmode;
};

struct AuthTree
{
	char Nick[128];
	char Ident[128];
	char Mask[128];
	Bool BotOwner;
	
	struct AuthTree *Next;
	struct AuthTree *Prev;
};

typedef enum { BLACK = 30, RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, WHITE, ENDCOLOR = 0 } ConsoleColor;

/*This function comes from main.c*/
extern void Bot_SetTextColor(ConsoleColor Color);

/*config.c*/
extern Bool Config_GetLineData(const char *InStream, char *OutStream, unsigned MaxSize);
extern Bool Config_ReadConfig(void);

/*netcore.c*/
extern Bool Net_Connect(const char *InHost, unsigned short PortNum, int *SocketDescriptor_);
extern Bool Net_Write(int SockDescriptor, const char *InMsg);
extern Bool Net_Read(int SockDescriptor, void *OutStream_, unsigned MaxLength, Bool TextStream);
extern Bool Net_Disconnect(int SockDescriptor);

/*irc.c*/
extern Bool IRC_Connect(void);
extern void IRC_Loop(void);
extern Bool IRC_Quit(const char *QuitMSG);
extern Bool IRC_JoinChannel(const char *Channel);
extern Bool IRC_LeaveChannel(const char *Channel);
extern Bool IRC_Message(const char *Target, const char *Message);
extern Bool IRC_Notice(const char *Target, const char *Notice);
extern Bool IRC_NickChange(const char *Nick);
extern void IRC_Pong(const char *Param);
extern struct ChannelTree *IRC_AddChannelToTree(const char *const Channel, const char *const Prefix);
extern Bool IRC_DelChannelFromTree(const char *Channel);
extern void IRC_ShutdownChannelTree(void);
extern MessageType IRC_GetMessageType(const char *InStream_);
extern Bool IRC_GetMessageData(const char *Message, char *OutData);
extern Bool IRC_BreakdownNick(const char *Message, char *NickOut, char *IdentOut, char *MaskOut);
extern Bool IRC_GetStatusCode(const char *Message, int *OutNumber);
extern Bool IRC_AddUserToChannel(const char *Channel, const char *Nick, const char *Ident, const char *Mask, Bool FullUser);
extern Bool IRC_DelUserFromChannel(const char *Channel, const char *Nick);
extern void IRC_CompleteChannelUser(const char *const Nick, const char *const Ident, const char *const Mask);
extern Bool IRC_DelUserFromChannelP(struct ChannelTree *const Channel, const char *const Nick);
extern Bool IRC_UserInChannel(const char *Channel, const char *Nick);
extern Bool IRC_UserInChannelP(const struct ChannelTree *Channel, const char *Nick);
extern void IRC_ShutdownChannelUsers(struct ChannelTree *Channel);
extern struct ChannelTree *IRC_GetChannelFromDB(const char *const Channel);
extern Bool IRC_StripControlCodes(char *Stream);

/*commands.c*/
extern void CMD_ProcessCommand(const char *InStream);
extern Bool CMD_AddToTellDB(const char *Target, const char *Source, const char *Message);
extern Bool CMD_ReadTellDB(const char *Target);
extern unsigned CMD_AddToStickyDB(const char *Owner, const char *Sticky);
extern void CMD_UpdateSeenDB(long Time, const char *Nick, const char *Channel, const char *LastMessage);
extern Bool CMD_SaveSeenDB(void);
extern void CMD_LoadSeenDB(void);
extern void CMD_AddUserMode(const char *Nick, const char *Ident, const char *Mask, const char *Mode,
							const char *Target, const char *Channel);
extern Bool CMD_DelUserMode(const char *Nick, const char *Ident, const char *Mask, const char *Mode, const char *Target, const char *Channel);
extern Bool CMD_LoadUserModes(void);
extern Bool CMD_SaveUserModes(void);
extern void CMD_ListUserModes(const char *SendTo);
extern void CMD_ProcessUserModes(const char *Nick, const char *Ident, const char *Mask, const char *Channel);

/*auth.c*/
extern Bool Auth_AddAdmin(const char *Nick, const char *Ident, const char *Mask, Bool BotOwner);
extern Bool Auth_DelAdmin(const char *Nick, const char *Ident, const char *Mask, Bool OwnersToo);
extern Bool Auth_IsAdmin(const char *Nick, const char *Ident, const char *Mask, Bool *BotOwner);
extern void Auth_ListAdmins(const char *SendTo);
extern void Auth_ShutdownAdmin(void);
extern Bool Auth_IsBlacklisted(const char *Nick, const char *Ident, const char *Mask);
extern Bool Auth_BlacklistDel(const char *Nick, const char *Ident, const char *Mask);
extern Bool Auth_BlacklistAdd(const char *Nick, const char *Ident, const char *Mask);
extern void Auth_ShutdownBlacklist(void);
extern void Auth_BlacklistLoad(void);
extern Bool Auth_BlacklistSave(void);
extern void Auth_BlacklistSendList(const char *SendTo);

/*logging.c*/
extern Bool Log_WriteMsg(const char *InStream, MessageType MType);
extern Bool Log_CoreWrite(const char *InStream, const char *FileTitle);
extern Bool Log_TailLog(const char *const ChannelOrNick, const int NumLinesToOut, char *const OutStream, const int Capacity);

/*wz.c*/
extern Bool WZ_GetGamesList(const char *Server, unsigned short Port, const char *SendTo, Bool WZLegacy);

/*ddg.c*/
extern Bool DDG_Query(const char *Search, const char *SendTo);

/*curlcore.c*/
extern Bool CurlCore_GetHTTP(const char *const URL, void *const OutStream, const unsigned MaxOutBytes);

/*Various globals.*/
extern int SocketDescriptor;
extern struct _ServerInfo ServerInfo;
extern struct ChannelTree *Channels;
extern char GlobalCmdPrefix[128];
extern unsigned short SendDelay;
extern Bool ShowOutput;
extern Bool Logging;
extern Bool LogPMs;
extern int _argc;
extern char **_argv;
extern struct AuthTree *AdminAuths;
extern Bool NoControlCodes;
