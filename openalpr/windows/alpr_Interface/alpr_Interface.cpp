#include "alpr_Interface.h"

const std::string MAIN_WINDOW_NAME = "ALPR main window";

const bool SAVE_LAST_VIDEO_STILL = false;
const std::string LAST_VIDEO_STILL_LOCATION = "/tmp/laststill.jpg";
const std::string WEBCAM_PREFIX = "/dev/video";
MotionDetector motiondetector;//�˶������
bool do_motiondetection_qt = true;

/** Function Headers */
bool detectandshow(Alpr* alpr, cv::Mat frame, std::string region, bool writeJson);
bool is_supported_image(std::string image_file);

bool measureProcessingTime = false;
std::string templatePattern;

// This boolean is set to false when the user hits terminates (e.g., CTRL+C )
// so we can end infinite loops for things like video processing.
bool program_active = true;



alpr_Interface::alpr_Interface(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);
}
int alpr_Interface::test()
{
	std::vector<std::string> filenames;
	std::string configFile = "";
	bool outputJson = false;
	int seektoms = 0;
	bool detectRegion = false;
	std::string country;
	int topn;
	bool debug_mode = false;

	TCLAP::CmdLine cmd("OpenAlpr Command Line Utility", ' ', Alpr::getVersion());

	TCLAP::UnlabeledMultiArg<std::string>  fileArg("image_file", "Image containing license plates", true, "", "image_file_path");


	TCLAP::ValueArg<std::string> countryCodeArg("c", "country", "Country code to identify (either us for USA or eu for Europe).  Default=us", false, "us", "country_code");
	TCLAP::ValueArg<int> seekToMsArg("", "seek", "Seek to the specified millisecond in a video file. Default=0", false, 0, "integer_ms");
	TCLAP::ValueArg<std::string> configFileArg("", "config", "Path to the openalpr.conf file", false, "", "config_file");
	TCLAP::ValueArg<std::string> templatePatternArg("p", "pattern", "Attempt to match the plate number against a plate pattern (e.g., md for Maryland, ca for California)", false, "", "pattern code");
	TCLAP::ValueArg<int> topNArg("n", "topn", "Max number of possible plate numbers to return.  Default=10", false, 10, "topN");

	TCLAP::SwitchArg jsonSwitch("j", "json", "Output recognition results in JSON format.  Default=off", cmd, false);
	TCLAP::SwitchArg debugSwitch("", "debug", "Enable debug output.  Default=off", cmd, false);
	TCLAP::SwitchArg detectRegionSwitch("d", "detect_region", "Attempt to detect the region of the plate image.  [Experimental]  Default=off", cmd, false);
	TCLAP::SwitchArg clockSwitch("", "clock", "Measure/print the total time to process image and all plates.  Default=off", cmd, false);
	TCLAP::SwitchArg motiondetect("", "motion", "Use motion detection on video file or stream.  Default=off", cmd, false);

	try
	{
		cmd.add(templatePatternArg);
		cmd.add(seekToMsArg);
		cmd.add(topNArg);
		cmd.add(configFileArg);
		cmd.add(fileArg);
		cmd.add(countryCodeArg);


// 		if (cmd.parse(argc, argv) == false)
// 		{
// 			// Error occurred while parsing.  Exit now.
// 			return 1;
// 		}

		filenames = fileArg.getValue();

		country = countryCodeArg.getValue();
		seektoms = seekToMsArg.getValue();
		outputJson = jsonSwitch.getValue();
		debug_mode = debugSwitch.getValue();
		configFile = configFileArg.getValue();
		detectRegion = detectRegionSwitch.getValue();
		templatePattern = templatePatternArg.getValue();
		topn = topNArg.getValue();
		measureProcessingTime = clockSwitch.getValue();
		do_motiondetection_qt = motiondetect.getValue();
	}
	catch (TCLAP::ArgException &e)    // catch any exceptions
	{
		std::cerr << "error: " << e.error() << " for arg " << e.argId() << std::endl;
		return 1;
	}


	cv::Mat frame;

	Alpr alpr(country, configFile);
	alpr.setTopN(topn);

	if (debug_mode)
	{
		alpr.getConfig()->setDebug(true);
	}

	if (detectRegion)
		alpr.setDetectRegion(detectRegion);

	if (templatePattern.empty() == false)
		alpr.setDefaultRegion(templatePattern);

	if (alpr.isLoaded() == false)
	{
		std::cerr << "Error loading OpenALPR" << std::endl;
		return 1;
	}

	for (unsigned int i = 0; i < filenames.size(); i++)
	{
		std::string filename = filenames[i];

		if (filename == "-")
		{
			std::vector<uchar> data;
			int c;

			while ((c = fgetc(stdin)) != EOF)
			{
				data.push_back((uchar)c);
			}
			//imdecode�Ǵ�ָ�����ڴ滺���ж�ȡһ��ͼ��imread�Ǵ�ָ���ļ�����һ��ͼ��
			frame = cv::imdecode(cv::Mat(data), 1);
			if (!frame.empty())
			{
				detectandshow(&alpr, frame, "", outputJson);
			}
			else
			{
				std::cerr << "Image invalid: " << filename << std::endl;
			}
		}
		else if (filename == "stdin")
		{
			std::string filename;
			while (std::getline(std::cin, filename))
			{
				if (fileExists(filename.c_str()))
				{
					frame = cv::imread(filename);
					detectandshow(&alpr, frame, "", outputJson);
				}
				else
				{
					std::cerr << "Image file not found: " << filename << std::endl;
				}

			}
		}
		else if (filename == "webcam" || startsWith(filename, WEBCAM_PREFIX))
		{
			int webcamnumber = 0;

			// If they supplied "/dev/video[number]" parse the "number" here
			if (startsWith(filename, WEBCAM_PREFIX) && filename.length() > WEBCAM_PREFIX.length())
			{
				webcamnumber = atoi(filename.substr(WEBCAM_PREFIX.length()).c_str());
			}

			int framenum = 0;
			cv::VideoCapture cap(webcamnumber);
			if (!cap.isOpened())
			{
				std::cerr << "Error opening webcam" << std::endl;
				return 1;
			}

			while (cap.read(frame))
			{
				if (framenum == 0)
					motiondetector.ResetMotionDetection(&frame);
				detectandshow(&alpr, frame, "", outputJson);
				sleep_ms(10);
				framenum++;
			}
		}
		else if (startsWith(filename, "http://") || startsWith(filename, "https://"))
		{
			int framenum = 0;

			VideoBuffer videoBuffer;

			videoBuffer.connect(filename, 5);

			cv::Mat latestFrame;

			while (program_active)
			{
				std::vector<cv::Rect> regionsOfInterest;
				int response = videoBuffer.getLatestFrame(&latestFrame, regionsOfInterest);

				if (response != -1)
				{
					if (framenum == 0)
						motiondetector.ResetMotionDetection(&latestFrame);
					detectandshow(&alpr, latestFrame, "", outputJson);
				}

				// Sleep 10ms
				sleep_ms(10);
				framenum++;
			}

			videoBuffer.disconnect();

			std::cout << "Video processing ended" << std::endl;
		}
		else if (hasEndingInsensitive(filename, ".avi") || hasEndingInsensitive(filename, ".mp4") ||
			hasEndingInsensitive(filename, ".webm") ||
			hasEndingInsensitive(filename, ".flv") || hasEndingInsensitive(filename, ".mjpg") ||
			hasEndingInsensitive(filename, ".mjpeg") ||
			hasEndingInsensitive(filename, ".mkv")
			)
		{
			if (fileExists(filename.c_str()))
			{
				int framenum = 0;

				cv::VideoCapture cap = cv::VideoCapture();
				cap.open(filename);
				cap.set(CV_CAP_PROP_POS_MSEC, seektoms);

				while (cap.read(frame))
				{
					if (SAVE_LAST_VIDEO_STILL)
					{
						cv::imwrite(LAST_VIDEO_STILL_LOCATION, frame);
					}
					if (!outputJson)
						std::cout << "Frame: " << framenum << std::endl;

					if (framenum == 0)
						motiondetector.ResetMotionDetection(&frame);
					detectandshow(&alpr, frame, "", outputJson);
					//create a 1ms delay
					sleep_ms(1);
					framenum++;
				}
			}
			else
			{
				std::cerr << "Video file not found: " << filename << std::endl;
			}
		}
		else if (is_supported_image(filename))
		{
			if (fileExists(filename.c_str()))
			{
				frame = cv::imread(filename);

				bool plate_found = detectandshow(&alpr, frame, "", outputJson);

				if (!plate_found && !outputJson)
					std::cout << "No license plates found." << std::endl;
			}
			else
			{
				std::cerr << "Image file not found: " << filename << std::endl;
			}
		}
		else if (DirectoryExists(filename.c_str()))
		{
			std::vector<std::string> files = getFilesInDir(filename.c_str());

			std::sort(files.begin(), files.end(), stringCompare);

			for (int i = 0; i < files.size(); i++)
			{
				if (is_supported_image(files[i]))
				{
					std::string fullpath = filename + "/" + files[i];
					std::cout << fullpath << std::endl;
					frame = cv::imread(fullpath.c_str());
					if (detectandshow(&alpr, frame, "", outputJson))
					{
						//while ((char) cv::waitKey(50) != 'c') { }
					}
					else
					{
						//cv::waitKey(50);
					}
				}
			}
		}
		else
		{
			std::cerr << "Unknown file type" << std::endl;
			return 1;
		}
	}

	return 0;
}

