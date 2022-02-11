#define OPENCV
#define CUDNN
#include "yolo_v2_class.hpp"

#ifdef DEBUG
#pragma comment(lib, "yolo_cpp_dll.lib")
#else
#pragma comment(lib, "yolo_cpp_dll.lib")
#endif

#pragma once
class CYoloModule
{
public:
	CYoloModule();
	~CYoloModule();

	BOOL LoadWeight(std::string strCfg, std::string strWeight);
	BOOL Detect(const cv::Mat& src, cv::Mat& dst);
	BOOL Detect(const cv::Mat& src, std::vector<cv::Rect>& bboxes);

private:
	Detector* m_pDetector = nullptr;
	
};

