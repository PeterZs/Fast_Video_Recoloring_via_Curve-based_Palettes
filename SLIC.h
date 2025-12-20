#pragma once
#include <opencv2/opencv.hpp>
using namespace std;

typedef short int sint;

void RGB2LAB(const int& sR, const int& sG, const int& sB, double& lval, double& aval, double& bval);

void RGB2XYZ(const int& sR, const int& sG, const int& sB, double& X, double& Y, double& Z);

void GenerateSuperpixels(
	const sint* imageR,
	const sint* imageG,
	const sint* imageB,
	vector<double>& kseedsl,
	vector<double>& kseedsa,
	vector<double>& kseedsb,
	const int& K,
	const int& cols,
	const int& rows,
	vector<int>& superpixelSize);

void DoRGBtoLABConversion(
	const sint* imageR,
	const sint* imageG,
	const sint* imageB,
	double* imgL,
	double* imgA,
	double* imgB,
	const int& size
);

void DetectLabEdges(
	const double* lvec,
	const double* avec,
	const double* bvec,
	const int& width,
	const int& height,
	vector<double>& edges);

void GetLABXYSeeds_ForGivenK(
	const double* imgL,
	const double* imgA,
	const double* imgB,
	vector<double>& kseedsl,
	vector<double>& kseedsa,
	vector<double>& kseedsb,
	vector<double>& kseedsx,
	vector<double>& kseedsy,
	const int& K,
	const int& m_width,
	const int& m_height,
	const vector<double>& edgemag);

void PerturbSeeds(
	const double* imgL,
	const double* imgA,
	const double* imgB,
	vector<double>& kseedsl,
	vector<double>& kseedsa,
	vector<double>& kseedsb,
	vector<double>& kseedsx,
	vector<double>& kseedsy,
	const int& m_width,
	const int& m_height,
	const vector<double>& edges);


void PerformSuperpixelSegmentation_VariableSandM(
	const double* imgL,
	const double* imgA,
	const double* imgB,
	vector<double>& kseedsl,
	vector<double>& kseedsa,
	vector<double>& kseedsb,
	vector<double>& kseedsx,
	vector<double>& kseedsy,
	const int& m_width,
	const int& m_height,
	vector<int>& klabels,
	const int& STEP,
	const int& NUMITR,
	vector<int>& superpixelSize);