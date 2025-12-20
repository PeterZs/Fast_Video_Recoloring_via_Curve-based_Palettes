#include "utility.h"

void RGB2LAB(cv::Vec3d& rgb) {
    double var_r = rgb[0] / 255.0;
    double var_g = rgb[1] / 255.0;
    double var_b = rgb[2] / 255.0;

    if (var_r > 0.04045) var_r = pow((var_r + 0.055) / 1.055, 2.4);
    else var_r /= 12.92;
    if (var_g > 0.04045) var_g = pow((var_g + 0.055) / 1.055, 2.4);
    else var_g /= 12.92;
    if (var_b > 0.04045) var_b = pow((var_b + 0.055) / 1.055, 2.4);
    else var_b /= 12.92;

    var_r *= 100; var_g *= 100; var_b *= 100;

    double X = var_r * 0.4124 + var_g * 0.3576 + var_b * 0.1805;
    double Y = var_r * 0.2126 + var_g * 0.7152 + var_b * 0.0722;
    double Z = var_r * 0.0193 + var_g * 0.1192 + var_b * 0.9505;

    double var_x = X / 95.047;
    double var_y = Y / 100.0;
    double var_z = Z / 108.883;
    if (var_x > 0.008856) var_x = pow(var_x, 1.0 / 3.0);
    else var_x = 7.787 * var_x + 16.0 / 116.0;
    if (var_y > 0.008856) var_y = pow(var_y, 1.0 / 3.0);
    else var_y = 7.787 * var_y + 16.0 / 116.0;
    if (var_z > 0.008856) var_z = pow(var_z, 1.0 / 3.0);
    else var_z = 7.787 * var_z + 16.0 / 116.0;

    rgb[0] = 116.0 * var_y - 16.0;
    rgb[1] = 500.0 * (var_x - var_y);
    rgb[2] = 200.0 * (var_y - var_z);
}

void LAB2RGB(cv::Vec3d& lab) {
    double var_y = (lab[0] + 16.0) / 116.0;
    double var_x = lab[1] / 500.0 + var_y;
    double var_z = var_y - lab[2] / 200.0;

    if (var_y > 0.206893034422) var_y = pow(var_y, 3.0);
    else var_y = (var_y - 16.0 / 116.0) / 7.787;
    if (var_x > 0.206893034422) var_x = pow(var_x, 3.0);
    else var_x = (var_x - 16.0 / 116.0) / 7.787;
    if (var_z > 0.206893034422) var_z = pow(var_z, 3.0);
    else var_z = (var_z - 16.0 / 116.0) / 7.787;

    double X = 95.047 * var_x;
    double Y = 100.0 * var_y;
    double Z = 108.883 * var_z;

    var_x = X / 100.0;
    var_y = Y / 100.0;
    var_z = Z / 100.0;

    double var_r = var_x * 3.2406 + var_y * -1.5372 + var_z * -0.4986;
    double var_g = var_x * -0.9689 + var_y * 1.8758 + var_z * 0.0415;
    double var_b = var_x * 0.0557 + var_y * -0.2040 + var_z * 1.0570;

    if (var_r > 0.0031308) var_r = 1.055 * pow(var_r, 1.0 / 2.4) - 0.055;
    else var_r *= 12.92;
    if (var_g > 0.0031308) var_g = 1.055 * pow(var_g, 1.0 / 2.4) - 0.055;
    else var_g *= 12.92;
    if (var_b > 0.0031308) var_b = 1.055 * pow(var_b, 1.0 / 2.4) - 0.055;
    else var_b *= 12.92;

    lab[0] = var_r * 255.0;
    lab[1] = var_g * 255.0;
    lab[2] = var_b * 255.0;
}

double lab_distance(double r1, double g1, double b1, double r2, double g2, double b2) {
    double K1 = 0.045, K2 = 0.015;
    double del_L = r1 - r2;
    double C1 = sqrt(g1 * g1 + b1 * b1);
    double C2 = sqrt(g2 * g2 + b2 * b2);
    double C_AB = C1 - C2;
    double H_AB = (g1 - g2) * (g1 - g2) + (b1 - b2) * (b1 - b2) - C_AB * C_AB;
    return sqrt(del_L * del_L + C_AB * C_AB / (1.0 + K1 * C1) / (1.0 + K1 * C1) + H_AB / (1.0 + K2 * C1) / (1.0 + K2 * C1));
}


double squareDistance(const vector<cv::Vec3d>& a, const vector<cv::Vec3d>& b) {
    double sum = 0;
    for (size_t i = 0; i < a.size(); i++) {
        sum += pow(a[i][0] - b[i][0], 2) + pow(a[i][1] - b[i][1], 2) + pow(a[i][2] - b[i][2], 2);
    }
    return sum;
}

