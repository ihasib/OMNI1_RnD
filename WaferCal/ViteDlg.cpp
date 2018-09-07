// ViteDlg.cpp : implementation file
//

#include "stdafx.h"
// #include "wafercal.h"
#include "AppGlobal.h"
#include "ViteDlg.h"
#include <cmath>
#include "SRC/DOSUtil.h"
#include "OONIRQUEST/OceanOpNIRQuest.h"
#include "OONIRQUEST/FSMSpectrometer.h"
#include <algorithm>
#include "CRecipe.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CViteDlg dialog

#define NSPMAX 2048
#define IDT_VITE_REFRESH 9989
#define IDT_VITE_READ 9988

CViteDlg::CViteDlg(CWnd* pParent)
	: CDialog(CViteDlg::IDD, pParent)
	, bNorm(FALSE)

{
	//{{AFX_DATA_INIT(CViteDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_fPolyThick = 0;
	m_fThinFilmMinThk = 0.1;
	m_fThinFilmMaxThk = 100.0;
	m_fThinFilmIndex = 3.689;
	m_fExcMinThk = 0.0;
	m_fExcMaxThk = 0.0;
	m_bFilter = FALSE;
	m_nMethod = 0;
	m_nlmin = 898;
	m_nlmax = 1716;
	m_fCalCoeff = 1.0;
	m_nMeastime = 30;
	m_Nwave = NSPMAX;
	//}}AFX_DATA_INIT
	pOceanOp = NULL;
	fThkFactor = 1.0;
	fPeakMin = 1.0;
	nSignalMin = 0;
	nSignalMax = 65535;
	nBoxcar = 1;
	m_nRepeat = 1;

	wave = NULL;
	Rmeas = NULL;
	m_points = NULL;
	bFileOpen = FALSE;
	bDialogOpen = FALSE;

	Rmeasmin = 1e20;
	Rmeasmax = -1e20;
	stdev = 0.0;

	int i;
	//	m_hMeasureEndEvent = NULL;
	m_hThread = NULL;
	m_bStopFlag = TRUE;
	fStDev = 1e20;
	nSpecType = SIMUSPEC;
	m_nFFTtype = 0;
	nZP = 2048;
	bRemote = FALSE;
	m_bProbe2 = FALSE;
	m_bThickerEtalon = FALSE;
	m_fEtalonThk = 0.0;
	m_bShutter1 = 0;
	m_bShutter2 = 0;

	m_threshold=5000;
	m_jumpThreshold=10;
	m_radius = 10;
}

void CViteDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CViteDlg)
	// NOTE: the ClassWizard will add DDX and DDV calls here
	DDX_Control(pDX, IDC_VITETHICKNESS, m_cPolyThick);
	DDX_Text(pDX, IDC_VITE_THICKNESS_MIN, m_fThinFilmMinThk);
	DDV_MinMaxDouble(pDX, m_fThinFilmMinThk, 0., 3000.);
	DDX_Text(pDX, IDC_VITE_THICKNESS_MAX, m_fThinFilmMaxThk);
	DDV_MinMaxDouble(pDX, m_fThinFilmMaxThk, 0., 3000.);
	DDX_Text(pDX, IDC_VITE_INDEX, m_fThinFilmIndex);
	DDV_MinMaxDouble(pDX, m_fThinFilmIndex, 1., 10.);
	DDX_Text(pDX, IDC_VITE_EXC_THICKNESS_MAX, m_fExcMaxThk);
	DDV_MinMaxDouble(pDX, m_fExcMaxThk, 0., 3000.);
	DDX_Text(pDX, IDC_VITE_EXC_THICKNESS_MIN, m_fExcMinThk);
	DDV_MinMaxDouble(pDX, m_fExcMinThk, 0., 3000.);
	DDX_Check(pDX, IDC_VITE_FFT, m_bFilter);
	DDX_Text(pDX, IDC_VITE_METHOD, m_nMethod);
	DDV_MinMaxInt(pDX, m_nMethod, 0, 3);//7);
	DDX_Text(pDX, IDC_VITE_LAMBDA_MIN, m_nlmin);
	DDV_MinMaxInt(pDX, m_nlmin, 150, 10000);
	DDX_Text(pDX, IDC_VITE_LAMBDA_MAX, m_nlmax);
	DDV_MinMaxInt(pDX, m_nlmax, 150, 10000);
	DDX_Text(pDX, IDC_VITE_CAL_COEFF, m_fCalCoeff);
	DDV_MinMaxDouble(pDX, m_fCalCoeff, 0.001, 1000.);
	DDX_Text(pDX, IDC_VITE_MEASTIME, m_nMeastime);
	DDV_MinMaxInt(pDX, m_nMeastime, 1, 10000);
	DDX_Text(pDX, IDC_VITE_NREPEAT, m_nRepeat);
	DDV_MinMaxInt(pDX, m_nRepeat, 1, 100);
	DDX_Check(pDX, IDC_VITE_AUTOSCALE, m_bAutoScale);
	DDX_Text(pDX, IDC_VITE_FFTTYPE, m_nFFTtype);
	DDV_MinMaxInt(pDX, m_nFFTtype, 0, 1);
	DDX_Text(pDX, IDC_ETALONPOS, m_fEtalonThk);
	//}}AFX_DATA_MAP
	DDX_Check(pDX, IDC_VITE_FFT2, bNorm);
	DDX_Text(pDX,IDC_THRESHOLD,m_threshold);
	DDX_Text(pDX, IDC_JUMPTHRESHOLD, m_jumpThreshold);
	DDX_Text(pDX, IDC_RADIUS, m_radius);

}

BEGIN_MESSAGE_MAP(CViteDlg, CDialog)
	//{{AFX_MSG_MAP(CViteDlg)
		// NOTE: the ClassWizard will add message map macros here
	ON_BN_CLICKED(IDC_VITE_MEASUREBTN, OnMeasurebtn)
	ON_BN_CLICKED(IDC_VITE_BACKGROUND, OnThinFilmBackground)
	ON_BN_CLICKED(IDC_VITE_CALIBRATION, OnThinFilmCalibration)
	ON_BN_CLICKED(IDC_VITE_RESET, OnThinFilmReset)
	ON_EN_KILLFOCUS(IDC_VITE_INDEX, OnKillfocusThinFilmIndex)
	ON_EN_KILLFOCUS(IDC_VITE_THICKNESS_MAX, OnKillfocusThinFilmThicknessMax)
	ON_EN_KILLFOCUS(IDC_VITE_THICKNESS_MIN, OnKillfocusThinFilmThicknessMin)
	ON_EN_KILLFOCUS(IDC_VITE_METHOD, OnKillfocusThinFilmMethod)
	ON_EN_KILLFOCUS(IDC_VITE_LAMBDA_MAX, OnKillfocusThinFilmLambdaMax)
	ON_EN_KILLFOCUS(IDC_VITE_LAMBDA_MIN, OnKillfocusThinFilmLambdaMin)
	ON_EN_KILLFOCUS(IDC_VITE_CAL_COEFF, OnKillfocusThinFilmCalCoeff)
	ON_EN_KILLFOCUS(IDC_VITE_MEASTIME, OnKillfocusMeastime)
	ON_EN_KILLFOCUS(IDC_VITE_NREPEAT, OnKillfocusThinFilmNrepeat)
	ON_WM_CLOSE()
	ON_BN_CLICKED(IDC_VITE_EXIT, OnExit)
	ON_BN_CLICKED(IDC_VITE_SAVE_SPECTRUM, OnSaveSpectrum)
	ON_BN_CLICKED(IDC_VITE_SAVE_RESULT, OnSaveResult)
	ON_BN_CLICKED(IDC_VITE_START, OnStart)
	ON_WM_PAINT()
	ON_WM_TIMER()
	ON_EN_KILLFOCUS(IDC_VITE_FFTTYPE, OnEnKillfocusTfFfttype)
	ON_BN_CLICKED(IDC_ETALONRST, OnBnClickedEtalonrst)
	ON_BN_CLICKED(IDC_ETALONPOS1, OnBnClickedEtalonpos1)
	ON_BN_CLICKED(IDC_ETALONPOS2, OnBnClickedEtalonpos2)
	ON_BN_CLICKED(IDC_ETALONPOS3, OnBnClickedEtalonpos3)
	ON_BN_CLICKED(IDC_ETALONPOS4, OnBnClickedEtalonpos4)
	ON_BN_CLICKED(IDC_SWITCHPOS2, OnBnClickedSwitchpos2)
	ON_BN_CLICKED(IDC_THICKER_ETALON, OnBnClickedThickerEtalon)
	ON_BN_CLICKED(IDC_CLOSESHUTTER1, OnBnClickedCloseshutter1)
	ON_BN_CLICKED(IDC_CLOSESHUTTER2, OnBnClickedCloseshutter2)
	ON_EN_KILLFOCUS(IDC_VITE_EXC_THICKNESS_MIN, OnEnKillfocusTfExcThicknessMin)
	ON_EN_KILLFOCUS(IDC_VITE_EXC_THICKNESS_MAX, OnEnKillfocusTfExcThicknessMax)
	ON_BN_CLICKED(IDC_VITE_CALIBRATE, OnBnClickedCalibrate)
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_VITE_FFT2, &CViteDlg::OnBnClickedViteFft2)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CViteDlg message handlers

BOOL CViteDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	bDialogOpen = TRUE;

	Etalon.Initialize();
	OpSwitch.Initialize();

	Initialize();

	UpdateData(FALSE);
	UpdatePars();

	if (pOceanOp)
	{
		pOceanOp->Nwave = 512;
		pOceanOp->Measure();
		pOceanOp->GetSpectrumData(FALSE, wave, Rmeas, NSPMAX);
	}
	// TODO: Add extra initialization here

