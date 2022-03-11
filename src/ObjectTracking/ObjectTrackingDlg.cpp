
// ObjectTrackingDlg.cpp: 구현 파일
//

#include "pch.h"
#include "framework.h"
#include "ObjectTracking.h"
#include "ObjectTrackingDlg.h"
#include "afxdialogex.h"
#include "TMatView.h"
#include "KalmanTracker.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif



CObjectTrackingDlg::CObjectTrackingDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_OBJECTTRACKING_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

CObjectTrackingDlg::~CObjectTrackingDlg() {
	if (m_pThread && m_pThread->joinable()) {
		m_pThread->join();
		delete m_pThread;
	}
}

void CObjectTrackingDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CObjectTrackingDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BTN_PLAY_VIDEO, &CObjectTrackingDlg::OnBnClickedBtnPlayVideo)
	ON_BN_CLICKED(IDC_BTN_SELECT, &CObjectTrackingDlg::OnBnClickedBtnSelect)
END_MESSAGE_MAP()


BOOL CObjectTrackingDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	SetIcon(m_hIcon, TRUE);
	SetIcon(m_hIcon, FALSE);

	CWnd* pWnd = GetDlgItem(IDC_STATIC_VIEW);
	CRect rect;
	pWnd->GetWindowRect(rect);
	ScreenToClient(rect);

	// Create Viwer
	if (!m_pViewer && pWnd) {
		m_pViewer = new CTMatView();
		if (m_pViewer->Create(NULL, NULL, WS_VISIBLE | WS_CHILD | WS_BORDER, CRect(), this, IDC_STATIC_VIEW)) {
			m_pViewer->MoveWindow(rect);
			m_pViewer->ShowTool(true);
		}
		pWnd->DestroyWindow();
	}

	// YOLO Create, Model Load
	if (!m_pYolo) {
		m_pYolo = new CYoloModule();
		if (!m_pYolo->LoadWeight("./cfg/yolov4.cfg", "./model/yolov4.weights")) {
			AfxMessageBox(_T("YOLO Model Load failed"));
		}
	}

	// Make Random Color
	for (int i = 0; i < 255; ++i) {
		m_rColor[i] = cv::Scalar((double)std::rand() / RAND_MAX * 255, (double)std::rand() / RAND_MAX * 255, (double)std::rand() / RAND_MAX * 255);
	}

	return TRUE;
}

void CObjectTrackingDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this);

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

HCURSOR CObjectTrackingDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

static inline Point calcPoint(Point2f center, double R, double angle)
{
	return center + Point2f((float)cos(angle), (float)-sin(angle))*(float)R;
}


// Play Video
void CObjectTrackingDlg::OnBnClickedBtnPlayVideo()
{
	static TCHAR BASED_CODE szFilter[] = _T("동영상 파일(*.MP4) | *.MP4; |모든파일(*.*)|*.*||");
	CFileDialog dlg(TRUE, _T("*.mp4"), _T("video"), OFN_HIDEREADONLY, szFilter);

	if (IDOK == dlg.DoModal()) {
		CString pathName = dlg.GetPathName();



		if (!m_cap.open(std::string(CT2CA(pathName))))
			return;
	}

	// Set Params
	std::vector<cv::Rect>				detect_boxes;
	std::vector<TrackingBox>			tResults;
	std::vector<std::vector<cv::Point>>	t_points(999);

	double	iouThresh		= 0.3;
	int		frame_count		= 0;
	int		max_age			= 5;
	int		min_hits		= 3;
	cv::Mat image, dst;
	double dStartTime, dTotalTime;

	SORT sort;
	sort.SetParams(iouThresh, frame_count, max_age, min_hits);

	// Tracking Loop
	while (true) {
		dStartTime = GetTickCount64();

		m_cap >> image;
		if (image.empty())
			break;

		dst = image.clone();

		if (m_bTracking) {
			// 1. Detecting
			m_pYolo->Detect(image, detect_boxes);

			// 2. Tracking
			sort.Tracking(detect_boxes, tResults);

			// 3. Draw Result
			for (int i = 0; i < tResults.size(); ++i) {
				cv::Point ptCetner = (tResults[i].box.tl() + tResults[i].box.br()) / 2;
				t_points[tResults[i].id].push_back(ptCetner);

				if (t_points[tResults[i].id].size() > 50) {
					t_points[tResults[i].id].erase(t_points[tResults[i].id].begin(), t_points[tResults[i].id].begin() + 3);
				}

				for (int j = 0; j < t_points[tResults[i].id].size(); ++j) {
					cv::circle(dst, t_points[tResults[i].id][j], 5, m_rColor[tResults[i].id % 255], -1);
				}
				cv::rectangle(dst, tResults[i].box, m_rColor[tResults[i].id % 255], 2, 8, 0);
				cv::putText(dst, cv::format("%d", tResults[i].id), tResults[i].box.tl(), 0, 1, cv::Scalar(0, 255, 0), 2);
			}
		}

		// Show FPS
		dTotalTime = GetTickCount64() - dStartTime;
		if (dTotalTime != 0) {
			cv::putText(dst, cv::format("FPS:%.1lf", 1. / dTotalTime * 1000), cv::Point(50, 50), 0, 1, cv::Scalar(0, 0, 255), 2);
		}
		else {
			cv::putText(dst, "FPS:", cv::Point(50, 50), 0, 1, cv::Scalar(0, 0, 255), 2);
		}

		m_pViewer->SetImage(dst);
		cv::waitKey(1);
	}
}


void CObjectTrackingDlg::OnBnClickedBtnSelect()
{
	m_bTracking = !m_bTracking;
}
