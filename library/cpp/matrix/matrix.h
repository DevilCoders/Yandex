#pragma once

#include <util/generic/ymath.h>
#include <util/generic/vector.h>
#include <util/string/vector.h>

namespace NMatrix {
    class TMatrix {
    public:
        typedef float ElementType;

    public:
        explicit TMatrix(size_t rows = 1, size_t columns = 1)
            : Rows(rows)
            , Columns(columns)
            , Elements(rows, TVector<ElementType>(columns, 0))
        {
        }

        inline void Set(size_t row, size_t col, ElementType value) {
            Elements.at(row).at(col) = value;
        }

        inline ElementType Get(size_t row, size_t col) const {
            return Elements.at(row).at(col);
        }

        void Clear() {
            for (size_t i = 0; i < Rows; ++i)
                for (size_t j = 0; j < Columns; ++j)
                    Elements[i][j] = 0;
        }

        inline size_t GetRows() const {
            return Rows;
        }

        inline size_t GetColumns() const {
            return Columns;
        }

        TMatrix& operator-=(const TMatrix& matrix) {
            if ((Rows != matrix.Rows) || (Columns != matrix.Columns)) {
                ythrow yexception() << "Matrices have different sizes!";
            }
            for (size_t i = 0; i < Rows; ++i) {
                for (size_t j = 0; j < Columns; ++j) {
                    Elements[i][j] -= matrix.Elements[i][j];
                }
            }
            return *this;
        }

        TMatrix operator-(const TMatrix& matrix) const {
            TMatrix result(*this);
            result -= matrix;
            return result;
        }

        TMatrix& operator+=(const TMatrix& matrix) {
            if ((Rows != matrix.Rows) || (Columns != matrix.Columns)) {
                ythrow yexception() << "Matrices have different sizes!";
            }
            for (size_t i = 0; i < Rows; ++i) {
                for (size_t j = 0; j < Columns; ++j) {
                    Elements[i][j] += matrix.Elements[i][j];
                }
            }
            return *this;
        }

        TMatrix operator+(const TMatrix& matrix) const {
            TMatrix result(*this);
            result -= matrix;
            return result;
        }

        TMatrix& operator*=(ElementType value) {
            for (size_t i = 0; i < Rows; ++i) {
                for (size_t j = 0; j < Columns; ++j) {
                    Elements[i][j] *= value;
                }
            }
            return *this;
        }

        TMatrix operator*(ElementType value) const {
            TMatrix result(*this);
            result *= value;
            return result;
        }

        friend TMatrix operator*(ElementType value, const TMatrix& matrix) {
            return matrix * value;
        }

        TMatrix operator*(const TMatrix& matrix) const {
            if (Columns != matrix.Rows) {
                ythrow yexception() << "Matrices are incompatible!";
            }
            TMatrix result(Rows, matrix.Columns);
            for (size_t i = 0; i < Rows; ++i) {
                for (size_t k = 0; k < Columns; ++k) {
                    for (size_t j = 0; j < matrix.Columns; ++j) {
                        result.Elements[i][j] += Elements[i][k] * matrix.Elements[k][j];
                    }
                }
            }
            return result;
        }

        TMatrix Inverse() const {
            TMatrix gResult(Rows, Columns * 2);
            for (size_t i = 0; i < Rows; ++i) {
                for (size_t j = 0; j < Columns * 2; ++j) {
                    if (j >= Columns) {
                        gResult.Elements[i][j] = 0;
                        if (j - Columns == i)
                            gResult.Elements[i][j] = 1;
                    } else {
                        gResult.Elements[i][j] = Elements[i][j];
                    }
                }
            }
            gResult.Gauss();
            TMatrix result(Rows, Columns);
            for (size_t i = 0; i < Rows; ++i)
                for (size_t j = 0; j < Columns; ++j)
                    result.Elements[i][j] = gResult.Elements[i][j + Columns];
            return result;
        }