//	SetTimer(IDT_VITE_REFRESH, 60000, NULL);
//	SetTimer(IDT_VITE_READ, 300000, NULL);

	// TODO: Add extra initialization here
	if (Etalon.GetPort() == 0)
	{
		GetDlgItem(IDC_ETALONRST)->EnableWindow(FALSE);
		GetDlgItem(IDC_ETALONPOS1)->EnableWindow(FALSE);
		GetDlgItem(IDC_ETALONPOS2)->EnableWindow(FALSE);
		GetDlgItem(IDC_ETALONPOS3)->EnableWindow(FALSE);
		GetDlgItem(IDC_ETALONPOS4)->EnableWindow(FALSE);
	}
	if (OpSwitch.GetPort() == 0)
	{
		GetDlgItem(IDC_SWITCHPOS2)->EnableWindow(FALSE);
		GetDlgItem(IDC_CLOSESHUTTER2)->EnableWindow(FALSE);
	}
	m_cPolyThick.SetText(" 0.000");
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CViteDlg::OnMeasurebtn()
{
	// TODO: Add your control notification handler code here
	CButton* pFilter = (CButton*)GetDlgItem(IDC_VITE_FFT);
	m_bFilter = pFilter->GetCheck() && 0x0001;
	pFilter = (CButton*)GetDlgItem(IDC_VITE_AUTOSCALE);
	m_bAutoScale = pFilter->GetCheck() && 0x0001;
	//	UpdateData(TRUE);

	UpdatePars();

	EnableButtons(FALSE);
	double ret;

	bBusy = TRUE;
	if (pOceanOp->Status() == 0)
	{
		pOceanOp->Start();

		Sleep(1);
		pOceanOp->Result(m_fPolyThick);
		if ((Etalon.nEtalonCurPos > 0) && (m_fThinFilmIndex > 0))
		{
			if (m_bThickerEtalon)
				m_fPolyThick = Etalon.fEtalonRefr[Etalon.nEtalonCurPos - 1] * Etalon.fEtalonThk[Etalon.nEtalonCurPos - 1] / m_fThinFilmIndex - m_fPolyThick;
			else
				m_fPolyThick += Etalon.fEtalonRefr[Etalon.nEtalonCurPos - 1] * Etalon.fEtalonThk[Etalon.nEtalonCurPos - 1] / m_fThinFilmIndex;
		}
		CString s1, s2;
		s1.Format("%.3f", m_fPolyThick);
		s2.Format("% 5s", s1);
		m_cPolyThick.SetText(s2);
		//		UpdateData(FALSE);
	}
	else
		AfxMessageBox("Sensor is not ready");

	DisplaySpectrum();

	//FSMB 08132018
	// finding moving peak by analyzing TWO FFT data

	UpdateData(TRUE);
	
	if (pOceanOp->Status() == 0)
	{
		// Reading First FFT Data before moving Z motor
		int nNumberOfData = pOceanOp->GetDataNumber(TRUE); // true for fft

		double wave1[NSPMAX], Rmeas1[NSPMAX], peak_wave1[NSPMAX], peak_Rmeas1[NSPMAX];
		int npeak1;
		
		if (nNumberOfData > 0)
		{
			pOceanOp->GetSpectrumData(TRUE, wave, Rmeas, NSPMAX); /// true for fft

			for (int i = 0; i < nNumberOfData; i++)
			{
				wave1[i] = wave[i];
				Rmeas1[i] = Rmeas[i];
			}
			/*FILE *f = fopen("Z Data.txt", "w+");
			for (int i = 0; i < nNumberOfData; i++)
			{
				fprintf(f, "%.3f %.3f\n", wave1[i], Rmeas1[i]);
			}
			fclose(f);*/

			//added by  hasib 08202018
			double scan1, scan2;
			FILE *pFile = fopen("C:\\413 OMNI1\\FFT64.txt", "r");
			if (pFile != NULL)
			{
				double scan1, scan2;
				nNumberOfData = 0;
				while (fscanf(pFile, "%lf %lf", &scan1, &scan2) != EOF)
				{
					wave1[nNumberOfData] = scan1;
					Rmeas1[nNumberOfData] = scan2;
					nNumberOfData++;
				}
				fclose(pFile);
			}

			//ended by  hasib 08202018

			find_peaks_func(wave1, Rmeas1, nNumberOfData, peak_wave1, peak_Rmeas1, npeak1,m_threshold,m_jumpThreshold,m_radius);

		}

		// Reading Second FFT Data after moving Z motor
		double wave2[NSPMAX], Rmeas2[NSPMAX], peak_wave2[NSPMAX], peak_Rmeas2[NSPMAX];
		int npeak2;

		if (AGV.pS8Dlg)
		{
			char inifile[_MAX_PATH];
			char drive[_MAX_DRIVE];
			char dir[_MAX_DIR];
			char fname[_MAX_FNAME];
			char ext[_MAX_EXT];
			GetModuleFileName(NULL, inifile, _MAX_PATH);
			_splitpath(inifile, drive, dir, fname, ext);
			_makepath(inifile, drive, dir, "FSMLocalCfg", "ini");

			char str[255];
			GetPrivateProfileString("VITEMeasurement", "DeltaZ", str, str, 255, inifile);
			float deltaZ = atof(str);
			float fZ1 = 0, fZ2;
			AGV.pS8Dlg->pMc->GetZPos(&fZ1, &fZ2);

			if (AGV.pS8Dlg->pMc->vMoveZ1Fast(fZ1 + deltaZ))
			{
				Sleep(200);
				pOceanOp->Start();
				pOceanOp->Result(m_fPolyThick);

				nNumberOfData = pOceanOp->GetDataNumber(TRUE);
				pOceanOp->GetSpectrumData(TRUE, wave, Rmeas, NSPMAX); /// true for fft

				for (int i = 0; i < nNumberOfData; i++)//converting float to double
				{
					wave2[i] = wave[i];
					Rmeas2[i] = Rmeas[i];
				}

				/*FILE *f = fopen("Z2 Data.txt", "w+");
				for (int i = 0; i < nNumberOfData; i++)
				{
					fprintf(f, "%.3lf %.3lf\n", wave2[i], Rmeas2[i]);
				}
				fclose(f);*/

				//added by  hasib 08202018
				double scan1, scan2;
				FILE *pFile = fopen("C:\\413 OMNI1\\FFT65.txt", "r");
				if (pFile != NULL)
				{
					double scan1, scan2;
					nNumberOfData = 0;
					while (fscanf(pFile, "%lf %lf", &scan1, &scan2) != EOF)
					{
						wave2[nNumberOfData] = scan1;
						Rmeas2[nNumberOfData] = scan2;
						nNumberOfData++;
					}
					fclose(pFile);
				}

				//ended by  hasib 08202018

				find_peaks_func(wave2, Rmeas2, nNumberOfData, peak_wave2, peak_Rmeas2, npeak2, m_threshold, m_jumpThreshold, m_radius);
				
				// Move the Z motor to previous position
				//AGV.pS8Dlg->m_fZ = oldZ;
				AGV.pS8Dlg->pMc->vMoveZ1Fast(fZ1);
			}

			int status1[NSPMAX] = { 0 }, status2[NSPMAX] = { 0 };
			//hasib debug
			//fZ1 = 6.4, fZ2 = 6.5;
			deltaZ = (fZ2 - fZ1);//Z motor position difference[microns].Checks if we are moving the probe towards the sample or opposite direction
			classify_peaks_func(peak_wave1, peak_wave2, (double)fZ1, (double)(fZ1 + deltaZ), npeak1, npeak2, status1, status2);

			FILE *fp = fopen("z_class.txt", "w+");
			for (int i = 0; i < npeak1; i++)
			{
				fprintf(fp, "s1=%d s2=%d\n", status1[i], status2[i]);
			}
			fclose(fp);

			float xpos[50];
			int noOfMoving = 0;
			for (int i = 0; i < npeak1; i++)
				if (status1[i] == 1)
					xpos[noOfMoving++] = peak_wave1[i];
			/*for (int i = 0; i < npeak2; i++)
				if (status2[i] == 1)
					xpos[noOfMoving++] = peak_wave2[i];
			sort(xpos, xpos + noOfMoving);*/

			/*FILE *f3 = fopen("z_sorted.txt", "w+");
			for (int i = 0; i < noOfMoving; i++)
			{
				fprintf(f3, "%.3f \n", xpos[i]);
			}
			fclose(f3);*/

			if (noOfMoving == 0)
			{
				AfxMessageBox(_T("No moving peak found!"));
			}
			else //show output in Recipe Setting Dialog box
			{
				float  lastPeak = 0, curPeak, fTotalThk=0;
				int lastPeakInd = 0;

				CRecipe *rp = AGV.GetRecipe();

				if (noOfMoving>0)
				{
					rp->fFilmThk = (xpos[lastPeakInd] - lastPeak) / rp->fPolyIndex;
					fTotalThk += rp->fFilmThk;
					//m_fPolyThick = rp->fFilmThk;
					lastPeak = xpos[lastPeakInd];
					lastPeakInd++;
					noOfMoving--;

					CString s1, s2;
					s1.Format("%.3f", m_fPolyThick);
					s2.Format("% 5s", s1);
					//m_cPolyThick.SetText(s2);
				}

				if (noOfMoving>0)
				{
					rp->fSiThk = (xpos[lastPeakInd] - lastPeak) / rp->fSiIndex;
					fTotalThk += rp->fSiThk;
					lastPeak = xpos[lastPeakInd];
					lastPeakInd++;
					noOfMoving--;
				}
				if (noOfMoving>0)
				{
					rp->fTapeThk = (xpos[lastPeakInd] - lastPeak) / rp->fTapeIndex;
					fTotalThk += rp->fTapeThk;
					lastPeak = xpos[lastPeakInd];
					lastPeakInd++;
					noOfMoving--;
				}

				if (noOfMoving>0)
				{
					rp->fTape2Thk = (xpos[lastPeakInd] - lastPeak) / rp->fTapeIndex2;
					fTotalThk += rp->fTape2Thk;
					lastPeak = xpos[lastPeakInd];
					lastPeakInd++;
					noOfMoving--;
				}
			}
		}

		// storing spectrum for every points of recipe
		char inifile[_MAX_PATH];
		char drive[_MAX_DRIVE];
		char dir[_MAX_DIR];
		char fname[_MAX_FNAME];
		char ext[_MAX_EXT];
		GetModuleFileName(NULL, inifile, _MAX_PATH);
		_splitpath(inifile, drive, dir, fname, ext);
		_makepath(inifile, drive, dir, "FSMLocalCfg", "ini");

		char str[255];
		GetPrivateProfileString("VITEMeasurement", "DeltaZ", str, str, 255, inifile);
		float deltaZ = atof(str);
		float fZ1 = 0, fZ2;
		AGV.pS8Dlg->pMc->GetZPos(&fZ1, &fZ2);

		CRecipe *rp = AGV.GetRecipe();
		if (rp)
		{
			_splitpath(rp->name, drive, dir, fname, ext);

			CObList MPList;
			rp->GetMPList(MPList);
			POSITION pos = MPList.GetHeadPosition();
			int count = 0;
			while (pos)
			{
				CMPoint * p = (CMPoint *)MPList.GetNext(pos);
				if (p)
				{
					AGV.pS8Dlg->pMc->GotoXY(p->Co.x, p->Co.y);
					Sleep(100);

					saveSpectrumData(p->Co.x, p->Co.y,fZ1, fZ1 + deltaZ, fname, ++count);
				}
			}
		}
	}

	EnableButtons(TRUE);
	bBusy = FALSE;
}
void CViteDlg::saveSpectrumData(float fX, float fY, float fZ1, float fZ2, CString fileName, int nPoint)
{
	Sleep(200);
	pOceanOp->Start();
	pOceanOp->Result(m_fPolyThick);

	int nNumberOfData = pOceanOp->GetDataNumber(TRUE);
	pOceanOp->GetSpectrumData(TRUE, wave, Rmeas, NSPMAX); /// true for fft

	CString strTemp;
	char strDir[_MAX_PATH] = { 0 }, Dir[_MAX_PATH], Drive[52], Fname[_MAX_PATH], Ext[26];
	::GetModuleFileName(NULL, strDir, _MAX_PATH);
	::_splitpath(strDir, Drive, Dir, Fname, Ext);
	strTemp = Drive;
	strTemp += Dir;
	strTemp += _T("data\\");
	strTemp += fileName + _T("\\");
	if (-1 == GetFileAttributes(strTemp))
	{
		// Does not exist so create
		if (NULL == ::CreateDirectory(strTemp, NULL))
		{
			AfxMessageBox("Cannot create default data directory. Exiting", MB_OK);
			return;
		}
	}
	CString  stNPoint, sZ1, sZ2,sX,sY;
	stNPoint.Format("%d", nPoint);

	sX.Format("%.3f",fX);
	sX.Replace('.', '_');

	sY.Format("%.3f", fY);
	sY.Replace('.', '_');

	sZ1.Format("%.3f", fZ1);
	sZ1.Replace('.', '_');

	sZ2.Format("%.3f", fZ2);
	sZ2.Replace('.', '_');


	FILE *f = fopen(strTemp + "\\P" + stNPoint + "_X_"+sX+ "_Y_" + sY + "_Z_"+ sZ1 + ".txt", "w+");
	for (int i = 0; i < nNumberOfData; i++)
		fprintf(f, "%.3f	%.3f\n", wave[i], Rmeas[i]);
	fclose(f);

	//move z motor to new position
	if (AGV.pS8Dlg->pMc->vMoveZ1Fast(fZ2))
	{
		Sleep(50);
		pOceanOp->Start();
		pOceanOp->Result(m_fPolyThick);

		nNumberOfData = pOceanOp->GetDataNumber(TRUE);
		pOceanOp->GetSpectrumData(TRUE, wave, Rmeas, NSPMAX); /// true for fft

		FILE *f2 = fopen(strTemp + "\\P" + stNPoint + "_X_" + sX + "_Y_" + sY + "_Z_" + sZ2 + ".txt", "w+");
		for (int i = 0; i < nNumberOfData; i++)
			fprintf(f2, "%.3f	%.3f\n", wave[i], Rmeas[i]);
		fclose(f2);

		AGV.pS8Dlg->pMc->vMoveZ1Fast(fZ1);
	}
	
}

