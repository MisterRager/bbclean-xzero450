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

#include <iostream>

#ifndef __INITPARSER_H
#define __INITPARSER_H

#include <string>

using namespace std;

class argParser
{
	public:
		// Constructor/Destructor
		argParser();
		~argParser();
		// Public functions
		void ParseString(string InitString);
		bool HasArgument(string Argument);
		string GetArgument(string Argument);
		string GetArgumentEx(string Argument, string Default);
	private:
		struct ParseStruct{
			string key;
			string arg;
		};
		ParseStruct ParsedArgumentStruct[255];
		int ParsedArguments;
};

#endif // __INITPARSER_H
