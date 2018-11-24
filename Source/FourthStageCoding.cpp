#include "FourthStageCoding.h"

namespace VideoCoding
{
	FourthVideoEnc::FourthVideoEnc(string ifname, string ofname, char mode, uint predictMode, uint blockSize, uint searchArea, uint periodicity, char thresholdMode, double t, double base_threshold, double ceil_threshold, double min_threshold, char searchMode, double Kb, double Kr)
	{
		this->mode = mode;
		switch(mode)
		{
			case 'e':
				if(predictMode > 7) WrongPredictModeException(predictMode);
				if(searchArea > 1024) WrongModeException(searchArea);
				if(blockSize > 1024) WrongModeException(blockSize);
				if(periodicity >= 256) WrongModeException(periodicity);
				break;
			case 'd':
				break;
			default:
				WrongModeException(mode);
		}

        this->predictMode = predictMode;
		this->ifname = ifname;
		this->ofname = ofname;
		this->blockSize = blockSize;
		this->searchArea = searchArea;
		this->periodicity = periodicity;
		this->thresholdMode = thresholdMode;
		this->searchMode = searchMode;
		this->t = t;
		this->base_threshold = base_threshold;
		this->ceil_threshold = ceil_threshold;
		this->min_threshold = min_threshold;
		this->Kb = Kb;
		this->Kr = Kr;
		dummy = Mat::eye(blockSize, blockSize, CV_8UC3) * 255;
		BGR2YCrCb(&dummy);
	}

	FourthVideoEnc::~FourthVideoEnc()
	{
		save();
	}
	
	void FourthVideoEnc::BGR2YCrCb(Mat* frame)
	{
		uchar B, G, R;
		uchar Y, Cr, Cb;

		uchar* pixel = frame->ptr<uchar>(0);
		
		for(size_t pixels = frame->rows * frame->cols; pixels > 0 ; pixels--)
		{
			B = *pixel;
			G = *(pixel+1);
			R = *(pixel+2);
			Y = Kr * R + (1 - Kr - Kb) * G + Kb * B;
			Cb = 128 + 0.5 * (B - Y) / (1 - Kb);
			Cr = 128 + 0.5 * (R - Y) / (1 - Kr);
			// Using same order as opencv (YCrCb) instead of default (YCbCr)
			*pixel++ = Y; // Y
			*pixel++ = Cr; // Cr
			*pixel++ = Cb; // Cb
		}
	}
	
	void FourthVideoEnc::YCrCb2BGR(Mat* frame)
	{
		double B, G, R;
		uchar Y, Cr, Cb;

		uchar* pixel = frame->ptr<uchar>(0);
		
		
		for(size_t pixels = frame->rows * frame->cols; pixels > 0 ; pixels--)
		{
			Y = *pixel;
			Cr = *(pixel+1);
			Cb = *(pixel+2);
			B = 2 * (Cb - 128) * (1 - Kb) + Y;
			if(B < 0) B = 0; // fix colour distortion
			R = 2 * (Cr - 128) * (1 - Kr) + Y;
			if(R < 0) R = 0; // fix coulour distortion
			G = (Y - Kr * R - Kb * B) / (1 - Kr - Kb);
			if(G < 0) G = 0; // fix coulour distortion
			// Using same order as opencv (BGR) instead of default (RGB)
			*pixel++ = (uchar)B; // B
			*pixel++ = (uchar)G; // G
			*pixel++ = (uchar)R; // R
		}
	}
	
	void FourthVideoEnc::writeHeader(Mat* frame, uint fps)
	{
		if (golomb==NULL)
		{
			golomb.reset(new Golomb(ofname, 'e'));
		}
		rows = frame->rows;
		cols = frame->cols;
		
		golomb->writeHeader(frame->rows, 12);	// Height
		golomb->writeHeader(frame->cols, 12);	// Width
		golomb->writeHeader(fps, 6);			// FPS
		golomb->writeHeader(periodicity, 8);	// Periodicity
		golomb->writeHeader(blockSize - 1, 10);	// Blocksize (max 1024)
		golomb->writeHeader(searchArea - 1, 10); // Search perimeter (max 1024)
		golomb->writeHeader(frame->type(), 6);	// Frame type
		golomb->writeHeader((frame->type() - ((frame->channels() - 1) << 3)), 3);	// Channel type
		golomb->writeHeader(predictMode, 3);	// Predictor Mode
		union double2int {
			double d;
			uint64_t i;
		};
		double2int Kbi;
		double2int Kri;
		Kbi.d = Kb;
		Kri.d = Kr;
		golomb->writeHeader(Kbi.i, 64);	// Kb
		golomb->writeHeader(Kri.i, 64);	// Ki
	}
	