void CViteDlg::OnThinFilmBackground()
{
	// TODO: Add your control notification handler code here
	m_fPolyThick = 0;
	EnableButtons(FALSE);
	if (pOceanOp) pOceanOp->GetBackground();
	EnableButtons(TRUE);
}

void CViteDlg::OnThinFilmCalibration()
{
	// TODO: Add your control notification handler code here
	m_fPolyThick = 0;
	EnableButtons(FALSE);
	if (pOceanOp) pOceanOp->Calibrate();
	EnableButtons(TRUE);
}

void CViteDlg::OnThinFilmReset()
{
	// TODO: Add your control notification handler code here
	m_fPolyThick = 0;
	EnableButtons(FALSE);
	if (pOceanOp) pOceanOp->CalibrationReset();
	EnableButtons(TRUE);
}

void CViteDlg::OnKillfocusThinFilmIndex()
{
	// TODO: Add your control notification handler code here
	UpdateData(TRUE);
	UpdatePars();
}

void CViteDlg::OnKillfocusThinFilmThicknessMin()
{
	// TODO: Add your control notification handler code here
	UpdateData(TRUE);
	UpdatePars();
}

void CViteDlg::OnKillfocusThinFilmThicknessMax()
{
	// TODO: Add your control notification handler code here
	UpdateData(TRUE);
	UpdatePars();
}

void CViteDlg::NormalizeSpectrum(float* R, int N)
{
	//normalize measured data

/////changed 01/28/2006
	int nmin = m_nlmin;
	int nmax = m_nlmax;
	m_nlmin = pOceanOp->oopars.lambdamin;
	m_nlmax = pOceanOp->oopars.lambdamax;

	if (m_nMethod > 7)
	{
		float* R2 = new float[N];
		int i;
		for (i = 1; i < N - 1; i++)
		{
			R2[i] = (R[i + 1] - R[i - 1]) / 2;
		}
		R2[0] = R2[1];
		R2[N - 1] = R2[N - 2];
		for (i = 0; i < N; i++)
		{
			R[i] = (R2[i]) / 2;
		}
		if (R2)
		{
			delete[] R2;
			R2 = NULL;
		}
	}
	/////
	Rmeasmax = -1e20;
	Rmeasmin = 1e20;
	if ((m_nMethod <= 3) && (m_bFilter))
	{
		for (int i = 0; i < N; i++)
		{
			if ((wave[i] >= m_fThinFilmMinThk) && (wave[i] <= m_fThinFilmMaxThk) && (R[i] > Rmeasmax))
				Rmeasmax = R[i];
			if ((wave[i] >= m_fThinFilmMinThk) && (wave[i] <= m_fThinFilmMaxThk) && (R[i] < Rmeasmin))
				Rmeasmin = R[i];
		}
	}
	else
	{
		for (int i = 0; i < N; i++)
		{
			if (((m_nMethod <= 3) || ((wave[i] >= m_nlmin) && (wave[i] <= m_nlmax))) && (R[i] > Rmeasmax))
				Rmeasmax = R[i];
			if (((m_nMethod <= 3) || ((wave[i] >= m_nlmin) && (wave[i] <= m_nlmax))) && (R[i] < Rmeasmin))
				Rmeasmin = R[i];
		}
	}

	double Rmaxabs = max(fabs(Rmeasmin), fabs(Rmeasmax));

	if ((m_nMethod >= 4) && (m_nMethod <= 5) && (Rmeasmax > Rmeasmin))
	{
		for (int i = 0; i < N; i++)
		{
			R[i] = (R[i] - Rmeasmin) / (Rmeasmax - Rmeasmin);
		}
		Rmeasmin = 0.0;
		Rmeasmax = 1.0;
	}
	if ((m_nMethod > 7) && (Rmaxabs > 0))
	{
		for (int i = 0; i < N; i++)
		{
			R[i] /= Rmaxabs;
		}
		Rmaxabs = 1.0;
	}
	m_nlmin = nmin;
	m_nlmax = nmax;
}

void CViteDlg::DisplaySpectrum()
{
	int nlmin = m_nlmin;
	int nlmax = m_nlmax;
	if (pOceanOp)
	{
		CButton* pBtn = (CButton*)GetDlgItem(IDC_VITE_FFT);
		m_bFilter = pBtn->GetCheck() && 0x0001;
		pBtn = (CButton*)GetDlgItem(IDC_VITE_AUTOSCALE);
		m_bAutoScale = pBtn->GetCheck() && 0x0001;

		int nNumberOfData = pOceanOp->GetDataNumber(m_bFilter);

		if (nNumberOfData > 0)
		{
			memset(wave, 0, NSPMAX * sizeof(float));
			bFileOpen = TRUE;
			memset(Rmeas, 0, NSPMAX * sizeof(float));

			pOceanOp->GetSpectrumData(m_bFilter, wave, Rmeas, NSPMAX);

			if (!m_bFilter)
			{
				if (wave[0] > wave[nNumberOfData - 1])
				{
					m_nlmax = wave[0];
					m_nlmin = wave[nNumberOfData - 1];
				}
				else
				{
					m_nlmin = wave[0];
					m_nlmax = wave[nNumberOfData - 1];
				}
			}

			NormalizeSpectrum(Rmeas, nNumberOfData);
			if (!m_bFilter)
				m_Nwave = nNumberOfData;
			DrawPicture();
		}
	}
	m_nlmin = nlmin;
	m_nlmax = nlmax;
}

int CViteDlg::GetAverageData()
{
	if (pOceanOp)
	{
		pOceanOp->oopars.nMethod = 0;
		pOceanOp->Measure();
		int nPts = pOceanOp->GetDataNumber(FALSE);
		float fSum = 0;

		if (nPts > 0)
		{
			float *fX, *fY;
			fX = new float[nPts];
			fY = new float[nPts];
			pOceanOp->GetSpectrumData(FALSE, fX, fY, NSPMAX);
			for (int i = 0; i < nPts; i++)
			{
				fSum += fY[i];
			}
			if (nPts > 1)
				fSum /= nPts;
			if (fX)
				delete[] fX;
			if (fY)
				delete[] fY;
		}
		pOceanOp->oopars.nMethod = m_nMethod;
		return fSum;
	}
	else
		return -100;
}

BOOL CViteDlg::MeasureReference()
{
	if (pOceanOp)
	{
		int nM = pOceanOp->oopars.nMethod;
		pOceanOp->oopars.nMethod = 0;
		pOceanOp->Calibrate();
		pOceanOp->oopars.nMethod = nM;
		return TRUE;
	}
	else
		return FALSE;
}

BOOL CViteDlg::MeasureBackground()
{
	if (pOceanOp)
	{
		int nM = pOceanOp->oopars.nMethod;
		pOceanOp->oopars.nMethod = 0;
		pOceanOp->GetBackground();
		pOceanOp->oopars.nMethod = nM;
		return TRUE;
	}
	else
		return FALSE;
}

void CViteDlg::OnKillfocusThinFilmMethod()
{
	UpdateData(TRUE);
	UpdatePars();
}

void CViteDlg::OnKillfocusThinFilmLambdaMax()
{
	UpdateData(TRUE);
	UpdatePars();
}

