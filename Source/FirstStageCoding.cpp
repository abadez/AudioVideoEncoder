#include "FirstStageCoding.h"

namespace VideoCoding
{
	FirstVideoEnc::FirstVideoEnc(string ifname, string ofname, char mode, uint predictMode)
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
	}

	FirstVideoEnc::~FirstVideoEnc()
	{
		save();
	}
	
	void FirstVideoEnc::writeHeader(Mat* frame, uint fps)
	{
		if (golomb==NULL)
		{
			golomb.reset(new Golomb(ofname, 'e'));
		}
		golomb->writeHeader(frame->rows, 12);	// Height
		golomb->writeHeader(frame->cols, 12);	// Width
		golomb->writeHeader(fps, 6);			// FPS
		golomb->writeHeader(frame->type(), 6);  // Frame type
		golomb->writeHeader((frame->type() - ((frame->channels() - 1) << 3)), 3);	// Channel type
		golomb->writeHeader(predictMode, 3);	// Predictor Mode
	}

	Mat FirstVideoEnc::encode(Mat* frame, uint fps)
	{
		if(mode == 'd') WrongModeException(mode);
		
		if (golomb==NULL)
		{
			writeHeader(frame, fps);
		}

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
		return frame->clone();
	}

	bool FirstVideoEnc::decode(Mat* frame)
	{
		if(mode == 'e') WrongModeException(mode);
		
		if (golomb==NULL)
		{
			golomb.reset(new Golomb(ifname, 'd'));
			if(!golomb->readHeader(&rows, 12)) WrongModeException('d');
			if(!golomb->readHeader(&cols, 12)) WrongModeException('d');
			if(!golomb->readHeader(&fps, 6)) WrongModeException('d');
			if(!golomb->readHeader(&frameType, 6)) WrongModeException('d');
			if(!golomb->readHeader(&channelType, 3)) WrongModeException('d');
			if(!golomb->readHeader(&predictMode, 3)) WrongModeException('d');
		}

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
	
	uint FirstVideoEnc::getFPS()
	{
		return fps;
	}

	void FirstVideoEnc::encodeEnded()
	{
		save();
	}

	void FirstVideoEnc::save()
	{
		if(golomb != NULL)
			golomb->close();
	}
}
