#include "functions.h"

void help(char* p)
{
	cout << "\
Encode\tFirst Stage:" << endl << p << " --encode --stage 1 [--preview] [--source SOURCE] [--predictor PREDICTOR] INPUTFILE [ENCODEDFILE]" << endl << endl << "\
Encode\tSecond Stage: " << endl << p << " --encode --stage 2 [--preview] [--source SOURCE] [--predictor PREDICTOR] [--blocksize BLOCKSIZE]" << endl << "\
\t\t[--thresholdmode THRESHOLDMODE] [--diff DIFF] [--base_t BASE_THRESHOLD] INPUTFILE [ENCODEDFILE]" << endl << "\
Encode\tThird Stage: " << endl << p << " --encode --stage 3 [--preview] [--source SOURCE] [--predictor PREDICTOR] [--blocksize BLOCKSIZE]" << endl << "\
\t\t[--searchmode SEARCHMODE] [--searcharea SEARCHAREA] [--periodicity PERIODICITY]" << endl << "\
\t\t[--thresholdmode THRESHOLDMODE] [--diff DIFF] [--base_t BASE_THRESHOLD] [--ceil_t CEIL_THRESHOLD] [--min_t MIN_THRESHOLD] INPUTFILE [ENCODEDFILE]" << endl << endl << "\
Encode\tFourth Stage: " << endl << p << " --encode --stage 4 [--preview] [--source SOURCE] [--predictor PREDICTOR] [--blocksize BLOCKSIZE]" << endl << "\
\t\t[--searchmode SEARCHMODE] [--searcharea SEARCHAREA] [--periodicity PERIODICITY]" << endl << "\
\t\t[--thresholdmode THRESHOLDMODE] [--diff DIFF] [--base_t BASE_THRESHOLD] [--ceil_t CEIL_THRESHOLD] [--min_t MIN_THRESHOLD]" << endl << "\
\t\t[--Kb Kb] [--Kr Kr] INPUTFILE [ENCODEDFILE]" << endl << "\
" << endl << "\
Decode: " << p << " --decode [--stage STAGE] ENCODEDFILE [OUTPUTFILE]" << endl << "\
" << endl << "\
 Options:" << endl << "\
  --encode     | -e  Encode mode" << endl << "\
  --decode     | -d  Decode mode" << endl << "\
  --stage            Stage [1-4] (default: 1)" << endl << "\
  --source           Source [r: RGB file | c: CAM | m: MOVIE] (default: r)" << endl << "\
  --predictor  | -p  JPEG Predictor Mode [0-7](default: 6)" << endl << "\
  --blocksize  | -l  Size of the block 'n x n' to encode (default: 8)" << endl << "\
  --searcharea | -a  Area '(2*a*n)^2' to search for a block (default: 4)" << endl << "\
  --searchmode       Search mode [s: Spiral Search (faster) | n: Normal Search] (default: s)" << endl << "\
  --periodicity      Periodicity of Intra-frame (default: -1 <-- video fps)" << endl << "\
  --thresholdmode    Threshold mode [r: Normal Calc (faster) | t: Ignoring differences below 'diff'] (default: r)" << endl << "\
  --diff             Diff variable (default: 1.0)" << endl << "\
  --base_t     | -t  Base threshold to encode a block (less is better quality) (default: 12.0)" << endl << "\
  --ceil_t     | -c  Ceil threshold to accept a block (less is better quality) (default: 4.0)" << endl << "\
  --min_t      | -m  Min threshold to search for a block (less is better quality) (default: 40.0)" << endl << "\
  --Kb         | -b  Kb used in convertion from RGB to YCbCr (default: 0.114)" << endl << "\
  --Kr         | -r  Kr used in convertion from RGB to YCbCr (default: 0.299)" << endl << "\
\n\
  --help            usage message" << endl << "\
  --version         version number and copyright" << endl;
}

void version()
{
	cout << "Video Codec version 2.0\n\
\n\
Copyright (C) 2016\n\
Pedro Abade (abade.p@ua.pt) nº 59385\n\
Claudio Patricio <cpatricio@ua.pt> nº 73284\n\
\n\
CAV @ ECT\n\
Universidade de Aveiro" << endl;
}

// Random function with double
double frand(double fmin, double fmax)
{
	double f = (double)std::rand() / RAND_MAX;
	return fmin + f * (fmax - fmin);
}


// Integer to String
string itoa(int n)
{
	return boost::lexical_cast<std::string>(n);
}

// Clear console
void clr()
{
	std::cout << "\033[2J\033[1;1H";
}

// Clear Stdin
void clrstdin()
{
	std::cin.clear();
	std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
	fflush(stdin);
	//std::cin.ignore(1,'\0');
	fflush(stdout);
}

// Wait for Enter to Continue
void PressEnterToContinue()
{
	int c=1;
	clrstdin();
	std::cout << "Press ENTER to continue... ";
	do c = getchar(); while (c != 10);
}


