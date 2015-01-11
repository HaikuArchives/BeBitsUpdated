//----------------------------------------------------------------------
//	BIniFile.h - Copyright 2002 Tyler Dauwalder	
//	This software is release under the MIT License
//	See the accompanying License file, or wander
//	over to: http://www.opensource.org/licenses/mit-license.html
//----------------------------------------------------------------------
#ifndef _B_INI_FILE_H_
#define _B_INI_FILE_H_

#include "IniFile.h"

#include <Message.h>

// BeOS specific extensions to IniFile. As with IniFile, and of
// these functions may throw an IniFile::EInsufficientMemory
// exception if a dynamic memory allocation fails. 
class BIniFile : public IniFile {
public:
	BIniFile() {};
	virtual ~BIniFile() {};
	
	bool WriteMessage(const char *Section, const char *Key, const BMessage &Message);
	
	// The first version calls the second version with Message for both the Message and Default parameters.
	// The third version always returns a newly allocated BMessage object or NULL. It's therefore your
	// responsiblity to *always* call "delete result" on it when you're done with it.
	BMessage& ReadMessage(const char *Section, const char *Key, BMessage &Message) const;
	BMessage& ReadMessage(const char *Section, const char *Key, BMessage &Message, const BMessage &Default) const;
	BMessage* ReadMessage(const char *Section, const char *Key, const BMessage *Default = NULL) const;	
};


#endif
