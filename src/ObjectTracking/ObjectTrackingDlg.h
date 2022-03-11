#pragma once
#include "CYoloModule.h"


class CTMatView;
class CObjectTrackingDlg : public CDialogEx
{
public:
	CObjectTrackingDlg(CWnd* pParent = nullptr);	// 표준 생성자입니다.
	~CObjectTrackingDlg();

#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_OBJECTTRACKING_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 지원입니다.


private:
	CTMatView*			m_pViewer			= nullptr;
	CYoloModule*		m_pYolo				= nullptr;

	bool				m_bPlay				= false;
	bool				m_bTracking			= false;

	cv::Rect			m_rectSelected;

	std::thread*		m_pThread;

	cv::VideoCapture	m_cap;
	cv::Scalar			m_rColor[255];

protected:
	HICON				m_hIcon;

	// 생성된 메시지 맵 함수
	virtual BOOL		OnInitDialog();
	afx_msg void		OnPaint();
	afx_msg HCURSOR		OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void		OnBnClickedBtnPlayVideo();
	afx_msg void		OnBnClickedBtnSelect();
};
