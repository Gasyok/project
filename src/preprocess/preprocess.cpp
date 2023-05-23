#include <preprocess/preprocess.h>
#include <fstream>
#include <cstdlib>
#include <sstream>

pair<int, int> PreProcess::GetPoint(int l, pair<int, int> point) {
    int i = point.first;
    int j = point.second;
    int i1 = (x_size + (i - 1)) % x_size;
    int i2 = (x_size + (i + 1)) % x_size;

    int j1 = (y_size + (j - 1)) % y_size;
    int j2 = (y_size + (j + 1)) % y_size;

    const vector<pair<int, int>> offsets = {{i1, j}, {i, j}, {i2, j}, {i, j2}, {i, j1}, {i2, j1}};
    return offsets[l];
}
Vector2d PreProcess::bisection(double x1, double x2, double y1, double y2, double tol=1e-17, int max_iter = 3000) {
    double x0 = (x1 + x2) / 2.0;
    double y0 = (y1 + y2) / 2.0;

    int iter = 0;
    double prev_x0, prev_y0;

    while ((std::abs(func(x0, y0)) > tol || std::abs(prev_x0 - x0) > tol || std::abs(prev_y0 - y0) > tol) && iter < max_iter) {
        prev_x0 = x0;
        prev_y0 = y0;

        // Find y0
        if (func(x0, y1) * func(x0, y0) < 0.0) {
            y2 = y0;
        } else {
            y1 = y0;
        }
        y0 = (y1 + y2) / 2.0;

        // Find x0
        if (func(x1, y0) * func(x0, y0) < 0.0) {
            x2 = x0;
        } else {
            x1 = x0;
        }
        x0 = (x1 + x2) / 2.0;

        iter++;
    }
    return Eigen::Vector2d(x0, y0);
}
Vector2d PreProcess::GetOrigin(pair<int, int> point) {
    int i = point.first;
    int j = point.second;
    int i1 = (x_size + (i - 1)) % x_size;
    int i2 = (x_size + (i + 1)) % x_size;

    int j1 = (y_size + (j - 1)) % y_size;
    int j2 = (y_size + (j + 1)) % y_size;


    const vector<pair<int, int>> offsets = {{i1, j}, {i, j}, {i2, j}, {i, j2}, {i, j1}};
    // const vector<pair<int, int>> offsets = {{i, j1}, {i, j}, {i, j2}, {i2, j}, {i1, j}};
    Vector2d res;

    for (const auto& offset : offsets) {
        int new_i = offset.first;
        int new_j = offset.second;

        if(is_opposite(i, j, new_i, new_j)) {
            res = bisection(get_x(i), get_x(new_i), get_y(j), get_y(new_j));
            break;
        }
    }
    // double p;
    // double _alpha = 0.4;
    // p = 0.3 + tan(0.4) * get_x(i) - get_y(j);
    // Vector2d tmp (get_x(i) - p * cos(_alpha) * sin(_alpha), get_y(j) + p * cos(_alpha) * cos(_alpha));
    // res = tmp;
    return res;
}
Eigen::Vector2d PreProcess::normalize(const Eigen::Vector2d& vec) {
    double norm = vec.norm();
    if (norm > 0) {
        return vec / norm;
    } else {
        return Eigen::Vector2d::Zero();
    }
}
Vector2d PreProcess::GetRotationalCoord(int l, pair<int, int>point) {
    // Получение координат x_l, y_l
    std::pair<int, int> point_l = GetPoint(l, point);
    double x_l = get_x(point_l.first);
    double y_l = get_y(point_l.second);
    Vector2d vec_point (x_l, y_l);
    Vector2d origin = GetOrigin(point);

    // Создание матрицы обратного поворота R(-β)
    // Vector2d Deriv (DerivationX(origin(0), origin(1)), DerivationY(origin(0), origin(1)));
    // Vector2d Deriv (diff(func, origin(0), origin(1), true), diff(func, origin(0), origin(1), false));
    Vector2d Deriv(diff([this](double x, double y) { return func(x, y); }, origin(0), origin(1), true),
                   diff([this](double x, double y) { return func(x, y); }, origin(0), origin(1), false));

    Matrix2d R;
    double f_x = diff([this](double x, double y) { return func(x, y); }, origin(0), origin(1), true);
    double f_y = diff([this](double x, double y) { return func(x, y); }, origin(0), origin(1), false);
    double sinus = f_y / sqrt(f_x * f_x + f_y * f_y);
    double cosinus = f_x / sqrt(f_x * f_x + f_y * f_y);
    R << cosinus, -sinus,
        sinus, cosinus;
    // Умножение матрицы обратного поворота на вектор (x_l - x₀, y_l - y₀)
    Eigen::Vector2d result = R.inverse() * (vec_point - origin);
    return result;
}
Matrix3d PreProcess::RotateMatrix(pair<int, int> point, Matrix3d matrix) {
    Matrix3d Q_zero;
    Vector2d origin = GetOrigin(point);

    Vector2d Deriv(diff([this](double x, double y) { return func(x, y); }, origin(0), origin(1), true),
                   diff([this](double x, double y) { return func(x, y); }, origin(0), origin(1), false));

    Vector2d normalized = normalize(Deriv);
    double f_x = diff([this](double x, double y) { return func(x, y); }, origin(0), origin(1), true);
    double f_y = diff([this](double x, double y) { return func(x, y); }, origin(0), origin(1), false);
    double sinus = f_y / sqrt(f_x * f_x + f_y * f_y);
    double cosinus = f_x / sqrt(f_x * f_x + f_y * f_y);

    Q_zero << cosinus, sinus, 0,
            -sinus, cosinus, 0,
            0, 0, 1;
    return Q_zero * matrix * Q_zero.inverse();
}
vector<Matrix3d> PreProcess::GetDefaultQ (pair<int, int> point) {
    double rho_temp, k_temp, c_temp;
    const double eps = 1e-12;
    rho_temp = rho_minus / rho_plus;
    k_temp = k_minus / k_plus;
    c_temp = c_minus / c_plus;
    if (func(get_x(point.first), get_y(point.second)) > 0.0) {
        rho_temp = 1 / rho_temp;
        k_temp = 1 / k_temp;
        c_temp = 1 / c_temp;
    }
    Vector3d q1_diag (1, rho_temp, 1);
    Vector3d q2_diag (k_temp, 1,  1 / rho_temp);

    Matrix3d Zero1, Zero2;
    Zero1 << 0, 1, 0,
        0, 0, 0,
        0, 0, 0;

    Zero2 << 0, 1, 0,
        -1, 0, 0,
        0, 0, 0;

    Matrix3d q1 = q1_diag.asDiagonal();
    Matrix3d q2 = q2_diag.asDiagonal();
    Matrix3d q3 = rho_temp * (c_temp * c_temp - 1) * Zero1;
    Matrix3d q4 = Zero2;

    vector<Matrix3d> result {q1, q2, q3, q4};
    return result;
}
Matrix3d PreProcess::OppositeQ(int i, int l, pair<int, int> point) {
    Vector2d new_coord = GetRotationalCoord(l, point);
    double xi = new_coord(0);
    const double eps = 1e-12;
    double eta = new_coord(1);
    double c_temp = c_minus / c_plus;
    double t = 1;
    if (func(get_x(point.first), get_y(point.second)) > 0.0) {
        c_temp = 1 / c_temp;
        t = -1;
    }
    Vector2d origin = GetOrigin(point);

    double f_x = diff([this](double x, double y) { return func(x, y); }, origin(0), origin(1), true);
    double f_y = diff([this](double x, double y) { return func(x, y); }, origin(0), origin(1), false);
    double f_xx = diff([this, &f_x](double x, double y) { return diff([this](double x, double y) { return func(x, y); }, x, y, true);}, origin(0), origin(1), true);
    double f_xy = diff([this, &f_y](double x, double y) { return diff([this](double x, double y) { return func(x, y); }, x, y, true);}, origin(0), origin(1), false);
    double f_yx = diff([this, &f_x](double x, double y) { return diff([this](double x, double y) { return func(x, y); }, x, y, false);}, origin(0), origin(1), true);
    double f_yy = diff([this, &f_y](double x, double y) { return diff([this](double x, double y) { return func(x, y); }, x, y, false);}, origin(0), origin(1), false);
    Matrix2d R;
    double sinus = f_y / sqrt(f_x * f_x + f_y * f_y);
    double cosinus = f_x / sqrt(f_x * f_x + f_y * f_y);
    R << cosinus, -sinus,
        sinus, cosinus;

    Vector2d Deriv1 (f_x, f_y);

    Matrix2d R_inv = R.inverse();
    Matrix2d Deriv2;
    Deriv2 << f_xx, f_xy,
                f_yx, f_yy;

    Vector2d normalized = normalize(Deriv1);

    Vector2d new_deriv1 = R_inv * Deriv1;
    Matrix2d new_deriv2 = R_inv * Deriv2 * R;
    // std::cout << "deriv 1" <<  new_deriv1 << std::endl;
    // std::cout << "deriv 2" << new_deriv2 << std::endl;

    double denom = new_deriv1(0);
    double num = new_deriv2(1, 1);
    // std::cout << "num " << num << "\ndenom " << denom << std::endl;
    double new_curvature_value = num / denom;

    double old_curvature_value = curvature(origin(0), origin(1));
    std::cout << "Curv: " << new_curvature_value << "\nOld " << old_curvature_value << std::endl;
    // double curvature_value = 1 / (0.2 + h / 2);
    double curvature_value = new_curvature_value;

    vector<Matrix3d> matrices_default = GetDefaultQ(point);
    Matrix3d q1 = matrices_default[0];
    Matrix3d q2 = (xi / h) * matrices_default[1] + 0.5 * (1 / (h * h)) * curvature_value * (xi * xi * Matrix3d::Identity() - eta * eta * Matrix3d::Identity() + 2 * xi * eta * matrices_default[3]) * (matrices_default[1] - matrices_default[0]);
    Matrix3d q3 = (eta / h) * matrices_default[0] + (xi / h) * matrices_default[2] + 0.5 / (h * h) * curvature_value * (xi * xi * Matrix3d::Identity() - eta * eta * Matrix3d::Identity() + 2 * xi * eta * matrices_default[3]) * matrices_default[2];
    Matrix3d q4 = (xi / h) * (xi / h) * c_temp * c_temp * matrices_default[0];
    Matrix3d q5 = 2 * xi * eta / (h * h) * matrices_default[1];
    Matrix3d q6 = (eta / h) * (eta / h) * matrices_default[0] + 2 * xi * eta / (h * h) * matrices_default[2] + (xi / h) * (xi / h) * (c_temp * c_temp - 1) * matrices_default[0];

    vector<Matrix3d> matrices = {q1, q2, q3, q4, q5, q6};
    return matrices[i];
}
Matrix3d PreProcess::BesideQ(int i, int l, pair<int, int> point) {
    Vector2d new_coord = GetRotationalCoord(l, point);
    double xi = new_coord(0);
    double eta = new_coord(1);
    Matrix3d q1 = Matrix3d::Identity();
    Matrix3d q2 = (xi / h) * Matrix3d::Identity();
    Matrix3d q3 = (eta / h) * Matrix3d::Identity();
    Matrix3d q4 = (xi * xi / (h * h)) * Matrix3d::Identity();
    Matrix3d q5 = 2 * xi * eta / (h * h) * Matrix3d::Identity();
    Matrix3d q6 = eta * eta / (h * h) * Matrix3d::Identity();
    // Matrix3d q1 = Matrix3d::Identity();
    // Matrix3d q2 = xi * Matrix3d::Identity();
    // Matrix3d q3 = eta * Matrix3d::Identity();
    // Matrix3d q4 = xi * xi * Matrix3d::Identity();
    // Matrix3d q5 = 2 * xi * eta * Matrix3d::Identity();
    // Matrix3d q6 = eta * eta * Matrix3d::Identity();

    vector<Matrix3d> matrices = {q1, q2, q3, q4, q5, q6};
    return matrices[i];
}
Matrix3d PreProcess::GetQmatrix(int i, int l, pair<int, int> point) {
    pair<int, int> new_point = GetPoint(l, point);
    if (is_opposite(point.first, point.second, new_point.first, new_point.second)) {
        return OppositeQ(i, l, point);
    }
    else {
        return BesideQ(i, l, point);
    }
}
Matrix3d PreProcess::GetFmatrix(int i, pair<int, int> point) {
    Matrix3d _A = A_minus;
    Matrix3d _B = B_minus;
    double _c = c_minus;

    Vector2d new_coord = GetRotationalCoord(1, point);
    double xi = new_coord(0);
    double eta = new_coord(1);

    if (func(get_x(point.first), get_y(point.second)) > 0.0) {
        _A = A_plus;
        _B = B_plus;
        _c = c_plus;
    }
    Matrix3d f1 = Matrix3d::Zero();
    Matrix3d f2 = -_A;
    Matrix3d f3 = -_B;
    Matrix3d f4 = (k / h) * _c * _c * Matrix3d::Identity() - 2 * xi / h * _A;
    Matrix3d f5 = -2 / h * (eta * _A + xi * _B);
    Matrix3d f6 = (k / h) * _c * _c * Matrix3d::Identity() - 2 / h * eta * _B;

    vector<Matrix3d> matrices = {f1, f2, f3, f4, f5, f6};
    return matrices[i];
}
vector<Matrix3d> PreProcess::CalcGammaMatrices(pair<int, int> point) {
    Matrix<double, 18, 18> Q;
    Eigen::Matrix<double, 18, 3> F;
    Q.setZero();
    F.setZero();

    for (int i = 0; i < 6; ++i) {
        for (int j = 0; j < 6; ++j) {
            Q.block<3, 3>(3 * i, 3 * j) = GetQmatrix(i, j, point).transpose();
        }
    }
    for (int i = 0; i < 6; ++i) {
        F.block<3, 3>(3 * i, 0) = GetFmatrix(i, point).transpose();
    }
    vector<Matrix3d> block_matrices;
    Matrix<double, 18, 3> G = Q.colPivHouseholderQr().solve(F);

    for (int i = 0; i < 18; i += 3) {
        Matrix3d block = G.block<3, 3>(i, 0);
        Matrix3d gamma = RotateMatrix(point, block.transpose());
        block_matrices.push_back(gamma);
        // block_matrices.push_back(block.transpose());
    }
    return block_matrices;
}
PreProcess::PreProcess(InitValues init) : Curve(init) {
    A_minus = init.A_minus;
    A_plus = init.A_plus;
    B_minus = init.B_minus;
    B_plus = init.B_plus;
    rho_minus = init.rho_minus;
    rho_plus = init.rho_plus;
    c_minus = init.c_minus;
    c_plus = init.c_plus;
    k_minus = init.k_minus;
    k_plus = init.k_plus;
    Solve();
}
void PreProcess::Solve() {
    for (const auto& point: GetIrregularPoints()) {
        vector<Matrix3d> gammas = CalcGammaMatrices(point);
        gamma_matrices[point] = gammas;
    }
}
