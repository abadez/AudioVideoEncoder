#pragma once
#include <iostream>
#include <unistd.h>
#include <sys/stat.h>
#include <fstream>
#include <map>

namespace file
{
	using std::string; using std::fstream; using std::ifstream; using std::ofstream;
	bool fileExists(string filename);
	std::string readFile(const char* filename);
	uint fileSize(string filename);
}