void CViteDlg::OnKillfocusThinFilmLambdaMin()
{
	UpdateData(TRUE);
	UpdatePars();
}

void CViteDlg::OnKillfocusMeastime()
{
	UpdateData(TRUE);
	UpdatePars();
}

void CViteDlg::OnKillfocusThinFilmCalCoeff()
{
	UpdateData(TRUE);
	UpdatePars();
}

void CViteDlg::OnKillfocusThinFilmNrepeat()
{
	UpdateData(TRUE);
	UpdatePars();
}

void CViteDlg::UpdatePars()
{
	if (pOceanOp)
	{
		pOceanOp->oopars.fThkMin = m_fThinFilmMinThk;
		pOceanOp->oopars.fThkMax = m_fThinFilmMaxThk;
		pOceanOp->oopars.nMethod = m_nMethod;
		pOceanOp->oopars.fCalCoeff = m_fCalCoeff;
		pOceanOp->oopars.lambdamin = m_nlmin;
		pOceanOp->oopars.lambdamax = m_nlmax;
		pOceanOp->oopars.nMeastime = m_nMeastime;
		pOceanOp->oopars.m_RefrIndex = m_fThinFilmIndex;
		pOceanOp->oopars.SignalMax = nSignalMax;
		pOceanOp->oopars.SignalMin = nSignalMin;
		pOceanOp->oopars.fThkFactor = fThkFactor;
		pOceanOp->oopars.fPeakMin = fPeakMin;
		float fFactor = pOceanOp->oopars.fThkFactor * pOceanOp->oopars.m_RefrIndex
			* 2e3 * (1.0 / pOceanOp->oopars.lambdamin - 1.0 / pOceanOp->oopars.lambdamax);
		if (pOceanOp->nZeroPadding > 0)
		{
			float fDeltaWave = (1.0 / pOceanOp->oopars.lambdamin - 1.0 / pOceanOp->oopars.lambdamax) / (512 - 1);
			fFactor *= (1.0 / pOceanOp->oopars.lambdamin - 1.0 / pOceanOp->oopars.lambdamax + fDeltaWave * pOceanOp->nZeroPadding)
				/ (1.0 / pOceanOp->oopars.lambdamin - 1.0 / pOceanOp->oopars.lambdamax);
		}
		pOceanOp->oopars.nFFTBeg = (int)(pOceanOp->oopars.fThkMin * fFactor);
		pOceanOp->oopars.nFFTEnd = (int)(pOceanOp->oopars.fThkMax *fFactor) + 1;
		if (pOceanOp->oopars.nFFTEnd < pOceanOp->oopars.nFFTBeg)
			pOceanOp->oopars.nFFTEnd = pOceanOp->oopars.nFFTBeg + 1;

		pOceanOp->oopars.nFFTBeg = (int)(pOceanOp->oopars.fThkMin * fFactor);
		pOceanOp->oopars.nFFTEnd = (int)(pOceanOp->oopars.fThkMax
			* pOceanOp->oopars.fThkFactor * fFactor) + 1;
		pOceanOp->oopars.boxcar = nBoxcar;
		pOceanOp->oopars.nPort = nPort;
		pOceanOp->oopars.nRepeat = m_nRepeat;
		pOceanOp->nFFTtype = m_nFFTtype;
		//		pOceanOp->nZP = nZP;
		pOceanOp->oopars.fExcThkMin = m_fExcMinThk;
		pOceanOp->oopars.fExcThkMax = m_fExcMaxThk;
		pOceanOp->oopars.nFFTExcBeg = (int)(pOceanOp->oopars.fExcThkMin * fFactor);
		pOceanOp->oopars.nFFTExcEnd = (int)(pOceanOp->oopars.fExcThkMax * fFactor);
		//

		//
	}

	if (bDialogOpen)
	{
		GetDlgItem(IDC_VITE_THICKNESS_MIN)->EnableWindow(TRUE);
		GetDlgItem(IDC_VITE_THICKNESS_MAX)->EnableWindow(TRUE);
		GetDlgItem(IDC_VITE_CAL_COEFF)->EnableWindow(TRUE);
		GetDlgItem(IDC_VITE_INDEX)->EnableWindow(TRUE);
		GetDlgItem(IDC_VITE_FFT)->EnableWindow(TRUE);
		GetDlgItem(IDC_VITE_START)->EnableWindow(TRUE);
	}

	char inifile[_MAX_PATH];
	char drive[_MAX_DRIVE];
	char dir[_MAX_DIR];
	char fname[_MAX_FNAME];
	char ext[_MAX_EXT];
	GetModuleFileName(NULL, inifile, _MAX_PATH);
	_splitpath(inifile, drive, dir, fname, ext);
	_makepath(inifile, drive, dir, "FSMLocalCfg", "ini");
	CString str, str2;
	int i;
	str.Format("%d", m_nlmax);
	WritePrivateProfileString("VITEMeasurement", "lambdamax", str, inifile);
	str.Format("%d", m_nlmin);
	WritePrivateProfileString("VITEMeasurement", "lambdamin", str, inifile);
	str.Format("%d", m_nMethod);
	WritePrivateProfileString("VITEMeasurement", "Method", str, inifile);
	str.Format("%.3f", m_fThinFilmMaxThk);
	WritePrivateProfileString("VITEMeasurement", "MaxFilmThk", str, inifile);
	str.Format("%.3f", m_fThinFilmMinThk);
	WritePrivateProfileString("VITEMeasurement", "MinFilmThk", str, inifile);
	str.Format("%.3f", m_fThinFilmIndex);
	WritePrivateProfileString("VITECalibration", "FilmIndex", str, inifile);
	//	AGV.m_FilmIndex = m_fThinFilmIndex;
	str.Format("%.3f", m_fCalCoeff);
	WritePrivateProfileString("VITEMeasurement", "CalCoeff", str, inifile);
	str.Format("%d", m_nMeastime);
	WritePrivateProfileString("VITEMeasurement", "Meastime", str, inifile);

	str.Format("%d", m_nRepeat);
	WritePrivateProfileString("VITEMeasurement", "Repeat", str, inifile);
}

int CViteDlg::GetDataNumber(BOOL bFFT)
{
	if (pOceanOp) return pOceanOp->GetDataNumber(bFFT);
	else return 100;
}

void CViteDlg::GetSpectrumData(BOOL bFFT, float *DataArray1, float *DataArray2, int N)
{
	if (pOceanOp) pOceanOp->GetSpectrumData(bFFT, DataArray1, DataArray2, N);
}
// Sets the Ranges of expected thicknesses for a measurement recipe
void CViteDlg::SetThkRange(float fThk1, float fThk2, float fIndex, float fExcThk1, float fExcThk2)
{
	if (pOceanOp) pOceanOp->SetThkRange(fThk1, fThk2, fIndex, fExcThk1, fExcThk2);
	m_fThinFilmMinThk = fThk1; // The expected minimum thickness of the Film layer
	m_fThinFilmMaxThk = fThk2; // The expected maximum thickness of the Film Layer
	m_fThinFilmIndex = fIndex; // The refractive index of the Film layer
	m_fExcMinThk = fExcThk1;
	m_fExcMaxThk = fExcThk2;
	char str[50];
	sprintf(str, "%.3f", m_fThinFilmMinThk);
	SetDlgItemText(IDC_VITE_THICKNESS_MIN, str);
	sprintf(str, "%.3f", m_fThinFilmMaxThk);
	SetDlgItemText(IDC_VITE_THICKNESS_MAX, str);
	sprintf(str, "%.3f", m_fThinFilmIndex);
	SetDlgItemText(IDC_VITE_INDEX, str);
	sprintf(str, "%.3f", m_fExcMinThk);
	SetDlgItemText(IDC_VITE_EXC_THICKNESS_MIN, str);
	sprintf(str, "%.3f", m_fExcMaxThk);
	SetDlgItemText(IDC_VITE_EXC_THICKNESS_MAX, str);
}

BOOL CViteDlg::Measure()
{
	if (pOceanOp) return pOceanOp->Measure();
	else return FALSE;
}

double CViteDlg::GetPolyThickness()
{
	double fPolyThk;
	if (pOceanOp)
	{
		fPolyThk = pOceanOp->GetPolyThickness();
		if ((fPolyThk <= m_fThinFilmMinThk) || (fPolyThk >= m_fThinFilmMaxThk))
		{
			fPolyThk = 0;
		}
		return fPolyThk;
	}
	else
	{
		return 0;
	}
}

