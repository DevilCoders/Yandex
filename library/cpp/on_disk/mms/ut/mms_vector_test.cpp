#include "tools.h"

#include <library/cpp/on_disk/mms/vector.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/generic/algorithm.h>

template <class T1, class T2>
bool operator==(const NMms::TVectorType<NMms::TMmapped, T2>& lhs, const TVector<T1>& rhs) {
    return rhs == lhs;
}

template <class T1, class T2>
bool operator==(const TVector<T1>& lhs, const NMms::TVectorType<NMms::TMmapped, T2>& rhs) {
    if (rhs.size() != lhs.size())
        return false;
    for (size_t i = 0; i < lhs.size(); ++i) {
        //test operator []
        if (!(lhs[i] == rhs[i]))
            return false;
    }
    return Equal(lhs.begin(), lhs.end(), rhs.begin()) &&
           Equal(rhs.rbegin(), rhs.rend(), lhs.rbegin());
}

Y_UNIT_TEST_SUITE(TMmsVectorTest) {
    template <class T>
    void CheckPrimitiveVector(const TVector<T>& v) {
        TMmsObjects objs;
        UNIT_ASSERT(v == objs.MakeMmappedVector(v));
    }

    Y_UNIT_TEST(VectorPrimitiveTest) {
        size_t maxSize = 1000;
        for (size_t size = 0; size <= maxSize; ++size) {
            CheckPrimitiveVector(GenVector<int>(12, size));
            CheckPrimitiveVector(GenVector<char>(12, size));
            CheckPrimitiveVector(GenVector<bool>(12, size));
            CheckPrimitiveVector(GenVector<size_t>(12, size));
            CheckPrimitiveVector(GenVector<ui16>(12, size));
            CheckPrimitiveVector(GenVector<size_t>(12, size));
        }
        UNIT_ASSERT((TVector<int>()) == (NMms::TVectorType<NMms::TMmapped, int>()));

        TMmsObjects objs;
        NMms::TVectorType<NMms::TMmapped, int> v = objs.MakeMmappedVector(GenVector<int>(12, 10));

        UNIT_ASSERT_EQUAL(static_cast<size_t>(std::distance(v.begin(), v.end())), v.size());
        UNIT_ASSERT_EQUAL(static_cast<size_t>(std::distance(v.rbegin(), v.rend())), v.size());
    }

    template <class T>
    void CheckPrimitiveVectorAlign(const TVector<T>& v) {
        TBufferOutput out;
        NMms::TWriter w(out);
        NMms::TVectorType<NMms::TStandalone, T> mmsVector(v);
        size_t ofs = NMms::UnsafeWrite(w, mmsVector);
        const TBuffer buf = out.Buffer();
        NMms::TVectorType<NMms::TMmapped, T> mmappedMatrix = *reinterpret_cast<const NMms::TVectorType<NMms::TMmapped, T>*>(buf.Data() + ofs);
        UNIT_ASSERT(ofs % sizeof(void*) == 0);
        UNIT_ASSERT(IsAligned(mmappedMatrix));
        UNIT_ASSERT_EQUAL(w.pos(),
                          AlignedSize(sizeof(T) * v.size()) + 2 * sizeof(size_t) //reference size
        );
    }

    Y_UNIT_TEST(AlignVectorTest) {
        size_t maxSize = 1000;
        for (size_t size = 0; size <= maxSize; ++size) {
            CheckPrimitiveVectorAlign(GenVector<int>(12, size));
            CheckPrimitiveVectorAlign(GenVector<char>(12, size));
            CheckPrimitiveVectorAlign(GenVector<bool>(12, size));
            CheckPrimitiveVectorAlign(GenVector<size_t>(12, size));
            CheckPrimitiveVectorAlign(GenVector<ui16>(12, size));
            CheckPrimitiveVectorAlign(GenVector<size_t>(12, size));
        }
    }

    namespace {
        TString BuildElement(size_t i, size_t j, size_t size) {
            TString result;
            result.reserve(size);
            for (size_t t = 0; t < size; ++t) {
                result.push_back('a' + i + j + t);
            }
            return result;
        }

        template <class Matrix>
        Matrix BuildMatrix(size_t size) {
            Matrix matrix;
            matrix.resize(size);
            for (size_t i = 0; i < size; ++i) {
                matrix[i].resize(size);
                for (size_t j = 0; j < size; ++j) {
                    matrix[i][j] = BuildElement(i, j, size);
                }
            }
            return matrix;
        }

    }

    Y_UNIT_TEST(AlignInnerVectorTest) {
        typedef TVector<TString> StdVector;
        typedef TVector<StdVector> StdMatrix;

        typedef NMms::TVectorType<NMms::TStandalone, NMms::TStringType<NMms::TStandalone>> StandaloneVector;
        typedef NMms::TVectorType<NMms::TStandalone, StandaloneVector> StandaloneMatrix;

        typedef NMms::TVectorType<NMms::TMmapped, NMms::TStringType<NMms::TMmapped>> MmappedVector;
        typedef NMms::TVectorType<NMms::TMmapped, MmappedVector> MmappedMatrix;

        const size_t size = 26;
        StdMatrix stdMatrix = BuildMatrix<StdMatrix>(size);
        StandaloneMatrix standaloneMatrix = BuildMatrix<StandaloneMatrix>(size);

        TBufferOutput out;
        NMms::TWriter w(out);
        size_t ofs = NMms::UnsafeWrite(w, standaloneMatrix);
        const TBuffer buf = out.Buffer();
        MmappedMatrix mmappedMatrix =
            *reinterpret_cast<const MmappedMatrix*>(buf.Data() + ofs);
        UNIT_ASSERT(ofs % sizeof(void*) == 0);
        UNIT_ASSERT(IsAligned(mmappedMatrix));

        MmappedMatrix::const_iterator it = mmappedMatrix.begin();
        for (size_t i = 0; i < size; ++i, ++it) {
            UNIT_ASSERT(it != mmappedMatrix.end());
            MmappedVector::const_iterator innerIt = it->begin();
            for (size_t j = 0; j < size; ++j, ++innerIt) {
                UNIT_ASSERT(innerIt != it->end());
                UNIT_ASSERT_EQUAL(mmappedMatrix[i][j], BuildElement(i, j, size));
                UNIT_ASSERT_EQUAL(it->at(j), BuildElement(i, j, size));
                UNIT_ASSERT_EQUAL(*innerIt, BuildElement(i, j, size));
            }
            UNIT_ASSERT(innerIt == it->end());
        }
        UNIT_ASSERT(it == mmappedMatrix.end());
        UNIT_ASSERT(stdMatrix == mmappedMatrix);
    }

    struct TTripleNotPod {
        TTripleNotPod() {
        }
        TTripleNotPod(double) {
        }
        double x, y;
        char z, t;
        ui64 u, v;

        template <class A>
        void traverseFields(A a) const Y_NO_SANITIZE("undefined") {
            a(x)(y)(z)(t)(u)(v);
        }

        typedef TTripleNotPod MmappedType;
    };

    Y_UNIT_TEST(VectorPairSizeTest) {
        NMms::TVectorType<NMms::TStandalone, TTripleNotPod> a;
        const size_t count = 128;
        a.resize(count);
        TBufferOutput out;
        NMms::Write(out, a);
        size_t dataSize = sizeof(TTripleNotPod) * count;
        dataSize += (sizeof(size_t) - dataSize % sizeof(size_t)) % sizeof(size_t);
        dataSize += 2 * sizeof(size_t); //Reference
        dataSize += sizeof(size_t);     //Version
        UNIT_ASSERT_EQUAL(dataSize, out.Buffer().Size());
    }
}
