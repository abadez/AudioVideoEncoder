#pragma once
#include <iostream>
#include <unistd.h>
#include <string>
#include <stdio.h>
#include <boost/lexical_cast.hpp>

using std::cin; using std::cout; using std::endl;
using std::string;

void help(char* p);
void version();
double frand(double fmin, double fmax);
std::string itoa(int n);
void clr();
void clrstdin();
void PressEnterToContinue();
