#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include "main_algorithm.h" 
using namespace std;

VideoRecolor::VideoRecolor(){
	currFrameId = 0;
	isPaletteCalc = false;
	isVideoOpen = false;
	isImgSizeReduce = false;
}

void VideoRecolor::Clear() {
	if (isVideoOpen) {
		for (size_t i = 0; i < frameCnt; i++) {
			delete[] oriVideo_R[i];		
			delete[] oriVideo_G[i];		
			delete[] oriVideo_B[i];
			delete[] changedVideo_R[i]; 
			delete[] changedVideo_G[i]; 
			delete[] changedVideo_B[i];
		}
		delete[] oriVideo_R;		
		delete[] oriVideo_G;		
		delete[] oriVideo_B;
		delete[] changedVideo_R;	
		delete[] changedVideo_G;	
		delete[] changedVideo_B;

		isVideoOpen = false;
	}

	if (isPaletteCalc) {
		for (size_t i = 0; i < framePaletteSize; i++) {
			delete[] oriPalette_R[i];		
			delete[] oriPalette_G[i];		
			delete[] oriPalette_B[i];
			delete[] changedPalette_R[i];	
			delete[] changedPalette_G[i];	
			delete[] changedPalette_B[i];
		}
		delete[] oriPalette_R;		
		delete[] oriPalette_G;		
		delete[] oriPalette_B;
		delete[] changedPalette_R;	
		delete[] changedPalette_G;	
		delete[] changedPalette_B;

		selectedFrame.clear();
		selectedId.clear();
		for (size_t i = 0; i < framePaletteSize; i++)
			selectedColor[i].clear();
		selectedColor.clear();

		for (size_t i = 0; i < framePaletteSize * frameCnt; i++)
			isSelected[i] = false;

		for (size_t i = 0; i < frameCnt; i++)
			delete[] weights[i];
		delete[] weights;

		isPaletteCalc = false;
	}
}

void VideoRecolor::OpenVideo(QString path) {
	this->Clear();

	//get basic info of the input video
	cv::VideoCapture capture(path.toStdString());
	if (!capture.isOpened()) return;

	QFileInfo fileinfo(path);
	this->videoName = (fileinfo.baseName()).toStdString();

	frameCnt = capture.get(cv::CAP_PROP_FRAME_COUNT);
	fps = capture.get(cv::CAP_PROP_FPS);
	frameRows = capture.get(cv::CAP_PROP_FRAME_HEIGHT);
	frameCols = capture.get(cv::CAP_PROP_FRAME_WIDTH);
	
	//resize the input video for acceleration （the resolution will recover when saving the recored video）
	isImgSizeReduce = false;
	if (frameRows > 500) {
		frameRows /= 2;
		frameCols /= 2;
		isImgSizeReduce = true;
	}
	frameSize = frameRows * frameCols;

	//the bezier curve's control point cnt
	BezierCtrlPointNum = frameCnt / 30;
	if (BezierCtrlPointNum < 6) BezierCtrlPointNum = 6;

	//save the video's all frames
	string videoFolder = "./" + videoName;
	string picFolder;
	if (_access(videoFolder.c_str(), 0) != 0) {
		_mkdir(videoFolder.c_str());

		picFolder = videoFolder + "/pics";
		_mkdir(picFolder.c_str());
		picFolder += "/";

		for (int i = 0; i < frameCnt; i++) {
			cv::Mat frame;
			capture >> frame;
			if (frame.empty()) {
				frameCnt = i;
				break;
			}
			if (isImgSizeReduce) {
				cv::Mat resizedImage;
				cv::resize(frame, resizedImage, cv::Size(), 0.5, 0.5, cv::INTER_CUBIC);
				frame = resizedImage;
			}
			string picPath = picFolder + to_string(i) + ".jpg";
			cv::imwrite(picPath, frame);
		}
	}
	else
		picFolder = videoFolder + "/pics/";

	capture.release();

	oriVideo_R = new sint * [frameCnt];				
	oriVideo_G = new sint * [frameCnt];			
	oriVideo_B = new sint * [frameCnt];
	changedVideo_R = new sint * [frameCnt];			
	changedVideo_G = new sint * [frameCnt];		
	changedVideo_B = new sint * [frameCnt];
	for (size_t i = 0; i < frameCnt; i++) {
		oriVideo_R[i] = new sint[frameSize];		
		oriVideo_G[i] = new sint[frameSize];		
		oriVideo_B[i] = new sint[frameSize];
		changedVideo_R[i] = new sint[frameSize];	
		changedVideo_G[i] = new sint[frameSize];	
		changedVideo_B[i] = new sint[frameSize];
	}
	isVideoOpen = true;

	//get all pixels' colors of the input video
	#pragma omp parallel for
	for (int i = 0; i < frameCnt; i++) {
		cv::Mat img = cv::imread(picFolder + to_string(i) + ".jpg");
		if (img.empty()) {
			this->frameCnt = i;
			break;
		}
		for (size_t row = 0; row < frameRows; row++) {
			uchar* uc_pixel = img.data + row * img.step;
			int index = row * frameCols;
			for (size_t col = 0; col < frameCols; col++) {
				oriVideo_R[i][index] = (int)uc_pixel[2];
				oriVideo_G[i][index] = (int)uc_pixel[1];
				oriVideo_B[i][index] = (int)uc_pixel[0];
				uc_pixel += 3;
				index++;
			}
		}
		memcpy(changedVideo_R[i], oriVideo_R[i], sizeof(sint) * frameSize);
		memcpy(changedVideo_G[i], oriVideo_G[i], sizeof(sint) * frameSize);
		memcpy(changedVideo_B[i], oriVideo_B[i], sizeof(sint) * frameSize);
	}
	emit updated();
}

