#pragma once

#include <opencv2/opencv.hpp>
#include "StageCoding.h"
#include "golomb.h"
#include "file.h"

namespace VideoCoding
{
	using namespace cv;
	class FirstVideoEnc : public virtual StageCoding
	{
		public:
			FirstVideoEnc(string ifname, string ofname, char mode, uint predictMode=6);
			~FirstVideoEnc();

			Mat encode(Mat* frame, uint fps = 0);
			bool decode(Mat* frame);
			void encodeEnded();
			uint getFPS();
		private:
			void writeHeader(Mat* frame, uint fps = 0);
			void save();
			uint predictMode;
			char mode;
			std::unique_ptr<VideoCoding::Golomb> golomb;
			string ifname;
			string ofname;
			bool endDecode;
			bool endEncode;
			Mat channels[3]; // 0 - blue ; 1 - green ; 2 - red
			int predValue[3]; // 0 - blue ; 1 - green ; 2 - red
			int decodedValue[3]; // 0 - blue ; 1 - green ; 2 - red
			uint mB, mG, mR;
			uint rows, cols;
			uint fps;
			uint frameType, channelType;
	};
}
