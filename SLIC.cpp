#include "SLIC.h"

// 将sRGB颜色值转换为CIE 1931 XYZ色彩空间
void RGB2XYZ(
	const int& sR,
	const int& sG,
	const int& sB,
	double& X,
	double& Y,
	double& Z)
{
	double R = sR / 255.0;
	double G = sG / 255.0;
	double B = sB / 255.0;

	double r, g, b;

	if (R <= 0.04045)	r = R / 12.92;
	else				r = pow((R + 0.055) / 1.055, 2.4);
	if (G <= 0.04045)	g = G / 12.92;
	else				g = pow((G + 0.055) / 1.055, 2.4);
	if (B <= 0.04045)	b = B / 12.92;
	else				b = pow((B + 0.055) / 1.055, 2.4);

	X = r * 0.4124564 + g * 0.3575761 + b * 0.1804375;
	Y = r * 0.2126729 + g * 0.7151522 + b * 0.0721750;
	Z = r * 0.0193339 + g * 0.1191920 + b * 0.9503041;
}

// 将sRGB颜色值转换为Lab色彩空间值
void RGB2LAB(const int& sR, const int& sG, const int& sB, double& lval, double& aval, double& bval)
{
	//------------------------
	// sRGB to XYZ conversion
	//------------------------
	double X, Y, Z;
	RGB2XYZ(sR, sG, sB, X, Y, Z);

	//------------------------
	// XYZ to LAB conversion
	//------------------------
	double epsilon = 0.008856;	//actual CIE standard
	double kappa = 903.3;		//actual CIE standard

	double Xr = 0.950456;	//reference white
	double Yr = 1.0;		//reference white
	double Zr = 1.088754;	//reference white

	double xr = X / Xr;
	double yr = Y / Yr;
	double zr = Z / Zr;

	double fx, fy, fz;
	if (xr > epsilon)	fx = pow(xr, 1.0 / 3.0);
	else				fx = (kappa * xr + 16.0) / 116.0;
	if (yr > epsilon)	fy = pow(yr, 1.0 / 3.0);
	else				fy = (kappa * yr + 16.0) / 116.0;
	if (zr > epsilon)	fz = pow(zr, 1.0 / 3.0);
	else				fz = (kappa * zr + 16.0) / 116.0;

	lval = 116.0 * fy - 16.0;
	aval = 500.0 * (fx - fy);
	bval = 200.0 * (fy - fz);
}

// 将一组RGB颜色值转换为相应的Lab色彩空间值
void DoRGBtoLABConversion(
	const sint* imageR,
	const sint* imageG,
	const sint* imageB,
	double* imgL,
	double* imgA,
	double* imgB,
	const int& size
) {

	for (int i = 0; i < size; i++) {
		RGB2LAB(imageR[i], imageG[i], imageB[i], imgL[i], imgA[i], imgB[i]);
	}

}