// extract the palette of the input video
void VideoRecolor::CalcVideoPalette() {
	if (!isVideoOpen) return;
	if (isPaletteCalc) {
		for (size_t i = 0; i < this->framePaletteSize; i++) {
			delete[] oriPalette_R[i];		
			delete[] oriPalette_G[i];		
			delete[] oriPalette_B[i];
			delete[] changedPalette_R[i];	
			delete[] changedPalette_G[i];	
			delete[] changedPalette_B[i];
		}
		delete[] oriPalette_R;		
		delete[] oriPalette_G;		
		delete[] oriPalette_B;
		delete[] changedPalette_R;	
		delete[] changedPalette_G;	
		delete[] changedPalette_B;

		for (size_t i = 0; i < frameCnt; i++) 
			delete[] weights[i];
		delete[] weights;

		isPaletteCalc = false;
	}
	isPaletteCalc = true;

	//1. calculate the frame palete size (bezier curve number)==================================================================================
	vector<vector<double>> seedsL(frameCnt), seedsA(frameCnt), seedsB(frameCnt);
	// predefine superpixel size
	int K = 200;
	// record each frame's superpixel size
	vector<vector<int>> superpixelSize(frameCnt);

	// segment each frame to super pixels
	#pragma omp parallel for
	for (int i = 0; i < frameCnt; i++)
		GenerateSuperpixels(oriVideo_R[i], oriVideo_G[i], oriVideo_B[i], seedsL[i], seedsA[i], seedsB[i], K, frameCols, frameRows, superpixelSize[i]);

	//calculate the average frame palette size
	int sample_step = 5;
	int sampleNum = frameCnt / sample_step;
	int aveDominantColorCnt = 0;
	int threshold = 4;	//fall off parameter

	#pragma omp parallel for 
	for (int i = 0; i < sampleNum; i++)
		aveDominantColorCnt += CalcFramePaletteSize(seedsL[i * sample_step], seedsA[i * sample_step], seedsB[i * sample_step], superpixelSize[i * sample_step], threshold);

	framePaletteSize = (int)round((aveDominantColorCnt * 1.0 / sampleNum));
	cout << "Average fame palette size : " << framePaletteSize << endl;

	oriPalette_R = new double* [framePaletteSize];	changedPalette_R = new double* [framePaletteSize];
	oriPalette_G = new double* [framePaletteSize];	changedPalette_G = new double* [framePaletteSize];
	oriPalette_B = new double* [framePaletteSize];	changedPalette_B = new double* [framePaletteSize];
	for (size_t i = 0; i < framePaletteSize; i++) {
		oriPalette_R[i] = new double[frameCnt];	changedPalette_R[i] = new double[frameCnt];
		oriPalette_G[i] = new double[frameCnt];	changedPalette_G[i] = new double[frameCnt];
		oriPalette_B[i] = new double[frameCnt];	changedPalette_B[i] = new double[frameCnt];
	}

	//extract each frame's palette
	ExtractFirstFramePalette(0, seedsL[0], seedsA[0], seedsB[0]);
	for (size_t i = 1; i < frameCnt; i++)
		ExtractOtherFramePalette(i, seedsL[i], seedsA[i], seedsB[i]);

	for (size_t i = 0; i < frameCnt; i++) {
		seedsL[i].clear(); seedsL[i].shrink_to_fit();
		seedsA[i].clear(); seedsA[i].shrink_to_fit();
		seedsB[i].clear(); seedsB[i].shrink_to_fit();
	}
	seedsL.clear(); seedsL.shrink_to_fit();
	seedsA.clear(); seedsA.shrink_to_fit();
	seedsB.clear(); seedsB.shrink_to_fit();

	//2. extract video palette by fitting bezier curves========================================================================================== 
	this->BezierCtrlPointNum = BezierCtrlPointNum;

	CtrlPointBernsteinCoeff = vector<double>(frameCnt * BezierCtrlPointNum);
	for (int i = 0; i < frameCnt; i++) {
		for (int j = 0; j < BezierCtrlPointNum; j++)
			CtrlPointBernsteinCoeff[i * BezierCtrlPointNum + j] = BernsteinNum(j, BezierCtrlPointNum - 1, (double)i / (frameCnt - 1));
	}

	bezeirControl_L = vector<vector<double>>(framePaletteSize, vector<double>(BezierCtrlPointNum));
	bezeirControl_A = vector<vector<double>>(framePaletteSize, vector<double>(BezierCtrlPointNum));
	bezeirControl_B = vector<vector<double>>(framePaletteSize, vector<double>(BezierCtrlPointNum));

	double* pointR, *pointG, *pointB;
	data3 data = { frameCnt, BezierCtrlPointNum, pointR, pointG, pointB, CtrlPointBernsteinCoeff };

	double minf;
	nlopt::opt opt(nlopt::LD_LBFGS, 3 * BezierCtrlPointNum);
	opt.set_lower_bounds(-500);
	opt.set_upper_bounds(500);
	opt.set_xtol_rel(1e-8);
	
	for (size_t i = 0; i < framePaletteSize; i++) {
		data.pointR = this->oriPalette_R[i];
		data.pointG = this->oriPalette_G[i];
		data.pointB = this->oriPalette_B[i];
		vector<double> x = vector<double>(3 * BezierCtrlPointNum);
		opt.set_min_objective(FitBezierLossFunction, &data);
		nlopt::result result = opt.optimize(x, minf);
		for (int j = 0; j < BezierCtrlPointNum; j++){
			bezeirControl_L[i][j] = x[j];
			bezeirControl_A[i][j] = x[BezierCtrlPointNum + j];
			bezeirControl_B[i][j] = x[2 * BezierCtrlPointNum + j];
		}
	}

	//3. calculate weights of each pixel with respect to the its frame palette colors===============================================================
	weights = new float* [frameCnt];
	#pragma omp parallel for
	for (int i = 0; i < frameCnt; i++) {
		weights[i] = new float[framePaletteSize * frameSize];
		CalcWeights(i);
	}

	//extract each frame's palette by cutting the bezier curves in each frame time
	#pragma omp parallel for
	for (int i = 0; i < framePaletteSize; i++) {
		for (size_t j = 0; j < frameCnt; j++) {
			cv::Vec3d color;
			for (size_t k = 0; k < BezierCtrlPointNum; k++) {
				color[0] += CtrlPointBernsteinCoeff[j * BezierCtrlPointNum + k] * bezeirControl_L[i][k];
				color[1] += CtrlPointBernsteinCoeff[j * BezierCtrlPointNum + k] * bezeirControl_A[i][k];
				color[2] += CtrlPointBernsteinCoeff[j * BezierCtrlPointNum + k] * bezeirControl_B[i][k];
			}
			LAB2RGB(color);
			oriPalette_R[i][j] = color[0];
			oriPalette_G[i][j] = color[1];
			oriPalette_B[i][j] = color[2];
		}
	}

	for (size_t i = 0; i < framePaletteSize; i++) {
		memmove(changedPalette_R[i], oriPalette_R[i], sizeof(double) * frameCnt);
		memmove(changedPalette_G[i], oriPalette_G[i], sizeof(double) * frameCnt);
		memmove(changedPalette_B[i], oriPalette_B[i], sizeof(double) * frameCnt);
	}

	isSelected = vector<bool>(framePaletteSize * frameCnt, false);
	selectedColor = vector<vector<int>>(framePaletteSize);

	//calculate the first derivative of each frame at each frame time
	bezierDerivCoeff = vector<double>((BezierCtrlPointNum - 1) * frameCnt);
	for (int i = 0; i < frameCnt; i++) {
		for (size_t j = 0; j < BezierCtrlPointNum - 1; j++)
			bezierDerivCoeff[i * (BezierCtrlPointNum - 1) + j] = (BezierCtrlPointNum - 1) * BernsteinNum(j, BezierCtrlPointNum - 2, (double)i / (frameCnt - 1));
	}

	oriBezierDeriv_L = vector<vector<double>>(framePaletteSize, vector<double>(frameCnt));
	oriBezierDeriv_A = vector<vector<double>>(framePaletteSize, vector<double>(frameCnt));
	oriBezierDeriv_B = vector<vector<double>>(framePaletteSize, vector<double>(frameCnt));

	for (int k = 0; k < framePaletteSize; k++) {
		for (size_t i = 0; i < frameCnt; i++) {
			for (size_t j = 0; j < BezierCtrlPointNum - 1; j++) {
				oriBezierDeriv_L[k][i] += bezierDerivCoeff[i * (BezierCtrlPointNum - 1) + j] * (bezeirControl_L[k][j + 1] - bezeirControl_L[k][j]);
				oriBezierDeriv_A[k][i] += bezierDerivCoeff[i * (BezierCtrlPointNum - 1) + j] * (bezeirControl_A[k][j + 1] - bezeirControl_A[k][j]);
				oriBezierDeriv_B[k][i] += bezierDerivCoeff[i * (BezierCtrlPointNum - 1) + j] * (bezeirControl_B[k][j + 1] - bezeirControl_B[k][j]);
			}
		}
	}
}

