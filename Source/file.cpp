//---------------------------------------------------------------------------
// Includes
//---------------------------------------------------------------------------
#include "file.h"

namespace file
{
	
	bool fileExists(string filename)
	{
		struct stat buffer; 
		return (stat (filename.c_str(), &buffer) == 0);
	}
	
	string readFile(const char* filename)
	{
		if(fileExists(filename))
		{
			ifstream fhandle;
			fhandle.open(filename);
			string buffer;
			string line;
			while(fhandle)
			{
				getline(fhandle, line);
				if(buffer.size() > 0 && line.size() > 0)
				{
					buffer.append(" ");
				}
				buffer.append(line);
			}
			fhandle.close();
			return buffer;
		}
		return "";
	}
	
	uint fileSize(string filename)
	{
		struct stat buffer; 
		return (stat (filename.c_str(), &buffer) == 0) ? buffer.st_size : 0;
	}
}