	double FourthVideoEnc::calcThreshold(Mat a, Mat b)
	{
		Mat diff;
		Scalar sdiff;
		Mat diffChannels[3];
		switch(thresholdMode)
		{
			case 't':
				cv::absdiff(a, b, diff);
				split(diff, diffChannels);
				cv::threshold(diffChannels[0], diffChannels[0], t, 255., CV_THRESH_TOZERO); // B (Only differences lower than 't' will be counted as equal)
				cv::threshold(diffChannels[1], diffChannels[1], t, 255., CV_THRESH_TOZERO); // G
				cv::threshold(diffChannels[2], diffChannels[2], t, 255., CV_THRESH_TOZERO); // R
				return (cv::sum(diffChannels[0])[0] + cv::sum(diffChannels[1])[0] + cv::sum(diffChannels[2])[0]) / (diff.cols * diff.rows);
			default: // r
				cv::absdiff(a, b, diff);
				sdiff = cv::sum(diff);
				return (sdiff[0] + sdiff[1] + sdiff[2]) / (diff.cols * diff.rows); // RMSE
		}
	}
	
	Point FourthVideoEnc::search(Mat s, Mat tpl, int x, int y, double* maxVal)
	{
		*maxVal = 1000.0;
		double aux;
		Point maxLoc;
		uint nrows = (uint)s.rows - (uint)tpl.rows;
		uint ncols = (uint)s.cols - (uint)tpl.cols;
		
		switch(searchMode)
		{
			case 'n':
				for(uint l = 0; l < nrows; l++)
				{
					for(uint c = 0; c < ncols; c++)
					{
						aux = calcThreshold(s(Rect(c, l, tpl.cols, tpl.rows)), tpl);
						if(aux < *maxVal)
						{
							*maxVal = aux;
							maxLoc = Point(c,l);
							if(*maxVal <= ceil_threshold) return maxLoc;
						}
					}
				}
				break;
			default: // Spiral
				uint direction = 0; // 0: x+, 1: x-, 2: y-, 3: y+
				int xmin = x;
				int xmax = x;
				int ymin = y;
				int ymax = y;
				
				for(uint i = nrows * ncols; i > 1; i--)
				{
					aux = calcThreshold(s(Rect(x, y, tpl.cols, tpl.rows)), tpl);
					if(aux < *maxVal)
					{
						*maxVal = aux;
						maxLoc = Point(x,y);
						if(*maxVal <= ceil_threshold) 
							return maxLoc;
					}
					
					while(true)
					{
						switch(direction)
						{
							case 0:
								if((int)x < xmax)
								{
									x++;
								}
								else
								{
									xmax++;
									x = xmax;
									direction = 2;
								}
								break;
							case 1:
								if((int)x > xmin)
								{
									x--;
								}
								else
								{
									xmin--;
									x = xmin;
									direction = 3;
								}
								break;
							case 2:
								if((int)y < ymax)
								{
									y++;
								}
								else
								{
									ymax++;
									y = ymax;
									direction = 1;
								}
								break;
							case 3:
								if((int)y > ymin)
								{
									y--;
								}
								else
								{
									ymin--;
									y = ymin;
									direction = 0;
								}
								break;
						}
						if(x >= 0 && x < (int)ncols && y >= 0 && y < (int)nrows)
							break;
					}
				}
		}
		return maxLoc;
	}
	