void VideoRecolor::ExtractFirstFramePalette(int fid, const vector<double>& seedsL, const vector<double>& seedsA, const vector<double>& seedsB) {
	int seedsNum = seedsL.size();
	vector<double> weights = vector<double>(seedsNum, 1.0);
	vector<cv::Vec3d> colors(seedsNum);
	for (size_t i = 0; i < seedsNum; i++) {
		colors[i][0] = seedsL[i];
		colors[i][1] = seedsA[i];
		colors[i][2] = seedsB[i];
	}
	vector<cv::Vec3d> centroids = vector<cv::Vec3d>(framePaletteSize);
	centroids[0] = colors[0];
	double min_d = 999;

	for (size_t i = 1; i < centroids.size(); i++) {
		cv::Vec3d last_center = centroids[i - 1];

		int max_index = 0;
		for (size_t j = 0; j < weights.size(); j++) {
			weights[j] *= (1 - exp(-pow(norm(colors[j], last_center) / 80.0, 2)));
			if (weights[j] > weights[max_index]) max_index = j;
		}
		centroids[i] = colors[max_index];
	}

	vector<cv::Vec3d> old_centroids = vector<cv::Vec3d>(framePaletteSize);
	vector<double> clusterCnt = vector<double>(framePaletteSize);
	for (size_t it = 0; it < 500; it++) {
		fill(old_centroids.begin(), old_centroids.end(), 0);
		fill(clusterCnt.begin(), clusterCnt.end(), 0);

		//label each point to its closet cluster centorid
		for (size_t i = 0; i < colors.size(); i++) {
			int label = 0;
			min_d = 9999;
			for (size_t j = 0; j < centroids.size(); j++) {
				double dis = norm(colors[i], centroids[j]);
				if (dis < min_d) {
					label = j;
					min_d = dis;
				}
			}
			old_centroids[label] += colors[i];
			clusterCnt[label]++;
		}

		//update each cluster's centorid
		for (size_t i = 0; i < old_centroids.size(); i++)
			old_centroids[i] = old_centroids[i] / clusterCnt[i];

		if (norm(old_centroids, centroids) < 1e-5) {
			centroids.assign(old_centroids.begin(), old_centroids.end());
			break;
		}
		centroids.assign(old_centroids.begin(), old_centroids.end());
	}

	for (size_t i = 0; i < framePaletteSize; i++) {
		oriPalette_R[i][fid] = centroids[i][0];
		oriPalette_G[i][fid] = centroids[i][1];
		oriPalette_B[i][fid] = centroids[i][2];
	}
}