void CViteDlg::Initialize()
{
	char str[255];
	CString inifile;
	CString str2;
	int i;

	DosUtil.GetLocalCfgFile(inifile);
	sprintf(str, "%.3f", m_fThinFilmMinThk);
	GetPrivateProfileString("VITEMeasurement", "MinFilmThk", str, str, 255, inifile);
	m_fThinFilmMinThk = atof(str);
	sprintf(str, "%.3f", m_fThinFilmMaxThk);
	GetPrivateProfileString("VITEMeasurement", "MaxFilmThk", str, str, 255, inifile);
	m_fThinFilmMaxThk = atof(str);
	sprintf(str, "%.3f", m_fThinFilmIndex);
	GetPrivateProfileString("VITECalibration", "FilmIndex", str, str, 255, inifile);
	m_fThinFilmIndex = atof(str);
	m_nMethod = GetPrivateProfileInt("VITEMeasurement", "Method", 0, inifile);
	m_nlmin = GetPrivateProfileInt("VITEMeasurement", "lambdamin", 416, inifile);
	m_nlmax = GetPrivateProfileInt("VITEMeasurement", "lambdamax", 833, inifile);
	sprintf(str, "%.3f", m_fCalCoeff);
	GetPrivateProfileString("VITEMeasurement", "CalCoeff", str, str, 255, inifile);
	m_fCalCoeff = atof(str);
	m_nMeastime = GetPrivateProfileInt("VITEMeasurement", "Meastime", 30, inifile);
	bSimu = GetPrivateProfileInt("VITEMeasurement", "bSimu", 0, inifile);

	nSignalMax = GetPrivateProfileInt("VITEMeasurement", "SignalMax", 65535, inifile);
	nSignalMin = GetPrivateProfileInt("VITEMeasurement", "SignalMin", 0, inifile);
	nBoxcar = GetPrivateProfileInt("VITEMeasurement", "Boxcar", 1, inifile);
	nPort = GetPrivateProfileInt("VITEMeasurement", "PortNo", 4, inifile);

	sprintf(str, "%.3f", fThkFactor);
	GetPrivateProfileString("VITEMeasurement", "ThkFactor", str, str, 255, inifile);
	fThkFactor = atof(str);
	sprintf(str, "%.3f", fPeakMin);
	GetPrivateProfileString("VITEMeasurement", "PeakMin", str, str, 255, inifile);
	fPeakMin = atof(str);
	m_nRepeat = GetPrivateProfileInt("VITEMeasurement", "Repeat", 1, inifile);

	if (m_nlmax <= m_nlmin)
		m_nlmax = m_nlmin + 1;

	m_Nwave = GetPrivateProfileInt("VITEMeasurement", "nwave", NSPMAX, inifile);
	int Nwave = m_Nwave;//GetPrivateProfileInt("Measurement", "nwave", NPIX, inifile);

	if (!pOceanOp)
	{
		if (bSimu)
		{
			pOceanOp = new COceanBase(0);
		}
		else
		{
			nSpecType = (SPECTYPE)GetPrivateProfileInt("VITESpectrometer", "Type", 1, inifile);
			switch (nSpecType)
			{
			case SPEC1:
			case SPEC2:
			case OCEAN: pOceanOp = new COceanOpNIRQuest(1); break;
			case FSMSPEC: pOceanOp = new CFSMSpectrometer(1); break;
			default: pOceanOp = new COceanBase(0); break;
			}
		}
	}
	if (pOceanOp)
	{
		m_points = new POINT[NSPMAX + pOceanOp->nZeroPadding];
	}
	else
	{
		m_points = new POINT[NSPMAX];
	}

	if (pOceanOp->nZeroPadding > 0)
	{
		Rmeas = new float[NSPMAX + pOceanOp->nZeroPadding];
		memset(Rmeas, 0, NSPMAX + pOceanOp->nZeroPadding);

		wave = new float[NSPMAX + pOceanOp->nZeroPadding];
		memset(wave, 0, NSPMAX + pOceanOp->nZeroPadding);
	}
	else
	{
		Rmeas = new float[NSPMAX];
		memset(Rmeas, 0, NSPMAX);
		wave = new float[NSPMAX];
		memset(wave, 0, NSPMAX);
	}
	for (i = 0; i < m_Nwave; i++)
		wave[i] = 1.0 / (1.0 / m_nlmax + i * (1.0 / m_nlmin - 1.0 / m_nlmax) / (m_Nwave - 1));
	bRemote = FALSE;//GetPrivateProfileInt("Measurement", "Remote", 0, inifile);

	//Etalon.Start();
	OpSwitch.Start();

	UpdatePars();
}

void CViteDlg::OnClose()
{
	if (bRemote)
		return;
	UpdateData(TRUE);
	/*
		UpdateData(TRUE);
	/*
		if(m_hMeasureEndEvent)
		{
			CloseHandle(m_hMeasureEndEvent);
			m_hMeasureEndEvent = NULL;
		}*/

	m_bStopFlag = TRUE;

	if (m_hThread)
	{
		WaitForSingleObject(m_hThread, 5000);
		TRACE("Close MeasureProc Thread %x\n", m_hThread);
		CloseHandle(m_hThread);
		m_hThread = NULL;
	}

	CDialog::OnClose();
}

void CViteDlg::EnableButtons(BOOL bEnable)
{
	GetDlgItem(IDC_VITE_MEASUREBTN)->EnableWindow(!bRemote && bEnable);
	GetDlgItem(IDC_VITE_BACKGROUND)->EnableWindow(!bRemote && bEnable);
	GetDlgItem(IDC_VITE_CALIBRATION)->EnableWindow(!bRemote && bEnable);
	GetDlgItem(IDC_VITE_RESET)->EnableWindow(!bRemote && bEnable);
	GetDlgItem(IDC_VITE_EXIT)->EnableWindow(bEnable);
	GetDlgItem(IDC_VITE_CALIBRATE)->EnableWindow(bEnable);
	GetDlgItem(IDC_VITE_START)->EnableWindow(!bRemote && (m_nMethod <= 3) && bEnable);
	//	if (AGV.pView)
	//	{
	//		AGV.pView->EnableButtons(bEnable);
	//	}
}

void CViteDlg::SetPolyIndex(double fIndex)
{
	m_fThinFilmIndex = fIndex;
	pOceanOp->SetThkRange(m_fThinFilmMinThk, m_fThinFilmMaxThk, m_fThinFilmIndex, m_fExcMinThk, m_fExcMaxThk);
}

void CViteDlg::OnExit()
{
	//	if (AGV.pView)
	//	{
	//		AGV.pView->UpdateView2(FALSE);
	//	}
	if (bRemote)
		return;
	m_bStopFlag = TRUE;

	CDialog::ShowWindow(SW_HIDE);
	//	CDialog::OnClose();
}

BOOL CViteDlg::DestroyWindow()
{
	/*	if(m_hMeasureEndEvent)
		{
			CloseHandle(m_hMeasureEndEvent);
			m_hMeasureEndEvent = NULL;
		}*/
	m_bStopFlag = TRUE;

	if (m_hThread)
	{
		WaitForSingleObject(m_hThread, 5000);
		TRACE("Close MeasureProc Thread %x\n", m_hThread);
		CloseHandle(m_hThread);
		m_hThread = NULL;
	}
	if (wave)
	{
		delete[] wave;
		wave = NULL;
	}
	if (Rmeas)
	{
		delete[] Rmeas;
		Rmeas = NULL;
	}

	if (pOceanOp)
	{
		delete pOceanOp;
		pOceanOp = NULL;
	}

	if (m_points)
	{
		delete[] m_points;
		m_points = NULL;
	}

	return CDialog::DestroyWindow();
}

void CViteDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM)dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
//		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		DrawPicture();
		CDialog::OnPaint();
	}
}

#include <afxmt.h>
CCriticalSection cs4;

void CViteDlg::DrawPicture()
{
	COLORREF m_bgColor2 = RGB(255, 255, 255); //background
	COLORREF m_lnColor1 = RGB(255, 0, 0);//theoretical curve
	COLORREF m_lnColor2 = RGB(0, 0, 255); //measured curve
	COLORREF m_lnColor3 = RGB(16, 16, 16); //grid lines
	COLORREF m_lnColorTxt = RGB(0, 0, 0);//text

	CDC* dc;
	CBitmap bmp;
	CDC dcMem;
	CPen pen;
	CPen pen2;
	CPen pen3;
	RECT rc;
	HBITMAP oldBmp = NULL;
	HPEN oldPen = NULL;
	float fRmin = 1e20;
	float fRmax = -1e20;
	float fXmin = 1e20;
	float fXmax = -1e20;
	CSingleLock Lock3(&cs4);
	Lock3.Lock();
	if (Lock3.IsLocked())
	{
		int i, j;
		RECT rc;
		GetClientRect(&rc);

		//prepare for the drawing
		/*CDC*/ dc = GetDC();
		//		CDC dcMem;
		dcMem.CreateCompatibleDC(dc);

		//		CBitmap bmp;
		bmp.CreateCompatibleBitmap(dc, (int)(0.75 * rc.right), (int)(0.8*rc.bottom));
		/*HBITMAP*/ oldBmp = (HBITMAP)dcMem.SelectObject(bmp);

		//draw background
		dcMem.FillSolidRect(0, 0, (int)(0.75 * rc.right), (int)(0.8 * rc.bottom), m_bgColor2);

		//		CPen pen;
		pen.CreatePen(PS_SOLID, 1, m_lnColor1);
		/*HPEN*/ oldPen = (HPEN)dcMem.SelectObject(pen);
		//draw theoretical curve
		j = 0;
		int NN = m_Nwave;
		if (m_bFilter && (pOceanOp->nZeroPadding > 0))
			NN = pOceanOp->GetDataNumber(m_bFilter);
		if (wave && Rmeas)
		{
			for (i = 0; i < NN; i++)
			{
				if (m_bFilter)
				{
					if ((wave[i] >= m_fThinFilmMinThk) && (wave[i] <= m_fThinFilmMaxThk))
					{
						if (Rmeas[i] > fRmax)
							fRmax = Rmeas[i];
						if (Rmeas[i] < fRmin)
							fRmin = Rmeas[i];
						if (1 / wave[i] > fXmax)
							fXmax = 1 / wave[i];
						if (1 / wave[i] < fXmin)
							fXmin = 1 / wave[i];
					}
				}
				else
				{
					if ((wave[i] >= m_nlmin) && (wave[i] <= m_nlmax))
					{
						if (Rmeas[i] > fRmax)
							fRmax = Rmeas[i];
						if (Rmeas[i] < fRmin)
							fRmin = Rmeas[i];
						if (1 / wave[i] > fXmax)
							fXmax = 1 / wave[i];
						if (1 / wave[i] < fXmin)
							fXmin = 1 / wave[i];
					}
				}
			}
		}
		j = 0;
		//draw measured curve
	//		CPen pen2;
		pen2.CreatePen(PS_SOLID, 1, m_lnColor2);
		dcMem.SelectObject(pen2);
		if (bFileOpen)
		{
			j = 0;
			if (wave && Rmeas)
			{
				for (i = 0; i < NN; i++)
				{
					if (Rmeasmax > Rmeasmin)
					{
						if (m_bFilter)
						{
							if ((wave[i] >= m_fThinFilmMinThk) && (wave[i] <= m_fThinFilmMaxThk))
							{
								m_points[j].x = (int)(0.75 * rc.right
									* (wave[i] - m_fThinFilmMinThk)
									/ (m_fThinFilmMaxThk - m_fThinFilmMinThk));
								m_points[j].y = (int)(0.8 * rc.bottom
									* (1 - (Rmeas[i] - Rmeasmin) / (Rmeasmax - Rmeasmin)));
								j++;
							}
						}
						else
						{
							if ((wave[i] >= m_nlmin) && (wave[i] <= m_nlmax))
							{
								m_points[j].x = (int)(0.75 * rc.right
									* (1.0 / wave[i] - fXmin) / (fXmax - fXmin));
								if (m_nMethod % 4 <= 1)
									m_points[j].y = (int)(0.8 * rc.bottom
										* (1 - (Rmeas[i] - Rmeasmin) / (Rmeasmax - Rmeasmin)));
								else
								{
									if (!m_bAutoScale)
										m_points[j].y = (int)(0.8 * rc.bottom * (1 - Rmeas[i]));
									else
										m_points[j].y = (int)(0.8 * rc.bottom
											* (1 - (Rmeas[i] - fRmin) / (fRmax - fRmin)));
								}
								j++;
							}
						}
					}
				}
			}
			if (m_points && (j > 1))
				dcMem.Polyline(m_points, j);
		}

		//draw grid
	//		CPen pen3;
		pen3.CreatePen(PS_DASH, 1, m_lnColor3);
		dcMem.SelectObject(pen3);
		for (i = 1; i <= 4; i++)
		{
			dcMem.MoveTo(0, (int)(0.16 * i * rc.bottom));
			dcMem.LineTo((int)(0.75 * rc.right), (int)(0.16 * i * rc.bottom));
		}
		for (i = 1; i <= 9; i++)
		{
			dcMem.MoveTo((int)(0.075 * i * rc.right), 0);
			dcMem.LineTo((int)(0.075 * i * rc.right), (int)(0.8 * rc.bottom));
		}
		if (oldPen) dcMem.SelectObject(oldPen);

		char str[255];
		dcMem.SetTextColor(m_lnColorTxt);
		for (i = 1; i <= 4; i++)
		{
			if (m_nMethod % 4 <= 1)
			{
				sprintf(str, "%.2e", Rmeasmax - 0.2*i*(Rmeasmax - Rmeasmin));
			}
			else
			{
				if (!m_bAutoScale)
					sprintf(str, "%.1f", 1 - 0.2*i);
				else
					//						sprintf(str,"%.3f", fRmax - 0.2*i*(fRmax-fRmin));
					sprintf(str, "%.2e", fRmax - 0.2*i*(fRmax - fRmin));
			}
			dcMem.TextOut(0, (int)(0.16 * i * rc.bottom) - 8, str);
		}
		for (i = 1; i <= 9; i++)
		{
			if ((m_nMethod <= 3) && m_bFilter)
			{
				if (i == 9)
					sprintf(str, "%.2f um", m_fThinFilmMinThk + 0.1 * i
						* (m_fThinFilmMaxThk - m_fThinFilmMinThk));
				else
					sprintf(str, "%.2f", m_fThinFilmMinThk + 0.1 * i
						* (m_fThinFilmMaxThk - m_fThinFilmMinThk));
			}
			else
			{
				if (i == 9)
					sprintf(str, "%.3f um-1", 1e3*(fXmin + 0.1 * i * (fXmax - fXmin)));
				else
					sprintf(str, "%.3f", 1e3*(fXmin + 0.1 * i * (fXmax - fXmin)));
			}
			dcMem.TextOut((int)(0.075 * i * rc.right) - 20, (int)(0.8 * rc.bottom) - 16, str);
		}
		if ((m_nMethod > 3) && bFileOpen)
		{
			sprintf(str, "StDev: %.3f", stdev);
			dcMem.SetTextColor(RGB(0, 0, 255));
			dcMem.TextOut(0, 0, str);
			dcMem.SetTextColor(RGB(0, 0, 0));
		}
		//copy bitmap to the screen
		dc->BitBlt((int)(0.25 * rc.right), 0, rc.right, (int)(0.8*rc.bottom), &dcMem, 0, 0, SRCCOPY);
		//
		if (oldBmp) dcMem.SelectObject(oldBmp);
		dcMem.DeleteDC();
		pen.DeleteObject();
		pen2.DeleteObject();
		pen3.DeleteObject();
		bmp.DeleteObject();
		ReleaseDC(dc);
	}
	Lock3.Unlock();

	oldBmp = NULL;
	oldPen = NULL;

	dc = NULL;
}

