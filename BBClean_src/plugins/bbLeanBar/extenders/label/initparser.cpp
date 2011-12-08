/*
 ============================================================================

	Handy-Dandy stuff to parse initialisation strings to extenders

 ============================================================================

	This file is part of the bbLean source code.

	Copyright © 2007 noccy
	http://dev.noccy.com/bbclean

	bbClean is free software, released under the GNU General Public License
	(GPL version 2 or later).

	http://www.fsf.org/licenses/gpl.html

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

 ============================================================================
*/


#include "initparser.h"

using namespace std;

// Constructor
argParser::argParser() { }

// Destructor
argParser::~argParser() { }

//
// Parser routine: Goes over the specified string and breaks it in sections
// while respecting quoted strings. No support for escaping, and only valid
// string quote char is single quote (')
//
void argParser::ParseString(std::string InitString)
{
	// Reset the previously parsed data
	ParsedArguments = 0;
	string argwords[256];

	// Separate the data at spaces
	bool quoted = false;
	string thisChar;
	for (int n=0; n<(int)InitString.length(); n++) {
		thisChar = InitString.substr(n,1);
		if (quoted) {
			if (thisChar == "'") {
				quoted = false;
			} else {
				argwords[ParsedArguments].append(thisChar);
			}
		} else {
			if (thisChar == "'") {
				quoted = true;
			} else {
				if (thisChar != " ") {
					argwords[ParsedArguments].append(thisChar);
				} else {
					ParsedArguments++;
					// Just to be sure...
					if (ParsedArguments > 255) ParsedArguments = 255;
				}
			}
		}
	}

	// We should have all the words separated and unquoted now.
	for (int n=0; n<=ParsedArguments; n++) {

		// Find index of "=" and extract the argument and the key.
		// If the key is found but no argument, it's assumed to be
		// a flag.
		if (argwords[n].find("=") == string::npos) {
			// Assume this is a flag and just set it
			this->ParsedArgumentStruct[n].key = argwords[n];
			this->ParsedArgumentStruct[n].arg = "";
		} else {
			int idx = argwords[n].find("=");
			this->ParsedArgumentStruct[n].key = argwords[n].substr(0,(int)(idx));
			this->ParsedArgumentStruct[n].arg = argwords[n].substr(idx+1,(int)(argwords[n].length() - idx -1));
		}

	}
	return;
}

//
//  HasArgument: Returns True if there is an argument with this key.
//  function is case sensitive.
//
bool argParser::HasArgument(string Argument)
{
	// Return true if the argument is present
	for (int n=0; n<=ParsedArguments; n++)
	{
		if (ParsedArgumentStruct[n].key == Argument) return(true);
	}
	return(false);
}

//
//  GetArgument: Returns the matching argument if any, or if no argument
//  with the requested key is there, NULL.
//
string argParser::GetArgument(string Argument)
{
	// Return the actual argument
	for (int n=0; n<=ParsedArguments; n++)	{
		if (ParsedArgumentStruct[n].key == Argument) return(ParsedArgumentStruct[n].arg);
	}
	return(NULL);
}

//
//  GetArgumentEx: Returns the specified argument if it's present, and
//  otherwise return the default string specified.
//
string argParser::GetArgumentEx(string Argument, string Default)
{
	if (this->HasArgument(Argument)) {
		return(this->GetArgument(Argument));
	} else {
		return(Default);
	}
}
