// ImageProcessDlg.cpp: 구현 파일
//

#include "pch.h"
#include "Glim.h"
#include "afxdialogex.h"
#include "ImageProcessDlg.h"
#include "GlimDlg.h"
#include "random"
#include "chrono"


// ImageProcessDlg 대화 상자

IMPLEMENT_DYNAMIC(ImageProcessDlg, CDialogEx)

ImageProcessDlg::ImageProcessDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_ImageProcessDlg, pParent)
{
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
	MoveWindow(20, 58, 600, 345);
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
	CBrush blackBrush(RGB(0, 0, 0));

	int LineThickness = GetParentLineThickness();
	CPen blackPen(PS_SOLID, LineThickness, RGB(0, 0, 0));

	// 큰 원 그리기 (점이 3개일 때만)
	if (m_Circles.size() == 3)
	{
		CBrush* pOldBrush = static_cast<CBrush*>(dc.SelectStockObject(NULL_BRUSH)); // 내부 색 없음
		CPen* pOldPen = dc.SelectObject(&blackPen);

		dc.Ellipse(m_MainCircle.center.x - m_MainCircle.radius,
			m_MainCircle.center.y - m_MainCircle.radius,
			m_MainCircle.center.x + m_MainCircle.radius,
			m_MainCircle.center.y + m_MainCircle.radius);

		dc.SelectObject(pOldBrush);
		dc.SelectObject(pOldPen);
	}

	// 작은 원 그리기
	int RadiusOfCircle = GetParentRadiusOfCircle();
	CPen dotPen(PS_SOLID, 1, RGB(0, 0, 0));

	CBrush* pOldBrush = dc.SelectObject(&blackBrush);
	CPen* pOldPen = dc.SelectObject(&dotPen);

	for (const auto& circle : m_Circles)
	{
		dc.Ellipse(circle.center.x - RadiusOfCircle,
			circle.center.y - RadiusOfCircle,
			circle.center.x + RadiusOfCircle,
			circle.center.y + RadiusOfCircle);
	}

	// 원래 브러시와 펜 복원
	dc.SelectObject(pOldBrush);
	dc.SelectObject(pOldPen);
}

void ImageProcessDlg::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.


	int clickedIndex = FindClickedCircle(point);

	if (clickedIndex != -1) // 기존 점을 클릭한 경우, 드래그 시작
	{
		if (clickedIndex >= 0 && clickedIndex < m_Circles.size())
		{
			m_bDragging = true;
			m_SelectedCircleIndex = clickedIndex;
			m_DragOffset = CPoint(point.x - m_Circles[clickedIndex].center.x,
				point.y - m_Circles[clickedIndex].center.y);
		}
	}
	else // 새로운 점 추가
	{
		if (m_Circles.size() >= 3) // 3개 이상 추가 불가
		{
			MessageBox(L"최대 3개의 점만 생성할 수 있습니다!", L"알림", MB_OK | MB_ICONWARNING);
			return;
		}

		// 새로운 점 추가
		Circle newCircle;
		newCircle.center = point;
		newCircle.radius = GetParentRadiusOfCircle();
		m_Circles.push_back(newCircle);

		if (m_Circles.size() == 3)
		{
			CalculateCircle(); // 원 계산
		}

		m_bDragging = false;  // 드래그 방지
		m_SelectedCircleIndex = -1; // 선택 해제
		Invalidate();
		UpdateEditControls();

		if (m_Circles.size() == 3)
		{
			CGlimDlg* pParent = (CGlimDlg*)GetParent();
			if (pParent)
			{
				pParent->EnableRandMoveButton(TRUE);
			}
		}
	}
	CDialogEx::OnLButtonDown(nFlags, point);
}

int ImageProcessDlg::FindClickedCircle(CPoint point)
{
	for (int i = m_Circles.size() - 1; i >= 0; --i) // 마지막 원부터 검사
	{
		int dx = point.x - m_Circles[i].center.x;
		int dy = point.y - m_Circles[i].center.y;
		if ((dx * dx + dy * dy) <= (m_Circles[i].radius * m_Circles[i].radius))
		{
			return i; // 클릭한 원의 인덱스 반환
		}
	}
	return -1; // 클릭한 원 없음
}
void ImageProcessDlg::OnMouseMove(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.

	if (m_bDragging && m_SelectedCircleIndex != -1)
	{
		m_Circles[m_SelectedCircleIndex].center =
			CPoint(point.x - m_DragOffset.x, point.y - m_DragOffset.y);

		if (m_Circles.size() == 3)
		{
			CalculateCircle();
		}

		Invalidate();
		UpdateEditControls();
	}

	CDialogEx::OnMouseMove(nFlags, point);
}

void ImageProcessDlg::OnLButtonUp(UINT nFlags, CPoint point)
{
	if (m_bDragging)
	{
		m_bDragging = false;
		m_SelectedCircleIndex = -1;
		UpdateEditControls();
	}
	CDialogEx::OnLButtonUp(nFlags, point);
}

bool ImageProcessDlg::CalculateCircle()
{
	if (m_Circles.size() != 3) return false;

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
	if (d == 0) return false; // 세 점이 일직선이면 원을 만들 수 없음

	// 원의 중심 좌표
	double cx = (c1 * b2 - c2 * b1) / d;
	double cy = (a1 * c2 - a2 * c1) / d;

	// 반지름 계산
	double r = sqrt((cx - x1) * (cx - x1) + (cy - y1) * (cy - y1));

	// 원 정보 저장
	m_MainCircle.center = CPoint(static_cast<int>(cx), static_cast<int>(cy));
	m_MainCircle.radius = static_cast<int>(r);
	return true;
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
	// 원과 점 데이터 초기화
	m_Circles.clear();
	m_bDragging = false;
	m_SelectedCircleIndex = -1;

	// 부모 다이얼로그의 Edit Control 초기화
	CWnd* pParent = GetParent();
	if (pParent)
	{
		pParent->SetDlgItemText(IDC_Point1, L"");
		pParent->SetDlgItemText(IDC_Point2, L"");
		pParent->SetDlgItemText(IDC_Point3, L"");
		pParent->SetDlgItemText(IDC_RAND_MOVE_TIME, L"");
	}

	// 화면 다시 그리기
	Invalidate();

	CGlimDlg* pParent_GrimDlg = (CGlimDlg*)GetParent();
	if (pParent_GrimDlg)
	{
		pParent_GrimDlg->EnableRandMoveButton(FALSE);
	}

	//실행 중인 쓰레드 종료
	StopRandomMove(); 
}

int ImageProcessDlg::GetParentLineThickness()
{
	CWnd* pParent = GetParent(); 
	if (!pParent) return 1; 

	CString strThickness;
	pParent->GetDlgItemText(IDC_LINE_THICKNESS, strThickness); 

	int thickness = _ttoi(strThickness); 
	if (thickness <= 0) thickness = 1;  

	return thickness;
}

int ImageProcessDlg::GetParentRadiusOfCircle()
{
	CWnd* pParent = GetParent();
	if (!pParent) return 1;

	CString strRadius;
	pParent->GetDlgItemText(IDC_RADIUS_OF_CIRCLE, strRadius);

	int Radius = _ttoi(strRadius);
	if (Radius <= 0) Radius = 1;

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

