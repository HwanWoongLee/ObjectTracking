#ifndef KALMAN_H
#define KALMAN_H 2

#include "opencv2/video/tracking.hpp"
#include "opencv2/highgui/highgui.hpp"

using namespace std;
using namespace cv;

#define StateType Rect_<float>


class KalmanTracker
{
public:
	KalmanTracker()
	{
		init_kf(StateType());
		m_time_since_update = 0;
		m_hits = 0;
		m_hit_streak = 0;
		m_age = 0;
		m_id = kf_count;
		//kf_count++;
	}
	KalmanTracker(StateType initRect)
	{
		init_kf(initRect);
		m_time_since_update = 0;
		m_hits = 0;
		m_hit_streak = 0;
		m_age = 0;
		m_id = kf_count;
		kf_count++;
	}

	~KalmanTracker()
	{
		m_history.clear();
	}

	StateType predict();
	void update(StateType stateMat);
	
	StateType get_state();
	StateType get_rect_xysr(float cx, float cy, float s, float r);

	static int kf_count;

	int m_time_since_update;
	int m_hits;
	int m_hit_streak;
	int m_age;
	int m_id;

private:
	void init_kf(StateType stateMat);

	cv::KalmanFilter kf;
	cv::Mat measurement;

	std::vector<StateType> m_history;
};

typedef struct TrackingBox
{
	int frame;
	int id;
	Rect_<float> box;
}TrackingBox;

// Simple Online and RealTime Tracking
class SORT {
public:
	SORT();
	~SORT();

	void Tracking(const std::vector<cv::Rect>& detect_boxes,		// detect bbox
						std::vector<TrackingBox>& track_boxes);		// tracking bbox

	void SetParams(double iouThresh, int frame_count = 0, int max_age = 5, int min_hits = 3);


private:
	double SORT::GetIOU(Rect_<float> bb_test, Rect_<float> bb_gt);

private:
	std::vector<KalmanTracker>		m_trackers;
	std::vector<cv::Rect>			m_predictBoxes;
	std::vector<vector<double>>		m_iouMatrix;
	std::vector<int>				m_assignment;
	//std::vector<TrackingBox>		frameTrackingResult;

	std::set<int>					unmatchedDetections;
	std::set<int>					unmatchedTrajectories;
	std::set<int>					allItems;
	std::set<int>					matchedItems;
	std::vector<cv::Point>			matchedPairs;

	double							m_iouThreshold	= 0.3;
	int								m_frame_count	= 0;
	int								m_max_age		= 5;
	int								m_min_hits		= 3;
};

#endif