void VideoRecolor::ExtractOtherFramePalette(int fid, const vector<double>& seedsL, const vector<double>& seedsA, const vector<double>& seedsB) {
	int seedsNum = seedsL.size();
	vector<cv::Vec3d> colors(seedsNum);
	for (size_t i = 0; i < seedsNum; i++) {
		colors[i][0] = seedsL[i];
		colors[i][1] = seedsA[i];
		colors[i][2] = seedsB[i];
	}

	vector<cv::Vec3d> centroids = vector<cv::Vec3d>(framePaletteSize);// 存储调色板颜色对应的LAB值
	vector<cv::Vec3d> old_centroids = vector<cv::Vec3d>(framePaletteSize);
	vector<double> clusterCnt = vector<double>(framePaletteSize);
	
	//initilize each cluster's centroid with previous frame's palette
	for (size_t i = 0; i < framePaletteSize; i++) {
		centroids[i][0] = oriPalette_R[i][fid - 1];
		centroids[i][1] = oriPalette_G[i][fid - 1];
		centroids[i][2] = oriPalette_B[i][fid - 1];
	}

	for (size_t it = 0; it < 10; it++) {
		fill(old_centroids.begin(), old_centroids.end(), 0);
		fill(clusterCnt.begin(), clusterCnt.end(), 0);

		//label cloest centroid
		for (size_t i = 0; i < colors.size(); i++) {
			int label = 0;
			int min_d = 9999;
			for (size_t j = 0; j < centroids.size(); j++) {
				double dis = squareDistance(colors[i], centroids[j]);
				if (dis < min_d) {
					label = j;
					min_d = dis;
				}
			}
			old_centroids[label] += colors[i];
			clusterCnt[label]++;
		}

		//update each cluster's centorid
		for (size_t i = 0; i < old_centroids.size(); i++) {
			if (clusterCnt[i] == 0) {
				cout << "empty centroid: frame " << fid << " /palette " << i << endl;
				old_centroids[i] = centroids[i];
				clusterCnt[i] = 1;
			}
			old_centroids[i] = old_centroids[i] / clusterCnt[i];
		}

		if (norm(old_centroids, centroids) < 1e-5) {
			centroids.assign(old_centroids.begin(), old_centroids.end());
			break;
		}
		centroids.assign(old_centroids.begin(), old_centroids.end());
	}

	for (size_t i = 0; i < framePaletteSize; i++) {
		oriPalette_R[i][fid] = old_centroids[i][0];
		oriPalette_G[i][fid] = old_centroids[i][1];
		oriPalette_B[i][fid] = old_centroids[i][2];
	}
}

