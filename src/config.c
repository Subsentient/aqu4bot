/*
aqu4, daughter of pr0t0, half-sister to Johnsbot! Copyright 2014 Subsentient.

This file is part of aqu4bot.

aqu4bot is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

aqu4bot is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with aqu4bot.  If not, see <http://www.gnu.org/licenses/>.
*/


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "aqu4.h"

Bool Config_GetLineData(const char *InStream, char *OutStream, unsigned long MaxSize)
{
	const char *Worker = InStream;
	
	while (*Worker != ' ' && *Worker != '\t' && *Worker != '=') ++Worker;
	
	if (*(Worker + 1) == '\0') return false;
	
	while (*Worker == ' ' || *Worker == '\t' || *Worker == '=') ++Worker;
	
	if (*Worker == '\0') return false;
	
	snprintf(OutStream, MaxSize, "%s", Worker);
	
	return true;
}

char *NextLine(const char *InStream)
{	
	if ((InStream = strchr(InStream, '\n')) == NULL) return NULL;
	++InStream;
	
	if (*InStream == '\0') return NULL;
	
	return (char*)InStream;
}
	
Bool Config_ReadConfig(void)
{
	FILE *Descriptor = fopen(CONFIG_FILE, "r");
	char *ConfigStream = NULL, *Worker = NULL;
	struct stat FileStat;
	unsigned long Inc = 0, LineNum = 1;
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
	
	do
	{
		if (*Worker == '\n' || *Worker == '#') continue;

		for (Inc = 0; Worker[Inc] != '\n' && Worker[Inc] != '\0' && Inc < sizeof CurrentLine - 1; ++Inc)
		{
			CurrentLine[Inc] = Worker[Inc];
		}
		CurrentLine[Inc] = '\0';
		
		for (Inc = 0; CurrentLine[Inc] != ' ' && CurrentLine[Inc] != '\t' &&
			CurrentLine[Inc] != '=' && CurrentLine[Inc] != '\0' && Inc < sizeof LineID - 1; ++Inc)
		{
			LineID[Inc] = CurrentLine[Inc];
		}
		LineID[Inc] = '\0';
		
		*LineData = '\0';
		Config_GetLineData(CurrentLine, LineData, sizeof LineData);	
	
		if (!strcmp(LineID, "Hostname"))
		{
			strncpy(ServerInfo.Hostname, LineData, sizeof ServerInfo.Hostname - 1);
			ServerInfo.Hostname[sizeof ServerInfo.Hostname - 1] = '\0';
		}
		else if (!strcmp(LineID, "PortNum"))
		{
			ServerInfo.PortNum = atoi(LineData);
		}
		else if (!strcmp(LineID, "Nick"))
		{
			strncpy(ServerInfo.Nick, LineData, sizeof ServerInfo.Nick - 1);
			ServerInfo.Nick[sizeof ServerInfo.Nick - 1] = '\0';
		}
		else if (!strcmp(LineID, "Ident"))
		{
			strncpy(ServerInfo.Ident, LineData, sizeof ServerInfo.Ident - 1);
			ServerInfo.Ident[sizeof ServerInfo.Ident - 1] = '\0';
		}
		else if (!strcmp(LineID, "RealName"))
		{
			strncpy(ServerInfo.RealName, LineData, sizeof ServerInfo.RealName - 1);
			ServerInfo.RealName[sizeof ServerInfo.RealName - 1] = '\0';
		}
		else if (!strcmp(LineID, "NickservPwd"))
		{
			strncpy(ServerInfo.NickservPwd, LineData, sizeof ServerInfo.NickservPwd - 1);
			ServerInfo.NickservPwd[sizeof ServerInfo.NickservPwd - 1] = '\0';
		}
		else if (!strcmp(LineID, "Channel"))
		{
			if (LineData[0] == '#') IRC_AddChannelToTree(LineData);
			else fprintf(stderr, "\033[31m* \033[0mBad channel value in config, line %lu.\n", LineNum);
		}
		else if (!strcmp(LineID, "Prefix"))
		{
			if (!strcmp(LineData, "NONE")) CmdPrefix[0] = '\0';
			else
			{
				strncpy(CmdPrefix, LineData, sizeof CmdPrefix - 1);
				CmdPrefix[sizeof CmdPrefix - 1] = '\0';
			}
		}
		else if (!strcmp(LineID, "BotOwner") || !strcmp(LineID, "Admin"))
		{
			char Nick[128], Ident[128], Mask[128];
			Bool BotOwner = !strcmp(LineID, "BotOwner");
			
			if (!IRC_BreakdownNick(LineData, Nick, Ident, Mask))
			{
				fprintf(stderr, "Invalid vhost format, line %lu\n", LineNum);
				return false;
			}

			if (!Auth_AddAdmin(Nick, Ident, Mask, BotOwner))
			{
				fprintf(stderr, "Failed to add admin, line %lu\n", LineNum);
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
			fprintf(stderr, "\033[31m*\033[0m Bad value in config, at line %lu.\n", LineNum);
			continue;
		}	
	} while (++LineNum, (Worker = NextLine(Worker)));
	
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
			"a hostname and a nickname in your \n" CONFIG_FILE " configuration file.\n");
	return false;
}
