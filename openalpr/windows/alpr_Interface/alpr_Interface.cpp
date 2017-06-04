#include "alpr_Interface.h"
#include <QFileDialog>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QMessageBox>
#include <QTableWidget>
#include <QPushButton>


const std::string MAIN_WINDOW_NAME = "ALPR main window";

const bool SAVE_LAST_VIDEO_STILL = false;
const std::string LAST_VIDEO_STILL_LOCATION = "/tmp/laststill.jpg";
const std::string WEBCAM_PREFIX = "/dev/video";
MotionDetector motiondetector;//运动检测器
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
	ui.pushButton_plate_recognize->setStyleSheet("QPushButton{ border-image:url(: /icon /close) }"); //正常状态下 
	ui.pushButton_plate_recognize->setStyleSheet("QPushButton:hover{ border - image:url(: / icon / close_on) }");//鼠标经过时显示这张  
	ui.pushButton_plate_recognize->setStyleSheet("QPushButton:hover:pressed{ border - image:url(: / icon / close); }");//按下按钮时

 

	//QTableWidget tableWidget;
	ui.tableWidget_Result->setEditTriggers(QAbstractItemView::NoEditTriggers);
	ui.tableWidget_Result->setColumnCount(3);
	ui.tableWidget_Result->setRowCount(5);

	QStringList headers;
	headers << "车牌号" << "概率"<<"模式匹配" ;
	ui.tableWidget_Result->setHorizontalHeaderLabels(headers);

// 	ui.tableWidget_Result->setItem(0, 0, new QTableWidgetItem(QString("0001")));
// 	ui.tableWidget_Result->setItem(1, 0, new QTableWidgetItem(QString("0002")));
// 	ui.tableWidget_Result->setItem(2, 0, new QTableWidgetItem(QString("0003")));
// 	ui.tableWidget_Result->setItem(3, 0, new QTableWidgetItem(QString("0004")));
// 	ui.tableWidget_Result->setItem(4, 0, new QTableWidgetItem(QString("0005")));
// 	ui.tableWidget_Result->setItem(0, 1, new QTableWidgetItem(QString("20100112")));
	//ui.horizontalLayout_3->addWidget(tableWidget);

	QAction *openAction = new QAction(QIcon("icon/open.png"), tr("&打开"), this);
	openAction->setShortcuts(QKeySequence::Open);
	openAction->setStatusTip(tr("Open an image file"));
	connect(openAction, SIGNAL(triggered()), this, SLOT(pushButton_open_clicked()));

	QMenu *file = menuBar()->addMenu(tr("&文件"));
	file->addAction(openAction);
	//QToolBar *toolBar = addToolBar(tr("&File"));
	ui.mainToolBar->addAction(openAction);

	setMinimumSize(740, 520);
	m_graphicsView_src = new QGraphicsView();
	ui.horizontalLayout->addWidget(m_graphicsView_src);     //将自定义的组件添加到布局中
	m_graphicsScene_src = new QGraphicsScene;  //new 一个新的场景对象
	m_graphicsScene_src->setSceneRect(-370, -250, 740, 500);     //限定场景对象的显示区域
	m_graphicsView_src->setScene(m_graphicsScene_src);          //将视图对象于场景相连

	m_graphicsView_dst = new QGraphicsView();
	ui.horizontalLayout->addWidget(m_graphicsView_dst);     //将自定义的组件添加到布局中
	m_graphicsScene_dst = new QGraphicsScene;  //new 一个新的场景对象
	m_graphicsScene_dst->setSceneRect(-370, -250, 740, 500);     //限定场景对象的显示区域
	m_graphicsView_dst->setScene(m_graphicsScene_dst);          //将视图对象于场景相连

	connect(ui.pushButton_plate_recognize, SIGNAL(clicked()), this, SLOT(pushButton_plate_recognize_clicked()));
}

