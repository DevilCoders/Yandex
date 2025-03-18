#include "tools.h"

#include <library/cpp/on_disk/mms/copy.h>
#include <library/cpp/on_disk/mms/declare_fields.h>
#include <library/cpp/on_disk/mms/map.h>
#include <library/cpp/on_disk/mms/set.h>
#include <library/cpp/on_disk/mms/string.h>
#include <library/cpp/on_disk/mms/vector.h>
#include <library/cpp/on_disk/mms/writer.h>

#include <library/cpp/testing/unittest/registar.h>

namespace {
    template <class T1, class T2, class T3>
    struct TTraverseSimple {
        T1 v1;
        T2 v2;
        T3 v3;

        MMS_DECLARE_FIELDS(v1, v2, v3);

        typedef TTraverseSimple<T1, T2, T3> MmappedType;
    };

    template <class T1, class T2, class T3>
    bool operator==(const TTraverseSimple<T1, T2, T3>& lhs,
                    const TTraverseSimple<T1, T2, T3>& rhs) {
        return (lhs.v1 == rhs.v1) && (lhs.v2 == rhs.v2) && (lhs.v3 == rhs.v3);
    }

    template <class T1, class T2, class T3>
    TTraverseSimple<T1, T2, T3> GenTraverseSimple(int seed) {
        TTraverseSimple<T1, T2, T3> pod;
        pod.v1 = GenT<T1>()(seed);
        pod.v2 = GenT<T2>()(seed * 57);
        pod.v3 = GenT<T3>()(seed * 1543);
        return pod;
    }

    template <class T1, class T2, class T3>
    void TestTraverseSimple(int seed) {
        typedef TTraverseSimple<T1, T2, T3> TSimple;
        TSimple podOrig = GenTraverseSimple<T1, T2, T3>(seed);
        TBufferOutput out;
        NMms::TWriter w(out);
        size_t ofs = NMms::UnsafeWrite(w, podOrig);
        UNIT_ASSERT_EQUAL(w.pos(), sizeof(TSimple));
        UNIT_ASSERT_EQUAL(ofs, 0);

        const TBuffer buf = out.Buffer();
        TSimple podReaded = *((const TSimple*)(buf.Data() + ofs));
        UNIT_ASSERT(podOrig == podReaded);

        TSimple copied;
        NMms::Copy(podReaded, copied);
        UNIT_ASSERT(podOrig == copied);
    }

}

Y_UNIT_TEST_SUITE(TMmsStructTest) {
    Y_UNIT_TEST(PodTest) {
        for (int seed = -1300; seed <= 157092; seed += 10212) {
            TestTraverseSimple<int, int, int>(seed);
            TestTraverseSimple<char, int, int>(seed);
            TestTraverseSimple<int, char, int>(seed);
            TestTraverseSimple<int, int, char>(seed);
            TestTraverseSimple<int, size_t, int>(seed);
            TestTraverseSimple<char, size_t, int>(seed);
            TestTraverseSimple<int, size_t, char>(seed);
            TestTraverseSimple<char, char, int>(seed);
            TestTraverseSimple<char, char, char>(seed);
            TestTraverseSimple<ui32, ui16, ui32>(seed);
            TestTraverseSimple<ui32, char, ui32>(seed);
            TestTraverseSimple<ui16, char, ui16>(seed);
        }
    }
}
