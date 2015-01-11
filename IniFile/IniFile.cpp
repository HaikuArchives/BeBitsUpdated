//----------------------------------------------------------------------
//	IniFile.cpp - Copyright 2002 Tyler Dauwalder	
//	This software is release under the MIT License
//	See the accompanying License file, or wander
//	over to: http://www.opensource.org/licenses/mit-license.html
//----------------------------------------------------------------------
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdarg.h>

#include "IniFile.h"


//-----------------------------------------------------------------------------
// IniNode
//-----------------------------------------------------------------------------

IniNode::IniNode()
{
	IniNode("", "");
}

IniNode::IniNode(const char *Name, const char *Str)
	: zName(NULL), zStr(NULL), zChildList(NULL), zNext(NULL)
{
	zChildList = new NodeList();
	if (zChildList == NULL)
		throw IniFile::EInsufficientMemory();
	SetName(Name);
	SetStr(Str);
}

IniNode::IniNode(const IniNode &ref)
	: zName(NULL), zStr(NULL), zChildList(NULL), zNext(NULL)
{
	
	if (ref.zName != NULL)
		CopyStr(&zName, ref.zName);
	else
		zName = NULL;
		
	if (ref.zStr != NULL)
		CopyStr(&zStr, ref.zStr);
	else
		zStr = NULL;
		
	zNext = NULL;	// This may not be what you expect!
		
	zChildList = new NodeList(*ref.zChildList);
	if (zChildList == NULL)
		throw IniFile::EInsufficientMemory();
}

// Currently private
IniNode& IniNode::operator=(const IniNode &ref)
{
	return *this;
}

IniNode::~IniNode()
{
	delete zChildList;

	// Delete our name and string data
	DeleteStr(zName);
	DeleteStr(zStr);
}

char* IniNode::AllocStr(const size_t size)
{
	char *result = (char *)malloc(size);
	if (result == NULL)
		throw IniFile::EInsufficientMemory();
	return result;
}

void IniNode::DeleteStr(char *str)
{
	free(str);
	str = NULL;
}

void IniNode::CopyStr(char **str, const char *val)
{
	if (val == NULL || str == NULL)
		return;

	DeleteStr(*str);

	*str = AllocStr(strlen(val) + 1);
	if (str == NULL)
		throw IniFile::EInsufficientMemory();
	strcpy(*str, val);
}

void IniNode::SetName(const char *Name)
{
	CopyStr(&zName, Name);
}

void IniNode::SetStr(const char *Str)
{
	CopyStr(&zStr, Str);
}

void IniNode::PrintContents(const char *indentStr) const
{
	char nullChar = 0;
	if (indentStr == NULL)
		indentStr = &nullChar;
	
	if (zStr[0] == 0)	
		printf("%s\"%s\"\n", indentStr, zName);
	else
		printf("%s\"%s\" >> \"%s\"\n", indentStr, zName, zStr);
	IniNode *node = zChildList->zStart;
	while (node != NULL)
	{
		node->PrintContents("  ");
		node = node->zNext;
	}
}

//-----------------------------------------------------------------------------
// NodeList
//-----------------------------------------------------------------------------

NodeList::NodeList()
	: zStart(NULL), zEnd(NULL), zCount(0)
{
}

NodeList::NodeList(const NodeList &ref)
	: zStart(NULL), zEnd(NULL), zCount(0)
{
	Clone(ref);
}

void NodeList::Clone(const NodeList &ref)
{
	Clear();	

	// Make ourselves into a complete copy of said list
	IniNode *node, *copy;
	for (node = ref.zStart; node != NULL; node = node->zNext)
	{
		copy = new IniNode(*node);
		if (copy == NULL)
			throw IniFile::EInsufficientMemory();
		Add(copy);
	}		
}

NodeList& NodeList::operator=(const NodeList &ref)
{
	Clone(ref);
	return *this;
}

NodeList::~NodeList()
{
	Clear();
}

// Clears our list
void NodeList::Clear()
{
	IniNode *item;

	// Delete any items in our list
 	while ((item = Remove()) != NULL)
		delete item;
}

// Removes and returns the first item in the list
IniNode* NodeList::Remove()
{
	if (zStart == NULL)
		return NULL;

	IniNode* result = zStart;		// Store a pointer to the 1st item
	zStart = zStart->zNext;			// Reposition our beginning-of-list pointer
	zCount--;						// Dec our internal item count
	return result;					// Return the old 1st item
}