	Mat FourthVideoEnc::encode(Mat* frame, uint fps)
	{
		if(mode == 'd') WrongModeException(mode);
		BGR2YCrCb(frame);
		Mat buffer = frame->clone();
		if (golomb==NULL)
		{
			writeHeader(frame, fps);
		}
		
		if(lframe.empty() || (framecount == 0 && periodicity > 0)) // First, or periodicity when is not 0
		{
			encodeIntra(frame);
			if(lframe.empty())
			{
				mB = 2;
				mG = 2;
				mR = 2;
			}
			lframe = frame->clone();
		}
		else
		{
			if(calcThreshold(*frame, lframe) > min_threshold) // Scene change detection
			{
				golomb->writeHeader(1, 1);
				encodeIntra(frame);
				lframe = frame->clone();
				framecount = 0;
				//cout << "Scene change detection" << endl;
			}
			else
			{
				golomb->writeHeader(0, 1);
				Mat lBlock;
				Mat aBlock;
				
				Mat diff;
				
				uint w, h;
				uint minX, sX, minY, sY;
				uint x = blockSize + (searchArea << 2);
				uint y = x;
				Point p;
				double maxVal;
				for(uint l=0 ; l < rows ; l += blockSize)
				{
					for(uint c=0 ; c < cols ; c += blockSize)
					{
						w = blockSize - (((c + blockSize) % cols) % blockSize);
						h = blockSize - (((l + blockSize) % rows) % blockSize);

						if(c < searchArea)
						{
							minX = 0;
							sX = c + blockSize + searchArea;
							if(x >= sX - blockSize)
								x = c;
						}
						else if(c > (lframe.cols - blockSize - searchArea))
						{
							minX = c - searchArea;
							sX = lframe.cols - c + searchArea;
							if(x >= sX - blockSize)
								x = searchArea;
						}
						else
						{
							minX = c - searchArea;
							sX = searchArea * 2 + blockSize;
							if(x >= sX - blockSize)
								x = searchArea;
						}
						
						
						if(l < searchArea)
						{
							minY = 0;
							sY = l + blockSize + searchArea;
							if(y >= sY - blockSize)
								y = l;
						}
						else if(l > (lframe.rows - blockSize - searchArea))
						{
							minY = l - searchArea;
							sY = lframe.rows - l + searchArea;
							if(y >= sY - blockSize)
								y = searchArea;
						}
						else
						{
							minY = l - searchArea;
							sY = searchArea * 2 + blockSize;
							if(y >= sY - blockSize)
								y = searchArea;
						}
						
						if(calcThreshold((*frame)(Rect(c, l, w, h)), lframe((Rect(minX+x, minY+y, w, h)))) <= min_threshold) // should be similar
						{
							p = search(lframe(Rect(minX, minY, sX, sY)), (*frame)(Rect(c, l, w, h)), x, y, &maxVal);
							if(maxVal <= ceil_threshold)
							{
								x = p.x;
								y = p.y;
							}
							else // dirty trick to reset if not found
							{
								x = blockSize + (searchArea << 2);
								y = x;
							}
						}
						else
							maxVal = 1000.;
						
						if(maxVal <= base_threshold)
						{
							golomb->writeHeader(1, 1);
							golomb->writeHeader((uint)p.x, log2(searchArea * 2));
							golomb->writeHeader((uint)p.y, log2(searchArea * 2));
							lframe(Rect(minX + p.x, minY + p.y, w, h)).copyTo(buffer(Rect(c, l, w, h)));
							dummy(Rect(0, 0, w, h)).copyTo((*frame)(Rect(c, l, w, h)));
							blocks++;
						}
						else
						{
							golomb->writeHeader(0, 1);
							lBlock = lframe(Rect(c, l, w, h)).clone();		// block from previous frame
							aBlock = (*frame)(Rect(c, l, w, h)).clone();	// block from actual frame
							encodeInter(&aBlock, &lBlock);
						}
						totalBlocks++;
					}
				}
				lframe = buffer.clone();
			}
			
		}
		framecount = (framecount + 1) % periodicity;
		YCrCb2BGR(&buffer);
		YCrCb2BGR(frame);
		return buffer;
	}
	