double FitBezierLossFunction(const vector<double>& x, vector<double>& grad, void* data) {
    const data3* dt = reinterpret_cast<data3*>(data);
    int frameCnt = dt->frameCnt;
    int ctrlPointCnt = dt->BezierCtrlPointNum;

    vector<double> Rs = vector<double>(frameCnt);
    vector<double> Gs = vector<double>(frameCnt);
    vector<double> Bs = vector<double>(frameCnt);
    for (int i = 0; i < frameCnt; i++) {
        for (int j = 0; j < ctrlPointCnt; j++) {
            Rs[i] += dt->A[i * ctrlPointCnt + j] * x[j];
            Gs[i] += dt->A[i * ctrlPointCnt + j] * x[j + ctrlPointCnt];
            Bs[i] += dt->A[i * ctrlPointCnt + j] * x[j + 2 * ctrlPointCnt];
        }
    }

    for (int j = 0; j < ctrlPointCnt; j++) {
        grad[j] = 0;
        for (int i = 0; i < frameCnt; i++)
            grad[j] += 2 * dt->A[i * ctrlPointCnt + j] * (Rs[i] - dt->pointR[i]);
    }
    for (int j = ctrlPointCnt; j < 2 * ctrlPointCnt; j++) {
        grad[j] = 0;
        for (int i = 0; i < frameCnt; i++)
            grad[j] += 2 * dt->A[i * ctrlPointCnt + j - ctrlPointCnt] * (Gs[i] - dt->pointG[i]);
    }
    for (int j = 2 * ctrlPointCnt; j < 3 * ctrlPointCnt; j++) {
        grad[j] = 0;
        for (int i = 0; i < frameCnt; i++)
            grad[j] += 2 * dt->A[i * ctrlPointCnt + j - 2 * ctrlPointCnt] * (Bs[i] - dt->pointB[i]);
    }

    double loss = 0;
    for (int i = 0; i < frameCnt; i++) {
        double tmp = pow(Rs[i] - dt->pointR[i], 2) + pow(Gs[i] - dt->pointG[i], 2) + pow(Bs[i] - dt->pointB[i], 2);
        loss += tmp / 3;
    }
    return loss;
}

double RefitBezierLossFunction(const vector<double>& x, vector<double>& grad, void* data) {
    const data2* dt = reinterpret_cast<data2*>(data);
    int m = dt->frameCnt;
    int n = dt->BezierCtrlPointNum;
    int k = dt->changedIndex.size();
    double result = 0;

    int index = 0;
    vector<double> reCurve = vector<double>(k);
    for (int i = 0; i < k; i++) {
        index = dt->changedIndex[i];
        for (int j = 0; j < n; j++) {
            reCurve[i] += dt->A[index * n + j] * x[j];
        }
    }

    vector<double> refitBezierDeriv = vector<double>(m);
    for (size_t i = 0; i < m; i++) {
        for (size_t j = 0; j < n - 1; j++) {
            refitBezierDeriv[i] += dt->coeff1[i * (n - 1) + j] * (x[j + 1] - x[j]);
        }
    }

    double loss1 = 0;
    double loss2 = 0;
    for (size_t i = 0; i < m; i++) {
        loss1 += pow(refitBezierDeriv[i] - dt->oriBezierDeriv[i], 2);
    }
    loss1 /= abs(m);

    for (size_t i = 0; i < k; i++) {
        loss2 += pow(reCurve[i] - dt->changedPoint[i], 2);
    }
    cout <<"mnum:" << k << endl;

    if (k != 0) {
        loss2 /= k;
    }

    result = loss1 * dt->lamda + loss2;
    cout <<"loss1:" << loss1 << "   loss2:" << loss2 << endl;
    cout << "result: " << result <<endl;
    
    return result;
}

double adjustDistance(const cv::Vec3d& lab1, const cv::Vec3d& lab2, double lamda) {
    double d = lamda * pow(lab1[0] - lab2[0], 2) + pow(lab1[1] - lab2[1], 2) + pow(lab1[2] - lab2[2], 2);
    return sqrt(d);
}

void exportPoint(int f, const vector<cv::Vec3d>& superPixel, const vector<cv::Vec3d>& centroids) {
    ofstream out1("./superPixel/" + to_string(f) + ".txt");
    ofstream out2("./centroids/" + to_string(f) + ".txt");

    for (int i = 0; i < superPixel.size(); i++) {
        out1 << superPixel[i][0] << " " << superPixel[i][1] << " " << superPixel[i][2] << endl;
    }
    for (int i = 0; i < centroids.size(); i++) {
        cv::Vec3d color = centroids[i];
        out2 << centroids[i][0] << " " << centroids[i][1] << " " << centroids[i][2] << " ";
        LAB2RGB(color);
        out2 << color[0] << " " << color[1] << " " << color[2] << endl;
    }

    out1.close();
    out2.close();
}

double clamp_(double s, double lo, double hi) {
    if (s < lo) return lo;
    else if (s > hi) return hi;
    else return s;
}