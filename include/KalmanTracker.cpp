///////////////////////////////////////////////////////////////////////////////
// KalmanTracker.cpp: KalmanTracker Class Implementation Declaration

#include "pch.h"
#include "KalmanTracker.h"
#include "Hungarian.h"

int KalmanTracker::kf_count = 0;


// initialize Kalman filter
void KalmanTracker::init_kf(StateType stateMat)
{
	int stateNum = 7;
	int measureNum = 4;
	kf = KalmanFilter(stateNum, measureNum, 0);

	measurement = Mat::zeros(measureNum, 1, CV_32F);

	kf.transitionMatrix = (Mat_<float>(stateNum, stateNum) <<
		1, 0, 0, 0, 1, 0, 0,
		0, 1, 0, 0, 0, 1, 0,
		0, 0, 1, 0, 0, 0, 1,
		0, 0, 0, 1, 0, 0, 0,
		0, 0, 0, 0, 1, 0, 0,
		0, 0, 0, 0, 0, 1, 0,
		0, 0, 0, 0, 0, 0, 1);

	setIdentity(kf.measurementMatrix);
	setIdentity(kf.processNoiseCov, Scalar::all(1e-2));
	setIdentity(kf.measurementNoiseCov, Scalar::all(1e-1));
	setIdentity(kf.errorCovPost, Scalar::all(.1));
	
	// initialize state vector with bounding box in [cx,cy,s,r] style
	kf.statePost.at<float>(0, 0) = stateMat.x + stateMat.width / 2;
	kf.statePost.at<float>(1, 0) = stateMat.y + stateMat.height / 2;
	kf.statePost.at<float>(2, 0) = stateMat.area();
	kf.statePost.at<float>(3, 0) = stateMat.width / stateMat.height;
}


// Predict the estimated bounding box.
StateType KalmanTracker::predict()
{
	// predict
	Mat p = kf.predict();
	m_age += 1;

	if (m_time_since_update > 0)
		m_hit_streak = 0;
	m_time_since_update += 1;

	StateType predictBox = get_rect_xysr(p.at<float>(0, 0), p.at<float>(1, 0), p.at<float>(2, 0), p.at<float>(3, 0));

	m_history.push_back(predictBox);
	return m_history.back();
}


// Update the state vector with observed bounding box.
void KalmanTracker::update(StateType stateMat)
{
	m_time_since_update = 0;
	m_history.clear();
	m_hits += 1;
	m_hit_streak += 1;

	// measurement
	measurement.at<float>(0, 0) = stateMat.x + stateMat.width / 2;
	measurement.at<float>(1, 0) = stateMat.y + stateMat.height / 2;
	measurement.at<float>(2, 0) = stateMat.area();
	measurement.at<float>(3, 0) = stateMat.width / stateMat.height;

	// update
	kf.correct(measurement);
}


// Return the current state vector
StateType KalmanTracker::get_state()
{
	Mat s = kf.statePost;
	return get_rect_xysr(s.at<float>(0, 0), s.at<float>(1, 0), s.at<float>(2, 0), s.at<float>(3, 0));
}


