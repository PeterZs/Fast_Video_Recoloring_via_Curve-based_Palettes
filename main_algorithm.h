#ifndef DATA_H
#define DATA_H

#include<algorithm>
#include<QString>
#include<QObject>
#include<vector>
#include <QThread>
#include "utility.h"
#include <string>
#include <array>
#include <opencv2/opencv.hpp>
#include <QFile>
#include <QFileinfo.h>
#include <QDebug>
#include <cmath>
#include <QProgressDialog>
#include <QMessagebox>
#include <QTime>
#include <omp.h>
#include <map>
#include "my_util.h"
#include <fstream>
#include <cstring>
#include <io.h>
#include <direct.h>
#include <nlopt.hpp>
#include <Eigen/Dense>
#include "SLIC.h"
using namespace std;

typedef short int sint;

class VideoRecolor : public QObject{
	Q_OBJECT
public:
	int currFrameId;
	bool isPaletteCalc;
	bool isVideoOpen;
	bool isImgSizeReduce;
	vector<bool> isSelected;

	void	OpenVideo(QString path);
	void	CalcVideoPalette();
	double	CalcFramePaletteSize(const vector<double>& seedsL, const vector<double>& seedsA, const vector<double>& seedsB, const vector<int>& superpixelSize, int threshold );
	void	ExtractFirstFramePalette(int index, const vector<double>& seedsL, const vector<double>& seedsA, const vector<double>& seedsB);
	void	ExtractOtherFramePalette(int index, const vector<double>& seedsL, const vector<double>& seedsA, const vector<double>& seedsB);
	double	calc_param(int index, vector<double>& lamda);
	void	CalcSinglePointWeights(int findex, const cv::Vec3d& point, double param, const vector<double>& lamda, vector<double>& singleWeight);
	void	CalcWeights(int findex);
	void	RecolorFrame(int findex);
	void	RecolorVideo();
	void	DeformBezierCurve();
	int		GetWidth() const { return frameCols; }
	int		GetHeight() const { return frameRows; }
	int		GetFrameCnt() const { return frameCnt; }
	int		GetFramePaletteSize() const { return framePaletteSize; }
	double	GetFps() const { return fps; }
	sint*	GetCurrentImage_R(bool isAfter) const { return isAfter ? changedVideo_R[currFrameId] : oriVideo_R[currFrameId]; }
	sint*	GetCurrentImage_G(bool isAfter) const { return isAfter ? changedVideo_G[currFrameId] : oriVideo_G[currFrameId]; }
	sint*	GetCurrentImage_B(bool isAfter) const { return isAfter ? changedVideo_B[currFrameId] : oriVideo_B[currFrameId]; }
	void	ChangeFrameTime(int frameId);
	void	ExportRecoloredVideo(QString filename);
	
	VideoRecolor();
	void Clear();
	
	double** GetChangedPalette_R() { return changedPalette_R; }
	double** GetChangedPalette_G() { return changedPalette_G; }
	double** GetChangedPalette_B() { return changedPalette_B; }
	double** GetOriginalPalette_R() { return oriPalette_R; }
	double** GetOriginalPalette_G() { return oriPalette_G; }
	double** GetOriginalPalette_B() { return oriPalette_B; }

	double* GetCurrentChangedPalette_R() { return changedPalette_R[currFrameId]; }
	double* GetCurrentChangedPalette_G() { return changedPalette_G[currFrameId]; }
	double* GetCurrentChangedPalette_B() { return changedPalette_B[currFrameId]; }
	double* GetCurrentOriPaletet_R() { return oriPalette_R[currFrameId]; }
	double* GetCurrentOriPaletet_G() { return oriPalette_G[currFrameId]; }
	double* GetCurrentOriPaletet_B() { return oriPalette_B[currFrameId]; }

	void	SetPaletteColor(int id, QColor c);
	void	ImportEditedFramePalettes(QString filename);
	void	ExportEditedFramePalettes(QString filename);
	void	ResetCurrFramePaletteColor(int id);
	void	ResetCurrFrameAllPaletteColors();
	void	ResetAllFramePalettes();
	void	RemoveSelection(int id);

	vector<double> CtrlPointBernsteinCoeff;

public slots:
signals:
	void updated();

private:
	string videoName;	
	sint** oriVideo_R;
	sint** oriVideo_G;		
	sint** oriVideo_B;		
	sint** changedVideo_R;
	sint** changedVideo_G;	
	sint** changedVideo_B;	

	double** oriPalette_R;
	double** oriPalette_G;		
	double** oriPalette_B;		
	double** changedPalette_R;
	double** changedPalette_G;	
	double** changedPalette_B;	

	float** weights; //weight of each pixel with respect to the frame palette colors

	vector<vector<double>> bezeirControl_L;
	vector<vector<double>> bezeirControl_A;	
	vector<vector<double>> bezeirControl_B;	

	vector<vector<int>> selectedColor;
	vector<int> selectedFrame;
	vector<int> selectedId;
	vector<double> bezierDerivCoeff;
	vector<vector<double>> oriBezierDeriv_L;
	vector<vector<double>> oriBezierDeriv_A;
	vector<vector<double>> oriBezierDeriv_B;

	int framePaletteSize;
	int BezierCtrlPointNum;
	int frameCnt;
	int frameRows;
	int frameCols;
	int frameSize;
	int fps;
};

#endif // DATA_H