//ͼ�������Ƿ�֧�֣�ͨ���鿴ͼ���ļ�����׺��ȷ��
bool alpr_Interface::is_supported_image(std::string image_file)
{
	return (hasEndingInsensitive(image_file, ".png") || hasEndingInsensitive(image_file, ".jpg") ||
		hasEndingInsensitive(image_file, ".tif") || hasEndingInsensitive(image_file, ".bmp") ||
		hasEndingInsensitive(image_file, ".jpeg") || hasEndingInsensitive(image_file, ".gif"));
}

//���һ��ͼ���еĳ���
bool alpr_Interface::detectandshow(Alpr* alpr, cv::Mat frame, std::string region, bool writeJson)
{
	//��¼��ʼʱ��
	timespec startTime;
	getTimeMonotonic(&startTime);

	std::vector<AlprRegionOfInterest> regionsOfInterest;//�洢����Ȥ����
														//�����Ҫ�˶��м��
	if (do_motiondetection_qt)
	{
		cv::Rect rectan = motiondetector.MotionDetect(&frame);
		if (rectan.width > 0) regionsOfInterest.push_back(AlprRegionOfInterest(rectan.x, rectan.y, rectan.width, rectan.height));
	}
	else regionsOfInterest.push_back(AlprRegionOfInterest(0, 0, frame.cols, frame.rows));

	AlprResults results;//���Ƽ����
	if (regionsOfInterest.size() > 0) results = alpr->recognize(frame.data, frame.elemSize(), frame.cols, frame.rows, regionsOfInterest);

	//��¼����ʱ��
	timespec endTime;
	getTimeMonotonic(&endTime);
	//�ܴ���ʱ��
	double totalProcessingTime = diffclock(startTime, endTime);
	if (measureProcessingTime)
		std::cout << "Total Time to process image: " << totalProcessingTime << "ms." << std::endl;

	//�����Ҫ���򽫳��ƽ��д�뵽json��
	if (writeJson)
	{
		std::cout << alpr->toJson(results) << std::endl;
	}
	else
	{
		for (int i = 0; i < results.plates.size(); i++)
		{
			std::cout << "plate" << i << ": " << results.plates[i].topNPlates.size() << " results";
			if (measureProcessingTime)
				std::cout << " -- Processing Time = " << results.plates[i].processing_time_ms << "ms.";
			std::cout << std::endl;

			if (results.plates[i].regionConfidence > 0)
				std::cout << "State ID: " << results.plates[i].region << " (" << results.plates[i].regionConfidence << "% confidence)" << std::endl;

			for (int k = 0; k < results.plates[i].topNPlates.size(); k++)
			{
				// Replace the multiline newline character with a dash
				std::string no_newline = results.plates[i].topNPlates[k].characters;
				std::replace(no_newline.begin(), no_newline.end(), '\n', '-');

				std::cout << "    - " << no_newline << "\t confidence: " << results.plates[i].topNPlates[k].overall_confidence;
				if (templatePattern.size() > 0 || results.plates[i].regionConfidence > 0)
					std::cout << "\t pattern_match: " << results.plates[i].topNPlates[k].matches_template;

				std::cout << std::endl;
			}
		}
	}



	return results.plates.size() > 0;
}