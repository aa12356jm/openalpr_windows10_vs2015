#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_alpr_Interface.h"
#include "pixitem.h"

#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#include "tclap/CmdLine.h"
#include "support/filesystem.h"
#include "support/timing.h"
#include "support/platform.h"
#include "video/videobuffer.h"
#include "motiondetector.h"
#include "alpr.h"

using namespace std;
using namespace alpr;
using namespace cv;
#pragma execution_character_set("utf-8")

class alpr_Interface : public QMainWindow
{
	Q_OBJECT

public:
	alpr_Interface(QWidget *parent = Q_NULLPTR); 
	QImage cvMat2QImage(const cv::Mat& mat);

	bool detectandshow(Alpr* alpr, cv::Mat frame, std::string region, bool writeJson);
	bool is_supported_image(std::string image_file);
	int test();

private slots:
	void pushButton_plate_recognize_clicked();

	void pushButton_open_clicked();
private:
	Ui::alpr_InterfaceClass ui;

	QString fileName;
	vector<Mat> vPlateMat;

	PixItem *pixItem_src;       //自定义的图元类
	QGraphicsScene *m_graphicsScene_src;  //场景
	QGraphicsView *m_graphicsView_src;

	PixItem *pixItem_dst;       //自定义的图元类
	QGraphicsScene *m_graphicsScene_dst;  //场景
	QGraphicsView *m_graphicsView_dst;
};
