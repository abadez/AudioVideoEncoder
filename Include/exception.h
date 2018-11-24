#pragma once
#include <iostream>
#include <stdexcept>
#include "functions.h"

namespace Exception
{
	using std::string; using std::runtime_error;
	
	class CustomException : public runtime_error // Class to create custom errors (exceptions). DO NOT EDIT!!!
	{
		public:
			CustomException(const char *file, int line) : runtime_error(string(file) + ":" + itoa(line) + ": error: An exception occured")
			{
				this->line = line;
				this->file = string(file);
			}
			~CustomException() throw() { }
			virtual const int ret() const throw() { return 1; }
			virtual const string getFile() { return file; }
			virtual const int getLine() { return line; }
		private:
			int line;
			string file;
	};
	
	class FileNotFound : public CustomException
	{
		public:
			FileNotFound(const string fname, const char *file, int line) : CustomException(file, line)
			{
				this->msg = string(file) + ":" + itoa(line) + ": error: File '" + fname + "' not found!!!";
				this->fname = fname;
			}
			~FileNotFound() throw() { }
			virtual const char* what() const throw() { return msg.c_str(); }
			
		private:
			string msg;
			string fname;
	};
	
	class AccessDenied : public CustomException
	{
		public:
			AccessDenied(const string fname, const char *file, int line) : CustomException(file, line)
			{
				this->msg = string(file) + ":" + itoa(line) + ": error: Access denied to file '" + fname + "'!!!";
				this->fname = fname;
			}
			~AccessDenied() throw() { }
			virtual const char* what() const throw() { return msg.c_str(); }
			
		private:
			string msg;
			string fname;
	};
	
	class WrongMode : public CustomException
	{
		public:
			WrongMode(const char mode, const char *file, int line) : CustomException(file, line)
			{
				this->msg = string(file) + ":" + itoa(line) + ": error: Mode '" + mode + "' is not valid or you are trying to do an invalid operation!!!";
				this->mode = mode;
			}
			~WrongMode() throw() { }
			virtual const char* what() const throw() { return msg.c_str(); }
			
		private:
			string msg;
			string mode;
	};
	
	class WrongM : public CustomException
	{
		public:
			WrongM(unsigned int m, const char *file, int line) : CustomException(file, line)
			{
				this->msg = string(file) + ":" + itoa(line) + ": error: M '" + itoa(m) + "' is not valid, M should be greater than 0 and less then 16385!!!";
				this->m = m;
			}
			~WrongM() throw() { }
			virtual const char* what() const throw() { return msg.c_str(); }
			
		private:
			string msg;
			unsigned int m;
	};
	
	class WrongOrder : public CustomException
	{
		public:
			WrongOrder(unsigned int m, const char *file, int line) : CustomException(file, line)
			{
				this->msg = string(file) + ":" + itoa(line) + ": error: Order '" + itoa(m) + "' is not valid, order should be greater or equal to 0 and less or equal to 12!!!";
				this->m = m;
			}
			~WrongOrder() throw() { }
			virtual const char* what() const throw() { return msg.c_str(); }
			
		private:
			string msg;
			unsigned int m;
	};
	
	class WrongPredictMode : public CustomException
	{
		public:
			WrongPredictMode(unsigned int mode, const char *file, int line) : CustomException(file, line)
			{
				this->msg = string(file) + ":" + itoa(line) + ": error: Predict Mode '" + itoa(mode) + "' is not valid or you are trying to do an invalid operation!!!";
				this->mode = mode;
			}
			~WrongPredictMode() throw() { }
			virtual const char* what() const throw() { return msg.c_str(); }
			
		private:
			string msg;
			unsigned int mode;
	};

	class Histogram : public CustomException
	{
		public:
			Histogram(const char *error, const char *file, int line) : CustomException(file, line)
			{
				this->msg = string(file) + ":" + itoa(line) + ": error: " + string(error);
			}
			~Histogram() throw() { }
			virtual const char* what() const throw() { return msg.c_str(); }
			
		private:
			string msg;
	};
	
	class WebCam : public CustomException
	{
		public:
			WebCam(const char *error, const char *file, int line) : CustomException(file, line)
			{
				this->msg = string(file) + ":" + itoa(line) + ": error: " + string(error);
			}
			~WebCam() throw() { }
			virtual const char* what() const throw() { return msg.c_str(); }
			
		private:
			string msg;
	};
}
#define FileNotFoundException(arg) throw Exception::FileNotFound(arg, __FILE__, __LINE__);
#define AccessDeniedException(arg) throw Exception::AccessDenied(arg, __FILE__, __LINE__);
#define WrongModeException(arg) throw Exception::WrongMode(arg, __FILE__, __LINE__);
#define WrongMException(arg) throw Exception::WrongM(arg, __FILE__, __LINE__);
#define WrongOrderException(arg) throw Exception::WrongOrder(arg, __FILE__, __LINE__);
#define WrongPredictModeException(arg) throw Exception::WrongOrder(arg, __FILE__, __LINE__);
#define HistogramException(arg) throw Exception::Histogram(arg, __FILE__, __LINE__);
#define WebCamException(arg) throw Exception::WebCam(arg, __FILE__, __LINE__);
