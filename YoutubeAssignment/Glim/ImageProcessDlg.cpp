// ImageProcessDlg.cpp: 구현 파일
//

#include "pch.h"
#include "Glim.h"
#include "afxdialogex.h"
#include "ImageProcessDlg.h"
#include "GlimDlg.h"
#include "random"
#include "chrono"
#include <algorithm>


// ImageProcessDlg 대화 상자

IMPLEMENT_DYNAMIC(ImageProcessDlg, CDialogEx)

ImageProcessDlg::ImageProcessDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_ImageProcessDlg, pParent)
{
	WindowWidth = 600;
	WindowHeight = 345;
	m_bDragging = false;
	m_SelectedCircleIndex = -1;
	m_bStopThread = false;
	m_Circles.clear();

}

ImageProcessDlg::~ImageProcessDlg()
{
	if (m_RandomMoveThread.joinable()) {
		m_RandomMoveThread.join();  // 스레드 종료 대기
	}
}

void ImageProcessDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(ImageProcessDlg, CDialogEx)
	ON_WM_DESTROY()
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONUP()
END_MESSAGE_MAP()


// ImageProcessDlg 메시지 처리기



BOOL ImageProcessDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	//다이얼로그 위치
	MoveWindow(20, 58, WindowWidth, WindowHeight);

	// 화면을 흰색(255)으로 초기화
	m_image.Create(WindowWidth, -WindowHeight, 8);
	if (m_image.GetBPP() == 8) {
		RGBQUAD palette[256];
		for (int i = 0; i < 256; i++) {
			palette[i].rgbBlue = palette[i].rgbGreen = palette[i].rgbRed = (BYTE)i;
			palette[i].rgbReserved = 0;
		}
		m_image.SetColorTable(0, 256, palette);
	}
	memset(m_image.GetBits(), 255, WindowWidth * WindowHeight);

	return TRUE;  
}

void ImageProcessDlg::OnDestroy()
{
	CDialogEx::OnDestroy();
	// TODO: 여기에 메시지 처리기 코드를 추가합니다.

	StopRandomMove(); 
	m_Circles.clear();
}

void ImageProcessDlg::OnPaint()
{
	CPaintDC dc(this);
	if (!m_image.IsNull()) {
		m_image.Draw(dc, 0, 0);
	}
}

void ImageProcessDlg::OnLButtonDown(UINT nFlags, CPoint point)
{
	// 원을 클릭하여 선택
	m_SelectedCircleIndex = FindClickedCircle(point);

	if (m_SelectedCircleIndex != -1) {
		m_bDragging = true;
		// 드래그 시작 시점에 마우스 오프셋을 계산하여 저장 (선택된 원의 중심에서 마우스 포인터까지의 거리)
		m_DragOffset = point - m_Circles[m_SelectedCircleIndex].center;
	}
	else {
		// 원을 새로 추가
		if (m_Circles.size() < 3) {
			Circle newCircle;
			newCircle.center = point;
			m_Circles.push_back(newCircle);

			int nRadius = GetParentRadiusOfCircle();
			if (nRadius <= 0) nRadius = 5; // 기본값 설정

			DrawSmallCircle(point.x, point.y, nRadius, 0);
			Invalidate(FALSE);

			if (m_Circles.size() == 3) {
				CalculateCircle(); // 3번째 점을 추가한 즉시 원을 그림
			}
		}
	}

	UpdateEditControls();
	CDialogEx::OnLButtonDown(nFlags, point);
}

