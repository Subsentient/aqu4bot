/*
aqu4, daughter of pr0t0, half-sister to Johnsbot! Copyright 2014 Subsentient.

This file is part of aqu4bot.
aqu4bot is public domain software.
See the file UNLICENSE.TXT for more information.
*/


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/stat.h>
#include "substrings/substrings.h"
#include "aqu4.h"

bool Config_ReadConfig(void)
{
	FILE *Descriptor = fopen(CONFIG_FILE, "r");
	char *ConfigStream = NULL;
	const char *Worker = NULL;
	struct stat FileStat;
	unsigned LineNum = 1;
	char CurrentLine[2048];
	char LineID[1024];
	char LineData[1024];
	
	printf("Reading configuration... "), fflush(stdout);
	
	if (!Descriptor) goto Error;
	
	if (stat(CONFIG_FILE, &FileStat) != 0) goto Error;
	
	Worker = ConfigStream = malloc(FileStat.st_size + 1);
	
	fread(ConfigStream, 1, FileStat.st_size, Descriptor);
	ConfigStream[FileStat.st_size] = '\0';
	fclose(Descriptor);
	
	for (; SubStrings.Line.GetLine(CurrentLine, sizeof CurrentLine, &Worker); ++LineNum)
	{
		const char *Ptr = CurrentLine;

		if (*CurrentLine == '#' || *CurrentLine == '\r' || *CurrentLine == '\n') continue;
		
		//Get the line ID, aka config attribute.
		SubStrings.CopyUntilC(LineID, sizeof LineID, &Ptr, "\t =");
		
		//Get the data for the attribute.
		SubStrings.Copy(LineData, Ptr, sizeof LineData);
	
		if (!strcmp(LineID, "Hostname"))
		{
			SubStrings.Copy(ServerInfo.Hostname, LineData, sizeof ServerInfo.Hostname);
		}
		else if (!strcmp(LineID, "PortNum"))
		{
			ServerInfo.PortNum = atoi(LineData);
		}
		else if (!strcmp(LineID, "Nick"))
		{
			SubStrings.Copy(ServerInfo.Nick, LineData, sizeof ServerInfo.Nick);
		}
		else if (!strcmp(LineID, "Ident"))
		{
			SubStrings.Copy(ServerInfo.Ident, LineData, sizeof ServerInfo.Ident);
		}
		else if (!strcmp(LineID, "RealName"))
		{
			SubStrings.Copy(ServerInfo.RealName, LineData, sizeof ServerInfo.RealName);
		}
		else if (!strcmp(LineID, "NickservPwd"))
		{
			SubStrings.Copy(ServerInfo.NickservPwd, LineData, sizeof ServerInfo.NickservPwd);
		}
		else if (!strcmp(LineID, "ServerPassword"))
		{
			SubStrings.Copy(ServerInfo.ServerPassword, LineData, sizeof ServerInfo.ServerPassword);
		}
		else if (!strcmp(LineID, "Channel"))
		{
			char Chan[sizeof LineData], Prefix[sizeof Chan] = { 0 }, *Worker = Chan;
			
			SubStrings.Copy(Chan, LineData, sizeof Chan);
			
			if ((Worker = SubStrings.CFind(',', 1, Chan)))
			{ /*Copy in any prefixes.*/
				*Worker++ = '\0';
				
				if (!*Worker)
				{
					Bot_SetTextColor(RED);
					putc('*', stderr);
					Bot_SetTextColor(ENDCOLOR);
					
					fprintf(stderr, " Bad channel prefix value \"%s\" in config, line %u.\n", Prefix, LineNum);
				}
				else
				{ //get the prefix
					SubStrings.Copy(Prefix, Worker, sizeof Prefix);
				}
			}
			
			if (Chan[0] == '#' || Chan[0] == '@')  /*at sign says use auto title read.*/
			{
				//Lower case it.
				SubStrings.ASCII.LowerS(Chan);
				
				if (Chan[0] == '@')
				{ /*They want auto link title reading.*/
					struct ChannelTree *NewChannel = IRC_AddChannelToTree(Chan + 1, *Prefix ? Prefix : NULL);
					NewChannel->AutoLinkTitle = true;
				}
				else
				{ /*They didn't say, so assume no auto-link-title-reading.*/
					IRC_AddChannelToTree(Chan, *Prefix ? Prefix : NULL);
				}
			}
			else
			{
				Bot_SetTextColor(RED);
				putc('*', stderr);
				Bot_SetTextColor(ENDCOLOR);
				
				fprintf(stderr, " Bad channel value \"%s\" in config, line %u.\n", Chan, LineNum);
			}
		}
		else if (!strcmp(LineID, "Prefix"))
		{
			if (!strcmp(LineData, "NONE")) GlobalCmdPrefix[0] = '\0';
			else
			{
				SubStrings.Copy(GlobalCmdPrefix, LineData, sizeof GlobalCmdPrefix);
			}
		}
		else if (!strcmp(LineID, "BotOwner") || !strcmp(LineID, "Admin"))
		{
			char Nick[128], Ident[128], Mask[128];
			bool BotOwner = !strcmp(LineID, "BotOwner");
			
			if (!IRC_BreakdownNick(LineData, Nick, Ident, Mask))
			{
				fprintf(stderr, "Invalid vhost format, line %u\n", LineNum);
				return false;
			}

			if (!Auth_AddAdmin(Nick, Ident, Mask, BotOwner))
			{
				fprintf(stderr, "Failed to add admin, line %u\n", LineNum);
				return false;
			}
		}
		else if (!strcmp(LineID, "SendDelay"))
		{
			SendDelay = atoi(LineData);
		}
		else if (!strcmp(LineID, "Logging"))
		{
			Logging = !strcmp(LineData, "true");
		}
		else if (!strcmp(LineID, "NoControlCodes"))
		{
			NoControlCodes = !strcmp(LineData, "true");
		}
		else if (!strcmp(LineID, "LogPMs"))
		{
			LogPMs = !strcmp(LineData, "true");
		}
		else if (!strcmp(LineID, "SetBotmode"))
		{
			if (!strcmp(LineData, "true")) ServerInfo.SetBotmode = true;
		}
		else
		{
			Bot_SetTextColor(RED);
			putc('*', stderr);
			Bot_SetTextColor(ENDCOLOR);
			
			fprintf(stderr, " Bad value in config, at line %u.\n", LineNum);
			continue;
		}	
	}
	
	free(ConfigStream);
	
	if (ServerInfo.Hostname[0] == 0) goto Error;
	if (ServerInfo.PortNum == 0) ServerInfo.PortNum = 6667; /*I always thought this was a creepy port number. I use freenode on port 8000.*/
	if (ServerInfo.Nick[0] == 0) goto Error;
	if (ServerInfo.Ident[0] == 0) strcpy(ServerInfo.Ident, ServerInfo.Nick);
	if (ServerInfo.RealName[0] == 0) strcpy(ServerInfo.RealName, ServerInfo.Nick);
	if (!AdminAuths) fprintf(stderr,"\n *** WARNING ***: You have not specified any owners or admins.\n"
							"Many commands require admin or owner permissions and will not work.\n");
	
	puts("OK");
	
	return true;
Error:
	fprintf(stderr, "Failed.\n\nEither your configuration file doesn't exist, "
			"is unreadable, or is missing info.\nEnsure that you have specified at least "
			"a hostname and a nickname in your \n" CONFIG_FILE " configuration file.\n"
			"If you have no config file, try passing --genconfig to generate it.\n");
	return false;
}