        TMatrix Transpose() const {
            TMatrix result(Columns, Rows);
            for (size_t i = 0; i < Rows; ++i)
                for (size_t j = 0; j < Columns; ++j)
                    result.Elements[j][i] = Elements[i][j];
            return result;
        }

        void Output() const {
            for (size_t i = 0; i < Rows; ++i) {
                for (size_t j = 0; j < Columns; ++j) {
                    Cout << Elements[i][j] << "\t";
                }
                Cout << "\n";
            }
        }

        void Normalize() {
            ElementType sum = 0;
            for (size_t i = 0; i < Rows; ++i) {
                for (size_t j = 0; j < Columns; ++j) {
                    sum += Elements[i][j];
                }
            }
            for (size_t i = 0; i < Rows; ++i) {
                for (size_t j = 0; j < Columns; ++j) {
                    Elements[i][j] /= sum;
                }
            }
        }

        void NormalizeBySoftMax(ElementType gamma = 1) {
            ElementType softMaxSum = 0;
            for (size_t i = 0; i < Rows; ++i) {
                for (size_t j = 0; j < Columns; ++j) {
                    softMaxSum += exp(-gamma * Elements[i][j]);
                }
            }
            for (size_t i = 0; i < Rows; ++i) {
                for (size_t j = 0; j < Columns; ++j) {
                    Elements[i][j] = exp(-gamma * Elements[i][j]) / softMaxSum;
                }
            }
        }

        void NormalizeByMinMax() {
            ElementType minElement = Elements[0][0];
            ElementType maxElement = Elements[0][0];
            for (size_t i = 0; i < Rows; ++i) {
                for (size_t j = 0; j < Columns; ++j) {
                    if (minElement < Elements[i][j])
                        minElement = Elements[i][j];
                    if (maxElement > Elements[i][j])
                        maxElement = Elements[i][j];
                }
            }
            for (size_t i = 0; i < Rows; ++i) {
                for (size_t j = 0; j < Columns; ++j) {
                    Elements[i][j] = (Elements[i][j] - minElement) / (maxElement - minElement);
                }
            }
        }

    public:
        static TMatrix GetIdentity(size_t size) {
            TMatrix result(size, size);
            for (size_t i = 0; i < size; ++i) {
                result.Set(i, i, 1);
            }
            return result;
        }

    private:
        ElementType Gauss() {
            size_t iterationsCount = Rows;
            if (Rows > Columns)
                iterationsCount = Columns;
            ElementType determinant = 1;

            for (size_t j = 0; j < iterationsCount; ++j) {
                size_t index = j;
                for (size_t i = j; i < Rows; ++i) {
                    if (fabs(Elements[i][j]) > fabs(Elements[index][j]))
                        index = i;
                }
                if (index != j) {
                    SwapRows(index, j);
                    determinant *= -1;
                }
                determinant *= Elements[j][j];
                MultiplyRow(j, 1.0 / Elements[j][j]);
                for (size_t i = 0; i < Rows; ++i) {
                    if (i != j) {
                        AddRow(j, i, -Elements[i][j]);
                    }
                }
            }
            return determinant;
        }

        void MultiplyRow(size_t row, ElementType value) {
            for (size_t j = 0; j < Columns; ++j) {
                Elements[row][j] *= value;
            }
        }

        void AddRow(size_t fromRow, size_t toRow, ElementType factor = 1.0) {
            for (size_t j = 0; j < Columns; ++j) {
                Elements[toRow][j] += factor * Elements[fromRow][j];
            }
        }

        void SwapRows(size_t firstRow, size_t secondRow) {
            for (size_t j = 0; j < Columns; ++j) {
                ElementType t = Elements[firstRow][j];
                Elements[firstRow][j] = Elements[secondRow][j];
                Elements[secondRow][j] = t;
            }
        }

    private:
        size_t Rows;
        size_t Columns;
        TVector<TVector<ElementType>> Elements;
    };

}
