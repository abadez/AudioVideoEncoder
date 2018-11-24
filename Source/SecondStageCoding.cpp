#include "SecondStageCoding.h"

namespace VideoCoding
{
	SecondVideoEnc::SecondVideoEnc(string ifname, string ofname, char mode, uint predictMode, uint blockSize, char thresholdMode, double t, double base_threshold)
	{
		this->mode = mode;
		switch(mode)
		{
			case 'e':
				if (predictMode > 7) WrongPredictModeException(predictMode);
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
		this->base_threshold = base_threshold;
		this->thresholdMode = thresholdMode;
		this->t = t;
		dummy = Mat::eye(blockSize, blockSize, CV_8UC3)*255;
	}

	SecondVideoEnc::~SecondVideoEnc()
	{
		save();
	}
	
	void SecondVideoEnc::writeHeader(Mat* frame, uint fps)
	{
		if (golomb==NULL)
		{
			golomb.reset(new Golomb(ofname, 'e'));
		}
		rows = frame->rows;
		cols = frame->cols;
		
		golomb->writeHeader(frame->rows, 12);	// Height
		golomb->writeHeader(frame->cols, 12);	// Width
		golomb->writeHeader(fps, 6);				// FPS
		golomb->writeHeader(blockSize - 1, 10);	// Blocksize (max 1024)
		golomb->writeHeader(frame->type(), 6);	// Frame type
		golomb->writeHeader((frame->type() - ((frame->channels() - 1) << 3)), 3);	// Channel type
		golomb->writeHeader(predictMode, 3);	// Predictor Mode
	}
	
	double SecondVideoEnc::calcThreshold(Mat a, Mat b)
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
				return (cv::sum(diffChannels[0])[0] * 0.342 + cv::sum(diffChannels[1])[0] * 1.761 + cv::sum(diffChannels[2])[0] * 0.897) / (diff.cols * diff.rows); // Based on Y calc (0.299R + 0.587G + 0.114B)
			default: // r
				cv::absdiff(a, b, diff);
				sdiff = cv::sum(diff);
				return (sdiff[0] + sdiff[1] + sdiff[2]) / (diff.cols * diff.rows); // RMSE
				
		}
	}
	
	Mat SecondVideoEnc::encode(Mat* frame, uint fps)
	{
		if(mode == 'd') WrongModeException(mode);
		Mat buffer = frame->clone();
		if (golomb==NULL)
		{
			writeHeader(frame, fps);
		}
		
		if(lframe.empty())
		{
			encodeIntra(frame);
			mB = 2;
			mG = 2;
			mR = 2;
			lframe = frame->clone();
		}
		else
		{
			Mat lBlock;
			Mat aBlock;
			uint w, h;
			
			for(uint l=0 ; l < rows ; l += blockSize)
			{
				for(uint c=0 ; c < cols ; c += blockSize)
				{
					w = blockSize - (((c + blockSize) % cols) % blockSize);
					h = blockSize - (((l + blockSize) % rows) % blockSize);
					
					// Check for differences
					lBlock = lframe(Rect(c, l, w, h)).clone();		// block from previous frame
					aBlock = (*frame)(Rect(c, l, w, h)).clone();	// block from actual frame
					
					if(calcThreshold(aBlock.clone(), lBlock.clone()) <= base_threshold)
					{
						blocks++;
						golomb->writeHeader(1, 1);
						lBlock.copyTo(buffer(Rect(c, l, w, h)));
						dummy(Rect(0, 0, w, h)).copyTo((*frame)(Rect(c, l, w, h)));
					}
					else
					{
						golomb->writeHeader(0, 1);
						encodeInter(&aBlock, &lBlock);
					}
					totalBlocks++;
				}
			}
			
			lframe = buffer.clone();
		}
		return buffer;
	}
	
	void SecondVideoEnc::encodeInter(Mat* aBlock, Mat* lBlock)
	{
		uchar aB, aG, aR;
		uchar lB, lG, lR;

		for (int l=0 ; l < aBlock->rows ; l++)
		{
			uchar* aPixel = aBlock->ptr<uchar>(l);
			uchar* lPixel = lBlock->ptr<uchar>(l);
			for (int c=0 ; c < aBlock->cols ; c++)
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
	}

	void SecondVideoEnc::encodeIntra(Mat* frame)
	{
		split(*frame, channels);
		predValue[0] = 0;
		predValue[1] = 0;
		predValue[2] = 0;

		mB = 128;
		mG = 128;
		mR = 128;
		for (uint l=0 ; l<(uint)frame->rows ; l++)
		{
			for (uint c=0 ; c<(uint)frame->cols ; c++)
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

	bool SecondVideoEnc::decode(Mat* frame)
	{
		if(mode == 'e') WrongModeException(mode);
		if(golomb==NULL)
		{
			golomb.reset(new Golomb(ifname, 'd'));
			if(!golomb->readHeader(&rows, 12)) WrongModeException('d');
			if(!golomb->readHeader(&cols, 12)) WrongModeException('d');
			if(!golomb->readHeader(&fps, 6)) WrongModeException('d');
			if(!golomb->readHeader(&blockSize, 10)) WrongModeException('d'); // Blocksize (max 1024)
			++blockSize;
			if(!golomb->readHeader(&frameType, 6)) WrongModeException('d');
			if(!golomb->readHeader(&channelType, 3)) WrongModeException('d');
			if(!golomb->readHeader(&predictMode, 3)) WrongModeException('d');
		}
		if(lframe.empty())
		{
			decodeIntra(frame);
			mB = 2;
			mG = 2;
			mR = 2;
			lframe = frame->clone();
		}
		else
		{
			Mat lBlock, aBlock;
			uint w, h;
			uint block_decision;
			
			*frame = Mat::zeros(rows, cols, frameType);
			for(uint l=0 ; l < rows ; l += blockSize)
			{
				for(uint c=0 ; c < cols ; c += blockSize)
				{
					w = blockSize - (((c + blockSize) % cols) % blockSize);
					h = blockSize - (((l + blockSize) % rows) % blockSize);
					lBlock = Mat::zeros(w, h, frameType);
					lBlock = lframe(Rect(c, l, w, h)); // block from previous frame
					
					if(golomb->eof()) return false;
					if(!golomb->readHeader(&block_decision, 1)) WrongModeException('d');
					
					if(block_decision == 1)
					{
						aBlock = lBlock.clone();
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
		
		return true;
	}

	bool SecondVideoEnc::decodeIntra(Mat* frame)
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
		
		for (uint l=0 ; l<rows ; l++)
		{
			for (uint c=0 ; c<cols ; c++)
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
	
	bool SecondVideoEnc::decodeInter(Mat* aBlock, Mat* lBlock)
	{
		*aBlock = Mat::zeros(lBlock->rows, lBlock->cols, frameType);
		
		for (int l=0 ; l < lBlock->rows ; l++)
		{
			uchar* aPixel = aBlock->ptr<uchar>(l);
			uchar* lPixel = lBlock->ptr<uchar>(l);
			for (int c=0 ; c < lBlock->cols ; c++)
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
		}
		return true;
	}
	
	uint SecondVideoEnc::getFPS()
	{
		return fps;
	}

	void SecondVideoEnc::encodeEnded()
	{
		cout << "Blocks not encoded: " << blocks << " (" << std::setprecision(4) << (float)blocks * 100 / totalBlocks << "%)" << endl;
		save();
	}

	void SecondVideoEnc::save()
	{
		if(golomb != NULL)
			golomb->close();
	}
}
