#pragma once

#include <getopt.h>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include "StageCoding.h"
#include "FirstStageCoding.h"
#include "SecondStageCoding.h"
#include "ThirdStageCoding.h"
#include "FourthStageCoding.h"
#include "rgbstream.h"

using std::cin; using std::cout; using std::endl;

namespace VideoCoding
{
	using namespace cv;
	class VideoEnc
	{
		public:
			VideoEnc(string ifname, string ofname, char mode, bool preview=false, uint stage=1, uint predictMode=6, uint blockSize=8, uint searchArea=4, int periodicity=-1, char thresholdMode='r', double t=1., double base_threshold=12., double ceil_threshold=4., double min_threshold=40., char searchMode='s', double Kb=0.114, double Kr=0.299);
			~VideoEnc();

			void capture();
			void watchVideo();
		private:
			std::unique_ptr<VideoCoding::StageCoding> fsc;
			std::unique_ptr<RGBStream> video;
			char mode;
			bool preview;
			uint predictMode;
	};
}
