/*
aqu4, daughter of pr0t0, half-sister to Johnsbot! Copyright 2014 Subsentient.

This file is part of aqu4bot.
aqu4bot is public domain software.
See the file UNLICENSE.TXT for more information.
*/

/**This file is nasty, mostly because it's designed to be compatible with Warzone 2100.**/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#ifdef WIN
#include <winsock2.h>
#else
#include <arpa/inet.h>
#endif

#include "aqu4.h"
#include "substrings/substrings.h"

typedef struct
{
	uint32_t StructVer;
	char GameName[64];
	
	struct
	{
		int32_t Size;
		int32_t Flags;
		char HostIP[40];
		int32_t MaxPlayers;
		int32_t CurPlayers;
		int32_t UserFlags[4];
	} NetSpecs;
	
	char SecondaryHosts[2][40];
	char Extra[159];
	char Map[40];
	char HostNick[40];
	char VersionString[64];
	char ModList[255];
	uint32_t MajorVer, MinorVer;
	uint32_t PrivateGame;
	uint32_t MapMod; /*Legacy still calls this PureGame and 3.1 PureGame.*/
	uint32_t Mods;
	
	uint32_t GameID;
	
	uint32_t Unused1;
	uint32_t Unused2;
	uint32_t Unused3;
} GameStruct;

static bool WZ_RecvGameStruct(int SockDescriptor, void *OutStruct);

static bool WZ_RecvGameStruct(int SockDescriptor, void *OutStruct)
{
	uint32_t Inc = 0;
	GameStruct RV = { 0 };
	unsigned char SuperBuffer[sizeof RV.StructVer +
							sizeof RV.GameName +
							sizeof(int32_t) * 8 +
							sizeof RV.NetSpecs.HostIP + 
							sizeof RV.SecondaryHosts + 
							sizeof RV.Extra + 
							sizeof RV.Map + 
							sizeof RV.HostNick + 
							sizeof RV.VersionString + 
							sizeof RV.ModList + 
							sizeof(uint32_t) * 9] = { 0 }, *Worker = SuperBuffer;
	
	if (!Net_Read(SockDescriptor, SuperBuffer, sizeof SuperBuffer, false)) return false;
	
	memcpy(&RV.StructVer, Worker, sizeof RV.StructVer);
	RV.StructVer = ntohl(RV.StructVer);
	Worker += sizeof RV.StructVer;
	
	memcpy(RV.GameName, Worker, sizeof RV.GameName);
	Worker += sizeof RV.GameName;
	
	memcpy(&RV.NetSpecs.Size, Worker, sizeof RV.NetSpecs.Size);
	RV.NetSpecs.Size = ntohl(RV.NetSpecs.Size);
	Worker += sizeof RV.NetSpecs.Size;
	
	memcpy(&RV.NetSpecs.Flags, Worker, sizeof RV.NetSpecs.Flags);
	RV.NetSpecs.Flags = ntohl(RV.NetSpecs.Flags);
	Worker += sizeof RV.NetSpecs.Flags;
	
	memcpy(RV.NetSpecs.HostIP, Worker, sizeof RV.NetSpecs.HostIP);
	Worker += sizeof RV.NetSpecs.HostIP;
	
	memcpy(&RV.NetSpecs.MaxPlayers, Worker, sizeof(uint32_t));
	RV.NetSpecs.MaxPlayers = ntohl(RV.NetSpecs.MaxPlayers);
	Worker += sizeof(uint32_t);
	
	memcpy(&RV.NetSpecs.CurPlayers, Worker, sizeof(uint32_t));
	RV.NetSpecs.CurPlayers = ntohl(RV.NetSpecs.CurPlayers);
	Worker += sizeof(uint32_t);
	
	memcpy(RV.NetSpecs.UserFlags, Worker, sizeof(uint32_t) * 4);
	for (Inc = 0; Inc < 4; ++Inc)
	{
		RV.NetSpecs.UserFlags[Inc] = ntohl(RV.NetSpecs.UserFlags[Inc]);
	}
	Worker += sizeof(uint32_t) * 4;
	
	memcpy(RV.SecondaryHosts, Worker, sizeof RV.SecondaryHosts);
	Worker += sizeof RV.SecondaryHosts;
	/*Extra Map Host*/
	memcpy(RV.Extra, Worker, sizeof RV.Extra);
	Worker += sizeof RV.Extra;
	
	memcpy(RV.Map, Worker, sizeof RV.Map);
	Worker += sizeof RV.Map;
	
	memcpy(RV.HostNick, Worker, sizeof RV.HostNick);
	Worker += sizeof RV.HostNick;
	
	memcpy(RV.VersionString, Worker, sizeof RV.VersionString);
	Worker += sizeof RV.VersionString;
	
	memcpy(RV.ModList, Worker, sizeof RV.ModList);
	Worker += sizeof RV.ModList;
	
	/*Fuck alignment. This is aligned and I don't wanna hear anything else.*/
	memcpy(&RV.MajorVer, Worker, sizeof(uint32_t) * 9);
	for (Inc = 0; Inc < 9; ++Inc)
	{
		(&RV.MajorVer)[Inc] = ntohl((&RV.MajorVer)[Inc]);
	}
	Worker += sizeof(uint32_t) * 9;
	
	memcpy(OutStruct, &RV, sizeof RV);
	
	return true;
}