// 对Lab色彩空间中的图像进行边缘检测
void DetectLabEdges(
	const double* lvec,
	const double* avec,
	const double* bvec,
	const int& width,
	const int& height,
	vector<double>& edges) {
	// 计算图像总像素数
	int sz = width * height;

	// 调整 edges 向量的大小，使其与图像像素数一致，并将所有元素初始化为 0
	edges.resize(sz, 0);

	// 遍历图像的垂直方向
	for (int j = 1; j < height - 1; j++)
	{
		// 遍历图像的水平方向
		for (int k = 1; k < width - 1; k++)
		{	
			// 计算出当前像素点所在的索引
			int i = j * width + k;

			// 计算当前像素点水平方向的梯度（Lab空间中两点的距离）的平方
			double dx = (lvec[i - 1] - lvec[i + 1]) * (lvec[i - 1] - lvec[i + 1]) +
				(avec[i - 1] - avec[i + 1]) * (avec[i - 1] - avec[i + 1]) +
				(bvec[i - 1] - bvec[i + 1]) * (bvec[i - 1] - bvec[i + 1]);

			// 计算当前像素点垂直方向的梯度（Lab空间中两点的距离）的平方
			double dy = (lvec[i - width] - lvec[i + width]) * (lvec[i - width] - lvec[i + width]) +
				(avec[i - width] - avec[i + width]) * (avec[i - width] - avec[i + width]) +
				(bvec[i - width] - bvec[i + width]) * (bvec[i - width] - bvec[i + width]);

			//edges[i] = (sqrt(dx) + sqrt(dy));
			// 将水平方向和垂直方向梯度的平方和作为梯度的幅值
			edges[i] = (dx + dy);
		}
	}
}

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
	const vector<double>& edges)
{
	// 规定8个方向的偏移量
	const int dx8[8] = { -1, -1,  0,  1, 1, 1, 0, -1 };
	const int dy8[8] = { 0, -1, -1, -1, 0, 1, 1,  1 };

	// 超像素中心的数量
	int numseeds = kseedsl.size();

	// 遍历所有超像素中心
	for (int n = 0; n < numseeds; n++)
	{
		// 获取当前超像素中心的原始位置和索引
		int ox = kseedsx[n];//original x
		int oy = kseedsy[n];//original y
		int oind = oy * m_width + ox;

		// 存储当前索引
		int storeind = oind;

		// 遍历超像素中心的八个方向
		for (int i = 0; i < 8; i++)
		{
			// 对超像素中心施加偏移，记录新的位置
			int nx = ox + dx8[i];//new x
			int ny = oy + dy8[i];//new y

			// 防止超出边界
			if (nx >= 0 && nx < m_width && ny >= 0 && ny < m_height)
			{
				// 计算当前的索引
				int nind = ny * m_width + nx;

				// 如果超像素中心的新位置的边缘强度低于当前存储的位置则更换存储索引
				if (edges[nind] < edges[storeind])
				{
					storeind = nind;
				}
			}
		}
		// 如果超像素中心的索引更改，则要同时更改超像素中心的位置和Lab色彩空间的值
		if (storeind != oind)
		{
			kseedsx[n] = storeind % m_width;
			kseedsy[n] = storeind / m_width;
			kseedsl[n] = imgL[storeind];
			kseedsa[n] = imgA[storeind];
			kseedsb[n] = imgB[storeind];
		}
	}
}

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
	const vector<double>& edgemag)
{
	// 计算图像的总像素数
	int sz = m_width * m_height;
	// 基于SILC算法，规定超像素之间的间隔，即步长
	double step = sqrt(double(sz) / double(K));
	// 步长取整
	int T = step;

	// 计算 x 和 y 方向的偏移，用于从步长的中心开始网格化。
	int xoff = step / 2;
	int yoff = step / 2;

	// 存储超像素的数量
	int n(0); 
	// 存储当前的行数
	int r(0);
	// 遍历图像的垂直方向
	for (int y = 0; y < m_height; y++)
	{
		// 超像素在垂直方向上的中心位置
		int Y = y * step + yoff;
		// 超像素在垂直方向上的中心位置超出边界说明超像素已全部分割完成
		if (Y > m_height - 1) break;

		// 遍历图像的水平方向
		for (int x = 0; x < m_width; x++)
		{
			//int X = x*step + xoff;//square grid
			// 超像素在水平方向上的中心位置，根据当前行数的奇偶偏移量有所不同，目的是为了使超像素分布更加均匀
			int X = x * step + (xoff << (r & 0x1));//hex grid

			// 超像素在水平方向上的中心位置超出边界说明该行的超像素已全部分割完成
			if (X > m_width - 1) break;

			// 计算出当前超像素中心在图像中所在的索引
			int i = Y * m_width + X;

			// 存储超像素中心的Lab色彩空间的值
			kseedsl.push_back(imgL[i]);
			kseedsa.push_back(imgA[i]);
			kseedsb.push_back(imgB[i]);
			// 存储超像素中心的位置信息
			kseedsx.push_back(X);
			kseedsy.push_back(Y);
			// 超像素数量加1
			n++;
		}
		// 对应行数加1
		r++;
	}

	PerturbSeeds(imgL, imgA, imgB, kseedsl, kseedsa, kseedsb, kseedsx, kseedsy, m_width, m_height, edgemag);
}

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
	vector<int>& superpixelSize)
{
	// 计算像素的数量
	int sz = m_width * m_height;
	// 获取超像素聚类中心的数量
	const int numk = kseedsl.size();

	//double cumerr(99999.9);
	// 存储迭代次数
	int numitr(0);

	//----------------
	// 调整超像素的大小
	int offset = STEP;
	if (STEP < 10) offset = STEP * 1.5;
	//----------------

	vector<double> sigmal(numk, 0);
	vector<double> sigmaa(numk, 0);
	vector<double> sigmab(numk, 0);
	vector<double> sigmax(numk, 0);
	vector<double> sigmay(numk, 0);
	vector<double> inv(numk, 0);//to store 1/superpixelSize[k] values
	vector<double> distxy(sz, DBL_MAX);
	vector<double> distlab(sz, DBL_MAX);
	vector<double> distvec(sz, DBL_MAX);
	vector<double> maxlab(numk, 10 * 10);//THIS IS THE VARIABLE VALUE OF M, just start with 10
	vector<double> maxxy(numk, STEP * STEP);//THIS IS THE VARIABLE VALUE OF M, just start with 10

	double invxywt = 1.0 / (STEP * STEP);//NOTE: this is different from how usual SLIC/LKM works

	// 规定迭代次数不超过10次
	while (numitr < NUMITR)
	{
		//------
		//cumerr = 0;
		numitr++;
		//------
		// 将每个像素点与超像素聚类中心的综合相似度重新赋值为双精度浮点数的最大值
		distvec.assign(sz, DBL_MAX);

		// 遍历每个超像素聚类中心
		for (int n = 0; n < numk; n++)
		{
			// 确定超像素的区域，以当前超像素的聚类中心为中心，y1 表示上边界，y2 表示下边界，x1 表示左边界，x2 表示右边界
			int y1 = std::max(0, (int)(kseedsy[n] - offset));
			int y2 = std::min(m_height, (int)(kseedsy[n] + offset));
			int x1 = std::max(0, (int)(kseedsx[n] - offset));
			int x2 = std::min(m_width, (int)(kseedsx[n] + offset));

			// 以垂直方向遍历超像素区域
			for (int y = y1; y < y2; y++)
			{
				// 以水平方向遍历超像素区域
				for (int x = x1; x < x2; x++)
				{
					// 计算当前像素的索引
					int i = y * m_width + x;
					// 判断是否越界，越界则终止程序
					_ASSERT(y < m_height&& x < m_width&& y >= 0 && x >= 0);

					// 记录当前像素的Lab色彩空间的值
					double l = imgL[i];
					double a = imgA[i];
					double b = imgB[i];

					// 计算当前像素与超像素中心的颜色相似度
					distlab[i] = (l - kseedsl[n]) * (l - kseedsl[n]) +
						(a - kseedsa[n]) * (a - kseedsa[n]) +
						(b - kseedsb[n]) * (b - kseedsb[n]);

					// 计算当前像素与超像素中心的空间相似度
					distxy[i] = (x - kseedsx[n]) * (x - kseedsx[n]) +
						(y - kseedsy[n]) * (y - kseedsy[n]);

					//------------------------------------------------------------------------
					// 计算出当前像素与超像素中心的综合相似度
					double dist = distlab[i] / maxlab[n] + distxy[i] * invxywt;//only varying m, prettier superpixels
					//double dist = distlab[i]/maxlab[n] + distxy[i]/maxxy[n];//varying both m and S
					//------------------------------------------------------------------------

					// 如果当前像素与当前超像素聚类中心的综合相似度低于记录的最小综合相似度
					if (dist < distvec[i])
					{
						// 更新当前像素与聚类中心的最小综合相似度
						distvec[i] = dist;
						// 将该像素标记为属于当前的超像素
						klabels[i] = n;
					}
				}
			}
		}
		//-----------------------------------------------------------------
		// Assign the max color distance for a cluster
		//-----------------------------------------------------------------
		// 初次迭代时，将二者赋值为0
		if (0 == numitr)
		{
			maxlab.assign(numk, 1);
			maxxy.assign(numk, 1);
		}
		// 更新最大的空间相似度和颜色形似度
		{for (int i = 0; i < sz; i++)
		{
			if (maxlab[klabels[i]] < distlab[i]) maxlab[klabels[i]] = distlab[i];
			if (maxxy[klabels[i]] < distxy[i]) maxxy[klabels[i]] = distxy[i];
		}}
		//-----------------------------------------------------------------
		// Recalculate the centroid and store in the seed values
		//-----------------------------------------------------------------
		// 将每个超像素的lab分量和xy分量的累积值，以及包含的像素数量重新赋值为0
		sigmal.assign(numk, 0);
		sigmaa.assign(numk, 0);
		sigmab.assign(numk, 0);
		sigmax.assign(numk, 0);
		sigmay.assign(numk, 0);
		superpixelSize.assign(numk, 0);

		// 遍历每个像素，统计各聚类中心的所属像素
		for (int j = 0; j < sz; j++)
		{
			// 获得当前像素所属的聚类中心的索引
			int temp = klabels[j];
			// 判断该索引是否大于0，否则终止程序
			_ASSERT(klabels[j] >= 0);

			// 将该像素的lab分量和xy分量累加进超像素的对应分量
			sigmal[klabels[j]] += imgL[j];
			sigmaa[klabels[j]] += imgA[j];
			sigmab[klabels[j]] += imgB[j];
			sigmax[klabels[j]] += (j % m_width);
			sigmay[klabels[j]] += (j / m_width);

			// 对应超像素的所属像素数量加一
			superpixelSize[klabels[j]]++;
		}

		// 确保每个聚类中心的像素数量不为0，并且计算超像素统括的像素数量的倒数
		{for (int k = 0; k < numk; k++)
		{
			//_ASSERT(superpixelSize[k] > 0);
			if (superpixelSize[k] <= 0) superpixelSize[k] = 1;
			inv[k] = 1.0 / double(superpixelSize[k]);//computing inverse now to multiply, than divide later
		}}


	}
}

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
	vector<int>& superpixelSize) {

	// 计算图像的像素数量
	int size = rows * cols;

	// 存储超像素中心的位置
	vector<double> kseedsx(0);
	vector<double> kseedsy(0);

	// 记录每个像素所属的超像素
	vector<int> labels(size, -1);

	// 存储像素的Lab色彩空间的值
	double* imgL, * imgA, * imgB;
	imgL = new double[size];
	imgA = new double[size];
	imgB = new double[size];

	// 将输入的 RGB 图像转换为 Lab 色彩空间,结果存储在 imgL、imgA、imgB 数组中
	DoRGBtoLABConversion(imageR, imageG, imageB, imgL, imgA, imgB, size);

	// 存储图像中每个像素点的边缘强度
	vector<double> edgemag(0);
	// 对Lab色彩空间中的图像进行边缘检测，将对应像素的边缘强度存储在edgemag数组中
	DetectLabEdges(imgL, imgA, imgB, cols, rows, edgemag);

	// 确定超像素中心的位置
	GetLABXYSeeds_ForGivenK(imgL, imgA, imgB, kseedsl, kseedsa, kseedsb, kseedsx, kseedsy, K, cols, rows, edgemag);

	// 规定超像素的步长
	int STEP = sqrt(double(size) / double(K)) + 2.0;

	// 分割超像素
	PerformSuperpixelSegmentation_VariableSandM(imgL, imgA, imgB, kseedsl, kseedsa, kseedsb, kseedsx, kseedsy, cols, rows, labels, STEP, 10, superpixelSize);

	delete[] imgL;
	delete[] imgA;
	delete[] imgB;
}