int ImageProcessDlg::FindClickedCircle(CPoint point)
{
	// 마지막 원부터 검사하여 클릭된 원을 찾음
	for (int i = m_Circles.size() - 1; i >= 0; --i) {
		int dx = point.x - m_Circles[i].center.x;
		int dy = point.y - m_Circles[i].center.y;
		int radius = GetParentRadiusOfCircle();

		if ((dx * dx + dy * dy) <= (radius * radius)) {
			return i;
		}
	}
	return -1; // 클릭한 원이 없음
}
void ImageProcessDlg::OnMouseMove(UINT nFlags, CPoint point)
{
	int imageWidth = m_image.GetWidth();
	int imageHeight = m_image.GetHeight();

	// 마우스가 CImage 영역을 벗어나면 드래그 종료
	if (m_bDragging && m_SelectedCircleIndex != -1) {
		// x, y축의 최대, 최소값 설정 (20 이상, 최대값은 imageWidth-20, imageHeight-20)
		int minX = 10;
		int maxX = imageWidth - 10;
		int minY = 10;
		int maxY = imageHeight - 10;

		CPoint constrainedPoint = point;
		constrainedPoint.x = max(min(constrainedPoint.x, maxX), minX);
		constrainedPoint.y = max(min(constrainedPoint.y, maxY), minY);

		// 작은 원을 드래그하여 이동
		m_Circles[m_SelectedCircleIndex].center = constrainedPoint - m_DragOffset;

		if (m_Circles.size() == 3) {
			CalculateCircle();
		}

		Invalidate();
		UpdateEditControls();
	}

	// 마우스가 CImage 영역 밖으로 나가면 드래그 종료 후 그 자리에 원을 그리도록 처리
	if (m_bDragging) {
		int minX = 10;
		int maxX = imageWidth - 10;
		int minY = 10;
		int maxY = imageHeight - 10;

		// CImage 영역 밖으로 나가면 드래그 종료
		if (point.x < minX || point.x > maxX || point.y < minY || point.y > maxY) {
			m_bDragging = false;

			// 드래그가 종료된 위치에 원을 설정
			CPoint finalPoint = point;
			finalPoint.x = max(min(finalPoint.x, maxX), minX);
			finalPoint.y = max(min(finalPoint.y, maxY), minY);

			m_Circles[m_SelectedCircleIndex].center = finalPoint - m_DragOffset;

			if (m_Circles.size() == 3) {
				CalculateCircle();
			}

			Invalidate();
			UpdateEditControls();
		}
	}

	CDialogEx::OnMouseMove(nFlags, point);
}

void ImageProcessDlg::OnLButtonUp(UINT nFlags, CPoint point)
{
	int imageWidth = m_image.GetWidth();
	int imageHeight = m_image.GetHeight();

	// 마우스를 놓았을 때 최종 위치에서 원을 그리도록 처리
	if (m_bDragging && m_SelectedCircleIndex != -1) {
		int minX = 10;
		int maxX = imageWidth - 10;
		int minY = 10;
		int maxY = imageHeight - 10;

		// 최종 위치 제한
		CPoint finalPoint = point;
		finalPoint.x = max(min(finalPoint.x, maxX), minX);
		finalPoint.y = max(min(finalPoint.y, maxY), minY);

		m_Circles[m_SelectedCircleIndex].center = finalPoint - m_DragOffset;

		if (m_Circles.size() == 3) {
			CalculateCircle();
		}

		Invalidate();
		UpdateEditControls();
	}

	m_bDragging = false;
	m_SelectedCircleIndex = -1;

    CDialogEx* pParentDlg = (CDialogEx*)GetParent();
    if (m_Circles.size() == 3) {
        pParentDlg->GetDlgItem(IDC_RAND_MOVE_BTN)->EnableWindow(TRUE);
    } else {
        pParentDlg->GetDlgItem(IDC_RAND_MOVE_BTN)->EnableWindow(FALSE);
    }

	CDialogEx::OnLButtonUp(nFlags, point);
}