	void FourthVideoEnc::encodeInter(Mat* aBlock, Mat* lBlock)
	{
		uchar aB, aG, aR;
		uchar lB, lG, lR;

		uchar* aPixel = aBlock->ptr<uchar>(0);
		uchar* lPixel = lBlock->ptr<uchar>(0);
		
		size_t pixels = aBlock->rows * aBlock->cols;
		for(; pixels > 0 ; pixels--)
		{
			aB = *aPixel++; aG = *aPixel++; aR = *aPixel++;
			lB = *lPixel++; lG = *lPixel++; lR = *lPixel++;
			
			golomb->encode(aB - lB, mB);
			golomb->encode(aG - lG, mG);
			golomb->encode(aR - lR, mR);

			// B Channel
			if(abs(aB - lB) > (mB << 1))
				mB <<= 1;
			else if(abs(aB - lB) < (mB >> 1))
				mB >>= 1;
			// G Channel
			if(abs(aG - lG) > (mG << 1))
				mG <<= 1;
			else if(abs(aG - lG) < (mG >> 1))
				mG >>= 1;
			// R Channel
			if(abs(aR - lR) > (mR << 1))
				mR <<= 1;
			else if(abs(aR - lR) < (mR >> 1))
				mR >>= 1;
		}
	}

	void FourthVideoEnc::encodeIntra(Mat* frame)
	{
		split(*frame, channels);
		predValue[0] = 0;
		predValue[1] = 0;
		predValue[2] = 0;

		mB = 128;
		mG = 128;
		mR = 128;
		for (int l=0 ; l < frame->rows ; l++)
		{
			for (int c=0 ; c < frame->cols ; c++)
			{
				switch(predictMode)
				{
					case 0: // non-linear predictor
							if (l==0 || c==0)
							{
								predValue[0] = 0;
								predValue[1] = 0;
								predValue[2] = 0;
							}
							else
							{
								for (uint color=0 ; color<3 ; color++)
								{
									// min(a,b) if c>=max(a,b)
									if (channels[color].at<uchar>(l-1,c-1) >= max(channels[color].at<uchar>(l,c-1),channels[color].at<uchar>(l-1,c)))
										predValue[color] = min(channels[color].at<uchar>(l,c-1),channels[color].at<uchar>(l-1,c));
									// max(a,b) if c<=min(a,b)
									else if (channels[color].at<uchar>(l-1,c-1) <= min(channels[color].at<uchar>(l,c-1),channels[color].at<uchar>(l-1,c)))
										predValue[color] = max(channels[color].at<uchar>(l,c-1),channels[color].at<uchar>(l-1,c));
									// a+b-c
									else
										predValue[color] = channels[color].at<uchar>(l,c-1) + channels[color].at<uchar>(l-1,c) - channels[color].at<uchar>(l-1,c-1);
								}
							}
							break;
					case 1: // a
							if (c==0)
							{
								predValue[0] = 0;
								predValue[1] = 0;
								predValue[2] = 0;
							}
							else
							{
								predValue[0] = channels[0].at<uchar>(l, c-1);
								predValue[1] = channels[1].at<uchar>(l, c-1);
								predValue[2] = channels[2].at<uchar>(l, c-1);
							}
							break;
					case 2: // b
							if (l==0)
							{
								predValue[0] = 0;
								predValue[1] = 0;
								predValue[2] = 0;
							}
							else
							{
								predValue[0] = channels[0].at<uchar>(l-1,c);
								predValue[1] = channels[1].at<uchar>(l-1,c);
								predValue[2] = channels[2].at<uchar>(l-1,c);
							}
							break;
					case 3: // c
							if (l==0 || c==0)
							{
								predValue[0] = 0;
								predValue[1] = 0;
								predValue[2] = 0;
							}
							else
							{
								predValue[0] = channels[0].at<uchar>(l-1,c-1);
								predValue[1] = channels[1].at<uchar>(l-1,c-1);
								predValue[2] = channels[2].at<uchar>(l-1,c-1);
							}
							break;
					case 4: // a+b-c
							if (l==0 || c==0)
							{
								predValue[0] = 0;
								predValue[1] = 0;
								predValue[2] = 0;
							}
							else
							{
								predValue[0] = channels[0].at<uchar>(l,c-1) + channels[0].at<uchar>(l-1,c) - channels[0].at<uchar>(l-1,c-1);
								predValue[1] = channels[1].at<uchar>(l,c-1) + channels[1].at<uchar>(l-1,c) - channels[1].at<uchar>(l-1,c-1);
								predValue[2] = channels[2].at<uchar>(l,c-1) + channels[2].at<uchar>(l-1,c) - channels[2].at<uchar>(l-1,c-1);
							}
							break;
					case 5: // a+(b-c)/2
							if (l==0 || c==0)
								predValue[0] = predValue[1] = predValue[2] = 0;
							else
							{
								predValue[0] = channels[0].at<uchar>(l,c-1) + (channels[0].at<uchar>(l-1,c) - channels[0].at<uchar>(l-1,c-1))/2;
								predValue[1] = channels[1].at<uchar>(l,c-1) + (channels[1].at<uchar>(l-1,c) - channels[1].at<uchar>(l-1,c-1))/2;
								predValue[2] = channels[2].at<uchar>(l,c-1) + (channels[2].at<uchar>(l-1,c) - channels[2].at<uchar>(l-1,c-1))/2;
							}
							break;
					case 6: // b+(a-c)/2
							if (l==0 || c==0)
							{
								predValue[0] = 0;
								predValue[1] = 0;
								predValue[2] = 0;
							}
							else
							{
								predValue[0] = channels[0].at<uchar>(l-1,c) + (channels[0].at<uchar>(l,c-1) - channels[0].at<uchar>(l-1,c-1)) / 2;
								predValue[1] = channels[1].at<uchar>(l-1,c) + (channels[1].at<uchar>(l,c-1) - channels[1].at<uchar>(l-1,c-1)) / 2;
								predValue[2] = channels[2].at<uchar>(l-1,c) + (channels[2].at<uchar>(l,c-1) - channels[2].at<uchar>(l-1,c-1)) / 2;
							}
							break;
					case 7: // (a+b)/2
							if (l==0 || c==0)
							{
								predValue[0] = 0;
								predValue[1] = 0;
								predValue[2] = 0;
							}
							else
							{
								predValue[0] = (channels[0].at<uchar>(l,c-1) + channels[0].at<uchar>(l-1,c)) / 2;
								predValue[1] = (channels[1].at<uchar>(l,c-1) + channels[1].at<uchar>(l-1,c)) / 2;
								predValue[2] = (channels[2].at<uchar>(l,c-1) + channels[2].at<uchar>(l-1,c)) / 2;
							}
							break;
					default:
							WrongPredictModeException(predictMode);
				}
				
				golomb->encode(channels[0].at<uchar>(l,c) - predValue[0], mB);
				golomb->encode(channels[1].at<uchar>(l,c) - predValue[1], mG);
				golomb->encode(channels[2].at<uchar>(l,c) - predValue[2], mR);

				// B Channel
				if(abs(channels[0].at<uchar>(l,c) - predValue[0]) > (mB << 1))
					mB <<= 1;
				else if(abs(channels[0].at<uchar>(l,c) - predValue[0]) < (mB >> 1))
					mB >>= 1;
				// G Channel
				if(abs(channels[1].at<uchar>(l,c) - predValue[1]) > (mG << 1))
					mG <<= 1;
				else if(abs(channels[1].at<uchar>(l,c) - predValue[1]) < (mG >> 1))
					mG >>= 1;
				// R Channel
				if(abs(channels[2].at<uchar>(l,c) - predValue[2]) > (mR << 1))
					mR <<= 1;
				else if(abs(channels[2].at<uchar>(l,c) - predValue[2]) < (mR >> 1))
					mR >>= 1;
			}
		}
	}

