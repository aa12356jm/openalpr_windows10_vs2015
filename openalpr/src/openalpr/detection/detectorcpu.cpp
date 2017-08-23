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

    equalizeHist( frame, frame );//ֱ��ͼ���⻯
    
	//�����Լ���ͼƬ�����еĳ��ƣ�����������vector����������Ƶ����ꡢ��С���þ��α�ʾ���������ɷ������������
	//1��frame�����ͼƬ
	//2��plates���������ľ��ο�������
	//3��config->detection_iteration_increase��ʾ��ǰ��������̵�ɨ���У��������ڵı���ϵ����Ĭ��Ϊ1.1��ÿ������������������10%
	//4��config->detectionStrictness����ʾ���ɼ��Ŀ������ھ��ε���С����(Ĭ��Ϊ3��)�������ɼ��Ŀ���С���εĸ�����С�� config->detectionStrictness - 1 ���ᱻ�ų���
	//���config->detectionStrictness Ϊ 0, ���������κβ����ͷ������еı����ѡ���ο������趨ֵһ�������û��Զ���Լ��������ϳ����ϣ�
	//5��CV_HAAR_DO_CANNY_PRUNING Ҫôʹ��Ĭ��ֵ��Ҫôʹ��CV_HAAR_DO_CANNY_PRUNING���������ΪCV_HAAR_DO_CANNY_PRUNING����ô��������ʹ��Canny��Ե������ų���Ե�������ٵ�����
	//�����Щ����ͨ�������ǳ�����������
	//6��7 min_plate_size��max_plate_size���������Ƶõ���Ŀ������ķ�Χ
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
