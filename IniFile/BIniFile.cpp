//----------------------------------------------------------------------
//	BIniFile.cpp - Copyright 2002 Tyler Dauwalder	
//	This software is release under the MIT License
//	See the accompanying License file, or wander
//	over to: http://www.opensource.org/licenses/mit-license.html
//----------------------------------------------------------------------
#include "BIniFile.h"

#include <stdlib.h>

bool BIniFile::WriteMessage(const char *Section, const char *Key, const BMessage &Message)
{
	bool result = false;

	int size = Message.FlattenedSize();
	void *buffer = malloc(size);
	if (buffer == NULL)
		throw IniFile::EInsufficientMemory();
		
	try
	{
		if (Message.Flatten((char *)buffer, size) == B_OK)
		{
			WriteData(Section, Key, buffer, size);
			result = true;
		}
	}
	catch (...)
	{
		free(buffer);
		buffer = NULL;
		throw;
	}
	
	free(buffer);
	return result;
}

BMessage& BIniFile::ReadMessage(const char *Section, const char *Key, BMessage &Message) const
{
	return ReadMessage(Section, Key, Message, Message);
}

BMessage& BIniFile::ReadMessage(const char *Section, const char *Key, BMessage &Message, const BMessage &Default) const
{
	void *buffer = NULL;
	int size;
	bool result;
	
	BMessage Result;

	size = ReadData(Section, Key, &buffer);
	try
	{
		BMemoryIO memIO((const void *)buffer, size);
		Message = (Result.Unflatten(&memIO) == B_OK) ? Result : Default;
	}
	catch (...)
	{
		free(buffer);
		buffer = NULL;
		throw;
	}
	
	free(buffer);
	return Message;	
}

BMessage* BIniFile::ReadMessage(const char *Section, const char *Key, const BMessage *Default) const
{
	void *buffer = NULL;
	int size;
	BMessage *message = NULL;

	size = ReadData(Section, Key, &buffer);
	try
	{		
		BMemoryIO memIO((const void *)buffer, size);
		message = new BMessage();
		if (message == NULL)
			throw IniFile::EInsufficientMemory();
		if (message->Unflatten(&memIO) != B_OK)
		{
			if (Default == NULL)
			{
				delete message;
				message = NULL;
			}
			else
				*message = *Default;
		
		}
	}
	catch (...)
	{
		free(buffer);
		buffer = NULL;
		throw;
	}
	
	free(buffer);
	return message;	
}

