#include "videoenc.h"

namespace VideoCoding
{
	VideoEnc::VideoEnc(string ifname, string ofname, char mode, bool preview, uint stage, uint predictMode, uint blockSize, uint searchArea, int periodicity, char thresholdMode, double t, double base_threshold, double ceil_threshold, double min_threshold, char searchMode, double Kb, double Kr)
	{
		this->mode = mode;
		this->predictMode = predictMode;
		this->preview = preview;
		switch(mode)
		{
			case 'r':
				video.reset(new RGBReader(ifname));
				mode = 'e';
				break;
			case 'c':
				mode = 'e';
				video.reset(new RGBVideo());
				try
				{
					video->open(atoi(ifname.c_str()));
				}
				catch(int e) { }
				if(!video->isOpened()) WebCamException("Webcam0 not available!");
				break;
			case 'm': // mp4, avi, mpeg, wmv
				mode = 'e';
				video.reset(new RGBVideo());
				try
				{
					video->open(ifname);
				}
				catch(int e) { }
				if(!video->isOpened()) WebCamException("Webcam not available!");
				break;
			case 'd':
				if(ofname.length() > 0)
					video.reset(new RGBWriter(ofname));
				break;
		}
		
		if(periodicity < 0)
		{
			if(mode == 'e')
				periodicity = (int)video->getFPS();
			else
				periodicity = 0;
		}
		
		switch(stage)
		{
			case 1:
				fsc.reset(new VideoCoding::FirstVideoEnc(ifname, ofname, mode, predictMode));
				break;
			case 2:
				fsc.reset(new VideoCoding::SecondVideoEnc(ifname, ofname, mode, predictMode, blockSize, thresholdMode, t, base_threshold));
				break;
			case 3:
				
				fsc.reset(new VideoCoding::ThirdVideoEnc(ifname, ofname, mode, predictMode, blockSize, searchArea, periodicity, thresholdMode, t, base_threshold, ceil_threshold, min_threshold, searchMode));
				break;
			case 4:
				fsc.reset(new VideoCoding::FourthVideoEnc(ifname, ofname, mode, predictMode, blockSize, searchArea, periodicity, thresholdMode, t, base_threshold, ceil_threshold, min_threshold, searchMode, Kb, Kr));
				break;
			default:
				fsc.reset(new VideoCoding::FirstVideoEnc(ifname, ofname, mode, predictMode));
		}
		
	}

	VideoEnc::~VideoEnc()
	{
		auto * ptr_fsc = fsc.release();
		fsc.get_deleter() ( ptr_fsc );
		fsc.reset(nullptr);
		
		auto * ptr_video = video.release();
		video.get_deleter() ( ptr_video );
		video.reset(nullptr);
	}

	void VideoEnc::capture()
	{
		if(mode == 'd') WrongModeException(mode);
		if(preview)
		{
			namedWindow("Webcam", WINDOW_NORMAL);
			namedWindow("Encoded", WINDOW_NORMAL);
			namedWindow("New", WINDOW_NORMAL);
			
			resizeWindow("Webcam", 600, 600);
			resizeWindow("New", 600, 600);
			resizeWindow("Encoded", 600, 600);
			moveWindow("Webcam", 50, 0);
			moveWindow("New", 650, 0);
			moveWindow("Encoded", 1250, 0);
		}
		Mat New;
		
		Mat frame;
		
		while(video->read(frame))
		{
			if(preview)
				imshow("Webcam", frame);

			New = fsc->encode(&frame, video->getFPS());
			if(preview)
			{
				imshow("New", New);
				imshow("Encoded", frame);
			
				int keyboard = waitKey(1000/video->getFPS());
				//int keyboard = waitKey(0);
				if((keyboard % 256) == 'q') break;
			}
		}

		fsc->encodeEnded();
		
		video->release();
		destroyAllWindows();
	}