void CViteDlg::OnSaveSpectrum()
{
	CString strFile;
	CFileDialog dlg(FALSE, _T("txt"), _T("*.txt"),
		OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		_T("Spectrum (*.TXT)|*.TXT|"));
	if (dlg.DoModal() == IDOK)
	{
		strFile = dlg.GetPathName();
	}
	else
	{
		return;
	}
	char str[255];
	FILE*f = fopen(strFile, "w");
	if (f)
	{
		for (int i = 0; i < m_Nwave; i++)
			fprintf(f, "%.3f	%.3f\n", wave[i], Rmeas[i]);
		fclose(f);
	}
}

void CViteDlg::OnSaveResult()
{
	CString strFile;
	CFileDialog dlg(FALSE, _T("txt"), _T("*.txt"),
		OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		_T("Spectrum (*.TXT)|*.TXT|"));
	if (dlg.DoModal() == IDOK)
	{
		strFile = dlg.GetPathName();
	}
	else
	{
		return;
	}
	char str[255];
	double fThk, fNrefr, fSigma;
	FILE*f = fopen(strFile, "w");
	if (f)
	{
		fprintf(f, "Thk= %.3f um	n= %.3f\n",
			m_fPolyThick, m_fThinFilmIndex);
		fclose(f);
	}
}