double VideoRecolor::calc_param(int index, vector<double>& lamda) {
	lamda = vector<double>(framePaletteSize * framePaletteSize);

	// 累加所有调色板颜色间的LAB距离
	double sigma = 0;
	for (size_t i = 0; i < framePaletteSize; i++) {
		for (size_t j = i + 1; j < framePaletteSize; j++) {
			sigma += lab_distance(oriPalette_R[i][index], oriPalette_G[i][index], oriPalette_B[i][index],
				oriPalette_R[j][index], oriPalette_G[j][index], oriPalette_B[j][index]);
		}
	}

	sigma /= (framePaletteSize - 1) * framePaletteSize / 2;// 求平均距离
	double param = 5.0 / (sigma * sigma);

	Eigen::MatrixXd D(framePaletteSize, framePaletteSize);// 创建framePaletteSize行framePaletteSize列的动态矩阵

	for (size_t i = 0; i < framePaletteSize; i++) {
		for (size_t j = 0; j < framePaletteSize; j++)
			D(i, j) = phiFunction(	oriPalette_R[i][index], oriPalette_G[i][index], oriPalette_B[i][index],
									oriPalette_R[j][index], oriPalette_G[j][index], oriPalette_B[j][index], param);
	}

	D = D.inverse(); // 矩阵逆运算
	for (size_t i = 0; i < framePaletteSize; i++) {
		for (size_t j = 0; j < framePaletteSize; j++)
			lamda[i * framePaletteSize + j] = D(i, j);
	}
	return param;
}

void VideoRecolor::CalcSinglePointWeights(int findex, const cv::Vec3d& point, double param, const vector<double>& lamda, vector<double>& singleWeight) {
	singleWeight = vector<double>(this->framePaletteSize);

	for (size_t i = 0; i < framePaletteSize; i++) {
		for (size_t j = 0; j < framePaletteSize; j++)
			singleWeight[i] += lamda[i * framePaletteSize + j] * phiFunction(oriPalette_R[j][findex], oriPalette_G[j][findex], oriPalette_B[j][findex],point[0], point[1], point[2], param);
	}

	double sum = 0;
	for (size_t i = 0; i < framePaletteSize; i++) {
		if (singleWeight[i] > 0) sum += singleWeight[i];
		else singleWeight[i] = 0;
	}
	for (size_t i = 0; i < framePaletteSize; i++)
		singleWeight[i] /= sum;
}

void VideoRecolor::CalcWeights(int findex) {
	int gridn = 16;
	double step = 255.0 / gridn;
	vector<double> lamda;
	double param = calc_param(findex, lamda);
	//calc grid verts weights
	vector<vector<double>> grid_weights = vector<vector<double>>(framePaletteSize, vector<double>(pow(gridn + 1, 3)));

	vector<double> single_point_weights;
	cv::Vec3d gridv;
	int index = 0;
	for (size_t r = 0; r < gridn + 1; r++) {
		for (size_t g = 0; g < gridn + 1; g++) {
			for (size_t b = 0; b < gridn + 1; b++) {
				gridv = { r * step, g * step, b * step };
				RGB2LAB(gridv);
				CalcSinglePointWeights(findex, gridv, param, lamda, single_point_weights);
				index = r * (gridn + 1) * (gridn + 1) + g * (gridn + 1) + b;
				for (size_t i = 0; i < framePaletteSize; i++) {
					grid_weights[i][index] = single_point_weights[i];
				}
			}
		}
	}
	double r_frac, g_frac, b_frac;
	int r, g, b;

	for (size_t i = 0; i < frameSize; i++) {
		r = (int)(oriVideo_R[findex][i] / step);
		g = (int)(oriVideo_G[findex][i] / step);
		b = (int)(oriVideo_B[findex][i] / step);
		r_frac = oriVideo_R[findex][i] / step - r;
		g_frac = oriVideo_G[findex][i] / step - g;
		b_frac = oriVideo_B[findex][i] / step - b;

		if (abs(r - gridn) < 1e-4)  r = 15, r_frac = 1.0;
		if (abs(g - gridn) < 1e-4)	g = 15, g_frac = 1.0;
		if (abs(b - gridn) < 1e-4)	b = 15, b_frac = 1.0;

		// 将对应块的权重赋值给findex帧第j个颜色第i个像素
		for (size_t j = 0; j < framePaletteSize; j++) {
			weights[findex][j * frameSize + i] = grid_weights[j][r * (gridn + 1) * (gridn + 1) + g * (gridn + 1) + b] * (1 - r_frac) * (1 - g_frac) * (1 - b_frac)
				+ grid_weights[j][r * (gridn + 1) * (gridn + 1) + g * (gridn + 1) + b + 1] * (1 - r_frac) * (1 - g_frac) * b_frac
				+ grid_weights[j][r * (gridn + 1) * (gridn + 1) + (g + 1) * (gridn + 1) + b] * (1 - r_frac) * g_frac * (1 - b_frac)
				+ grid_weights[j][r * (gridn + 1) * (gridn + 1) + (g + 1) * (gridn + 1) + b + 1] * (1 - r_frac) * g_frac * b_frac
				+ grid_weights[j][(r + 1) * (gridn + 1) * (gridn + 1) + g * (gridn + 1) + b] * r_frac * (1 - g_frac) * (1 - b_frac)
				+ grid_weights[j][(r + 1) * (gridn + 1) * (gridn + 1) + g * (gridn + 1) + b + 1] * r_frac * (1 - g_frac) * b_frac
				+ grid_weights[j][(r + 1) * (gridn + 1) * (gridn + 1) + (g + 1) * (gridn + 1) + b] * r_frac * g_frac * (1 - b_frac)
				+ grid_weights[j][(r + 1) * (gridn + 1) * (gridn + 1) + (g + 1) * (gridn + 1) + b + 1] * r_frac * g_frac * b_frac;
		}
	}
}

