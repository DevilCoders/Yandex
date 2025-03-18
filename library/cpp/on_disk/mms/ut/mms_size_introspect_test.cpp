#include <library/cpp/on_disk/mms/cast.h>
#include <library/cpp/on_disk/mms/declare_fields.h>
#include <library/cpp/on_disk/mms/map.h>
#include <library/cpp/on_disk/mms/maybe.h>
#include <library/cpp/on_disk/mms/set.h>
#include <library/cpp/on_disk/mms/size_introspect.h>
#include <library/cpp/on_disk/mms/string.h>
#include <library/cpp/on_disk/mms/unordered_map.h>
#include <library/cpp/on_disk/mms/unordered_set.h>
#include <library/cpp/on_disk/mms/vector.h>
#include <library/cpp/on_disk/mms/writer.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/buffer.h>
#include <util/stream/buffer.h>

template <class P>
struct TB {
    int A;
    double B;
    NMms::TSetType<P, long> S;
    NMms::TMaybe<P, float> O;
    NMms::TMaybe<P, float> O2;
    NMms::TUnorderedMap<P, float, NMms::TUnorderedSet<P, NMms::TStringType<P>>> U;

    MMS_DECLARE_FIELDS(A, B, S, O, O2, U);
};

Y_UNIT_TEST_SUITE(TMmsSizeIntrospectTest) {
    Y_UNIT_TEST(SizeTest) {
        TB<NMms::TStandalone> b;

        b.B = 1.0;
        b.S.insert(1);
        b.S.insert(3);
        b.S.insert(5);

        b.O = 3.14;

        b.U[1].insert(NMms::TStringType<NMms::TStandalone>("yes"));
        b.U[1].insert("this");
        b.U[2].insert("is");
        b.U[3].insert("dog");

        TBufferOutput out;
        NMms::TWriter w(out);
        const size_t ofs = NMms::SafeWrite(w, b);

        const TBuffer& buf = out.Buffer();

        const TB<NMms::TMmapped>* constB = reinterpret_cast<const TB<NMms::TMmapped>*>(buf.Data() + ofs);

        auto report = NMms::IntrospectSize<TB<NMms::TMmapped>>(constB);
        UNIT_ASSERT_EQUAL(report.Size(), buf.Size());
        // report.PrettyPrint(&Cerr);
    }
}
