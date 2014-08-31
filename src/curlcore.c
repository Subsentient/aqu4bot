/*
aqu4, daughter of pr0t0, half-sister to Johnsbot! Copyright 2014 Subsentient.

This file is part of aqu4bot.
aqu4bot is public domain software.
See the file UNLICENSE.TXT for more information.
*/

#include <stdio.h>
#include <curl/curl.h>
#include "substrings/substrings.h"
#include "aqu4.h"

struct HTTPStruct
{
	char *OutStream;
	unsigned MaxWriteSize;
};

static size_t _HTTPWriteHandler(void *InStream, size_t SizePerUnit, size_t NumMembers, void *HTTPDesc_)
{	
	struct HTTPStruct *HTTPDesc = HTTPDesc_;

	SubStrings.Cat(HTTPDesc->OutStream, InStream, HTTPDesc->MaxWriteSize);

	return SizePerUnit * NumMembers; /*Just get something to pacify baby.*/
}

Bool CurlCore_GetHTTP(const char *const URL_, void *const OutStream_, const unsigned MaxOutBytes)
{
	struct HTTPStruct HTTPDescriptor = { NULL };
	int AttemptsRemaining = 3; /*Three tries by default.*/
	CURLcode Code;
	CURL *Curl = NULL;
	char URL[2048];
	char *OutStream = OutStream_;

	/*Get the correct URL.*/
	if (!SubStrings.NCompare("http://", sizeof "http://" - 1, URL_))
	{
		snprintf(URL, sizeof URL, "http://%s", URL_);
	}
	else
	{
		SubStrings.Copy(URL, URL_, sizeof URL);
	}
	
	HTTPDescriptor.OutStream = OutStream;
	HTTPDescriptor.MaxWriteSize = MaxOutBytes;
	
	do
	{
		*OutStream = 0; /*Clear it.*/
		
		Curl = curl_easy_init(); /*Fire up curl.*/
		
		/*Add the URL*/
		curl_easy_setopt(Curl, CURLOPT_URL, URL);
		
		/*Follow paths that redirect.*/
		curl_easy_setopt(Curl, CURLOPT_FOLLOWLOCATION, 1);
		
		/*No progress bar on stdout please.*/
		curl_easy_setopt(Curl, CURLOPT_NOPROGRESS, 1);
		
		curl_easy_setopt(Curl, CURLOPT_WRITEFUNCTION, _HTTPWriteHandler);
		curl_easy_setopt(Curl, CURLOPT_WRITEDATA, &HTTPDescriptor);
		curl_easy_setopt(Curl, CURLOPT_VERBOSE, 0L);
		
		Code = curl_easy_perform(Curl);
		
		curl_easy_cleanup(Curl);
	} while (--AttemptsRemaining, (Code != CURLE_OK && AttemptsRemaining > 0));
	
	return Code == CURLE_OK;
		
}