IniNode *NodeList::Add(const char *Name)
{
	IniNode* Item = new IniNode(Name, "");
	if (Item == NULL)
		throw IniFile::EInsufficientMemory();
	Add(Item);
	return Item;
}

// Adds the given item to the list
void NodeList::Add(IniNode *Item)
{
	if (Item == NULL)
		return;

	// Check to see if our list is empty. If it
	// is, make our node the start and end of the
	// list. If it's not, add our node to the end.
	if (zStart == NULL)
	{
		zStart = Item;
		zEnd = Item;
	}
	else
	{
		zEnd->zNext = Item;	// Add the item to the end of the list
		zEnd = Item;		// Reposition our end-of-list pointer
	}

	// Inc our internal item count
	zCount++;
}

int NodeList::Count()
{
	return zCount;
}

IniNode *NodeList::FindNode(const char *Name) const
{
	IniNode *Item;
	for (Item = zStart; Item != NULL; Item = Item->zNext)
	{
		if (strcmp(Name, Item->zName) == 0)
			return Item;
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// IniFile
//-----------------------------------------------------------------------------

IniFile::IniFile(const IniFile &ref)
	: zRootList(ref.zRootList)
{
}

IniFile::IniFile(const char *Filename)
{
	Load(Filename, true);
}

void IniFile::Clear()
{
	zRootList.Clear();
}

IniFile &IniFile::operator=(const IniFile &Ini)
{
	zRootList = NodeList(Ini.zRootList);
	return *this;
}

bool IniFile::Load(const char *Filename, const bool ThrowExceptionOnFileError)
{
	FILE *stream;

	// Try to open our file
	stream = fopen(Filename, "rt");
	if ( stream == NULL )
	{
		if (ThrowExceptionOnFileError)
			throw IniFile::EFileError();
		else
			return false;
	}

	IniNode *iSection = NULL, *iKey = NULL;
	char *line = NULL;
	
	try
	{
		while ( !feof(stream) && ReadLine(stream, &line) )
		{
	
			int start = 0;
			int pos = 0;
		
			enum LineType { SectionLine, DataLine, Nonsense } lineType = Nonsense;
		
			enum ParseState {	Start,
								LeadingWhitespace,
								Name,
								WhitespaceEmbeddedInOrTrailingAfterName,
								PostEqualsWhitespace,
								Value,
								WhitespaceEmbeddedInOrTrailingAfterValue,
								PostSectionStartWhitespace,
								Section,
								WhitespaceEmbeddedInOrTrailingAfterSection,
								
								DataLineFinished,
								SectionLineFinished,

								Error
							} state = Start;
									
			
			// Get rid of comments
			while (line[pos] != 0 && line[pos] != ';')
				pos++;
			if (line[pos] == ';')	// We've found a comment
				line[pos] = 0;		// Ignore the rest of the string
			
			pos = 0;
	
			// All these indices are with respect the line we just read into "line"
			int nameStart = 0;		// Index of beginning of StringName string
			int nameLen = 0;		// Length of StringName string
			int sectionStart = 0;	// Index of beginning of SectionName string
			int sectionLen = 0;		// Length of SectionName string
			int valueStart = 0;		// Index of beginning of Data string
			int valueLen = 0;		// Length of Data string
			int whitespaceLen = 0;	// Used to track the length of strings of potentially
									// embedded whitespace in the data string
			
			// Parse the string. This while loop is based off a
			// finite state diagram I drew up for parsing Ini files.
			while (state != Error
				&& state != DataLineFinished
				&& state != SectionLineFinished)
			{
				char ch = line[pos];	// Handy alias
				switch (state)
				{
					case Start:
						state = LeadingWhitespace;	// Just an alias; fall thru to LeadingWhitespace
					
					// Gets rid of leading whitespace
					case LeadingWhitespace:
						if ( IsWhitespace(ch) )
							pos++;
						else if ( IsNameStart(ch) )
						{
							nameStart = pos;
							pos++;
							nameLen++;
							state = Name;
						}
						else if ( IsSectionStart(ch) )
						{
							pos++;
							state = PostSectionStartWhitespace;
						}
						else
							state = Error;						
						break;
						
					// Reads in the name of a data item
					case Name:
						if ( IsNameChar(ch) )
						{
							pos++;
							nameLen++;
						}
						else if ( IsEquals(ch) )
						{
							pos++;
							state = PostEqualsWhitespace;
						}
						else if ( IsWhitespace(ch) )
						{
							whitespaceLen = 1;	// Reset embedded whitespace counter
							pos++;
							state = WhitespaceEmbeddedInOrTrailingAfterName;
						}
						else
							state = Error;
						break;
						
					// Handles (i.e. ignores) whitespace between the
					// StringName and the "="
					case WhitespaceEmbeddedInOrTrailingAfterName:
						if ( IsNameChar(ch) )
						{
							// Whitespace was embedded, so count it as
							// part of the name string
							nameLen += whitespaceLen + 1;
							pos++;
							state = Name;												
						}
						else if ( IsWhitespace(ch) )
						{
							whitespaceLen++;
							pos++;
						}
						else if ( IsEquals(ch) )
						{
							// Whitespace was trailing, so ignore it
							pos++;
							state = PostEqualsWhitespace;
						}
						else
							state = Error;						
						break;
						
					// Handles (i.e. ignores) whitespace between
					// the "=" and the beggining of the value string
					case PostEqualsWhitespace:
						if ( IsWhitespace(ch) )
							pos++;
						else if ( IsValueChar(ch) )
						{
							valueStart = pos;
							valueLen = 1;
							pos++;
							state = Value;
						}
						else
							state = Error;
						
						break;
					
					// Handles the data string until the end of the line
					// is found, or we hit some whitspace (at which point
					// we aren't sure yet if it's embedded whitespace or
					// extraneous whitespace)
					case Value:
						if ( IsValueChar(ch) )
						{
							pos++;
							valueLen++;
						}
						else if ( IsWhitespace(ch) )
						{
							whitespaceLen = 1;	// Reset embedded whitespace counter
							pos++;
							state = WhitespaceEmbeddedInOrTrailingAfterValue;
						}
						else if ( IsEndOfLine(ch) )
							state = DataLineFinished;
						else
							state = Error;
						break;
						
					// Handles whitespace that is either embedded in the
					// value string (in which case we want it to be part
					// of the data), or completely after the data string
					// (in which case it is ignored)
					case WhitespaceEmbeddedInOrTrailingAfterValue:
						if ( IsValueChar(ch) )
						{
							// Whitespace was embedded, so count it as
							// part of the data string
							valueLen += whitespaceLen + 1;
							pos++;
							state = Value;
						}
						else if ( IsWhitespace(ch) )
						{
							pos++;
							whitespaceLen++;
						}
						else if ( IsEndOfLine(ch) )
						{
							// Whitespace was trailing, so ignore it
							state = DataLineFinished;
						}
						else
							state = Error;						
						break;
						
					// Handles (i.e. ignores) whitespace between the "["
					// starting a section and the first character
					// of the section name
					case PostSectionStartWhitespace:
						if ( IsWhitespace(ch) )
							pos++;
						else if ( IsSectionChar(ch) )
						{
							sectionStart = pos;
							sectionLen = 1;
							pos++;
							state = Section;
						}
						else
							state = Error;
						break;
						
					// Handles the section identifier
					case Section:
						if ( IsSectionChar(ch) )
						{
							pos++;
							sectionLen++;
						}
						else if ( IsWhitespace(ch) )
						{
							whitespaceLen = 1;		// Reset embedded whitespace counter
							pos++;
							state = WhitespaceEmbeddedInOrTrailingAfterSection;
						}
						else if ( IsSectionEnd(ch) )
							state = SectionLineFinished;
						else
							state = Error;				
						break;
						
					case WhitespaceEmbeddedInOrTrailingAfterSection:
						if ( IsSectionChar(ch) )
						{
							// Whitespace was embedded, so count it as
							// part of the section string
							sectionLen += whitespaceLen + 1;
							pos++;
							state = Section;
						}
						else if ( IsWhitespace(ch) )
						{
							whitespaceLen++;
							pos++;
						}
						else if ( IsSectionEnd(ch) )
							state = SectionLineFinished;
						else
							state = Error;
						break;
	
					default:
						// Some unexpected error
						state = Error;
						break;			
				}		
			}
			
			
			// Now we see what our parser came up with
			switch (state)
			{
				case DataLineFinished:
				{
					// If iSection is NULL, we haven't come across any sections
					// yet, so this key doesn't belong to any section. Thus
					// we'll just ignore it
					if (iSection == NULL)
					{
						if (IniFile::DEBUG)
							printf("IGNR:   \"%s\"\n", line);
						break;
					}
	
					// Add NULL characters immediately after the characters
					// that make up the string name and the characters that
					// make up the data string in the line we just read and
					// parsed. Since there's always at least an "=" between
					// a string name and the data string, we don't have to
					// worry about overwriting anything.  Plus, we don't have
					// to alloc and copy any new strings, we just pass a pointer
					// to the first character of the given string
					char *name = &(line[nameStart]);
					line[nameStart + nameLen] = 0;
					char *value = &(line[valueStart]);
					line[valueStart + valueLen] = 0;
					
	
					// Get a pointer to the key
					iKey = FindCreateKey(iSection, name);
		
					// Set the key's new value
					iKey->SetStr(value);
					
					if (IniFile::DEBUG)
						printf("DATA: %s = '%s'\n", name, value);
		
					break;
				}
				
				case SectionLineFinished:
				{
					// Add NULL characters immediately after the characters
					// that make up the section name in the line we just read and
					// parsed. That way, we don't have to alloc and copy a new
					// string, we just pass a pointer to the first character of
					// the section name string
					char *name = line + sectionStart;
					line[sectionStart + sectionLen] = 0;
					
					// Find (if it exists) or create (if it doesn't already exist)
					// an IniNode object for the given section name.
					iSection = FindCreateSection(name);
					
					if (IniFile::DEBUG)
						printf("SECT: [%s]\n", name);
					
					break;
				}
					
				case Error:
				default:
					if (IniFile::DEBUG)
						printf("IGNR:   \"%s\"\n", line);
					break;
			}
		
		
			// Finally, we have to free the string alloc'd by ReadLine()
			free(line);
			line = NULL;
		}
	}
	catch (...)
	{
		free(line);
		line = NULL;
		fclose(stream);
		throw;
	}	
			
	// Close our file
	fclose(stream);

	return true;
}

bool IniFile::Store(const char *Filename, const bool ThrowExceptionOnFileError)
{
	FILE *stream;

	// Create athe file
	stream = fopen(Filename, "wt");
	if ( stream == NULL )
	{
		if (ThrowExceptionOnFileError)
			throw IniFile::EFileError();
		else
			return false;
	}

  IniNode *iSection, *iKey;

  // Write our ini tree to disk
  for (iSection = zRootList.zStart; iSection != NULL; iSection = iSection->zNext)
  {
    // Write our section
	fwrite("[", 1, 1, stream);
	fwrite(iSection->zName, strlen(iSection->zName), 1, stream);
	fwrite("]\n", 2, 1, stream);

  	for (iKey = iSection->zChildList->zStart; iKey != NULL; iKey = iKey->zNext)
    {
    	// Write our key and its value
    	fwrite(iKey->zName, strlen(iKey->zName), 1, stream);
    	fwrite("=", 1, 1, stream);
    	fwrite(iKey->zStr, strlen(iKey->zStr), 1, stream);
    	fwrite("\n", 1, 1, stream);
    }

    // Write a blank line for aesthetics
    fwrite("\n", 1, 1, stream);
  }

	// Close our file
	fclose(stream);

  return true;
}

char inline IniFile::ReadChar(FILE *stream)
{
	// Read in a character, returning a newline if no
	// character is actually read (i.e., eof)
	char ch;
	return fread(&ch, 1, 1, stream) == 1 ? ch : '\n';
}

// Reads the next line into the pointer pointed to by str
bool IniFile::ReadLine(FILE *stream, char **string)
{

	char *str = NULL;
	int size = 0;
	int pos = 0;
	int nextSize = 64;	// Seem like a reasonable default length to me
	char ch;
	
	for (ch = ReadChar(stream); ch != '\n' && ch != '\r'; ch = ReadChar(stream))
	{
		// If we've reached the end of our string, we need to
		// double its size before moving on. 
		if (pos == size)
		{
			str = (char *)realloc(str, nextSize);
			if (str == NULL)
				throw IniFile::EInsufficientMemory();
			size = nextSize;
			nextSize *= 2;
		}
		
		str[pos] = ch;
		pos++;
	}
	
	if (size != 0)
	{
		str[pos] = 0;
	}
	else
	{
		str = (char *)malloc(1);
		if (str == NULL)
			throw IniFile::EInsufficientMemory();
		str[0] = 0;
	}		
	*string = str;
	return true;
	
		
}

IniNode* IniFile::FindSection(const char *Section) const
{
	return zRootList.FindNode(Section);
}

IniNode *IniFile::FindCreateSection(const char *Section)
{
	IniNode *iSection;

	// Find the section, creating it if it doesn't exist
	if ((iSection = FindSection(Section)) == NULL)
		iSection = AddSection(Section);

	return iSection;
}

IniNode* IniFile::FindKey(const char *Section, const char *Key) const
{
	return FindKey(FindSection(Section), Key);
}

IniNode* IniFile::FindKey(const IniNode* iSection, const char *Key) const
{
	if (iSection == NULL || iSection->zChildList == NULL)
		return NULL;
	else
		return iSection->zChildList->FindNode(Key);
}

IniNode *IniFile::FindCreateKey(char *Section, const char *Key)
{
	return FindCreateKey(FindSection(Section), Key);
}

IniNode *IniFile::FindCreateKey(IniNode *iSection, const char *Key)
{
	// Check iSection
	if (iSection == NULL)
		return NULL;

	IniNode *iKey;

	// Find the key, creating it if it doesn't exist
	if ((iKey = FindKey(iSection, Key)) == NULL)
		iKey = AddKey(iSection, Key);

	return iKey;
}

IniNode *IniFile::AddSection(const char *Section)
{
	// Add the a new section node
	return zRootList.Add(Section);
}

IniNode *IniFile::AddKey(IniNode *iSection, const char *Key)
{
	// If we're not given a section, bail. Otherwise, add the
	// new key node
	if (iSection == NULL || iSection->zChildList == NULL)
		return NULL;
	else
		return iSection->zChildList->Add(Key);
}

void IniFile::WriteString(const char *Section, const char *Key, const char *Val)
{
	IniNode *iSection, *iKey;

	// Find the section, creating it if it doesn't exist
	iSection = FindCreateSection(Section);

	// Find the key, creating it if it doesn't exist
	if ((iKey = FindKey(iSection, Key)) == NULL)
		iKey = AddKey(iSection, Key);

	iKey->SetStr(Val);
}

void IniFile::WriteInt(const char *Section, const char *Key, const int Val)
{
	char str[40];
	sprintf(str, "%d", Val);
	WriteString(Section, Key, str);
}

void IniFile::WriteBool(const char *Section, const char *Key, const bool Val)
{
	WriteInt(Section, Key, Val);
}

void IniFile::WriteFloat(const char *Section, const char *Key, const float Val)
{
	char str[40];
	sprintf(str, "%f", Val);
	WriteString(Section, Key, str);
}

void IniFile::WriteData(const char *Section, const char *Key, const void *Data, const size_t Size)
{
	IniNode *iSection, *iKey;
	
	// Find the section, creating it if it doesn't exist
	iSection = FindCreateSection(Section);
	
	// Find the key creating it if it doesn't exist
	iKey = FindKey(iSection, Key);
	if (iKey == NULL)
		iKey = AddKey(iSection, Key);
		
	int blocks = Size / kBytesPerBlock + 2;	// Add two blocks for the sizeof(data) value
	int leftovers = Size % kBytesPerBlock;
	int charBlocks = (leftovers == 0) ? blocks : blocks + 1;
	int totalChars = charBlocks * kCharsPerBlock + 1;
	if (IniFile::DEBUG)
		printf("Size == %d, blocks == %d, leftoevers == %d, charBlocks == %d, totalChars == %d\n", Size, blocks, leftovers, charBlocks, totalChars);

	// We're going to manhandle this node a little bit
	// and manipulate its data members directly for the
	// sake of efficiency. First we have to free its
	// current string and alloc a new one with enough
	// characters for the number of blocks we have to
	// write.
	iKey->DeleteStr(iKey->zStr);
	char *str = iKey->AllocStr( totalChars + 200);
	

	// We'll keep track of our positions in our respective
	// buffers with these pointers
	unsigned char *blockData = (unsigned char *)Data;
	char *stringData = str;

	// First we write our 32-bit sizeof(data) value into the first two blocks
	// ---------------------
	// LITTLE ENDIAN ONLY!!!  Actually, I'm not totally sure about that. LITTLE ENDIAN ONLY MAYBE!!!
	// ---------------------
	unsigned char sizeBlock[ kBytesPerBlock * 2 ] = { 0, 0, 0, 0, 0, 0 };
	memcpy(sizeBlock, &Size, 4);	// ASSUMPTION: int == 4 bytes
	EncodeBlockToString(sizeBlock, stringData);	
	stringData += kCharsPerBlock;	
	EncodeBlockToString(&(sizeBlock[3]), stringData);
	stringData += kCharsPerBlock;
	blocks -= 2;	
	
	// Now we convert one full block of 8-bit data in Data to
	// one full block of 6-bit data (represented by 8-bit ASCII
	// characters) in str
	for (int i = 0; i < blocks; i++)
	{
		EncodeBlockToString(blockData, stringData);
		
		blockData += kBytesPerBlock;
		stringData += kCharsPerBlock;
	
	}
	
	// Then we handle the leftovers if necessary
	if (leftovers > 0)
	{
		unsigned char finalBlock[kBytesPerBlock];
		for (int i = 0; i < kBytesPerBlock; i++)
			finalBlock[i] = (i < leftovers) ? blockData[i] : 0;
			
		EncodeBlockToString(finalBlock, stringData);
		stringData += kCharsPerBlock;
	}
	
	// Null terminate like a good boy
	stringData[0] = 0;
	
	// Pass the string on to the node
	iKey->zStr = str;		
	
}

char* IniFile::ReadString(const char *Section, const char *Key, const char *Default = NULL)  const 
{
	IniNode *Item;	
	Item = FindKey(Section, Key);

	// We'll later copy from value to result, so value either
	// needs to point to the data, or our default string
	const char *value = ( Item != NULL && Item->zStr != NULL ) ? Item->zStr : Default;
	if (value == NULL)
		return NULL;	// If we need a default value, but it's NULL, just return NULL	
	char *result;
		
	// Allocate a new string, throwing our out of memory
	// exception if the malloc fails
	result = (char *)malloc( strlen(value) + 1 );
	if (result == NULL) 
		throw IniFile::EInsufficientMemory();
		
	// Now copy the result into our result string
	try
	{
		strcpy(result, value);
	}
	catch (...)
	{
		free(result);
		result = NULL;
		throw;
	}
	
	return result;		
}


char* IniFile::ReadString(const char *Section, const char *Key, char *Result, size_t Size, const char *Default = NULL) const
{

	IniNode *Item;

	Item = FindKey(Section, Key);
	if ( Item != NULL)
		strncpy(Result, Item->zStr, Size);
	else if (Default != NULL)
		strncpy(Result, Default, Size);
	else
		return NULL;
		
	return Result;
}

int IniFile::ReadInt(const char *Section, const char *Key, const int Default) const
{
	int val;
	char *str;
	char defaultStr[256];	// 255 digits ought to be enough for an int, right?
	sprintf(defaultStr, "%d", Default);

	// Read in the string for the given key
	str = ReadString(Section, Key, defaultStr);

	// Attempt to convert the string to an int
	if (sscanf(str, "%d", &val) != 1)
		val = Default;
	
	// Free the string
	free(str);

	return val;
}

bool IniFile::ReadBool(const char *Section, const char *Key, const bool Default) const
{
	return ReadInt(Section, Key, Default) != 0;
}

float IniFile::ReadFloat(const char *Section, const char *Key, const float Default) const
{
	float val;
	char *str;
	char defaultStr[256];	// 255 digits ought to be enough for a float, right?
	sprintf(defaultStr, "%f", Default);

	// Read in the string for the given key
	str = ReadString(Section, Key, defaultStr);

	// Attempt to convert the string to a float
	if (sscanf(str, "%f", &val) != 1)
		val = Default;
	
	// Free the string
	free(str);

	return val;
}

size_t IniFile::ReadData(const char *Section, const char *Key, void *Result, size_t Size) const
{
	IniNode *Item;

	Item = FindKey(Section, Key);
	if ( Item != NULL)
		return DecodeData(Item->zStr, strlen(Item->zStr), (unsigned char*)Result, Size);
	else
		return 0;
}

size_t IniFile::ReadData(const char *Section, const char *Key, void **Result) const
{
	IniNode *Item;

	Item = FindKey(Section, Key);
	if ( Item != NULL)
	{
		// Figure out how much of a buffer we need to allocate
		int stringLen = strlen(Item->zStr);
		int bytesNeeded = (stringLen / kCharsPerBlock) * kBytesPerBlock;
		*Result = malloc(bytesNeeded);
		if (*Result == NULL)
			throw IniFile::EInsufficientMemory();
		try
		{
			return DecodeData(Item->zStr, stringLen, (unsigned char*)*Result, bytesNeeded);
		}
		catch (...)
		{
			free(*Result);
			*Result = NULL;
			throw;
		}
	}
	else
	{
		*Result = NULL;
		return 0;
	}
}

int IniFile::DecodeData(const char *sourceData, const size_t sourceSize, unsigned char *destData, const size_t destSize) const
{
	size_t blocks = sourceSize / kCharsPerBlock;
		// For reading data, we just chop off any partial block at the end
		// because our write functions always write out complete blocks :-)
		// We will always read this many or fewer blocks from sourceData,
		// no more.

	// We have to at least have two blocks for the sizeof data
	if (blocks < 2)
		return 0;
		
	// We'll use these pointers to keep track of where we are
	char* sourceDataBlock = (char *)sourceData;		// Get the compiler to shut up about const :-)
	unsigned char* destDataBlock = destData;

	// Read in the stored data length value
	size_t storedSize = 0;
	unsigned char sizeBlock[ kBytesPerBlock * 2 ];
	DecodeBlockToBuffer(sourceDataBlock, sizeBlock);		// LITTLE ENDIAN ONLY!?!?!
	sourceDataBlock += kCharsPerBlock;
	DecodeBlockToBuffer(sourceDataBlock, &sizeBlock[3]);
	sourceDataBlock += kCharsPerBlock;
	memcpy(&storedSize, sizeBlock, sizeof storedSize);
	blocks -= 2;
	
	// We have to be prepared to read LESS data than was claimed
	// to have been stored, but not try to read MORE data than
	// was claimed to have been stored.
	size_t bytesAvailable = blocks * kBytesPerBlock;	// Data avaiable
	if (storedSize < bytesAvailable)
		bytesAvailable = storedSize;
		
/*
	if (DEBUG)
	{
		printf("\n");
		printf("sourceData == %s\n", sourceData);
		printf("sourceSize == %d\n", sourceSize);
		printf("destSize == %d\n", destSize);
		printf("blocks == %d\n", blocks);
		printf("bytesAvailable == %d\n", bytesAvailable);
		printf("\n");
	}
*/

	// We have to be prepared to read LESS data than is actually
	// available, but not try to read MORE data than is available
	size_t byteBlocks;
	size_t extras;
	if (destSize < bytesAvailable)
	{
		byteBlocks = destSize / kBytesPerBlock;
		extras = destSize % kBytesPerBlock;	
	}
	else
	{
		byteBlocks = bytesAvailable / kBytesPerBlock;
		extras = bytesAvailable % kBytesPerBlock;
	}
	
	// First we decode all the whole blocks we can
	for (size_t i = 0; i < byteBlocks; i++)
	{
		DecodeBlockToBuffer(sourceDataBlock, destDataBlock);
		sourceDataBlock += kCharsPerBlock;
		destDataBlock += kBytesPerBlock;	
	}
	
	// Now we decode any partial blocks
	if (extras > 0)
	{
		// We need an entire block to decode into
		unsigned char tempBlock[kBytesPerBlock];
		DecodeBlockToBuffer(sourceDataBlock, tempBlock);
		for (size_t i = 0; i < extras; i++)
			destDataBlock[i] = tempBlock[i];
	}	

	// If we made it this far...
	return byteBlocks * kBytesPerBlock + extras;
	
}


bool inline IniFile::IsWhitespace(const char ch)
{
	return ch == ' ' || ch == '\t';
}

bool inline IniFile::IsNewline(const char ch)
{
	return ch == '\n' || ch == '\r';
}

bool inline IniFile::IsEndOfLine(const char ch)
{
	return ch == 0;
}

bool inline IniFile::IsEquals(const char ch)
{
	return ch == '=';
}

bool inline IniFile::IsSectionStart(const char ch)
{
	return ch == '[';
}

bool inline IniFile::IsSectionEnd(const char ch)
{
	return ch == ']';
}

bool inline IniFile::IsValueChar(const char ch)
{
	return !IsWhitespace(ch) && !IsNewline(ch) && !IsEndOfLine(ch);
}

bool inline IniFile::IsNameChar(const char ch)
{
	return IsValueChar(ch) && !IsEquals(ch);
}

bool inline IniFile::IsNameStart(const char ch)
{
	return IsNameChar(ch) && !IsSectionStart(ch);
}

bool inline IniFile::IsSectionChar(const char ch)
{
	return IsValueChar(ch) && !IsSectionEnd(ch);
}


// Dumps the contents of this ini file to standard
// output with a little bit of pretty formatting
void IniFile::PrintContents() const
{
	IniNode *node = zRootList.zStart;
	while (node != NULL)
	{
		node->PrintContents("");
		node = node->zNext;
	}
}


// This function takes an 8-bit character (the least significant
// 6-bits of which it actually pays attention to) and returns the
// corresponding ASCII character in our super special 6-bit encoding
char inline IniFile::Binary8ToChar6(unsigned char binaryValue) const
{
	// Mask off the 2 bits we don't care about
	binaryValue &= 0x3F;		// 0x3F = 63 = 00111111 binary
	
	// Encode
	if (binaryValue < 26)
		return 'a' + binaryValue;
	else if (26 <= binaryValue && binaryValue < 26 + 26)
		return 'A' + (binaryValue - 26);
	else if (26 + 26 <= binaryValue && binaryValue < 26 + 26 + 10)
		return '0' + (binaryValue - 52);
	else if (binaryValue == 62)
		return '-';
	else
		return '_';
	
}

unsigned char inline IniFile::Char6ToBinary8(char charValue) const
{
	// Decode
	if ('a' <= charValue && charValue <= 'z')
		return 0 + charValue - 'a';
	else if ('A' <= charValue && charValue <= 'Z')
		return 26 + charValue - 'A';
	else if ('0' <= charValue && charValue <= '9')
		return 52 + charValue - '0';
	else if ('-' == charValue)
		return 62;
	else
		return 63;	
}

// Takes a three 8-bit-byte chunk of data and encodes it into a four 6-bit-byte
// chunk of ASCII chars. 
void IniFile::EncodeBlockToString(const unsigned char *blockData, char *stringData)
{
	// Encode them into four 6-bit values encoded in ASCII chars
	// ------ --|---- ----|-- ------   8-bit bytes become...
	// ++++++|++ ++++|++++ ++|++++++   6-bit bytes
	stringData[0] = Binary8ToChar6( blockData[0] >> 2 );		
	stringData[1] = Binary8ToChar6( ((blockData[0] << 6) >> 2) | (blockData[1] >> 4) );
	stringData[2] = Binary8ToChar6( ((blockData[1] << 4) >> 2) | (blockData[2] >> 6) );
	stringData[3] = Binary8ToChar6( blockData[2] );		// We know Binary8ToChar6 ignores the top two bits anyway
}

// Takes a four 6-bit-byte chunk of ASCII chars (that are actually 8-bits each, remember)
// and decodes it into a normal three 8-bit-byte chunk of binary data.
void inline IniFile::DecodeBlockToBuffer(const char *stringData, unsigned char *blockData) const
{
	// Get the 6 bit values represented by each string
	unsigned char partialData[kCharsPerBlock];
	partialData[0] = Char6ToBinary8(stringData[0]);
	partialData[1] = Char6ToBinary8(stringData[1]);
	partialData[2] = Char6ToBinary8(stringData[2]);
	partialData[3] = Char6ToBinary8(stringData[3]);
	
	// Reassemble them into three 8-bit values
	// ++++++|++ ++++|++++ ++|++++++   6-bit bytes become...
	// ------ --|---- ----|-- ------   8-bit bytes
	blockData[0] = (partialData[0] << 2) | (partialData[1] >> 4);
	blockData[1] = (partialData[1] << 4) | (partialData[2] >> 2);
	blockData[2] = (partialData[2] << 6) | partialData[3];
}

