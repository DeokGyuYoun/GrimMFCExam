
// GlimDlg.h: 헤더 파일
//

#include "ImageProcessDlg.h"
#pragma once


#define WM_UPDATE_TIME (WM_USER + 1) // 사용자 정의 메시지 추가

// CGlimDlg 대화 상자
class CGlimDlg : public CDialogEx
{
// 생성입니다.
public:
	CGlimDlg(CWnd* pParent = nullptr);	// 표준 생성자입니다.
	~CGlimDlg();

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_GLIM_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 지원입니다.

// 구현입니다.
protected:
	HICON m_hIcon;

	// 생성된 메시지 맵 함수
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg LRESULT OnUpdateTime(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()

public:
	ImageProcessDlg* m_pImageProcessDlg;
	void EnableRandMoveButton(BOOL bEnable);

private:
	CBrush m_BgBrush;
	CBrush m_EditBrush;

public:
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnBnClickedRandMoveBtn();
	afx_msg void OnBnClickedResetbtn();
	afx_msg void OnDestroy();

};