	bool FourthVideoEnc::decode(Mat* frame)
	{
		if(mode == 'e') WrongModeException(mode);
		if(golomb==NULL)
		{
			golomb.reset(new Golomb(ifname, 'd'));
			if(!golomb->readHeader(&rows, 12)) WrongModeException('d');
			if(!golomb->readHeader(&cols, 12)) WrongModeException('d');
			if(!golomb->readHeader(&fps, 6)) WrongModeException('d');
			if(!golomb->readHeader(&periodicity, 8)) WrongModeException('d');
			if(!golomb->readHeader(&blockSize, 10)) WrongModeException('d'); // Blocksize (max 1024)
			if(!golomb->readHeader(&searchArea, 10)) WrongModeException('d'); // searchArea (max 1024)
			++blockSize;
			++searchArea;
			if(!golomb->readHeader(&frameType, 6)) WrongModeException('d');
			if(!golomb->readHeader(&channelType, 3)) WrongModeException('d');
			if(!golomb->readHeader(&predictMode, 3)) WrongModeException('d');
			
			union int2double {
				uint64_t i;
				double d;
			};
			int2double Kbi;
			int2double Kri;
			if(!golomb->readHeader(&Kbi.i, 64)) WrongModeException('d');
			if(!golomb->readHeader(&Kri.i, 64)) WrongModeException('d');
			Kb = Kbi.d;
			Kr = Kri.d;
		}
		if(lframe.empty() || framecount == 0)
		{
			decodeIntra(frame);
			if(lframe.empty())
			{
				mB = 2;
				mG = 2;
				mR = 2;
			}
			lframe = frame->clone();
		}
		else
		{
			uint decintra = 0;
			if(!golomb->readHeader(&decintra, 1)) return false;
			if(decintra == 1)
			{
				if(!decodeIntra(frame)) return false;
				lframe = frame->clone();
				framecount = 0;
			}
			else
			{
				Mat lBlock, aBlock;
				Mat buffer;
				uint w, h;
				uint block_decision;
				uint x, y;
				uint minX, minY;
				
				*frame = Mat::zeros(rows, cols, frameType);
				for(uint l=0 ; l < rows ; l += blockSize)
				{
					for(uint c=0 ; c < cols ; c += blockSize)
					{
						w = blockSize - (((c + blockSize) % cols) % blockSize);
						h = blockSize - (((l + blockSize) % rows) % blockSize);
						lBlock = Mat::zeros(w, h, frameType);
						lBlock = lframe(Rect(c, l, w, h)).clone(); // block from previous frame
						
						if(golomb->eof()) return false;
						if(!golomb->readHeader(&block_decision, 1)) WrongModeException('d');
						
						if(block_decision == 1)
						{
							if(!golomb->readHeader(&x, log2(searchArea * 2))) return false;
							if(!golomb->readHeader(&y, log2(searchArea * 2))) return false;
							if(c < searchArea)
							{
								minX = 0;
							}
							else
							{
								minX = c - searchArea;
							}
							
							
							if(l < searchArea)
							{
								minY = 0;
							}
							else
							{
								minY = l - searchArea;
							}
							lframe(Rect(minX + x, minY + y, w, h)).copyTo(aBlock);
						}
						else
						{
							
							aBlock = Mat::zeros(w, h, frameType); // block from actual frame
							if(!decodeInter(&aBlock, &lBlock)) { return false; }
						}
						aBlock.copyTo((*frame)(Rect(c, l, w, h)));
						
						
					}
				}
				
				lframe = frame->clone();
			}
		}
		framecount = (framecount + 1) % periodicity;
		YCrCb2BGR(frame);
		return true;
	}

