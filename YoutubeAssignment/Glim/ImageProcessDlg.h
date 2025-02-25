#pragma once
#include "afxdialogex.h"
#include <vector>
#include <thread>
#include <atomic>

// ImageProcessDlg 대화 상자

class ImageProcessDlg : public CDialogEx
{
	DECLARE_DYNAMIC(ImageProcessDlg)

public:
	ImageProcessDlg(CWnd* pParent = nullptr);   // 표준 생성자입니다.
	virtual ~ImageProcessDlg();

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ImageProcessDlg };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

	DECLARE_MESSAGE_MAP()

public:
	void StartRandomMove();
	void StopRandomMove();
	void ResetDialog();
	void RandMove();

private:
	//다이얼로그 크기
	int WindowWidth;
	int WindowHeight;

	//원 그리기
	struct Circle {
		CPoint center;					// 원의 중심 좌표
		int radius;						// 반지름
	};
	std::vector<Circle> m_Circles;
	Circle m_bigCircle;
	CImage m_image;
	void DrawSmallCircle(int x, int y, int nRadius, int nColor);
	void DrawBigCircle(int centerX, int centerY, int radius, int thickness, int color);
	void CalculateCircle();
	bool IsOnCircleBorder(int x, int y, int centerX, int centerY, int radius, int thickness);
	bool IsInCircle(int i, int j, int nCenterX, int nCenterY, int nRadius);
	int FindClickedCircle(CPoint point);

	//이벤트 처리
	int m_SelectedCircleIndex;			// 드래그 중인 원의 인덱스 (-1이면 없음)
	CPoint m_DragOffset;				// 마우스와 원 중심 간의 거리
	bool m_bDragging;					// 드래그 상태
	std::atomic<bool> m_bStopThread; 
	std::thread m_RandomMoveThread;  

	//부모 다이얼로그 정보 업데이트
	void UpdateEditControls();
	int GetParentLineThickness();
	int GetParentRadiusOfCircle();

public:
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	afx_msg void OnPaint();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
};
