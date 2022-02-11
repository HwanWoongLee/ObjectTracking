
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
	if (m_pThread->joinable()) {
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


// CObjectTrackingDlg 메시지 처리기

BOOL CObjectTrackingDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 이 대화 상자의 아이콘을 설정합니다.  응용 프로그램의 주 창이 대화 상자가 아닐 경우에는
	//  프레임워크가 이 작업을 자동으로 수행합니다.
	SetIcon(m_hIcon, TRUE);			// 큰 아이콘을 설정합니다.
	SetIcon(m_hIcon, FALSE);		// 작은 아이콘을 설정합니다.

	CWnd* pWnd = GetDlgItem(IDC_STATIC_VIEW);
	CRect rect;
	pWnd->GetWindowRect(rect);
	ScreenToClient(rect);

	if (!m_pViewer && pWnd) {
		m_pViewer = new CTMatView();
		if (m_pViewer->Create(NULL, NULL, WS_VISIBLE | WS_CHILD | WS_BORDER, CRect(), this, IDC_STATIC_VIEW)) {
			m_pViewer->MoveWindow(rect);
			m_pViewer->ShowTool(true);
		}
		pWnd->DestroyWindow();
	}


	return TRUE;  // 포커스를 컨트롤에 설정하지 않으면 TRUE를 반환합니다.
}

// 대화 상자에 최소화 단추를 추가할 경우 아이콘을 그리려면
//  아래 코드가 필요합니다.  문서/뷰 모델을 사용하는 MFC 응용 프로그램의 경우에는
//  프레임워크에서 이 작업을 자동으로 수행합니다.

void CObjectTrackingDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 그리기를 위한 디바이스 컨텍스트입니다.

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 클라이언트 사각형에서 아이콘을 가운데에 맞춥니다.
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 아이콘을 그립니다.
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// 사용자가 최소화된 창을 끄는 동안에 커서가 표시되도록 시스템에서
//  이 함수를 호출합니다.
HCURSOR CObjectTrackingDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

static inline Point calcPoint(Point2f center, double R, double angle)
{
	return center + Point2f((float)cos(angle), (float)-sin(angle))*(float)R;
}

void CObjectTrackingDlg::OnBnClickedBtnPlayVideo()
{
	static TCHAR BASED_CODE szFilter[] = _T("동영상 파일(*.MP4) | *.MP4; |모든파일(*.*)|*.*||");
	CFileDialog dlg(TRUE, _T("*.mp4"), _T("video"), OFN_HIDEREADONLY, szFilter);

	if (IDOK == dlg.DoModal()) {
		CString pathName = dlg.GetPathName();
		cv::VideoCapture cap;

		cv::Scalar rColor[255];
		for (int i = 0; i < 255; ++i) {
			rColor[i] = cv::Scalar((double)std::rand() / RAND_MAX * 255, (double)std::rand() / RAND_MAX * 255, (double)std::rand() / RAND_MAX * 255);
		}

		if (!cap.open(std::string(CT2CA(pathName))))
			return;

		cv::Mat image, dst;

		// YOLO Load
		if (!m_pYolo) {
			m_pYolo = new CYoloModule();
			if (!m_pYolo->LoadWeight("C:/Project/ObjectTracking/cfg/yolov4.cfg",
				"C:/Project/ObjectTracking/model/yolov4.weights")) {
				AfxMessageBox(_T("YOLO Model Load failed"));
			}
		}

		// Tracker
		std::vector<cv::Rect>	bboxes;
		std::vector<TrackingBox> tResults;

		std::vector<std::vector<cv::Point>>	t_points(999);

		SORT sort;
		sort.SetParams(0.3, 0, 5, 3);

		while (true) {
			cap >> image;
			if (image.empty())
				break;
			
			dst = image.clone();

			if (m_bTracking) {
				m_pYolo->Detect(image, bboxes);
				sort.Tracking(bboxes, tResults);

				for (int i = 0; i < tResults.size(); ++i) {
					cv::Point ptCetner = (tResults[i].box.tl() + tResults[i].box.br()) / 2;
					t_points[tResults[i].id].push_back(ptCetner);
					if (t_points[tResults[i].id].size() > 50) {
						t_points[tResults[i].id].erase(t_points[tResults[i].id].begin(), t_points[tResults[i].id].begin() + 3);
					}


					cv::rectangle(dst, tResults[i].box, rColor[tResults[i].id % 255], 2, 8, 0);
					cv::putText(dst, cv::format("%d", tResults[i].id), tResults[i].box.tl(), 0, 1, cv::Scalar(0, 255, 0), 2);
					for (int j = 0; j < t_points[tResults[i].id].size(); ++j) {
						cv::circle(dst, t_points[tResults[i].id][j], 5, rColor[tResults[i].id % 255], -1);
					}
				}

				
			}

			m_pViewer->SetImage(dst);
			cv::waitKey(1);
		}
	}
}


void CObjectTrackingDlg::OnBnClickedBtnSelect()
{
	m_bTracking = !m_bTracking;
	return;
}