DWORD CViteDlg::MeasureProcVIT(LPVOID pParam)
{
	CViteDlg *pDlg = (CViteDlg*)pParam;
	CButton* pButton;

	COceanBase* pOceanOp = pDlg->pOceanOp;
	if (!pOceanOp)
	{
		AfxMessageBox("No spectrometer"); return -1;
	}

	while (!pDlg->m_bStopFlag)
	{
		//Sleep(1);

		CButton* pFilter = (CButton*)pDlg->GetDlgItem(IDC_VITE_FFT);
		pDlg->m_bFilter = pFilter->GetCheck() && 0x0001;
		pFilter = (CButton*)pDlg->GetDlgItem(IDC_VITE_AUTOSCALE);
		pDlg->m_bAutoScale = pFilter->GetCheck() && 0x0001;
		pDlg->UpdatePars();

		double ret;

		if (pOceanOp->Status() == 0)
		{
			//Sleep(1);
			pOceanOp->Start();
			Sleep(2);

			if (pDlg->bNorm) pOceanOp->NormalizePoints(pDlg->Rmeas, 1);
			pOceanOp->Result(pDlg->m_fPolyThick);
			if ((pDlg->Etalon.GetPort() > 0) && (pDlg->m_fThinFilmIndex > 0))
			{
				if (pDlg->m_bThickerEtalon)
					pDlg->m_fPolyThick = pDlg->Etalon.fEtalonRefr[pDlg->Etalon.nEtalonCurPos - 1] * pDlg->Etalon.fEtalonThk[pDlg->Etalon.nEtalonCurPos - 1] / pDlg->m_fThinFilmIndex - pDlg->m_fPolyThick;
				else
					pDlg->m_fPolyThick += pDlg->Etalon.fEtalonRefr[pDlg->Etalon.nEtalonCurPos - 1] * pDlg->Etalon.fEtalonThk[pDlg->Etalon.nEtalonCurPos - 1] / pDlg->m_fThinFilmIndex;
			}
			CString s1, s2;
			s1.Format("%.3f", pDlg->m_fPolyThick);
			s2.Format("% 5s", s1);
			pDlg->m_cPolyThick.SetText(s2);
		}
		else
		{
			AfxMessageBox("Sensor is not ready");
		}

		pDlg->DisplaySpectrum();
		MSG msg;
		while (PeekMessage(&msg, NULL, NULL, NULL, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	pButton = (CButton*)pDlg->GetDlgItem(IDC_VITE_EXIT);
	pButton->EnableWindow(TRUE);

	return 1;
}

void CViteDlg::OnStart()
{
	if (m_bStopFlag)
	{
		m_bStopFlag = FALSE;
		CWnd *pButton = GetDlgItem(IDC_VITE_START);
		pButton->SetWindowText("Stop");
		bBusy = TRUE;
		DWORD dwID;
		if (m_hThread)
		{
			WaitForSingleObject(m_hThread, 5000);
			TRACE("Close MeasureProc Thread %x\n", m_hThread);
			CloseHandle(m_hThread);
			m_hThread = NULL;
		}
		m_hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)MeasureProcVIT, this, 0, &dwID);
		TRACE("ThreadID in Dlg %x\n", dwID);

		GetDlgItem(IDC_VITE_MEASUREBTN)->EnableWindow(FALSE);
		GetDlgItem(IDC_VITE_BACKGROUND)->EnableWindow(FALSE);
		GetDlgItem(IDC_VITE_CALIBRATION)->EnableWindow(FALSE);
		GetDlgItem(IDC_VITE_RESET)->EnableWindow(FALSE);
		GetDlgItem(IDC_VITE_EXIT)->EnableWindow(FALSE);
		GetDlgItem(IDC_VITE_SAVE_SPECTRUM)->EnableWindow(FALSE);
		GetDlgItem(IDC_VITE_SAVE_RESULT)->EnableWindow(FALSE);
	}
	else
	{
		m_bStopFlag = TRUE;
		CWnd *pButton = GetDlgItem(IDC_VITE_START);
		pButton->SetWindowText("Start");

		GetDlgItem(IDC_VITE_MEASUREBTN)->EnableWindow(TRUE);
		GetDlgItem(IDC_VITE_BACKGROUND)->EnableWindow(TRUE);
		GetDlgItem(IDC_VITE_CALIBRATION)->EnableWindow(TRUE);
		GetDlgItem(IDC_VITE_RESET)->EnableWindow(TRUE);
		GetDlgItem(IDC_VITE_EXIT)->EnableWindow(TRUE);
		GetDlgItem(IDC_VITE_SAVE_SPECTRUM)->EnableWindow(TRUE);
		GetDlgItem(IDC_VITE_SAVE_RESULT)->EnableWindow(TRUE);
		bBusy = FALSE;
	}
}

void CViteDlg::OnTimer(UINT_PTR nIDEvent)
{
	BOOL bRet;
	if (nIDEvent == IDT_VITE_REFRESH)
	{
		KillTimer(IDT_VITE_REFRESH);
		if ((!bBusy) && (!bSimu))
		{
			bBusy = TRUE;
			EnableButtons(FALSE);

			if (pOceanOp->Status() == 0)
			{
				Sleep(1);

				pOceanOp->Start();

				Sleep(1);

				pOceanOp->Result(m_fPolyThick);
				if ((Etalon.nEtalonCurPos > 0) && (m_fThinFilmIndex > 0))
				{
					if (m_bThickerEtalon)
						m_fPolyThick = Etalon.fEtalonRefr[Etalon.nEtalonCurPos - 1] * Etalon.fEtalonThk[Etalon.nEtalonCurPos - 1] / m_fThinFilmIndex - m_fPolyThick;
					else
						m_fPolyThick += Etalon.fEtalonRefr[Etalon.nEtalonCurPos - 1] * Etalon.fEtalonThk[Etalon.nEtalonCurPos - 1] / m_fThinFilmIndex;
				}

				DisplaySpectrum();
			}
			EnableButtons(TRUE);
			bBusy = FALSE;
		}
		SetTimer(IDT_VITE_REFRESH, 60000, NULL);
	}
	if (nIDEvent == IDT_VITE_READ)
	{
		if ((!bBusy) && (!bSimu))
		{
			bBusy = TRUE;
			EnableButtons(FALSE);

			if (pOceanOp->Status() == 0)
			{
				Sleep(1);

				pOceanOp->Start();

				Sleep(1);

				pOceanOp->Result(m_fPolyThick);
				if ((Etalon.nEtalonCurPos > 0) && (m_fThinFilmIndex > 0))
				{
					if (m_bThickerEtalon)
						m_fPolyThick = Etalon.fEtalonRefr[Etalon.nEtalonCurPos - 1] * Etalon.fEtalonThk[Etalon.nEtalonCurPos - 1] / m_fThinFilmIndex - m_fPolyThick;
					else
						m_fPolyThick += Etalon.fEtalonRefr[Etalon.nEtalonCurPos - 1] * Etalon.fEtalonThk[Etalon.nEtalonCurPos - 1] / m_fThinFilmIndex;
				}

				DisplaySpectrum();

				char filename[_MAX_PATH];
				char drive[_MAX_DRIVE];
				char dir[_MAX_DIR];
				char fname[_MAX_FNAME];
				char ext[_MAX_EXT];
				GetModuleFileName(NULL, filename, _MAX_PATH);
				_splitpath(filename, drive, dir, fname, ext);
				_makepath(filename, drive, dir, "TFResult", "txt");
				FILE* f = fopen(filename, "a");
				if (f)
				{
					CTime CurTime = CTime::GetCurrentTime();
					fprintf(f, "%d-%d-%d%_%d:%d:%d",
						CurTime.GetYear(), CurTime.GetMonth(), CurTime.GetDay(),
						CurTime.GetHour(), CurTime.GetMinute(), CurTime.GetSecond());
					fprintf(f, "	%.1f	%.1f	%.1f	%.1f	%.1f",
						Rmeas[5], Rmeas[100], Rmeas[200], Rmeas[300], Rmeas[400]);
					fprintf(f, "	%.1f	%.1f	%.1f	%.1f	%.1f",
						Rmeas[500], Rmeas[600], Rmeas[700], Rmeas[800], Rmeas[900]);
					fprintf(f, "	%.1f	%.1f	%.1f	%.1f	%.1f",
						Rmeas[1000], Rmeas[1100], Rmeas[1200], Rmeas[1300], Rmeas[1400]);
					fprintf(f, "	%.1f	%.1f	%.1f	%.1f	%.1f\n",
						Rmeas[1500], Rmeas[1600], Rmeas[1700], Rmeas[1800], Rmeas[1900]);
					fclose(f);
				}
				CString s1, s2;
				s1.Format("%.3f", m_fPolyThick);
				s2.Format("% 5s", s1);
				m_cPolyThick.SetText(s2);
			}
			else
				AfxMessageBox("Sensor is not ready");

			EnableButtons(TRUE);
			bBusy = FALSE;
		}
		SetTimer(IDT_VITE_READ, 300000, NULL);
	}
	CDialog::OnTimer(nIDEvent);
}

void CViteDlg::OnEnKillfocusTfFfttype()
{
	UpdateData(TRUE);
	if (pOceanOp)
		pOceanOp->nFFTtype = m_nFFTtype;
}

void CViteDlg::ZeroMethod()
{
	if (pOceanOp)
		pOceanOp->oopars.nMethod = 0;
	m_nMethod = 0;
	SetDlgItemText(IDC_VITE_METHOD, "0");
}

void CViteDlg::OnBnClickedEtalonrst()
{
	Etalon.ResetEtalon();
	ZeroMethod();
	m_fEtalonThk = 0;
	CString str;
	str.Format("%.3f", m_fEtalonThk);
	SetDlgItemText(IDC_ETALONPOS, str);
}

void CViteDlg::OnBnClickedEtalonpos1()
{
	Etalon.MoveEtalonToPos(1);
	m_fEtalonThk = Etalon.GetThickness(1) / m_fThinFilmIndex;
	ZeroMethod();
	CString str;
	str.Format("%.3f", m_fEtalonThk);
	SetDlgItemText(IDC_ETALONPOS, str);
}

void CViteDlg::OnBnClickedEtalonpos2()
{
	Etalon.MoveEtalonToPos(2);
	m_fEtalonThk = Etalon.GetThickness(2) / m_fThinFilmIndex;
	ZeroMethod();
	CString str;
	str.Format("%.3f", m_fEtalonThk);
	SetDlgItemText(IDC_ETALONPOS, str);
}

void CViteDlg::OnBnClickedEtalonpos3()
{
	Etalon.MoveEtalonToPos(3);
	m_fEtalonThk = Etalon.GetThickness(3) / m_fThinFilmIndex;
	ZeroMethod();
	CString str;
	str.Format("%.3f", m_fEtalonThk);
	SetDlgItemText(IDC_ETALONPOS, str);
}

void CViteDlg::OnBnClickedEtalonpos4()
{
	Etalon.MoveEtalonToPos(4);
	m_fEtalonThk = Etalon.GetThickness(4) / m_fThinFilmIndex;
	ZeroMethod();
	CString str;
	str.Format("%.3f", m_fEtalonThk);
	SetDlgItemText(IDC_ETALONPOS, str);
}

void CViteDlg::OnBnClickedSwitchpos2()
{
	m_bProbe2 = !m_bProbe2;
	if (m_bProbe2)
		OpSwitch.SetOpticalSwitch(2);
	else
		OpSwitch.SetOpticalSwitch(1);
}

void CViteDlg::OnBnClickedThickerEtalon()
{
	m_bThickerEtalon = !m_bThickerEtalon;
}

void CViteDlg::OnBnClickedCloseshutter1()
{
	m_bShutter1 = !m_bShutter1;
	Etalon.CloseShutter(FALSE, m_bShutter1);
}

void CViteDlg::OnBnClickedCloseshutter2()
{
	m_bShutter2 = !m_bShutter2;
	Etalon.CloseShutter(TRUE, m_bShutter2);
}

void CViteDlg::OnEnKillfocusTfExcThicknessMin()
{
	UpdateData(TRUE);
	UpdatePars();
}

void CViteDlg::OnEnKillfocusTfExcThicknessMax()
{
	UpdateData(TRUE);
	UpdatePars();
}

void CViteDlg::OnBnClickedCalibrate()
{
	Calibrate();
}

void CViteDlg::Calibrate()
{
	EnableButtons(FALSE);
	float fX, fY;
	if ((AGV.nModel > 0) && (AGV.pS8Dlg))
	{
		AGV.pS8Dlg->GetXYPos(fX, fY);
		AGV.pS8Dlg->DoVITECalibration();
		AGV.pS8Dlg->GotoPos(fX, fY);
		AGV.Sleep2(200);
	}
	EnableButtons(TRUE);
}

void CViteDlg::OnBnClickedViteFft2()
{
	UpdateData(TRUE);
}

void CViteDlg::classify_peaks_func(double *x1, double *x2, double z1, double z2, int noOfGausPnt1, int noOfGausPnt2, int *status1, int *status2)
{
	z1 = z1 * 1000;
	z2 = z2 * 1000;

	int Classification[NSPMAX];
	double Max_Static_Difference = 1;
	int polarity = -1;

	double deltaZ = polarity*(z2 - z1); //Z motor position difference [microns]. Checks if we are moving the probe towards the sample or opposite direction
	double Zmotor_error = 2; // The associated error in position of the Z motor height [microns]

	for (int i = 0; i < noOfGausPnt1; i++)
	{
		for (int j = 0; j < noOfGausPnt2; j++)
		{
			double peak_difference = abs(x1[i] - x2[j]);
			if (peak_difference <= Max_Static_Difference)// Check if Static
			{
				//Classification[i] = -1;
				status1[i] = -1;
				status2[j] = -1;
				//Classification[]=-1;
			}
			else if (((x2[j] - x1[i]) > deltaZ - Zmotor_error) && ((x2[j] - x1[i]) < deltaZ + Zmotor_error))
			{
				status1[i] = 1;
				status2[j] = 1;
				//Classification[i] = 1;
			}
		}
	}
}

void CViteDlg::find_peaks_func(double *x, double *y, int noOfData, double *all_x_max_g, double *all_y_max_g, int &noOfResultantData,double Threshold,double jump_Threshold, int Radius)
{
	//--------------------remove zeropadding-----------------
	//double* x,*y;
	//removeZeroPad(xx,yy,&x,&y);

	

	int mxIndx;
	double maxX = 0, maxY = 0;
	for (int i = 0; i < noOfData; i++)
	{
		if (maxX < x[i])
		{
			maxX = x[i];
			mxIndx = i;
		}
		if (maxY < y[i])
		{
			maxY = y[i];
		}
	}

	noOfData = mxIndx + 1;

	//cout<<noOfData<<" \n";

	//---------------Finding index of all the peaks above threshold----------------

	int id_peak = 0;
	int peak_ind[NSPMAX];

	if (Threshold != 0)
	{
		for (int i = 0; i < noOfData; i++)
		{
			if (y[i] >= Threshold)
			{
				peak_ind[id_peak] = i;
				id_peak++;
			}
		}
	}
	else
	{
		for (int i = 0; i < noOfData; i++)
		{
			if (y[i] >= (maxY * 0.03))
			{
				peak_ind[id_peak] = i;
				id_peak++;
			}
		}
	}
	//fully done
	/*for(int i=0;i<id_peak;i++)
	cout<<peak_ind[i]<<endl;
	*/

	//-------------peak_ranges(numberOfRange,left/right interval index)-------------
	// Creating and finding the intervals that include the peaks.

	int range_nmb = 0; // Initialization of the selection of the peak
	//double jump_Threshold = 10; // Maximum distance of index to be considered a peak

	int peak_ranges[NSPMAX][2];
	for (int i = 0; i < id_peak - 1; i++)//since right sided(i+1) index is used, current index must < id_peak-1
	{
		if (i == 0)
		{
			peak_ranges[i][0] = peak_ind[0]; // Initializing the first Peak
			peak_ranges[i][1] = peak_ind[1]; // Initializing the first Peak
		}

		if (peak_ind[i] >= peak_ind[i + 1] - jump_Threshold) // Finding the right interval limit
		{
			peak_ranges[range_nmb][1] = peak_ind[i + 1];
		}
		else //--if distance betwn 2 consecutive peaks crosses jump_Threshold
		{
			range_nmb = range_nmb + 1; // Switching to the next interval and finding the left limit
			peak_ranges[range_nmb][0] = peak_ind[i + 1];
			peak_ranges[range_nmb][1] = peak_ind[i + 1];
		}
	}

	
	
	/*cout<<range_nmb<<endl;
	for(int i=0;i<=range_nmb;i++)
	{
	cout<<peak_ranges[i][0]<<" "<<peak_ranges[i][1]<<"\n";
	}*/

	// Calculating the highest peak that is complete
	// Peaks that are incomplete (Must have both sides leaving the threshold)
	//  are not included in the calculations
	double single_max = 0, single_peak;
	vector<int> peak_max_idx;
	int pk_mx_id;
	//int Radius = 10;

	
		//All peaks are calculated
		for (int i = 0; i <= range_nmb; i++)
			peak_max_idx.push_back(i);
	

	//fully done

	/*for(int i = 0;i<peak_max_idx.size();i++)
	cout<<peak_max_idx[i];
	*/

	//---------------------------------------gaussian fitting--------------
	//ppFile = fopen ("out.txt","w");
	double sigma, mu, A, xL, xR, yL, yR;
	int noOfGausPnt = 0;
	for (int i = 0; i < peak_max_idx.size(); i++)
	{
		//Finding the indexes of the maximum for each peak
		double local_max = 0, max_y;
		int max_idx;

		for (int j = peak_ranges[i][0]; j <= peak_ranges[i][1]; j++)//trying all indices in betwn interval
		{
			if (local_max < y[j])
			{
				local_max = y[j];
				max_idx = j;
			}
		}

		//max_idx =  find(y==local_max);
		max_y = y[max_idx];

		if (((max_idx + 1) - Radius) >= 1 && ((max_idx + 1) + Radius) <= noOfData) //? (+1 in 3 lines) For peaks not on the side of the X axis
		{
			//[sigma,mu,A] = mygaussfit(x(max_idx+(-Radius:Radius)),y(max_idx+(-Radius:Radius)));
			xL = (max_idx)+(-Radius);
			xR = (max_idx)+(Radius);
			yL = xL;
			yR = xR;
			mygaussfit(noOfData, x, xL, xR, y, yL, yR, sigma, mu, A, 1);
		}
		else if (((max_idx + 1) - Radius) < 1 && ((max_idx + 1) + Radius) <= noOfData) // For peaks on the left side of the X axis
		{
			//[sigma,mu,A] = mygaussfit(x(1:2*Radius),y(1:2*Radius));
			xL = 1;
			xR = 2 * Radius;
			yL = xL;
			yR = xR;
			mygaussfit(noOfData, x, xL, xR, y, yL, yR, sigma, mu, A, 2);


			// Will remove peaks that have their Max fitted outside of their
			// possible range value(Happens for the first Infinity peak)

			if ((mu > x[peak_ranges[i][1]]) || (mu < x[peak_ranges[i][0]]))
			{
				mu = x[max_idx]; // Maximum position not interpolated
				A = y[max_idx]; // Maximum intensity not interpolated
			}
		}
		else if (((max_idx + 1) - Radius) >= 1 && ((max_idx + 1) + Radius) > noOfData) // For peaks on the right side of the X axis
		{
			//[sigma,mu,A] = mygaussfit(x(max_idx+(-Radius:(size(y,1)-max_idx))),y(max_idx+(-Radius:(size(y,1)-max_idx))));
			xL = max_idx + (-Radius);
			xR = max_idx + noOfData - max_idx;
			yL = xL;
			yR = xR;
			mygaussfit(noOfData, x, xL, xR, y, yL, yR, sigma, mu, A, 3);
		}
		//mygaussfit(x,xL,xR,y,yL,yR,&sigma,&mu,&A,i);
		all_x_max_g[noOfGausPnt] = mu;
		all_y_max_g[noOfGausPnt] = A;
		noOfGausPnt++;

		
		//cout<<sigma<<" "<<mu<<" "<<A<<endl;
	}

	//fclose(ppFile);
	noOfResultantData = noOfGausPnt;
	//fully done
}

void CViteDlg::polyfit(double *x, double *y, int N, int n, double *p)
{
	//let Max n=2
	double X[5] = { 0 };                        //Array that will store the values of sigma(xi),sigma(xi^2),sigma(xi^3)....sigma(xi^2n)
	for (int i = 0; i < 2 * n + 1; i++)
	{
		X[i] = 0;
		for (int j = 0; j < N; j++)
			X[i] = X[i] + pow(x[j], i);        //consecutive positions of the array will store N,sigma(xi),sigma(xi^2),sigma(xi^3)....sigma(xi^2n)
	}

	double B[3][4] = { 0 }, a[3] = { 0 };            //B is the Normal matrix(augmented) that will store the equations, 'a' is for value of the final coefficients
	for (int i = 0; i <= n; i++)
		for (int j = 0; j <= n; j++)
			B[i][j] = X[i + j];            //Build the Normal matrix by storing the corresponding coefficients at the right positions except the last column of the matrix

	double Y[3] = { 0 };                    //Array to store the values of sigma(yi),sigma(xi*yi),sigma(xi^2*yi)...sigma(xi^n*yi)
	for (int i = 0; i < n + 1; i++)
	{
		Y[i] = 0;
		for (int j = 0; j < N; j++)
			Y[i] = Y[i] + pow(x[j], i)*y[j];        //consecutive positions will store sigma(yi),sigma(xi*yi),sigma(xi^2*yi)...sigma(xi^n*yi)
	}

	
	for (int i = 0; i <= n; i++)
		B[i][n + 1] = Y[i];                //load the values of Y as the last column of B(Normal Matrix but augmented)
	n = n + 1;                //n is made n+1 because the Gaussian Elimination part below was for n equations, but here n is the degree of polynomial and for n degree we get n+1 equations
							  /*cout<<"\nThe Normal(Augmented Matrix) is as follows:\n";
							  for (int i=0;i<n;i++)            //print the Normal-augmented matrix
							  {
							  for (int j=0;j<=n;j++)
							  cout<<B[i][j]<<setw(16);
							  cout<<"\n";
							  }*/
	

	for (int i = 0; i < n; i++)                    //From now Gaussian Elimination starts(can be ignored) to solve the set of linear equations (Pivotisation)
		for (int k = i + 1; k < n; k++)
			if (B[i][i] < B[k][i])
				for (int j = 0; j <= n; j++)
				{
					double temp = B[i][j];
					B[i][j] = B[k][j];
					B[k][j] = temp;
				}

	

	for (int i = 0; i < n - 1; i++)            //loop to perform the gauss elimination
		for (int k = i + 1; k < n; k++)
		{
			double t = B[k][i] / B[i][i];
			for (int j = 0; j <= n; j++)
				B[k][j] = B[k][j] - t * B[i][j];    //make the elements below the pivot elements equal to zero or elimnate the variables
		}

	

	for (int i = n - 1; i >= 0; i--)                //back-substitution
	{                        //x is an array whose values correspond to the values of x,y,z..
		a[i] = B[i][n];                //make the variable to be calculated equal to the rhs of the last equation
		for (int j = 0; j < n; j++)
			if (j != i)            //then subtract all the lhs values except the coefficient of the variable whose value                                   is being calculated
				a[i] = a[i] - B[i][j] * a[j];
		a[i] = a[i] / B[i][i];            //now finally divide the rhs by the coefficient of the variable to be calculated
	}

	
	for (int i = 0; i < n; i++)
		p[i] = a[i];

}

void CViteDlg::mygaussfit(double noOfData, double *x, int xL, int xR, double *y, int yL, int yR, double &sigma, double &mu, double &A, int ii)
{
	double h = 0.2, ymax = 0;

	for (int i = yL; i <= yR; i++)
	{
		ymax = max(ymax, y[i]);
	}
	//cout<<xL<<"xR="<<xR<<"\n";
	vector<double> xnew, ynew;
	for (int i = xL; i <= xR; i++)
	{
		if (y[i] > ymax*h)
		{
			xnew.push_back(x[i]);
			ynew.push_back(y[i]);
		}
	}

	int lenXnew = xnew.size();
	double ylog[NSPMAX], xlog[NSPMAX];
	for (int i = 0; i < lenXnew; i++)
	{
		xlog[i] = xnew[i];
		ylog[i] = log(ynew[i]);
	}

	double P[5];//co-efficients
	int degree = 2;
	polyfit(xlog, ylog, lenXnew, degree, P);
	//cout<<P[2]<<"----\n";
	sigma = sqrt(fabs(-1 / (2 * P[2])));
	mu = P[1] * (sigma*sigma);//sigma^2=sigma*sigma
	A = exp(P[0] + (mu*mu) / (2 * (sigma*sigma)));


	//mu=A1*sigma^2;
	//A=exp(A0+mu^2/(2*sigma^2));
	/*for(int i=0;i<=degree;i++)
	{
	cout<<P[i]<<" ";
	}
	cout<<"\n";*/
	//---------debug purpose---------
	/*    //cout<<ymax<<endl;
	for(int i=0;i<xnew.size();i++)
	cout<<xnew[i]<<" ";
	cout<<"\n";
	for(int i=0;i<ynew.size();i++)
	cout<<ynew[i]<<" ";
	cout<<"\n";
	*/
	//---------debug purpose---------
	/*  fprintf(ppFile,"ii=%d\n",ii);
	fprintf(ppFile,"---------x=\n");
	for(int i=xL;i<=xR;i++)
	fprintf(ppFile,"%.3lf\n",x[i]);
	fprintf(ppFile,"---------y=\n");
	for(int i=yL;i<=yR;i++)
	fprintf(ppFile,"%.3lf\n",y[i]);
	*/
}

void CViteDlg::removeZeroPad(vector<double> X, vector<double> Y, double *x, double *y)
{
	int xSize = X.size(), mxIndx;
	//cout<< x[1]<<"\n";
	double maxX = 0;
	for (int i = 0; i < xSize; i++)
	{
		if (maxX < X[i])
		{
			maxX = X[i];
			mxIndx = i;
		}
	}

	for (int i = 0; i < xSize; i++)
	{
		x[i] = X[i];
		y[i] = Y[i];
	}
}