void VideoRecolor::RecolorVideo() {
	if (!isPaletteCalc) return;

	time_t start, end;
	start = clock();

	#pragma omp parallel for
	for (int fid = 0; fid < frameCnt; fid++) {
		memcpy(changedVideo_R[fid], oriVideo_R[fid], sizeof(sint) * frameSize);
		memcpy(changedVideo_G[fid], oriVideo_G[fid], sizeof(sint) * frameSize);
		memcpy(changedVideo_B[fid], oriVideo_B[fid], sizeof(sint) * frameSize);
		for (int i = 0; i < frameSize; i++) {
			for (size_t j = 0; j < framePaletteSize; j++) {
				changedVideo_R[fid][i] += weights[fid][j * frameSize + i] * (changedPalette_R[j][fid] - oriPalette_R[j][fid]);
				changedVideo_G[fid][i] += weights[fid][j * frameSize + i] * (changedPalette_G[j][fid] - oriPalette_G[j][fid]);
				changedVideo_B[fid][i] += weights[fid][j * frameSize + i] * (changedPalette_B[j][fid] - oriPalette_B[j][fid]);
			}
		}

		for (size_t i = 0; i < frameSize; i++) {
			changedVideo_R[fid][i] = clamp_(changedVideo_R[fid][i], 0, 255);
			changedVideo_G[fid][i] = clamp_(changedVideo_G[fid][i], 0, 255);
			changedVideo_B[fid][i] = clamp_(changedVideo_B[fid][i], 0, 255);
		}
	}

	end = clock();
	cout << end - start << " ms" << endl;
}

void VideoRecolor::RecolorFrame(int fid) {
	if (!isPaletteCalc) return;

	#pragma omp parallel for
	for (int i = 0; i < frameSize; i++) {
		changedVideo_R[fid][i] = oriVideo_R[fid][i];
		changedVideo_G[fid][i] = oriVideo_G[fid][i];
		changedVideo_B[fid][i] = oriVideo_B[fid][i];
		for (size_t j = 0; j < framePaletteSize; j++) {
			changedVideo_R[fid][i] += weights[fid][j * frameSize + i] * (changedPalette_R[j][fid] - oriPalette_R[j][fid]);
			changedVideo_G[fid][i] += weights[fid][j * frameSize + i] * (changedPalette_G[j][fid] - oriPalette_G[j][fid]);
			changedVideo_B[fid][i] += weights[fid][j * frameSize + i] * (changedPalette_B[j][fid] - oriPalette_B[j][fid]);
		}
	}

	for (size_t i = 0; i < frameSize; i++) {
		changedVideo_R[fid][i] = clamp_(changedVideo_R[fid][i], 0, 255);
		changedVideo_G[fid][i] = clamp_(changedVideo_G[fid][i], 0, 255);
		changedVideo_B[fid][i] = clamp_(changedVideo_B[fid][i], 0, 255);
	}
	emit updated();
}

void VideoRecolor::ChangeFrameTime(int frameId) {
	this->currFrameId = frameId;
	RecolorFrame(frameId);
	emit updated();
}

