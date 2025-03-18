#include "tools.h"

#include <library/cpp/on_disk/mms/set.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/algorithm.h>

namespace {
    template <class T1, class T2, class Cmp1, template <class> class Cmp2>
    bool operator==(const TSet<T1, Cmp1>& l,
                    const NMms::TSetType<NMms::TMmapped, T2, Cmp2>& r) {
        // std::equal does not find this operator== implementation due to ADL,
        // so we compare the sets directly
        auto it = l.begin();
        auto jt = r.begin();
        while (it != l.end() && jt != r.end() && *it == *jt) {
            ++it;
            ++jt;
        }
        return (it == l.end() && jt == r.end());
    }
} //namespace

Y_UNIT_TEST_SUITE(TMmsSetTest) {
    namespace {
        template <class T>
        void CheckPrimitiveSet(TMmsObjects& objs, const TSet<T>& v) {
            UNIT_ASSERT_EQUAL(v, objs.MakeMmappedSet(v));
        }
    }

    Y_UNIT_TEST(SetPrimitiveTest) {
        TMmsObjects objs;
        const size_t maxSize = 100;
        for (size_t size = 0; size <= maxSize; ++size) {
            CheckPrimitiveSet(objs, GenSet<int>(12, size));
            CheckPrimitiveSet(objs, GenSet<char>(12, size));
            CheckPrimitiveSet(objs, GenSet<bool>(12, size));
            CheckPrimitiveSet(objs, GenSet<size_t>(12, size));
            CheckPrimitiveSet(objs, GenSet<ui16>(12, size));
            CheckPrimitiveSet(objs, GenSet<size_t>(12, size));
        }
        UNIT_ASSERT_EQUAL((TSet<int>()), (NMms::TSetType<NMms::TMmapped, int>()));
    }

    template <class T>
    void CheckPrimitiveSetAlign(const TSet<T>& s) {
        TBufferOutput out;
        NMms::TWriter w(out);
        NMms::TSetType<NMms::TStandalone, T> standaloneSet(s.begin(), s.end());
        size_t ofs = NMms::UnsafeWrite(w, standaloneSet);
        NMms::TSetType<NMms::TMmapped, T> mappedSet =
            *reinterpret_cast<const NMms::TSetType<NMms::TMmapped, T>*>(out.Buffer().Data() + ofs);

        UNIT_ASSERT(ofs % sizeof(void*) == 0);
        UNIT_ASSERT(IsAligned(mappedSet));
        UNIT_ASSERT_EQUAL(w.pos(),
                          AlignedSize(sizeof(T) * s.size()) + 2 * sizeof(size_t) // reference size
        );
    }

    Y_UNIT_TEST(AlignSetTest) {
        const size_t maxSize = 100;
        for (size_t size = 0; size <= maxSize; ++size) {
            CheckPrimitiveSetAlign(GenSet<int>(12, size));
            CheckPrimitiveSetAlign(GenSet<char>(12, size));
            CheckPrimitiveSetAlign(GenSet<bool>(12, size));
            CheckPrimitiveSetAlign(GenSet<size_t>(12, size));
            CheckPrimitiveSetAlign(GenSet<ui16>(12, size));
            CheckPrimitiveSetAlign(GenSet<size_t>(12, size));
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

        template <class Matrix, class Vector>
        Matrix BuildMatrix(size_t size) {
            Matrix matrix;
            for (size_t i = 0; i < size; ++i) {
                Vector TVectorType;
                for (size_t j = 0; j < size; ++j) {
                    TVectorType.insert(BuildElement(i, j, size));
                }
                matrix.insert(TVectorType);
            }
            return matrix;
        }

    }

    Y_UNIT_TEST(AlignInnerSetTest) {
        typedef TSet<TString> StdVector;
        typedef TSet<StdVector, LexicographicalCompare<StdVector>> StdMatrix;

        typedef NMms::TStringType<NMms::TStandalone> StandaloneString;
        typedef NMms::TSetType<NMms::TStandalone, StandaloneString> StandaloneVector;
        typedef NMms::TSetType<NMms::TStandalone, StandaloneVector, LexicographicalCompare> StandaloneMatrix;

        typedef NMms::TStringType<NMms::TMmapped> MmappedString;
        typedef NMms::TSetType<NMms::TMmapped, MmappedString> MmappedVector;
        typedef NMms::TSetType<NMms::TMmapped, MmappedVector, LexicographicalCompare> MmappedMatrix;

        const size_t size = 26;

        auto stdMatrix = BuildMatrix<StdMatrix, StdVector>(size);
        auto standaloneMatrix = BuildMatrix<StandaloneMatrix, StandaloneVector>(size);

        TBufferOutput out;
        size_t ofs = NMms::UnsafeWrite(out, standaloneMatrix);
        auto mmappedMatrix = *reinterpret_cast<const MmappedMatrix*>(out.Buffer().Data() + ofs);

        UNIT_ASSERT(ofs % sizeof(void*) == 0);
        UNIT_ASSERT(IsAligned(mmappedMatrix));

        MmappedMatrix::const_iterator it = mmappedMatrix.begin();
        for (size_t i = 0; i < size; ++i, ++it) {
            UNIT_ASSERT(it != mmappedMatrix.end());
            MmappedVector::const_iterator innerIt = it->begin();
            for (size_t j = 0; j < size; ++j, ++innerIt) {
                UNIT_ASSERT(innerIt != it->end());
                UNIT_ASSERT(it->find(*innerIt) == innerIt);
                UNIT_ASSERT_EQUAL(*innerIt, BuildElement(i, j, size));
            }
            UNIT_ASSERT(innerIt == it->end());
        }
        UNIT_ASSERT(it == mmappedMatrix.end());
        UNIT_ASSERT(stdMatrix == mmappedMatrix);
    }
}