void ImageProcessDlg::CalculateCircle()
{
	if (m_Circles.size() != 3) return;

	memset(m_image.GetBits(), 255, WindowWidth * WindowHeight);

	// 작은 원 다시 그리기
	for (const auto& circle : m_Circles) {
		int radius = GetParentRadiusOfCircle();
		DrawSmallCircle(circle.center.x, circle.center.y, radius, 0); 
	}

	// 큰 원 계산 후 그리기
	CPoint p1 = m_Circles[0].center;
	CPoint p2 = m_Circles[1].center;
	CPoint p3 = m_Circles[2].center;

	int x1 = p1.x, y1 = p1.y;
	int x2 = p2.x, y2 = p2.y;
	int x3 = p3.x, y3 = p3.y;

	int a1 = 2 * (x2 - x1);
	int b1 = 2 * (y2 - y1);
	int c1 = x2 * x2 + y2 * y2 - x1 * x1 - y1 * y1;

	int a2 = 2 * (x3 - x1);
	int b2 = 2 * (y3 - y1);
	int c2 = x3 * x3 + y3 * y3 - x1 * x1 - y1 * y1;

	double d = a1 * b2 - a2 * b1;
	if (d == 0) return; // 세 점이 일직선이면 원을 만들 수 없음

	// 원의 중심 좌표
	double cx = (c1 * b2 - c2 * b1) / d;
	double cy = (a1 * c2 - a2 * c1) / d;
	double r = sqrt((cx - x1) * (cx - x1) + (cy - y1) * (cy - y1)); //반지름

	// CImage에 큰 원 테두리 그리기
	int LineThickness = GetParentLineThickness(); //사용자 입력으로 선 굵기 가져오기
	DrawBigCircle(static_cast<int>(cx), static_cast<int>(cy), static_cast<int>(r), LineThickness, 0);
	Invalidate(FALSE);
}

void ImageProcessDlg::UpdateEditControls()
{
	CGlimDlg* pParent = (CGlimDlg*)GetParent();
	if (!pParent) return;

	CString str;
	if (m_Circles.size() > 0)
	{
		str.Format(L"(%d, %d)", m_Circles[0].center.x, m_Circles[0].center.y);
		pParent->SetDlgItemText(IDC_Point1, str);
	}
	if (m_Circles.size() > 1)
	{
		str.Format(L"(%d, %d)", m_Circles[1].center.x, m_Circles[1].center.y);
		pParent->SetDlgItemText(IDC_Point2, str);
	}
	if (m_Circles.size() > 2)
	{
		str.Format(L"(%d, %d)", m_Circles[2].center.x, m_Circles[2].center.y);
		pParent->SetDlgItemText(IDC_Point3, str);
	}
}

void ImageProcessDlg::ResetDialog()
{
	m_Circles.clear();
	memset(m_image.GetBits(), 255, WindowWidth * WindowHeight);
	Invalidate(FALSE);
	StopRandomMove();

	CGlimDlg* pParent = (CGlimDlg*)GetParent();
	if (!pParent) return;

	CString str;
	str.Format(L"");
	pParent->SetDlgItemTextW(IDC_Point1, str);
	pParent->SetDlgItemTextW(IDC_Point2, str);
	pParent->SetDlgItemTextW(IDC_Point3, str);
	pParent->SetDlgItemTextW(IDC_RAND_MOVE_TIME, str);
	pParent->GetDlgItem(IDC_RAND_MOVE_BTN)->EnableWindow(FALSE);
}

int ImageProcessDlg::GetParentLineThickness()
{
	CWnd* pParent = GetParent(); 
	if (!pParent) return 2; 

	CString strThickness;
	pParent->GetDlgItemText(IDC_LINE_THICKNESS, strThickness); 

	int thickness = _ttoi(strThickness); 
	if (thickness <= 1) thickness = 2;  

	return thickness;
}

int ImageProcessDlg::GetParentRadiusOfCircle()
{
	CWnd* pParent = GetParent();
	if (!pParent) return 5;

	CString strRadius;
	pParent->GetDlgItemText(IDC_RADIUS_OF_CIRCLE, strRadius);

	int Radius = _ttoi(strRadius);
	if (Radius <= 4) Radius = 5;

	return Radius;
}

void ImageProcessDlg::StartRandomMove()
{
	// 실행 중인 쓰레드가 있으면 먼저 종료
	StopRandomMove();

	m_bStopThread = false;

	m_RandomMoveThread = std::thread([this] {RandMove(); });
}