QImage alpr_Interface::cvMat2QImage(const cv::Mat& mat)
{
	// 8-bits unsigned, NO. OF CHANNELS = 1  
	if (mat.type() == CV_8UC1)
	{
		QImage image(mat.cols, mat.rows, QImage::Format_Indexed8);
		// Set the color table (used to translate colour indexes to qRgb values)  
		image.setColorCount(256);
		for (int i = 0; i < 256; i++)
		{
			image.setColor(i, qRgb(i, i, i));
		}
		// Copy input Mat  
		uchar *pSrc = mat.data;
		for (int row = 0; row < mat.rows; row++)
		{
			uchar *pDest = image.scanLine(row);
			memcpy(pDest, pSrc, mat.cols);
			pSrc += mat.step;
		}
		return image;
	}
	// 8-bits unsigned, NO. OF CHANNELS = 3  
	else if (mat.type() == CV_8UC3)
	{
		// Copy input Mat  
		const uchar *pSrc = (const uchar*)mat.data;
		// Create QImage with same dimensions as input Mat  
		QImage image(pSrc, mat.cols, mat.rows, mat.step, QImage::Format_RGB888);
		return image.rgbSwapped();
	}
	else if (mat.type() == CV_8UC4)
	{
		//qDebug() << "CV_8UC4";
		// Copy input Mat  
		const uchar *pSrc = (const uchar*)mat.data;
		// Create QImage with same dimensions as input Mat  
		QImage image(pSrc, mat.cols, mat.rows, mat.step, QImage::Format_ARGB32);
		return image.copy();
	}
	else
	{
		//qDebug() << "ERROR: Mat could not be converted to QImage.";
		return QImage();
	}
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
			//imdecode是从指定的内存缓存中读取一幅图像，imread是从指定文件载入一幅图像
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

void alpr_Interface::pushButton_plate_recognize_clicked()
{

	if (fileName.isEmpty())
	{
		QMessageBox::warning(this, "警告", "请先打开图像", QMessageBox::Default);
		return;
	}
	Alpr alpr("us", "openalpr.conf");
	alpr.setTopN(5);

	alpr.setDefaultRegion("md");

	if (alpr.isLoaded() == false)
	{
		std::cerr << "Error loading OpenALPR" << std::endl;
		return ;
	}
	AlprResults results = alpr.recognize(fileName.toStdString());
	vector<string> vplate_Characters;
	vector<float> vplate_Confidence;
	vector<bool> vplate_Pattern_match;
	for (int i = 0; i < results.plates.size(); i++)
	{
		alpr::AlprPlateResult plate = results.plates[i];
		std::cout << "plate" << i << ": " << plate.topNPlates.size() << " results" << std::endl;

		for (int k = 0; k < plate.topNPlates.size(); k++)
		{
			alpr::AlprPlate candidate = plate.topNPlates[k];
			vplate_Characters.push_back(candidate.characters);
			vplate_Confidence.push_back(candidate.overall_confidence);
			vplate_Pattern_match.push_back(candidate.matches_template);

			std::cout << "    - " << candidate.characters << "\t confidence: " << candidate.overall_confidence;
			std::cout << "\t pattern_match: " << candidate.matches_template << std::endl;
		}
	}

	ui.tableWidget_Result->setColumnCount(3);
	ui.tableWidget_Result->setRowCount(results.plates.size());
	for (int i = 0; i < results.plates.size(); i++)
	{
		ui.tableWidget_Result->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(vplate_Characters[i])));
		ui.tableWidget_Result->setItem(i, 1, new QTableWidgetItem(QString("%1").arg(vplate_Confidence[i])));
		
	}
}

void alpr_Interface::pushButton_open_clicked()
{
	m_graphicsScene_src->clear();
	fileName = QFileDialog::getOpenFileName(this,
		tr("打开图像"),//对话框名称
		".",//默认打开文件位置“.”文件目录"/"根目录
		tr("image files(*.jpg *.png *.bmp)"));//筛选器

Cv:Mat srcImg = imread(fileName.toStdString());

	pixItem_src = new PixItem(&QPixmap::fromImage(cvMat2QImage(srcImg)));
	//将该图元对象添加到场景中，并设置此图元在场景中的位置为中心（0，0）
	m_graphicsScene_src->addItem(pixItem_src);
	pixItem_src->setPos(0, 0);


	//ui.label_src->setPixmap(QPixmap::fromImage(cvMat2QImage(srcImg)));
}

//图像类型是否支持，通过查看图像文件名后缀来确认
bool alpr_Interface::is_supported_image(std::string image_file)
{
	return (hasEndingInsensitive(image_file, ".png") || hasEndingInsensitive(image_file, ".jpg") ||
		hasEndingInsensitive(image_file, ".tif") || hasEndingInsensitive(image_file, ".bmp") ||
		hasEndingInsensitive(image_file, ".jpeg") || hasEndingInsensitive(image_file, ".gif"));
}

//检测一幅图像中的车牌
bool alpr_Interface::detectandshow(Alpr* alpr, cv::Mat frame, std::string region, bool writeJson)
{
	//记录开始时间
	timespec startTime;
	getTimeMonotonic(&startTime);

	std::vector<AlprRegionOfInterest> regionsOfInterest;//存储感兴趣区域
														//如果需要运动中检测
	if (do_motiondetection_qt)
	{
		cv::Rect rectan = motiondetector.MotionDetect(&frame);
		if (rectan.width > 0) regionsOfInterest.push_back(AlprRegionOfInterest(rectan.x, rectan.y, rectan.width, rectan.height));
	}
	else regionsOfInterest.push_back(AlprRegionOfInterest(0, 0, frame.cols, frame.rows));

	AlprResults results;//车牌检测结果
	if (regionsOfInterest.size() > 0) results = alpr->recognize(frame.data, frame.elemSize(), frame.cols, frame.rows, regionsOfInterest);

	//记录结束时间
	timespec endTime;
	getTimeMonotonic(&endTime);
	//总处理时间
	double totalProcessingTime = diffclock(startTime, endTime);
	if (measureProcessingTime)
		std::cout << "Total Time to process image: " << totalProcessingTime << "ms." << std::endl;

	//如果需要，则将车牌结果写入到json中
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
