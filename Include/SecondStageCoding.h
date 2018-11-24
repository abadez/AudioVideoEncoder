#pragma once

#include <opencv2/opencv.hpp>
#include <iomanip>
#include "StageCoding.h"
#include "golomb.h"
#include "file.h"

namespace VideoCoding
{
	using std::cin; using std::cout; using std::endl;
	using namespace cv;
	class SecondVideoEnc : public virtual StageCoding
	{
		public:
			SecondVideoEnc(string ifname, string ofname, char mode, uint predictMode=6, uint blockSize=8, char thresholdMode='r', double t=1., double base_threshold=12.);
			~SecondVideoEnc();

			Mat encode(Mat* frame, uint fps = 0);
			bool decode(Mat* frame);
			void encodeEnded();
			uint getFPS();
		private:
			void writeHeader(Mat* frame, uint fps);
			void save();
			double calcThreshold(Mat a, Mat b);
			void encodeIntra(Mat* frame);
			void encodeInter(Mat* aBlock, Mat* lBlock);
			bool decodeIntra(Mat* frame);
			bool decodeInter(Mat* aBlock, Mat* lBlock);
			uint predictMode;
			char mode;
			char thresholdMode;
			std::unique_ptr<VideoCoding::Golomb> golomb;
			string ifname;
			string ofname;
			uint blockSize;
			bool endDecode;
			bool endEncode;
			Mat lframe;
			Mat channels[3]; // 0 - blue ; 1 - green ; 2 - red
			int predValue[3]; // 0 - blue ; 1 - green ; 2 - red
			int decodedValue[3]; // 0 - blue ; 1 - green ; 2 - red
			uint mB, mG, mR;
			uint rows, cols;
			uint fps;
			uint frameType, channelType;
			uint blocks = 0;
			uint totalBlocks = 0;
			double t;
			double base_threshold;
			Mat dummy; // Dummy mat to show blocks not encoded.
	};
}