bool WZ_GetGamesList(const char *Server, unsigned short Port, const char *SendTo, bool WZLegacy)
{
	GameStruct *GamesList = NULL;
	int WZSocket = 0;
	char OutBuf[2048];
	uint32_t GamesAvailable = 0, Inc = 0;
	uint32_t LastHosted = 0;
	
	if (!Net_Connect(Server, Port, &WZSocket))
	{
		IRC_Message(SendTo, "Unable to connect to lobby server!");
		return false;
	}
	
	if (!Net_Write(WZSocket, "list\r\n"))
	{
		IRC_Message(SendTo, "Unable to write LIST command to lobby server!");
		return false;
	}
	
	/*Get number of available games.*/
	if (!Net_Read(WZSocket, &GamesAvailable, sizeof(uint32_t), false))
	{
		IRC_Message(SendTo, "Unable to read data from connection to lobby server!");
		return false;
	}
	
	GamesAvailable = ntohl(GamesAvailable);
	
	/*Allocate space for them.*/
	GamesList = malloc(sizeof(GameStruct) * GamesAvailable);

	for (; Inc < GamesAvailable; ++Inc)
	{ /*Receive the listings.*/
		if (!WZ_RecvGameStruct(WZSocket, GamesList + Inc))
		{
			free(GamesList);
			return false;
		}
	}
	
	/*If we're Legacy protocol, retrieve the time since last hosted.*/
	if (WZLegacy)
	{
		if (!Net_Read(WZSocket, &LastHosted, sizeof(uint32_t), false))
		{
			free(GamesList);
			return false;
		}
		
		LastHosted = ntohl(LastHosted);
	}
	
	Net_Disconnect(WZSocket);
	
	if (!GamesAvailable)
	{ /*No games in lobby.*/
		if (WZLegacy)
		{ /*Legacy has some helpful info on when games were recently hosted.*/
			if (((LastHosted / 60) / 60) / 24)
			{ /*Days.*/
				snprintf(OutBuf, sizeof OutBuf, "No games available. Last game was hosted %u days ago.",
						(unsigned)((LastHosted / 60) / 60) / 24);
			}
			else if (LastHosted / 60 > 60)
			{ /*Hours.*/
				snprintf(OutBuf, sizeof OutBuf, "No games available. Last game was hosted %u hours ago.",
						(unsigned)((LastHosted / 60) / 60));
			}
			else if (LastHosted >= 60)
			{ /*Minutes.*/
				snprintf(OutBuf, sizeof OutBuf, "No games available. Last game was hosted %u minutes ago.",
						(unsigned)LastHosted / 60);
			}
			else
			{ /*Seconds.*/
				snprintf(OutBuf, sizeof OutBuf, "No games available. Last game was hosted %u seconds ago.",
						(unsigned)LastHosted);
			}
		}
		else
		{ /*But alas, 3.1 lobby does not. Except for a long MOTD we won't read.*/
			snprintf(OutBuf, sizeof OutBuf, "No games available.");
		}
		
		IRC_Message(SendTo, OutBuf);
	}
		
	/*Now send them to the user.*/
	for (Inc = 0; Inc < GamesAvailable; ++Inc)
	{
		char ModBuf[512] = { '\0' };
		char MapBuf[128] = { '\0' };
		char ColTag[2] = { '\0' };
		
		/**Check for map-mod.**/		
		if (!WZLegacy && GamesList[Inc].MapMod)
		{ /*Legacy can't detect map-mods so it'd be wrong.*/
			snprintf(MapBuf, sizeof MapBuf, "\0034%s\003 (map-mod)", GamesList[Inc].Map);
		}
		else
		{ /*Either Legacy or no mod.*/
			SubStrings.Copy(MapBuf, GamesList[Inc].Map, sizeof MapBuf);
		}
		
		/**Colors for number in braces.**/
		if (GamesList[Inc].NetSpecs.CurPlayers >= GamesList[Inc].NetSpecs.MaxPlayers)
		{ /*game full.*/
			*ColTag = '4'; /*Red.*/
		}
		else if (GamesList[Inc].PrivateGame)
		{ /*private game.*/
			*ColTag = '8'; /*Yellow.*/
		}
		else if (*GamesList[Inc].ModList != '\0')
		{ /*mods.*/
			*ColTag = '6'; /*purple.*/
		}
		else
		{ /*normal.*/
			*ColTag = '3'; /*green.*/
		}
			
		/**Game has loaded mods (not map-mods)**/
		if (GamesList[Inc].ModList[0] != '\0')
		{
			snprintf(ModBuf, sizeof ModBuf, " \0034(mods: %s)\x3", GamesList[Inc].ModList);
		}
		
		snprintf(OutBuf, sizeof OutBuf, "\2\3%s[%u of %u]\3\2 \02Name\02: %s | \02Map\02: %s | \02Host\02: %s | "
				"\02Players\02: %d/%d %s| \02IP\02: %s | \02Version\02: %s%s",
				ColTag, (unsigned)Inc + 1, (unsigned)GamesAvailable, GamesList[Inc].GameName, MapBuf,
				GamesList[Inc].HostNick, GamesList[Inc].NetSpecs.CurPlayers, GamesList[Inc].NetSpecs.MaxPlayers,
				GamesList[Inc].PrivateGame ? "\0038(private)\x3 " : "", GamesList[Inc].NetSpecs.HostIP,
				GamesList[Inc].VersionString, ModBuf);
		IRC_Message(SendTo, OutBuf);
	}
	
	if (GamesList) free(GamesList);
		
	
	return true;
	
}
