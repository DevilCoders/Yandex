#include <library/cpp/matrix/matrix.h>

#include <util/string/vector.h>

int main() {
    size_t size = 2;
    NMatrix::TMatrix A(size, size);
    for (size_t i = 0; i < size; ++i) {
        for (size_t j = 0; j < size; ++j) {
            A.Set(i, j, 1.0 / (i + 1) * (j + 1));
        }
        A.Set(i, i, 1.0 / (10 + i));
    }
    Cout << "A:" << Endl;
    A.Output();
    NMatrix::TMatrix B = A.Inverse();
    Cout << "B:" << Endl;
    B.Output();
    NMatrix::TMatrix C = A * B;
    for (size_t i = 0; i < size; ++i) {
        for (size_t j = 0; j < size; ++j) {
            if (fabs(C.Get(i, j)) < 1e-12)
                C.Set(i, j, 0);
        }
    }
    Cout << "A * B:" << Endl;
    C.Output();
    NMatrix::TMatrix D = A * A;
    Cout << "A * A" << Endl;
    D.Output();

    for (size_t i = 0; i < 2; ++i) {
        Cout << "iteration " << i << Endl;
        A = A * A;
        A.Output();
    }
    Cout << "A normalized" << Endl;
    for (size_t i = 0; i < size; ++i) {
        double sumRow = 0;
        for (size_t j = 0; j < size; ++j)
            sumRow += A.Get(i, j);
        for (size_t j = 0; j < size; ++j)
            A.Set(i, j, A.Get(i, j) / sumRow);
    }
    for (size_t i = 0; i < 20; ++i) {
        Cout << "iteration " << i << Endl;
        A.Output();
        A = A * A;
    }
    return 0;
}
