#include "videoCmp.h"

int main(int argc, char* argv[])
{
	/*namedWindow("Video1", WINDOW_NORMAL);
	resizeWindow("Video1", 600, 600);
	namedWindow("Video2", WINDOW_NORMAL);
	resizeWindow("Video2", 600, 600);*/
	
	if(argc != 3)
	{
		std::cout << "Usage: " << argv[0] << " <video1.rgb> <video2.rgb>" << std::endl;
		return 1;
	}
	
	VideoCoding::RGBReader v1(argv[1]);
	VideoCoding::RGBReader v2(argv[2]);
	
	cv::Mat frame1, frame2, diff;
	unsigned int A = 255 * 255; // 8 bit
	double mse = 0.0;
	cv::Scalar s;
	double psnr = 0.0;
	double total_psnr = 0.0;
	double total_rmse = 0.0;
	unsigned int frames = 0;
	unsigned int frames_counted = 0;
	
	while(v1.read(frame1))
	{
		v2.read(frame2);
		
		/*imshow("Video1", frame1);
		imshow("Video2", frame2);*/
		
		cv::absdiff(frame1, frame2, diff);
		cv::pow(diff, 2., diff);
		s = cv::sum(diff);
		mse = (double)s[0] + (double)s[1] + (double)s[2];
		mse /= frame2.cols * frame2.rows * 3;
		psnr = 10. * log10((double)A / mse);
		if(psnr < std::numeric_limits<double>::max())
		{
			total_psnr += psnr;
			total_rmse += sqrt(mse);
			frames_counted++;
		}
		std::cout << "PSNR frame" << frames << ": " << psnr << " (RMSE: " << sqrt(mse) << ")" << std::endl;
		frames++;
		
		//waitKey(0);
	}
	
	
	std::cout << std::endl <<"PSNR: " << total_psnr / frames_counted << " (RMSE: " << total_rmse / frames << "|" << total_rmse / frames_counted << ")" << std::endl;
	return 0;
}
