#include "tools.h"

#include <library/cpp/on_disk/mms/map.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/algorithm.h>

namespace {
    template <class K1, class V1, class K2, class V2, class Cmp1, template <class> class Cmp2>
    bool operator==(const TMap<K1, V1, Cmp1>& lhs,
                    const NMms::TMapType<NMms::TMmapped, K2, V2, Cmp2>& rhs);

    struct TPairComparer {
        template <class F1, class S1, class F2, class S2>
        bool operator()(const std::pair<F1, S1>& l, const std::pair<F2, S2>& r) const {
            return l.first == r.first && l.second == r.second;
        }
    };

    template <class K1, class V1, class K2, class V2, class Cmp1, template <class> class Cmp2>
    bool operator==(const TMap<K1, V1, Cmp1>& lhs,
                    const NMms::TMapType<NMms::TMmapped, K2, V2, Cmp2>& rhs) {
        if (rhs.size() != lhs.size()) {
            return false;
        }

        typedef Cmp2<K2> MCmp;
        typedef typename NMms::TMapType<NMms::TMmapped, K2, V2, Cmp2>::const_iterator MmappedIterator;

        //Check mmapped container is valid
        for (const auto& kv : rhs) {
            UNIT_ASSERT_EQUAL(rhs.count(kv.first), 1);
            UNIT_ASSERT_EQUAL(rhs[kv.first], kv.second);
            MmappedIterator it = rhs.find(kv.first);
            UNIT_ASSERT(!MCmp()(it->first, kv.first));
            UNIT_ASSERT(!MCmp()(kv.first, it->first));
            UNIT_ASSERT(it->second == kv.second);
            MmappedIterator lb = rhs.lower_bound(kv.first);
            MmappedIterator ub = rhs.upper_bound(kv.first);
            UNIT_ASSERT(lb == it);
            UNIT_ASSERT(ub == (it + 1));
            UNIT_ASSERT(std::make_pair(lb, ub) == rhs.equal_range(kv.first));
        }

        return Equal(lhs.begin(), lhs.end(), rhs.begin(), TPairComparer());
    }
}

Y_UNIT_TEST_SUITE(TMmsMapTest) {
    namespace {
        template <class K, class V, class Cmp>
        void CheckPrimitiveMap(TMmsObjects& objs, const TMap<K, V, Cmp>& m) {
            UNIT_ASSERT(m == objs.MakeMmappedMap(m));
        }
    }

    Y_UNIT_TEST(MapPrimitiveTest) {
        TMmsObjects objs;
        const size_t maxSize = 100;
        for (size_t size = 0; size <= maxSize; ++size) {
            CheckPrimitiveMap(objs, GenMap<int, char>(12, size));
            CheckPrimitiveMap(objs, GenMap<char, char>(12, size));
            CheckPrimitiveMap(objs, GenMap<bool, char>(12, size));
            CheckPrimitiveMap(objs, GenMap<size_t, bool>(12, size));
            CheckPrimitiveMap(objs, GenMap<ui16, ui16>(12, size));
            CheckPrimitiveMap(objs, GenMap<size_t, ui16>(12, size));
            CheckPrimitiveMap(objs, GenMap<size_t, size_t>(12, size));
            CheckPrimitiveMap(objs, GenMap<int, int>(12, size));
            CheckPrimitiveMap(objs, GenMap<bool, int>(12, size));
            CheckPrimitiveMap(objs, GenMap<char, int>(12, size));
        }
        UNIT_ASSERT((TMap<int, int>()) == (NMms::TMapType<NMms::TMmapped, int, int>()));
    }

    namespace {
        template <class K, class V, class Cmp>
        void CheckPrimitiveMapAlign(const TMap<K, V, Cmp>& m) {
            TBufferOutput out;
            NMms::TWriter w(out);
            NMms::TMapType<NMms::TStandalone, K, V> standaloneMap(m.begin(), m.end());
            size_t ofs = NMms::UnsafeWrite(w, standaloneMap);
            NMms::TMapType<NMms::TMmapped, K, V> mappedMap =
                *reinterpret_cast<const NMms::TMapType<NMms::TMmapped, K, V>*>(out.Buffer().Data() + ofs);

            UNIT_ASSERT(ofs % sizeof(void*) == 0);
            UNIT_ASSERT(IsAligned(mappedMap));
            UNIT_ASSERT_VALUES_EQUAL(w.pos(),
                                     AlignedSize(sizeof(std::pair<K, V>) * m.size()) + 2 * sizeof(size_t) // reference size
            );
        }
    }

    Y_UNIT_TEST(AlignMapTest) {
        const size_t maxSize = 100;
        for (size_t size = 0; size <= maxSize; ++size) {
            CheckPrimitiveMapAlign(GenMap<int, char>(12, size));
            CheckPrimitiveMapAlign(GenMap<char, char>(12, size));
            CheckPrimitiveMapAlign(GenMap<bool, char>(12, size));
            CheckPrimitiveMapAlign(GenMap<size_t, bool>(12, size));
            CheckPrimitiveMapAlign(GenMap<ui16, ui16>(12, size));
            CheckPrimitiveMapAlign(GenMap<size_t, ui16>(12, size));
            CheckPrimitiveMapAlign(GenMap<size_t, size_t>(12, size));
            CheckPrimitiveMapAlign(GenMap<int, int>(12, size));
            CheckPrimitiveMapAlign(GenMap<bool, int>(12, size));
            CheckPrimitiveMapAlign(GenMap<char, int>(12, size));
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
                Vector vector;
                for (size_t j = 0; j < size; ++j) {
                    vector[BuildElement(i, j, size)] = BuildElement(i, j, size);
                }
                matrix[vector] = BuildElement(i, i, size);
            }
            return matrix;
        }

    }

    Y_UNIT_TEST(AlignInnerMapTest) {
        typedef TMap<TString, TString> StdVector;
        typedef TMap<StdVector, TString, LexicographicalCompare<StdVector>> StdMatrix;

        typedef NMms::TStringType<NMms::TStandalone> StandaloneString;
        typedef NMms::TMapType<NMms::TStandalone, StandaloneString, StandaloneString> StandaloneVector;
        typedef NMms::TMapType<NMms::TStandalone, StandaloneVector, StandaloneString, LexicographicalCompare> StandaloneMatrix;

        typedef NMms::TStringType<NMms::TMmapped> MmappedString;
        typedef NMms::TMapType<NMms::TMmapped, MmappedString, MmappedString> MmappedVector;
        typedef NMms::TMapType<NMms::TMmapped, MmappedVector, MmappedString, LexicographicalCompare> MmappedMatrix;

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
            UNIT_ASSERT_EQUAL(it->second, BuildElement(i, i, size));
            MmappedVector::const_iterator innerIt = it->first.begin();
            for (size_t j = 0; j < size; ++j, ++innerIt) {
                UNIT_ASSERT(innerIt != it->first.end());
                UNIT_ASSERT_EQUAL(it->first[innerIt->first], innerIt->second);
                UNIT_ASSERT_EQUAL(innerIt->first, BuildElement(i, j, size));
                UNIT_ASSERT_EQUAL(innerIt->second, BuildElement(i, j, size));
            }
            UNIT_ASSERT(innerIt == it->first.end());
        }
        UNIT_ASSERT(it == mmappedMatrix.end());
        UNIT_ASSERT(stdMatrix == mmappedMatrix);
    }
}
