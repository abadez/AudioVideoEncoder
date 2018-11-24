#pragma once

#include <opencv2/opencv.hpp>

namespace VideoCoding
{
	class StageCoding
	{
		public:
			virtual cv::Mat encode(cv::Mat* frame, uint fps = 0) = 0;
			virtual bool decode(cv::Mat* frame) = 0;
			virtual void encodeEnded() = 0;
			virtual uint getFPS() = 0;
	};
}