// Convert bounding box from [cx,cy,s,r] to [x,y,w,h] style.
StateType KalmanTracker::get_rect_xysr(float cx, float cy, float s, float r)
{
	float w = sqrt(s * r);
	float h = s / w;
	float x = (cx - w / 2);
	float y = (cy - h / 2);

	if (x < 0 && cx > 0)
		x = 0;
	if (y < 0 && cy > 0)
		y = 0;

	return StateType(x, y, w, h);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// SORT : Simple Online and RealTime Tracking
/////////////////////////////////////////////////////////////////////////////////////////////////////

SORT::SORT() {

}
SORT::~SORT() {

}

void SORT::SetParams(double iouThresh, int frame_count, int max_age, int min_hits) {
	m_iouThreshold	= iouThresh;
	m_frame_count	= frame_count;
	m_max_age		= max_age;
	m_min_hits		= min_hits;
}

// Computes IOU between two bounding boxes
double SORT::GetIOU(Rect_<float> bb_test, Rect_<float> bb_gt)
{
	float in = (bb_test & bb_gt).area();
	float un = bb_test.area() + bb_gt.area() - in;

	if (un < DBL_EPSILON)
		return 0;

	return (double)(in / un);
}

void SORT::Tracking(const std::vector<cv::Rect>& detect_boxes, 
						  std::vector<TrackingBox>& track_boxes) 
{
	// 1. Detect Boxes
	std::vector<cv::Rect> bboxes = detect_boxes;

	// 2. Trackers가 없을 시 Tracker 생성(Detection Box 개수 만큼)
		// 이미 Tracking 중이라면 유지
	if (m_trackers.size() == 0) {
		for (int i = 0; i < bboxes.size(); ++i) {
			KalmanTracker trk = KalmanTracker(bboxes[i]);
			m_trackers.push_back(trk);
		}
	}

	// 3. Tracker -> Predict
	m_predictBoxes.clear();
	for (auto it = m_trackers.begin(); it != m_trackers.end();) {
		auto pBox = (*it).predict();
		if (pBox.x >= 0 && pBox.y >= 0) {
			m_predictBoxes.push_back(pBox);
			it++;
		}
		else {
			it = m_trackers.erase(it);
		}
	}

	// 4. Predict와 Detection Box간의 IoU
	int trkNum = m_predictBoxes.size();
	int detNum = bboxes.size();

	m_iouMatrix.clear();
	m_iouMatrix.resize(trkNum, vector<double>(detNum, 0));

	for (unsigned int i = 0; i < trkNum; i++) {
		for (unsigned int j = 0; j < detNum; j++) {
			m_iouMatrix[i][j] = 1 - GetIOU(m_predictBoxes[i], bboxes[j]);
		}
	}

	// 5. Hungarian Algorithm을 이용한 매칭(assignment:배정, 할당) 
	// IoU 기반 점수 기반 매칭
	HungarianAlgorithm HungAlgo;
	m_assignment.clear();
	HungAlgo.Solve(m_iouMatrix, m_assignment);

	unmatchedTrajectories.clear();
	unmatchedDetections.clear();
	allItems.clear();
	matchedItems.clear();

	if (detNum > trkNum) //	there are unmatched detections
	{
		for (unsigned int n = 0; n < detNum; n++)
			allItems.insert(n);

		for (unsigned int i = 0; i < trkNum; ++i)
			matchedItems.insert(m_assignment[i]);

		set_difference(allItems.begin(), allItems.end(),
			matchedItems.begin(), matchedItems.end(),
			insert_iterator<set<int>>(unmatchedDetections, unmatchedDetections.begin()));
	}
	else if (detNum < trkNum) // there are unmatched trajectory/predictions
	{
		for (unsigned int i = 0; i < trkNum; ++i)
			if (m_assignment[i] == -1) // unassigned label will be set as -1 in the assignment algorithm
				unmatchedTrajectories.insert(i);
	}
	else;

	// filter out matched with low IOU
	matchedPairs.clear();
	for (unsigned int i = 0; i < trkNum; ++i)
	{
		if (m_assignment[i] == -1) // pass over invalid values
			continue;
		if (1 - m_iouMatrix[i][m_assignment[i]] < m_iouThreshold)
		{
			unmatchedTrajectories.insert(i);
			unmatchedDetections.insert(m_assignment[i]);
		}
		else
			matchedPairs.push_back(cv::Point(i, m_assignment[i]));
	}

	// 6. 매칭된 box로 tracker Update
	int detIdx, trkIdx;
	for (unsigned int i = 0; i < matchedPairs.size(); i++)
	{
		trkIdx = matchedPairs[i].x;
		detIdx = matchedPairs[i].y;
		m_trackers[trkIdx].update(bboxes[detIdx]);
	}

	// create and initialise new trackers for unmatched detections
	for (auto umd : unmatchedDetections)
	{
		KalmanTracker tracker = KalmanTracker(bboxes[umd]);
		m_trackers.push_back(tracker);
	}

	// 7. Get Result
	// frameTrackingResult.clear();
	track_boxes.clear();

	for (auto it = m_trackers.begin(); it != m_trackers.end();)
	{
		if (((*it).m_time_since_update < 1) &&
			((*it).m_hit_streak >= m_min_hits || m_frame_count <= m_min_hits))
		{
			TrackingBox res;
			res.box = (*it).get_state();
			res.id = (*it).m_id + 1;
			res.frame = m_frame_count;
			track_boxes.push_back(res);

			it++;
		}
		else
			it++;

		// remove dead tracklet
		if (it != m_trackers.end() && (*it).m_time_since_update > m_max_age)
			it = m_trackers.erase(it);
	}
}