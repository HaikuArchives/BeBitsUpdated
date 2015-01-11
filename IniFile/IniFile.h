//----------------------------------------------------------------------
//	IniFile.h - Copyright 2002 Tyler Dauwalder	
//	This software is release under the MIT License
//	See the accompanying License file, or wander
//	over to: http://www.opensource.org/licenses/mit-license.html
//----------------------------------------------------------------------
#ifndef _INI_FILE_H_
#define _INI_FILE_H_

#include <stdio.h>
#include <string.h>
#include <sys/types.h>

// The IniFile class declaration is down a ways...

class NodeList;	// Forward declaration

class IniNode {	// Used for sections and keys
	public:
		char *zName;
		char *zStr;
		NodeList *zChildList;
		IniNode *zNext;

		IniNode();
		IniNode(const char *Name, const char *Str);
		IniNode(const IniNode &ref);
		~IniNode();

		void SetName(const char *Name);
		void SetStr(const char *Str);
		
		void PrintContents(const char *indentStr) const;

		void DeleteStr(char *str);
		char* AllocStr(const size_t size);
	protected:
		void CopyStr(char **str, const char *val);
	private:
		IniNode& operator=(const IniNode &ref);
			// Currently there's no need for this operator
};

class NodeList {	// Keeps a list of IniNodes
	private:
		int zCount;
	protected:
		void Clone(const NodeList &ref);
			// Clears the list and makes it a copy of ref
	public:
		IniNode *zStart, *zEnd;

		NodeList();
		NodeList(const NodeList &ref);
		~NodeList();

		NodeList& operator=(const NodeList &ref);
		
		void Add(IniNode *Item);
			// Adds the given item to the list (does nothing if Item is NULL)			
		IniNode *Add(const char *name);
			// Creates a new node with the specified name and adds it to the list
		IniNode *Remove();
			// Removes and returns the first item in the list
		void Clear();
			// Clears (and frees) the list
		int Count();
			// Returns the # of items in the list
		IniNode *FindNode(const char *Name) const;
			// Returns the node with the given Name
};

class IniFile {
	public:
		// Constructors/Destructor
		IniFile() {};
		IniFile(const IniFile &ref);
		IniFile(const char *Filename);
		virtual ~IniFile() {};

		// Assignment
		IniFile &operator=(const IniFile &Ini);

		// Load and storing entire Ini files from disk
		bool Load(const char *Filename, const bool ThrowExceptionOnFileError = false);
		bool Store(const char *Filename, const bool ThrowExceptionOnFileError = false);
		
		// Clear the ini file
		void Clear();
			
		// Writing functions
		void WriteInt(const char *Section, const char *Key, const int Val);
		void WriteBool(const char *Section, const char *Key, const bool Val);
		void WriteFloat(const char *Section, const char *Key, const float Val);
		void WriteString(const char *Section, const char *Key, const char *Val);
		void WriteData(const char *Section, const char *Key, const void *Data, const size_t Size);
		
		// Reading functions
		int ReadInt(const char *Section, const char *Key, const int Default = 0) const;
		bool ReadBool(const char *Section, const char *Key, const bool Default = false) const;
		float ReadFloat(const char *Section, const char *Key, const float Default = 0.0) const;
		char* ReadString(const char *Section, const char *Key, char *Result, size_t Size, const char *Default = NULL) const;
		char* ReadString(const char *Section, const char *Key, const char *Default = NULL) const;
			/*	The first version of ReadStr (the one with the "int Size" argument)
				accepts	a pointer to a string Result of length Size-1 (remeber we
				need a NULL at the end :-) into which it copies the result and then
				returns. The second version allocates a string of necessary size,
				copies the result into it, and then returns it. It's *YOUR* responsibility
				to free(return value) the string if you use the second version.
				
				On a side note, if the default string is needed, it is first copied into
				the result and then the result string is returned. Thus you need to
				free(return value) the string returned by the second version in all cases,
				even if it's the default string that is returned.
			*/
			
