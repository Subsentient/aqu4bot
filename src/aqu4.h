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

#define WZSERVER_MAIN "lobby.wz2100.net"
#define WZSERVER_MAIN_PORT 9990

#define WZSERVER_LEGACY "warzonelegacy.org"
#define WZSERVER_LEGACY_PORT 9990

#define ROOT_URL "http://universe2.us/"
#define SOURCECODE_URL "http://github.com/Subsentient/aqu4bot/"

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
	IMSG_TOPICORIGIN
} MessageType;

struct ChannelTree
{
	char Channel[128];
	char Topic[1024];
	char TopicSetter[128];
	unsigned long TopicSetTime;
	
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

extern Bool Config_GetLineData(const char *InStream, char *OutStream, unsigned long MaxSize);
extern Bool Config_ReadConfig(void);

extern Bool Net_Connect(const char *InHost, unsigned short PortNum, int *SocketDescriptor_);
extern Bool Net_Write(int SockDescriptor, const char *InMsg);
extern Bool Net_Read(int SockDescriptor, void *OutStream_, unsigned long MaxLength, Bool TextStream);
extern Bool Net_Disconnect(int SockDescriptor);
extern Bool Net_GetHTTP(const char *Hostname, const char *Filename, unsigned long MaxData, char *OutStream);

extern Bool IRC_Connect(void);
extern void IRC_Loop(void);
extern Bool IRC_Quit(const char *QuitMSG);
extern Bool IRC_JoinChannel(const char *Channel);
extern Bool IRC_LeaveChannel(const char *Channel);
extern Bool IRC_Message(const char *Target, const char *Message);
extern Bool IRC_Notice(const char *Target, const char *Notice);
extern Bool IRC_NickChange(const char *Nick);
extern void IRC_Pong(const char *Param);
extern void IRC_AddChannelToTree(const char *Channel);
extern Bool IRC_DelChannelFromTree(const char *Channel);
extern void IRC_ShutdownChannelTree(void);
extern MessageType IRC_GetMessageType(const char *InStream_);
extern Bool IRC_GetMessageData(const char *Message, char *OutData);
extern Bool IRC_BreakdownNick(const char *Message, char *NickOut, char *IdentOut, char *MaskOut);
extern Bool IRC_GetStatusCode(const char *Message, int *OutNumber);

extern void CMD_ProcessCommand(const char *InStream);
extern Bool CMD_AddToTellDB(const char *Target, const char *Source, const char *Message);
extern Bool CMD_ReadTellDB(const char *Target);
extern unsigned long CMD_AddToStickyDB(const char *Owner, const char *Sticky);
extern void CMD_UpdateSeenDB(long Time, const char *Nick, const char *Channel, const char *LastMessage);
extern Bool CMD_SaveSeenDB(void);
extern void CMD_LoadSeenDB(void);
extern void CMD_AddUserMode(const char *Nick, const char *Ident, const char *Mask, const char *Mode, const char *Channel, Bool FullVhost);
extern Bool CMD_DelUserMode(const char *Nick, const char *Ident, const char *Mask, const char *Mode, const char *Channel);
extern Bool CMD_LoadUserModes(void);
extern Bool CMD_SaveUserModes(void);
extern void CMD_ListUserModes(const char *SendTo);
extern void CMD_ProcessUserModes(const char *Nick, const char *Ident, const char *Mask, const char *Channel);

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

extern Bool Log_WriteMsg(const char *InStream, MessageType MType);
extern Bool Log_CoreWrite(const char *InStream, const char *FileTitle);

extern Bool WZ_GetGamesList(const char *Server, unsigned short Port, const char *SendTo, Bool WZLegacy);

extern Bool DDG_Query(const char *Search, const char *SendTo);

extern int SocketDescriptor;
extern struct _ServerInfo ServerInfo;
extern struct ChannelTree *Channels;
extern char CmdPrefix[128];
extern unsigned short SendDelay;
extern Bool ShowOutput;
extern Bool Logging;
extern Bool LogPMs;
extern int _argc;
extern char **_argv;
extern struct AuthTree *AdminAuths;

