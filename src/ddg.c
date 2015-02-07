/*
aqu4, daughter of pr0t0, half-sister to Johnsbot! Copyright 2014 Subsentient.

This file is part of aqu4bot.
aqu4bot is public domain software.
See the file UNLICENSE.TXT for more information.
*/

/**DuckDuckGo search engine chunk.**/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "substrings/substrings.h"
#include "aqu4.h"

bool DDG_Query(const char *Search, const char *SendTo)
{ /*Results from DuckDuckGo. http://api.duckduckgo.com/ */
	char Results[8192] = { '\0' };
	int Counter = 1;
	char *Worker = Results;
	char Query[1024];
	char URL[1024], Title[1024];
	char OutBuf[1024];
	unsigned Inc = 0;
	
	snprintf(Query, sizeof Query, "https://duckduckgo.com/?q=%s&format=json&pretty=1&t=aqu4bot%%20IRC%%20bot", Search);
	
	if (!CurlCore_GetHTTP(Query, Results, sizeof Results))
	{
		IRC_Message(SendTo, "Unable to retrieve results, can't download results!");
		return false;
	}
	
	for (; (Worker = SubStrings.Find("\"Result\" :", 1, Worker)) && Counter <= 3; ++Counter)
	{ /*This is the kind of stuff where SubStrings shines.*/
		if (!(Worker = SubStrings.Find("\"FirstURL\" : \"", 1, Worker)))
		{
			IRC_Message(SendTo, "I received bad results from DuckDuckGo, or something's changed. I can't get results' URLs.");
			return false;
		}
		Worker += sizeof "\"FirstURL\" : \"" - 1;
		
		for (Inc = 0; Worker[Inc] != '\"' && Worker[Inc] != '\n' && Worker[Inc] != '\r' &&
			Worker[Inc] != '\0' && Inc < sizeof URL - 1; ++Inc)
		{
			URL[Inc] = Worker[Inc];
		}
		URL[Inc] = '\0';
		
		
		if (!(Worker = SubStrings.Find("\"Text\" : \"", 1, Worker)))
		{
			IRC_Message(SendTo, "I received bad results from DuckDuckGo, or something's changed. I can't get results' titles.");
			return false;
		}
		Worker += sizeof "\"Text\" : \"" - 1;
		
		for (Inc = 0; Worker[Inc] != '\"' && Worker[Inc] != '\n' && Worker[Inc] != '\r' &&
			Worker[Inc] != '\0' && Inc < sizeof Title - 1; ++Inc)
		{
			Title[Inc] = Worker[Inc];
		}
		Title[Inc] = '\0';
		
		snprintf(OutBuf, sizeof OutBuf, "[%d]: \002Title:\002 %s | \002URL:\002 \00312%s\003", Counter, Title, URL);
		IRC_Message(SendTo, OutBuf);
	}
	
	if (Counter == 1)
	{
		IRC_Message(SendTo, "No results found.");
	}
	else
	{
		IRC_Message(SendTo, "End of results.");
	}
	
	return true;
}
