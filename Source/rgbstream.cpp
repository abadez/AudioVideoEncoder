#include "rgbstream.h"

namespace VideoCoding
{
	RGBReader::RGBReader(string ifname)
	{
		if(file::fileExists(ifname))
		{
			string line;
			fhandle.open(ifname);
			getline(fhandle, line);
			sscanf(line.c_str(), "%d %d %d %s", &cols, &rows, &fps, color);
		}
		else
		{
			FileNotFoundException(ifname);
		}
	}
	
	RGBReader::~RGBReader()
	{
		fhandle.close();
	}
	
	void RGBReader::release()
	{
		fhandle.close();
	}
	
	uint RGBReader::getFPS()
	{
		return fps;
	}
	
	bool RGBReader::read(OutputArray frame)
	{
		if(!fhandle || fhandle.peek() == EOF) return false;
		
		Mat aux(rows, cols, CV_8UC3);
		
		uchar* fPixel = aux.ptr<uchar>(0);
		for(size_t pixels = cols * rows; pixels > 0; pixels--)
		{
			*(fPixel+2) = fhandle.get(); // B
			*(fPixel+1) = fhandle.get(); // G
			*fPixel = fhandle.get(); // R
			fPixel += 3;
		}
		frame.assign(aux);
		return true;
	}
	
	
	RGBWriter::RGBWriter(string ofname)
	{
		fhandle.open(ofname, std::fstream::out | std::fstream::binary);
	}
	
	RGBWriter::~RGBWriter()
	{
		fhandle.close();
	}
	
	void RGBWriter::release()
	{
		fhandle.close();
	}
	
	bool RGBWriter::write(Mat frame, uint fps)
	{
		if(!fhandle) return false;
		if(frames == 0)
		{
			rows = frame.rows;
			cols = frame.cols;
			this->fps = fps;
			fhandle << cols << " " << rows << " " << fps << " rgb" << endl; // Write header
		}
		
		uchar* pixel = frame.ptr<uchar>(0);
		for(size_t pixels = cols * rows; pixels > 0; pixels--)
		{
			fhandle.put(*(pixel+2)); // R
			fhandle.put(*(pixel+1)); // G
			fhandle.put(*pixel); // B
			pixel += 3;
		}
		frames++;
		return true;
	}
}