	bool FourthVideoEnc::decodeIntra(Mat* frame)
	{
		channels[0] = Mat::zeros(rows, cols, channelType);
		channels[1] = Mat::zeros(rows, cols, channelType);
		channels[2] = Mat::zeros(rows, cols, channelType);

		mB = 128;
		mG = 128;
		mR = 128;
		
		decodedValue[0] = 0;
		decodedValue[1] = 0;
		decodedValue[2] = 0;
		
		for (uint l=0 ; l < rows ; l++)
		{
			for (uint c=0 ; c < cols ; c++)
			{
				if(!golomb->decode(&decodedValue[0], mB) || !golomb->decode(&decodedValue[1], mG) || !golomb->decode(&decodedValue[2], mR)) { save(); return false; }

				switch(predictMode)
				{
					case 0: // non-linear predictor
							if (l==0 || c==0)
							{
								predValue[0] = 0;
								predValue[1] = 0;
								predValue[2] = 0;
							}
							else
							{
								for (uint color=0 ; color<3 ; color++)
								{
									// min(a,b) if c>=max(a,b)
									if (channels[color].at<uchar>(l-1,c-1) >= max(channels[color].at<uchar>(l,c-1),channels[color].at<uchar>(l-1,c)))
										predValue[color] = min(channels[color].at<uchar>(l,c-1),channels[color].at<uchar>(l-1,c));
									// max(a,b) if c<=min(a,b)
									else if (channels[color].at<uchar>(l-1,c-1) <= min(channels[color].at<uchar>(l,c-1),channels[color].at<uchar>(l-1,c)))
										predValue[color] = max(channels[color].at<uchar>(l,c-1),channels[color].at<uchar>(l-1,c));
									// a+b-c
									else
										predValue[color] = channels[color].at<uchar>(l,c-1) + channels[color].at<uchar>(l-1,c) - channels[color].at<uchar>(l-1,c-1);
								}
							}
							break;
					case 1: // a
							if (c==0)
							{
								predValue[0] = 0;
								predValue[1] = 0;
								predValue[2] = 0;
							}    
							else
							{
								predValue[0] = channels[0].at<uchar>(l, c-1);
								predValue[1] = channels[1].at<uchar>(l, c-1);
								predValue[2] = channels[2].at<uchar>(l, c-1);
							}
							break;
					case 2: // b
							if (l==0)
							{
								predValue[0] = 0;
								predValue[1] = 0;
								predValue[2] = 0;
							}
							else
							{
								predValue[0] = channels[0].at<uchar>(l-1,c);
								predValue[1] = channels[1].at<uchar>(l-1,c);
								predValue[2] = channels[2].at<uchar>(l-1,c);
							}
							break;
					case 3: // c
							if (l==0 || c==0)
							{
								predValue[0] = 0;
								predValue[1] = 0;
								predValue[2] = 0;
							}
							else
							{
								predValue[0] = channels[0].at<uchar>(l-1,c-1);
								predValue[1] = channels[1].at<uchar>(l-1,c-1);
								predValue[2] = channels[2].at<uchar>(l-1,c-1);
							}
							break;
					case 4: // a+b-c
							if (l==0 || c==0)
							{
								predValue[0] = 0;
								predValue[1] = 0;
								predValue[2] = 0;
							}
							else
							{
								predValue[0] = channels[0].at<uchar>(l,c-1) + channels[0].at<uchar>(l-1,c) - channels[0].at<uchar>(l-1,c-1);
								predValue[1] = channels[1].at<uchar>(l,c-1) + channels[1].at<uchar>(l-1,c) - channels[1].at<uchar>(l-1,c-1);
								predValue[2] = channels[2].at<uchar>(l,c-1) + channels[2].at<uchar>(l-1,c) - channels[2].at<uchar>(l-1,c-1);
							}
							break;
					case 5: // a+(b-c)/2
							if (l==0 || c==0)
							{
								predValue[0] = 0;
								predValue[1] = 0;
								predValue[2] = 0;
							}
							else
							{
								predValue[0] = channels[0].at<uchar>(l,c-1) + (channels[0].at<uchar>(l-1,c) - channels[0].at<uchar>(l-1,c-1))/2;
								predValue[1] = channels[1].at<uchar>(l,c-1) + (channels[1].at<uchar>(l-1,c) - channels[1].at<uchar>(l-1,c-1))/2;
								predValue[2] = channels[2].at<uchar>(l,c-1) + (channels[2].at<uchar>(l-1,c) - channels[2].at<uchar>(l-1,c-1))/2;
							}
							break;
					case 6: // b+(a-c)/2
							if (l==0 || c==0)
							{
								predValue[0] = 0;
								predValue[1] = 0;
								predValue[2] = 0;
							}
							else
							{
								predValue[0] = channels[0].at<uchar>(l-1,c) + (channels[0].at<uchar>(l,c-1) - channels[0].at<uchar>(l-1,c-1))/2;
								predValue[1] = channels[1].at<uchar>(l-1,c) + (channels[1].at<uchar>(l,c-1) - channels[1].at<uchar>(l-1,c-1))/2;
								predValue[2] = channels[2].at<uchar>(l-1,c) + (channels[2].at<uchar>(l,c-1) - channels[2].at<uchar>(l-1,c-1))/2;
							}
							break;
					case 7: // (a+b)/2
							if (l==0 || c==0)
							{
								predValue[0] = 0;
								predValue[1] = 0;
								predValue[2] = 0;
							}
							else
							{
								predValue[0] = (channels[0].at<uchar>(l,c-1) + channels[0].at<uchar>(l-1,c))/2;
								predValue[1] = (channels[1].at<uchar>(l,c-1) + channels[1].at<uchar>(l-1,c))/2;
								predValue[2] = (channels[2].at<uchar>(l,c-1) + channels[2].at<uchar>(l-1,c))/2;
							}
							break;
					default:
							WrongPredictModeException(predictMode);
				}

				channels[0].at<uchar>(l,c) = decodedValue[0] + predValue[0];
				channels[1].at<uchar>(l,c) = decodedValue[1] + predValue[1];
				channels[2].at<uchar>(l,c) = decodedValue[2] + predValue[2];

				// B Channel
				if(abs(decodedValue[0]) > (mB << 1))
					mB <<= 1;
				else if(abs(decodedValue[0]) < (mB >> 1))
					mB >>= 1;
				// G Channel
				if(abs(decodedValue[1]) > (mG << 1))
					mG <<= 1;
				else if(abs(decodedValue[1]) < (mG >> 1))
					mG >>= 1;
				// R Channel
				if(abs(decodedValue[2]) > (mR << 1))
					mR <<= 1;
				else if(abs(decodedValue[2]) < (mR >> 1))
					mR >>= 1;
			}
		}

		vector<Mat> array_to_merge;
		array_to_merge.push_back(channels[0]);
		array_to_merge.push_back(channels[1]);
		array_to_merge.push_back(channels[2]);
		
		*frame = Mat::zeros(rows, cols, frameType);
		
		merge(array_to_merge, *frame);

		return true;
	}
	
