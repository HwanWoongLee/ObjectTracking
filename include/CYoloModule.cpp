#include "pch.h"
#include "CYoloModule.h"


CYoloModule::CYoloModule() {

}

CYoloModule::~CYoloModule() {
	if (m_pDetector)
		delete m_pDetector;
}

BOOL CYoloModule::LoadWeight(std::string strCfg, std::string strWeight) {
	if (m_pDetector)
		delete m_pDetector;

	m_pDetector = new Detector(strCfg, strWeight);

	if (m_pDetector)
		return TRUE;
	else
		return FALSE;
}

BOOL CYoloModule::Detect(const cv::Mat& src, cv::Mat& dst) {
	if (!m_pDetector || src.empty())
		return FALSE;

	dst = src.clone();
	std::vector<bbox_t> result = m_pDetector->detect(src, 0.5);

	for (int i = 0; i < result.size(); ++i) {
		cv::rectangle(dst, 
			cv::Rect(result[i].x, result[i].y, result[i].w, result[i].h),
			cv::Scalar(0, 0, 255),
			2);
	}

	return TRUE;
}

BOOL CYoloModule::Detect(const cv::Mat& src, std::vector<cv::Rect>& bboxes) {
	if (!m_pDetector || src.empty())
		return FALSE;

	std::vector<bbox_t> result = m_pDetector->detect(src, 0.5);

	bboxes.clear();
	for (int i = 0; i < result.size(); ++i) {
		if (result[i].obj_id == 2)	// only car
			bboxes.push_back(cv::Rect(result[i].x, result[i].y, result[i].w, result[i].h));
	}

	return TRUE;
}