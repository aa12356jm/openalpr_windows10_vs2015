#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_alpr_Interface.h"

#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#include "tclap/CmdLine.h"
#include "support/filesystem.h"
#include "support/timing.h"
#include "support/platform.h"
#include "video/videobuffer.h"
#include "motiondetector.h"
#include "alpr.h"

using namespace alpr;


class alpr_Interface : public QMainWindow
{
	Q_OBJECT

public:
	bool detectandshow(Alpr* alpr, cv::Mat frame, std::string region, bool writeJson);
	bool is_supported_image(std::string image_file);
	alpr_Interface(QWidget *parent = Q_NULLPTR); 
	

	int test();
private:
	Ui::alpr_InterfaceClass ui;
};