	bool FourthVideoEnc::decodeInter(Mat* aBlock, Mat* lBlock)
	{
		*aBlock = Mat::zeros(lBlock->rows, lBlock->cols, frameType);
		uchar* aPixel = aBlock->ptr<uchar>(0);
		uchar* lPixel = lBlock->ptr<uchar>(0);
		size_t pixels = lBlock->rows * lBlock->cols;
		for(; pixels > 0 ; pixels--)
		{
			if(!golomb->decode(&decodedValue[0], mB) || !golomb->decode(&decodedValue[1], mG) || !golomb->decode(&decodedValue[2], mR)) { save(); return false; }

			*aPixel++ = decodedValue[0] + *lPixel++;
			*aPixel++ = decodedValue[1] + *lPixel++;
			*aPixel++ = decodedValue[2] + *lPixel++;

			// B Channel
			if(abs(decodedValue[0]) > (mB << 1))
				mB <<= 1;
			else if(abs(decodedValue[0]) < (mB >> 1))
				mB >>= 1;
			// G Channel
			if(abs(decodedValue[1]) > (mG << 1))
				mG <<= 1;
			else if(abs(decodedValue[1]) < (mG >> 1))
				mG >>= 1;
			// R Channel
			if(abs(decodedValue[2]) > (mR << 1))
				mR <<= 1;
			else if(abs(decodedValue[2]) < (mR >> 1))
				mR >>= 1;
		}
		return true;
	}
	
	uint FourthVideoEnc::getFPS()
	{
		return fps;
	}

	void FourthVideoEnc::encodeEnded()
	{
		cout << "Blocks not encoded: " << blocks << " (" << std::setprecision(4) << (float)blocks * 100 / totalBlocks << "%)" << endl;
		save();
	}

	void FourthVideoEnc::save()
	{
		if(golomb != NULL)
			golomb->close();
	}
}
