#pragma once

#include <opencv2/opencv.hpp>
#include <fstream>
#include <iostream>
#include "file.h"
#include "exception.h"

namespace VideoCoding
{
	using std::cin; using std::cout; using std::endl;
	using std::string;
	using std::fstream; using std::ofstream; using std::ifstream;
	using std::vector;
	using namespace cv;
	class RGBStream : public VideoCapture
	{
		public:
			RGBStream() : VideoCapture() { }
			virtual bool write(Mat frame, uint fps = 0) { return false; };
			virtual uint getFPS(void) { return (uint)get(CV_CAP_PROP_FPS); };
	};
	class RGBReader : public RGBStream
	{
		public:
			RGBReader(string ifname);
			~RGBReader();

			bool read(OutputArray frame);
			uint getFPS();
			void release();
		private:
			ifstream fhandle;
			string ifname;
			uint rows;
			uint cols;
			uint fps;
			char color[3];
	};
	class RGBWriter : public RGBStream
	{
		public:
			RGBWriter(string ofname);
			~RGBWriter();
			bool write(Mat frame, uint fps = 0);
			void release();
		private:
			ofstream fhandle;
			string ofname;
			uint frames = 0;
			uint rows;
			uint cols;
			uint fps = 0;
	};
	
	class RGBVideo : public RGBStream
	{
		public:
			RGBVideo() : RGBStream() { }
	};
}
