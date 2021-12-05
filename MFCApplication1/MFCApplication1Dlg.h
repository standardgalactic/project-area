
// MFCApplication1Dlg.h : header file
//

#pragma once
#include "Critter.h"


// CMFCApplication1Dlg dialog
class CMFCApplication1Dlg : public CDialogEx
{
// Construction
public:
	CMFCApplication1Dlg(CWnd* pParent = nullptr);	// standard constructor

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_MFCAPPLICATION1_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	void PaintCritter(CDC* dc, Critter* p);
	void PaintCritters(bool ClearFirst);
	afx_msg void OnBnClickedGo();
	void InitWorld();
	void OneStep();
	CStatic mfcWorld;
	int maxX, maxY;
	int m_numCritters;
	int m_numHidden;
	afx_msg void OnBnClickedInitworld();
	int m_numConnections;
	int m_numSteps;
	BOOL m_isReset;
	afx_msg void OnBnClickedRegen();
	afx_msg void OnBnClickedSelect();
	int m_selectId;
	afx_msg void OnBnClickedCancel();
};
