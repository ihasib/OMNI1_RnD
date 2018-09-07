#pragma once

#include "OceanBase.h"
#include "SerialPort.h"
#include "Etalon.h"
#include "OpSwitch.h"
#include "DIGI/Digistatic.h"

class CViteDlg : public CDialog
{
public:
	CEtalon Etalon;
	COpSwitch OpSwitch;
	COceanBase* pOceanOp;
	double stdev;
	CCmplx CC;
	int nNum;
	POINT*	m_points;
	float* wave;
	float* Rmeas;
	float fStDev;
	double Rmeasmin;
	double Rmeasmax;
	enum SPECTYPE
	{
		SIMUSPEC, SPEC1, SPEC2, OCEAN, FSMSPEC
	};
	SPECTYPE nSpecType;
	HANDLE m_hThread; // thread for measuring continuously
	BOOL	m_bStopFlag;
	int nZP;

public:
	CViteDlg(CWnd* pParent = NULL);   // standard constructor
	void EnableButtons(BOOL bEnable);
	float PolyThickness;
	int GetDataNumber(BOOL bFFT);
	void GetSpectrumData(BOOL bFFT, float *DataArray1, float *DataArray2, int N);
	void SetThkRange(float fThk1, float fThk2, float fIndex, float fExcThk1, float fExcThk2);
	void DisplaySpectrum();
	BOOL Measure();
	void UpdatePars();
	void UpdatePars2();
	double GetPolyThickness();
	void SetPolyIndex(double fIndex);
	void Initialize();
	void NormalizeSpectrum(float* R, int N);
	int GetAverageData();
	BOOL MeasureReference();
	BOOL MeasureBackground();
	void ZeroMethod();
	void Calibrate();

	//FSMB 08132018
	// finding moving peaks
	void saveSpectrumData(float fX,float fY, float z1, float z2, CString fileName, int nPoint);
	void removeZeroPad(vector<double> X, vector<double> Y, double *x, double *y);
	void mygaussfit(double noOfData, double *x, int xL, int xR, double *y, int yL, int yR, double &sigma, double &mu, double &A, int ii);
	void polyfit(double *xlog, double *ylog, int N, int n, double *p);
	void find_peaks_func(double *x, double *y, int noOfData, double *all_x_max_g, double *all_y_max_g, int &noOfResultantData, double Threshold, double jump_Threshold, int Radius);
	void classify_peaks_func(double *x1, double *x2, double z1, double z2, int noOfGausPnt1, int noOfGausPnt2, int *status1, int *status2);

	static DWORD MeasureProcVIT(LPVOID pParam);
	virtual BOOL DestroyWindow();

	// Dialog Data
		//{{AFX_DATA(CViteDlg)
	enum { IDD = IDD_VITE_DIALOG };
	// NOTE: the ClassWizard will add data members here
	double	m_fPolyThick;
	CDigiStatic	m_cPolyThick;
	double	m_fThinFilmMinThk;
	double	m_fThinFilmMaxThk;
	double	m_fThinFilmIndex;
	double	m_fExcMinThk;
	double	m_fExcMaxThk;
	BOOL	m_bFilter;
	int		m_nMethod;
	int		m_nlmin;
	int		m_nlmax;
	double	m_fCalCoeff;
	int		m_nMeastime;
	int		m_Nwave;
	int m_nRepeat;
	BOOL	m_bAutoScale;
	int		m_nFFTtype;
	BOOL m_bProbe2;
	BOOL m_bShutter1;
	BOOL m_bShutter2;
	//}}AFX_DATA

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CViteDlg)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	int nSignalMax;
	int nSignalMin;
	float fThkFactor;
	float fPeakMin;
	int nBoxcar;
	int nPort;

	void DrawPicture();
	BOOL bFileOpen;
	BOOL bDialogOpen;
	BOOL bBusy;
	BOOL bRemote;
	int MoveToPosition(double x, double y);
	BOOL bSimu;
	BOOL m_bThickerEtalon;
	float	m_fEtalonThk;
	BOOL bStopLiveFlag;
	HANDLE m_hLiveThread;
	static DWORD WINAPI LiveThreadFunc(LPVOID pParam);

	// Generated message map functions
	//{{AFX_MSG(CViteDlg)
		// NOTE: the ClassWizard will add member functions here
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnMeasurebtn();
	afx_msg void OnThinFilmBackground();
	afx_msg void OnThinFilmCalibration();
	afx_msg void OnThinFilmReset();
	afx_msg void OnKillfocusThinFilmIndex();
	afx_msg void OnKillfocusThinFilmThicknessMax();
	afx_msg void OnKillfocusThinFilmThicknessMin();
	afx_msg void OnKillfocusThinFilmMethod();
	afx_msg void OnKillfocusThinFilmLambdaMax();
	afx_msg void OnKillfocusThinFilmLambdaMin();
	afx_msg void OnKillfocusThinFilmCalCoeff();
	afx_msg void OnKillfocusMeastime();
	afx_msg void OnKillfocusThinFilmNrepeat();
	afx_msg void OnClose();
	afx_msg void OnExit();
	afx_msg void OnSaveSpectrum();
	afx_msg void OnSaveResult();
	afx_msg void OnStart();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnEnKillfocusTfFfttype();
	afx_msg void OnBnClickedEtalonrst();
	afx_msg void OnBnClickedEtalonpos1();
	afx_msg void OnBnClickedEtalonpos2();
	afx_msg void OnBnClickedEtalonpos3();
	afx_msg void OnBnClickedEtalonpos4();
	afx_msg void OnBnClickedSwitchpos2();
	afx_msg void OnBnClickedThickerEtalon();
	afx_msg void OnBnClickedCloseshutter1();
	afx_msg void OnBnClickedCloseshutter2();
	afx_msg void OnEnKillfocusTfExcThicknessMin();
	afx_msg void OnEnKillfocusTfExcThicknessMax();
	afx_msg void OnBnClickedCalibrate();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
	BOOL bNorm;
	afx_msg void OnBnClickedViteFft2();
	int m_threshold;
	int m_jumpThreshold;
	int m_radius;
};