void VideoRecolor::DeformBezierCurve() {
	if (!isPaletteCalc) return;

	double minf;
	nlopt::opt opt(nlopt::LN_COBYLA, BezierCtrlPointNum);
	opt.set_lower_bounds(-500);
	opt.set_upper_bounds(500);
	opt.set_xtol_rel(1e-3);

	time_t start, end;
	start = clock_t();

	for (int pindex = 0; pindex < framePaletteSize; pindex++) {
		int mnum = selectedColor[pindex].size();
		if (mnum < 1) continue;

		vector<double> changedL(mnum);
		vector<double> changedA(mnum);
		vector<double> changedB(mnum);

		for (size_t i = 0; i < mnum; i++) {
			int findex = selectedColor[pindex][i];
			cv::Vec3d color = { changedPalette_R[pindex][findex],changedPalette_G[pindex][findex],changedPalette_B[pindex][findex] };
			RGB2LAB(color);
			changedL[i] = color[0];
			changedA[i] = color[1];
			changedB[i] = color[2];
		}
		double lamda = 0.001;
		data2 dt = { BezierCtrlPointNum, frameCnt, CtrlPointBernsteinCoeff, oriBezierDeriv_L[pindex], selectedColor[pindex], changedL, bezierDerivCoeff, lamda};
		opt.set_min_objective(RefitBezierLossFunction, &dt);
		cout << "minf:" << minf << endl;

		vector<double>x(bezeirControl_L[pindex]);
		nlopt::result result = opt.optimize(x, minf);

		for (size_t i = 0; i < frameCnt; i++) {
			changedPalette_R[pindex][i] = 0;
			changedPalette_G[pindex][i] = 0;
			changedPalette_B[pindex][i] = 0;
		}

		for (size_t i = 0; i < frameCnt; i++) {
			for (size_t j = 0; j < BezierCtrlPointNum; j++)
				changedPalette_R[pindex][i] += CtrlPointBernsteinCoeff[i * BezierCtrlPointNum + j] * x[j];
		}

		dt.oriBezierDeriv = oriBezierDeriv_A[pindex];
		dt.changedPoint = changedA;
		opt.set_min_objective(RefitBezierLossFunction, &dt);

		x = bezeirControl_A[pindex];
		result = opt.optimize(x, minf);
		cout << "minf:" << minf << endl;

		for (size_t i = 0; i < frameCnt; i++) {
			for (size_t j = 0; j < BezierCtrlPointNum; j++)
				changedPalette_G[pindex][i] += CtrlPointBernsteinCoeff[i * BezierCtrlPointNum + j] * x[j];
		}

		dt.oriBezierDeriv = oriBezierDeriv_B[pindex];
		dt.changedPoint = changedB;
		opt.set_min_objective(RefitBezierLossFunction, &dt);

		x = bezeirControl_B[pindex];
		result = opt.optimize(x, minf);
		cout << "minf:" << minf << endl;

		for (size_t i = 0; i < frameCnt; i++) {
			for (size_t j = 0; j < BezierCtrlPointNum; j++)
				changedPalette_B[pindex][i] += CtrlPointBernsteinCoeff[i * BezierCtrlPointNum + j] * x[j];
		}

		for (size_t i = 0; i < frameCnt; i++) {
			cv::Vec3d color = { changedPalette_R[pindex][i], changedPalette_G[pindex][i] , changedPalette_B[pindex][i] };
			LAB2RGB(color);
			changedPalette_R[pindex][i] = color[0];
			changedPalette_G[pindex][i] = color[1];
			changedPalette_B[pindex][i] = color[2];
		}
	}
	end = clock_t();
	cout << "palette reshape time: " << end - start << " ms" << endl;
	emit updated();
}

void VideoRecolor::ExportRecoloredVideo(QString filename) {
	RecolorVideo();

	string Path = filename.toStdString() + "/" + videoName + "_recolor.mp4";
	cv::VideoWriter videoWriter(Path, cv::VideoWriter::fourcc('m', 'p', '4', 'v'), fps, cv::Size(frameCols, frameRows), true);
	for (size_t i = 0; i < frameCnt; i++) {
		cv::Mat frame(frameRows, frameCols, CV_8UC3);
		for (size_t row = 0; row < frameRows; row++) {
			uchar* uc_pixel = frame.data + row * frame.step;
			int index = row * frameCols;
			for (size_t col = 0; col < frameCols; col++) {
				uc_pixel[0] = changedVideo_B[i][index];
				uc_pixel[1] = changedVideo_G[i][index];
				uc_pixel[2] = changedVideo_R[i][index];

				uc_pixel += 3;
				index++;
			}
		}
		videoWriter << frame;
	}
	videoWriter.release();
}

void VideoRecolor::SetPaletteColor(int id, QColor c) {
	changedPalette_R[id][currFrameId] = qRed(c.rgb());
	changedPalette_G[id][currFrameId] = qGreen(c.rgb());
	changedPalette_B[id][currFrameId] = qBlue(c.rgb());

	int index = currFrameId * framePaletteSize + id;
	if (!isSelected[index]) {
		isSelected[index] = true;
		selectedColor[id].push_back(currFrameId);
	}
}