	void VideoEnc::watchVideo()
	{
		if(mode == 'e') WrongModeException(mode);
		if(preview)
			namedWindow("Webcam", WINDOW_AUTOSIZE);
		Mat frame;
		
		while(fsc->decode(&frame))
		{
			if(preview)
				imshow("Webcam", frame);
			if(video != NULL)
				video->write(frame, fsc->getFPS());
			if(preview)
			{
				//(int)waitKey(0);
				(int)waitKey(1000/fsc->getFPS());
			}
		}
		if(video != NULL)
			video->release();
		destroyAllWindows();
	}
}


int main(int argc, char* argv[])
{
	uint stage = 1;
	char mode = '?';
	char source = 'r';
	uint predictorMode = 6;
	uint blockSize=8;
	uint searchArea=4;
	int periodicity=-1;
	char thresholdMode = 'r';
	char searchMode = 's';
	double diff=1.;
	double base_threshold=12., ceil_threshold=4., min_threshold=40.;
	double Kb=0.114, Kr=0.299;
	bool preview=false;
	std::string ifname, ofname="";
	
	int index;
	static struct option options[] =
	{
		{"stage",         required_argument,  0, 's'},
		{"encode",        no_argument,        0, 'e'},
		{"decode",        no_argument,        0, 'd'},
		{"preview",       no_argument,        0, 'w'},
		{"source",        required_argument,  0, 'x'},
		{"predictor",     required_argument,  0, 'p'},
		{"blocksize",     required_argument,  0, 'l'},
		{"searcharea",    required_argument,  0, 'a'}, 
		{"periodicity",   required_argument,  0, 'f'},
		{"base_t",        required_argument,  0, 't'},
		{"ceil_t",        required_argument,  0, 'c'},
		{"min_t",         required_argument,  0, 'm'},
		{"Kb",            required_argument,  0, 'b'},
		{"Kr",            required_argument,  0, 'r'},
		{"diff",          required_argument,  0, 'o'},
		{"searchmode",    required_argument,  0, 'k'},
		{"thresholdmode", required_argument,  0, 'j'},
		{"version",       no_argument,        0, 'v'},
		{"help",          no_argument,        0, 'h'},
		{0, 0}
	};
	
	int c;
	int option_index;
	for(int i=0 ;; i++)
	{
		option_index = 0;
		c = getopt_long(argc, argv, "s:edwx:p:l:a:f:t:c:m:b:r:o:k:j:vh", options, &option_index);
		/* Detect the end of the options. */
		if (c == -1)
		{
			if(i == 0) { help(argv[0]); return 0; }
			break;
		}
		
		switch (c)
		{
			case 0: /* If this option set a flag, do nothing else now. */
					if (options[option_index].flag != 0)
						break;
					printf ("option %s", options[option_index].name);
					if (optarg)
						printf (" with arg %s", optarg);
					printf("\n");
					break;
			case 's': // Stage
					stage = atoi(optarg);
					break;
			case 'e': // Encode
					if(mode!='?') {std::cerr << "ERROR: Unable to run multiple modes at the same time!!!" << std::endl; help(argv[0]); abort();}
					mode = 'e';
					break;
			case 'd': // Decode
					if(mode!='?') {std::cerr << "ERROR: Unable to run multiple modes at the same time!!!" << std::endl; help(argv[0]); abort();}
					mode = 'd';
					break;
			case 'w': // Preview
					preview = true;
					break;
			case 'x': // Source (r/c/m)
					source = optarg[0];
					break;
			case 'p': // Predictor Mode
					predictorMode = atoi(optarg);
					break;
			case 'l': // Block size
					blockSize = atoi(optarg);
					break;
			case 'a': // Size for search area
					searchArea = atoi(optarg);
					break;
			case 'f': // Periodicity of Intraframes
					periodicity = atoi(optarg);
					break;
			case 't': // Base Threshold
					base_threshold = atof(optarg);
					break;
			case 'c': // Ceil Threshold
					ceil_threshold = atof(optarg);
					break;
			case 'm': // Min Threshold
					min_threshold = atof(optarg);
					break;
			case 'b': // Kb value
					Kb = atof(optarg);
					break;
			case 'r': // Kr value
					Kr = atof(optarg);
					break;
			case 'o': // diff
					diff = atof(optarg);
					break;
			case 'j': // Threshold Mode
					thresholdMode = optarg[0];
					break;
			case 'k': // Search Mode
					searchMode = optarg[0];
					break;
			case 'v': // Version and Copyright
					version();
					return 0;
					break;
			case 'h': // Help Menu
					help(argv[0]);
					return 0;
					break;
			case '?':
					/* getopt_long already printed an error message. */
					help(argv[0]);
					return 0;
					break;
			default:
				abort();
		}
	}
	
	if(mode == 'd' || mode == 'e' || mode == 'c' || mode == 'm')
	{
		index = optind;
		if(index < argc)
			ifname = argv[index];
		else {std::cerr << "ERROR: Encode/Decode requires an input filename" << std::endl; help(argv[0]); abort();}
		index++;
		if(index < argc)
			ofname = argv[index];
		else
			if(mode == 'e' || mode == 'c' || mode == 'm')
				ofname = ifname.substr(0, ifname.find_last_of(".")) + ".glb";
	}
	
	switch(mode)
	{
		case 'e':
		case 'd':
		case 'c':
		case 'm':
			switch(stage)
			{
				case 1:
				case 2:
				case 3:
				case 4:
						{
							switch(mode)
							{
								case 'c':
								case 'm':
								case 'e': 
										{
											if(source == 'r' || source == 'c' || source == 'm')
											{
												VideoCoding::VideoEnc v(ifname, ofname, source, preview, stage, predictorMode, blockSize, searchArea, periodicity, thresholdMode, diff, base_threshold, ceil_threshold, min_threshold, searchMode, Kb, Kr);
												clock_t time = clock();
												v.capture();
												cout << "Encode time: " << (double)(clock() - time)/CLOCKS_PER_SEC << " s" << endl;
											}
											else
												std::cerr << "ERROR: You need to select a valid source (r: RGB file, c: CAM, m: MOVIE)!!!" << endl;
										}
										break;
								case 'd':
										{
											VideoCoding::VideoEnc v(ifname, ofname, 'd', preview, stage, predictorMode, blockSize, searchArea, periodicity, thresholdMode, diff, base_threshold, ceil_threshold, min_threshold, searchMode, Kb, Kr);
											clock_t time = clock();
											v.watchVideo();
											cout << "Decode time: " << (double)(clock() - time)/CLOCKS_PER_SEC << " s" << endl;
										}
										break;
								default:
									abort();
							}
						}
						break;
				default:
					std::cerr << "ERROR: Stage '" << stage << "' is not a valid stage!!!" << endl;
					help(argv[0]);
					abort();
			}
			break;
		default:
			std::cerr << "ERROR: You need to select a mode (encode/decode)!!!" << endl;
			help(argv[0]);
			abort();
	}
	
	
	/*// encode
	clock_t time = clock();
	VideoCoding::VideoEnc ve("videos/cambada1.rgb", "encoded.glb", 'e', 3, 6);
	//VideoCoding::VideoEnc ve("0", "encoded.glb", 'c', 3, 6); // cam
	//VideoCoding::VideoEnc ve("commercial.wmv", "encoded.glb", 'm', 0, 6); // movie
	ve.capture();
	cout << "Encode time: " << (double)(clock() - time)/CLOCKS_PER_SEC << " s" << endl;


	// decode
	time = clock();
	VideoCoding::VideoEnc vd("encoded.glb", "video.rgb", 'd', 3);
	vd.watchVideo();
	cout << "Decode time: " << (double)(clock() - time)/CLOCKS_PER_SEC << " s" << endl;
	*/
	return 0;
}