		size_t ReadData(const char *Section, const char *Key, void *Result, size_t Size) const;
		size_t ReadData(const char *Section, const char *Key, void **Result) const;
			/*	A similar warning applies to the second ReadData() function. The second
				version (the one with void **Result) allocates a new chunk of memory
				into the pointer pointed to by Result (i.e. into *Result). If the read
				fails for some reason, *Result will be set to NULL. Either way, it's
				safe (and your responsibility) to free(result) whatever is placed into
				*Result.
			*/
		
		// Exception classes
		class EGeneralError {};
		class EInsufficientMemory : public EGeneralError {};
			/* 	THROWN BY:
					Damn near everything :-)
			*/
		class EFileError : public EGeneralError {};
			/* 	THROWN BY:
					IniFile(const char *Filename)
					Load(...) but only when ThrowExceptionOnFileError == true
					Store(...) but only when ThrowExceptionOnFileError == true
			*/
		
		// Handy debugging kinda function
		void PrintContents() const;

	protected:
		static const bool DEBUG = false;
			/*	Turns debugging output on or off. If I recall, this only has an effect
				on Load(), which prints out whether each line it reads in is a section
				line, a data line, or an ignored line. Kinda handy if you're stumped as
				to why things don't appear to be loading correctly.
				
				Since this is a compile time constant, the compiler will remove any
				debugging code if DEBUG is false.
			*/

		// For the *Data functions, we'll process data in blocks of 3 bytes.
		// We'll be storing data with 64 different ASCII characters, thus
		// converting a 3x8-bit-byte block into a 4x6-bit-byte block.
		static const int kBytesPerBlock = 3;
		static const int kCharsPerBlock = 4;
		
		// Inline functions used by *Data functions to convert 8-bit binary
		// data to and from our custom 6-bit ASCII encoding (the ASCII chars
		// used are still regular 8-bit chars, we're just representing 6-bits
		// of data with each of them).
		char inline Binary8ToChar6(unsigned char binaryValue) const;
		unsigned char inline Char6ToBinary8(char charValue) const;
		int DecodeData(const char *sourceData, const size_t sourceSize, unsigned char *destData, const size_t destSize) const;

	private:
		// Our list of sections
		NodeList zRootList;

		// Handy functions for reading chars and lines from files
		char inline ReadChar(FILE *stream);
		bool ReadLine(FILE *stream, char **str);

		IniNode *FindSection(const char *Section) const;
		IniNode *FindKey(const char *Section, const char *Key) const;
		IniNode *FindKey(const IniNode *iSection, const char *Key) const;

		IniNode *AddSection(const char *Section);
		IniNode *AddKey(IniNode *iSection, const char *Key);

		// Return the given section or key if it already exists, otherwise
		// create a new section or key with the given name
		IniNode *FindCreateSection(const char *Section);
		IniNode *FindCreateKey(char *Section, const char *Key);
		IniNode *FindCreateKey(IniNode *iSection, const char *Key);
		
		// Inline functions used for parsing. Edit these functions
		// if you wish to support a different character set
		bool inline IsWhitespace(const char ch);
		bool inline IsNewline(const char ch);
		bool inline IsEndOfLine(const char ch);
		bool inline IsEquals(const char ch);
		bool inline IsSectionStart(const char ch);
		bool inline IsSectionEnd(const char ch);
		bool inline IsValueChar(const char ch);
		bool inline IsNameChar(const char ch);
		bool inline IsNameStart(const char ch);
		bool inline IsSectionChar(const char ch);
		
		// Inline helper function called by WriteData to write the block of bytes
		// at blockData to a block of ASCII encoded 6-bit values at stringData
		void EncodeBlockToString(const unsigned char *blockData, char *stringData);
		void inline DecodeBlockToBuffer(const char *stringData, unsigned char *blockData) const;		
		
};


#endif
