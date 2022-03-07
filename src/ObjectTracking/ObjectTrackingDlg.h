#include "CYoloModule.h"
#pragma once


class CTMatView;
class CObjectTrackingDlg : public CDialogEx
{
// 생성입니다.
public:
	CObjectTrackingDlg(CWnd* pParent = nullptr);	// 표준 생성자입니다.
	~CObjectTrackingDlg();

// 대화 상자 데이터입니다.
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