void VideoRecolor::ImportEditedFramePalettes(QString filename) {
	int palette_id, palette_frame, palette_r, palette_g, palette_b;
	ifstream file(filename.toStdString());
	string line;

	if (!file.is_open()){
		cout << "Failed to open file.\n";
		return;
	}

	while (getline(file, line)) {
		if (line.find("Palette ID:") != string::npos) {
			stringstream ss;
			string temp;
			char ch;

			ss.str(line);
			ss >> temp >> temp >> palette_id >> temp >> temp >> palette_frame;

			if (getline(file, line)) {
				ss.clear();
				ss.str(line);
				ss >> temp >> palette_r >> temp >> palette_g >> temp >> palette_b;
				changedPalette_R[palette_id][palette_frame] = palette_r;
				changedPalette_G[palette_id][palette_frame] = palette_g;
				changedPalette_B[palette_id][palette_frame] = palette_b;

				int index = palette_frame * framePaletteSize + palette_id;
				if (!isSelected[index]) {
					isSelected[index] = true;
					selectedColor[palette_id].push_back(palette_frame);
				}
				RecolorFrame(palette_frame);
			}
		}
	}
	file.close();
}

void VideoRecolor::ExportEditedFramePalettes(QString filename) {
	std::ofstream file(filename.toStdString() + "/" + videoName + "_edited_palette.txt", std::ios::out);
	if (file.is_open()) {
		for (size_t i = 0; i < framePaletteSize; i++){
			size_t ed = selectedColor[i].size();
			for (size_t j = 0; j < ed; j++){
				file << "Palette ID: " << i << ", Frame: " << selectedColor[i][j] << "\n";
				file << "Colors:R= " << int(changedPalette_R[i][selectedColor[i][j]]);
				file << " ,G= " << int(changedPalette_G[i][selectedColor[i][j]]);
				file << " ,B= " << int(changedPalette_B[i][selectedColor[i][j]]) << "\n";
			}
		}
		file.close(); 
	}
}

void VideoRecolor::ResetCurrFramePaletteColor(int id) {
	changedPalette_R[id][currFrameId] = oriPalette_R[id][currFrameId];
	changedPalette_G[id][currFrameId] = oriPalette_G[id][currFrameId];
	changedPalette_B[id][currFrameId] = oriPalette_B[id][currFrameId];
}

void VideoRecolor::ResetCurrFrameAllPaletteColors() {
	for (size_t i = 0; i < framePaletteSize; i++) {
		changedPalette_R[i][currFrameId] = oriPalette_R[i][currFrameId];
		changedPalette_G[i][currFrameId] = oriPalette_G[i][currFrameId];
		changedPalette_B[i][currFrameId] = oriPalette_B[i][currFrameId];
	}
}

void VideoRecolor::ResetAllFramePalettes() {
	if (!isVideoOpen || !isPaletteCalc) return;
	for (size_t i = 0; i < framePaletteSize; i++) {
		memmove(changedPalette_R[i], oriPalette_R[i], sizeof(double) * frameCnt);
		memmove(changedPalette_G[i], oriPalette_G[i], sizeof(double) * frameCnt);
		memmove(changedPalette_B[i], oriPalette_B[i], sizeof(double) * frameCnt);
	}

	selectedFrame.clear();
	selectedId.clear();

	for (size_t i = 0; i < framePaletteSize; i++) {
		selectedColor[i].clear();
	}
	for (size_t i = 0; i < framePaletteSize * frameCnt; i++) {
		isSelected[i] = false;
	}
	emit updated();
}

void VideoRecolor::RemoveSelection(int id) {
	int index = -1;
	for (size_t i = 0; i < selectedColor[id].size(); i++)
		if (selectedColor[id][i] == currFrameId) index = i;
	if (index == -1) return;

	for (vector<int>::iterator it = selectedColor[id].begin(); it != selectedColor[id].end();) {
		if (*it == currFrameId) {
			selectedColor[id].erase(it);
			break;
		}
		else it++;
	}
	isSelected[currFrameId * framePaletteSize + id] = false;
}

double VideoRecolor::CalcFramePaletteSize(const vector<double>& seedsL, const vector<double>& seedsA, const vector<double>& seedsB, const vector<int>& superpixelSize, int threshold) {
	vector<cv::Vec3d> sample;
	vector<double> weights;

	for (size_t i = 0; i < seedsL.size(); i++) {
		cv::Vec3d color((double)seedsL[i], (double)seedsA[i], (double)seedsB[i]);
		sample.push_back(color);
		weights.push_back(superpixelSize[i]);
	}
	int max_index = 0;
	double max_value = 0;
	vector<cv::Vec3d> palette;

	for (size_t i = 0; i < sample.size(); i++) {
		max_value = 0;
		for (size_t j = 0; j < sample.size(); j++) {
			if (weights[j] > max_value) {
				max_index = j;
				max_value = weights[j];
			}
		}
		if (max_value < threshold) break;

		palette.push_back(sample[max_index]);

		for (size_t j = 0; j < sample.size(); j++)
			weights[j] *= (1 - exp(-pow(norm(sample[max_index], sample[j]) / 80, 4)));
	}
	return palette.size();
}