void ImageProcessDlg::StopRandomMove()
{
	// 쓰레드가 실행 중이라면 종료 요청
	m_bStopThread = true;

	if (m_RandomMoveThread.joinable())  
	{
		m_RandomMoveThread.join();
	}

	m_bStopThread = false;
}

void ImageProcessDlg::RandMove()
{
	auto start = std::chrono::system_clock::now();

	//난수 생성
	std::random_device rd;
	std::mt19937 gen(rd());		
	std::uniform_int_distribution<int> distX(10, 590); // X좌표 범위
	std::uniform_int_distribution<int> distY(10, 335); // Y좌표 범위

	for (int i = 0; i < 10 && !m_bStopThread; i++) // 10번 반복
	{
		for (auto& circle : m_Circles)
		{
			circle.center.x = distX(gen);
			circle.center.y = distY(gen);
		}

		CalculateCircle();
		UpdateEditControls();
		Invalidate();

		std::this_thread::sleep_for(std::chrono::milliseconds(500)); // 초당 2회 실행
	}

	auto end = std::chrono::system_clock::now();
	auto millisec = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

	CWnd* pParent = GetParent();
	if (pParent)
	{
		pParent->PostMessage(WM_UPDATE_TIME, 0, static_cast<LPARAM>(millisec.count()));
	}
}

void ImageProcessDlg::DrawSmallCircle(int x, int y, int nRadius, int nColor)
{
	int nPitch = m_image.GetPitch();
	unsigned char* fm = (unsigned char*)m_image.GetBits();

	int nCenterX = x;
	int nCenterY = y;

	// 다이얼로그 크기 초과 방지
	int minX = max(0, nCenterX - nRadius);
	int maxX = min(WindowWidth - 1, nCenterX + nRadius);
	int minY = max(0, nCenterY - nRadius);
	int maxY = min(WindowHeight - 1, nCenterY + nRadius);

	for (int j = minY; j <= maxY; j++) {
		for (int i = minX; i <= maxX; i++) {
			if (IsInCircle(i, j, nCenterX, nCenterY, nRadius)) {
				fm[j * nPitch + i] = static_cast<unsigned char>(nColor);
			}
		}
	}
}

bool ImageProcessDlg::IsInCircle(int i, int j, int nCenterX, int nCenterY, int nRadius)
{
	bool bRet = false;

	double dX = i - nCenterX;
	double dY = j - nCenterY;
	double dDist = dX * dX + dY * dY;

	if (dDist <= nRadius * nRadius) {
		bRet = true;
	}

	return bRet;
}

void ImageProcessDlg::DrawBigCircle(int centerX, int centerY, int radius, int thickness, int color) {
	int nPitch = m_image.GetPitch();
	unsigned char* fm = (unsigned char*)m_image.GetBits();

	// 다이얼로그 크기 초과 방지
	int minX = max(0, centerX - radius - thickness);
	int maxX = min(WindowWidth - 1, centerX + radius + thickness);
	int minY = max(0, centerY - radius - thickness);
	int maxY = min(WindowHeight - 1, centerY + radius + thickness);

	for (int y = minY; y <= maxY; y++) {
		for (int x = minX; x <= maxX; x++) {
			if (IsOnCircleBorder(x, y, centerX, centerY, radius, thickness)) {
				fm[y * nPitch + x] = static_cast<unsigned char>(color); // 원 테두리 픽셀 설정
			}
		}
	}
}

// 특정 좌표가 원의 테두리에 속하는지 확인하는 함수 (굵기 고려)
bool ImageProcessDlg::IsOnCircleBorder(int x, int y, int centerX, int centerY, int radius, int thickness) {
	int dx = x - centerX;
	int dy = y - centerY;
	int distSquared = dx * dx + dy * dy;
	int outerRadiusSquared = (radius + thickness / 2) * (radius + thickness / 2);
	int innerRadiusSquared = (radius - thickness / 2) * (radius - thickness / 2);
	return (distSquared <= outerRadiusSquared && distSquared >= innerRadiusSquared);
}