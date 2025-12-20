#ifndef UTILITY_H
#define UTILITY_H

#include "my_util.h"
#include <opencv2/opencv.hpp>
#include <fstream>
#include <vector>
using namespace std;

// 计算组合数C(n, i)
inline long long Combnum(int i, int n) {
	long long res = 1;
	for (int j = 1; j <= i; j++)
		res = res * (n - i + j) / j;
	return res;
}

// 计算 Bernstein 多项式值
inline double BernsteinNum(int i, int n, double t) {
	if (i < 0 || i > n) return 0;
	return Combnum(i, n) * pow(t, i) * pow(1.0f - t, n - i);
}

void RGB2LAB(cv::Vec3d& rgb);
void LAB2RGB(cv::Vec3d& lab);

inline double squareDistance(const cv::Vec3d& a, const cv::Vec3d& b) {
	return pow(a[0] - b[0], 2) + pow(a[1] - b[1], 2) + pow(a[2] - b[2], 2);
}

double squareDistance(const vector<cv::Vec3d>& a, const vector<cv::Vec3d>& b);
double lab_distance(double r1, double g1, double b1, double r2, double g2, double b2);

inline double phiFunction(double r1, double g1, double b1, double r2, double g2, double b2, double param) {
	return exp(-pow(lab_distance(r1, g1, b1, r2, g2, b2), 2) * param);
}

double FitBezierLossFunction(const vector<double>& x, vector<double>& grad, void* data);
double RefitBezierLossFunction(const vector<double>& x, vector<double>& grad, void* data);

struct data1 {
	int pointNum;
	int BezierCtrlPointNum;
	double* point;
	vector<double> A;
};

struct data2 {
	int BezierCtrlPointNum;
	int frameCnt;
	vector<double> A;
	vector<double> oriBezierDeriv;
	vector<int> changedIndex;
	vector<double> changedPoint;
	vector<double> coeff1;
	double lamda;
};

struct data3 {
	int frameCnt;
	int BezierCtrlPointNum;
	double* pointR;
	double* pointG;
	double* pointB;
	vector<double> A;
};

double clamp_(double s, double lo, double hi);
double adjustDistance(const cv::Vec3d& lab1, const cv::Vec3d& lab2, double lamda);
void exportPoint(int f, const vector<cv::Vec3d>& superPixel, const vector<cv::Vec3d>& centroids);

#endif // UTILITY_H
