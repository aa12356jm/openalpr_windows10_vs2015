/*
 * Copyright (c) 2015 OpenALPR Technology, Inc.
 * Open source Automated License Plate Recognition [http://www.openalpr.com]
 *
 * This file is part of OpenALPR.
 *
 * OpenALPR is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License
 * version 3 as published by the Free Software Foundation
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include "detectorcpu.h"

using namespace cv;
using namespace std;

namespace alpr
{


  DetectorCPU::DetectorCPU(Config* config, PreWarp* prewarp) : Detector(config, prewarp) {


    
    if( this->plate_cascade.load( get_detector_file() ) )
    {
      this->loaded = true;
    }
    else
    {
      this->loaded = false;
      printf("--(!)Error loading CPU classifier %s\n", get_detector_file().c_str());
    }
  }


  DetectorCPU::~DetectorCPU() {
  }



  
  vector<Rect> DetectorCPU::find_plates(Mat frame, cv::Size min_plate_size, cv::Size max_plate_size)
  {

    vector<Rect> plates;
   
    //-- Detect plates
    timespec startTime;
    getTimeMonotonic(&startTime);

    equalizeHist( frame, frame );//直方图均衡化
    
	//它可以检测出图片中所有的车牌，并将车牌用vector保存各个车牌的坐标、大小（用矩形表示），函数由分类器对象调用
	//1，frame待检测图片
	//2，plates被检测物体的矩形框向量组
	//3，config->detection_iteration_increase表示在前后两次相继的扫描中，搜索窗口的比例系数。默认为1.1即每次搜索窗口依次扩大10%
	//4，config->detectionStrictness，表示构成检测目标的相邻矩形的最小个数(默认为3个)。如果组成检测目标的小矩形的个数和小于 config->detectionStrictness - 1 都会被排除。
	//如果config->detectionStrictness 为 0, 则函数不做任何操作就返回所有的被检候选矩形框，这种设定值一般用在用户自定义对检测结果的组合程序上；
	//5，CV_HAAR_DO_CANNY_PRUNING 要么使用默认值，要么使用CV_HAAR_DO_CANNY_PRUNING，如果设置为CV_HAAR_DO_CANNY_PRUNING，那么函数将会使用Canny边缘检测来排除边缘过多或过少的区域，
	//因此这些区域通常不会是车牌所在区域；
	//6，7 min_plate_size，max_plate_size：用来限制得到的目标区域的范围
    plate_cascade.detectMultiScale( frame, plates, config->detection_iteration_increase, config->detectionStrictness,
                                      CV_HAAR_DO_CANNY_PRUNING,
                                      //0|CV_HAAR_SCALE_IMAGE,
                                      min_plate_size, max_plate_size );
    if (config->debugTiming)
    {
      timespec endTime;
      getTimeMonotonic(&endTime);
      cout << "LBP Time: " << diffclock(startTime, endTime) << "ms." << endl;
    }

    return plates;

  }

}
