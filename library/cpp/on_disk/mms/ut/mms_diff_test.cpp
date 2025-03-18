#include <library/cpp/on_disk/mms/cast.h>
#include <library/cpp/on_disk/mms/declare_fields.h>
#include <library/cpp/on_disk/mms/map.h>
#include <library/cpp/on_disk/mms/set.h>
#include <library/cpp/on_disk/mms/string.h>
#include <library/cpp/on_disk/mms/vector.h>
#include <library/cpp/on_disk/mms/writer.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/buffer.h>
#include <util/stream/buffer.h>

namespace {
    template <class T>
    struct TIntCmp {
        bool operator()(const T& lhs, const T& rhs) const {
            return FromString<int>(lhs.data() + 1) < FromString<int>(rhs.data() + 1);
        }
    };

    template <class P>
    struct TTest {
        typedef NMms::TMapType<P, NMms::TStringType<P>, int, TIntCmp> Map;
        typedef NMms::TSetType<P, NMms::TStringType<P>, TIntCmp> Set;
        typedef NMms::TVectorType<P, NMms::TStringType<P>> Vector;

        TTest() {
        }
        TTest(int ii, const TString& ss)
            : i(ii)
            , str(ss)
        {
        }

        int i;
        NMms::TStringType<P> str;
        Map m;
        Set s;
        Vector v;

        MMS_DECLARE_FIELDS(i, str, m, s, v);
    };

    TString GenString(int id) {
        return "%" + ToString(10 * id + 13);
    }

    int GenInt(int id) {
        return 13 * id + 17;
    }

    typedef NMms::TVectorType<NMms::TMmapped, TTest<NMms::TMmapped>> MTests;

    void CheckTests(const MTests& t2) {
        int pos = 0;
        for (MTests::const_iterator i = t2.begin(), ie = t2.end();
             i != ie; ++i, ++pos) {
            UNIT_ASSERT_EQUAL(i->i, GenInt(pos));
            UNIT_ASSERT_EQUAL(i->str[0], i->str[0]);
            UNIT_ASSERT_EQUAL(i->str, i->str);
            UNIT_ASSERT(!(i->str > i->str));
            UNIT_ASSERT(!(i->str < i->str));
            UNIT_ASSERT((i->str >= i->str));
            UNIT_ASSERT((i->str <= i->str));
            UNIT_ASSERT(!(i->str != i->str));

            UNIT_ASSERT_EQUAL(i->str, GenString(pos));
            UNIT_ASSERT(!(i->str > GenString(pos)));
            UNIT_ASSERT(!(i->str < GenString(pos)));
            UNIT_ASSERT((i->str >= GenString(pos)));
            UNIT_ASSERT((i->str <= GenString(pos)));
            UNIT_ASSERT(!(i->str != GenString(pos)));

            UNIT_ASSERT_EQUAL(GenString(pos), i->str);
            UNIT_ASSERT(!(GenString(pos) > i->str));
            UNIT_ASSERT(!(GenString(pos) < i->str));
            UNIT_ASSERT((GenString(pos) >= i->str));
            UNIT_ASSERT((GenString(pos) <= i->str));
            UNIT_ASSERT(!(GenString(pos) != i->str));

            TTest<NMms::TMmapped>::Vector::const_iterator
                vIt = i->v.begin(),
                vEnd = i->v.end();
            TTest<NMms::TMmapped>::Map::const_iterator
                mIt = i->m.begin(),
                mEnd = i->m.end();
            TTest<NMms::TMmapped>::Set::const_iterator
                sIt = i->s.begin(),
                sEnd = i->s.end();

            UNIT_ASSERT_EQUAL(i->v.size(), 2);
            UNIT_ASSERT_EQUAL(i->m.size(), 2);
            UNIT_ASSERT_EQUAL(i->s.size(), 2);

            int idx = 0;
            for (; vIt != vEnd; ++vIt, ++mIt, ++sIt, ++idx) {
                UNIT_ASSERT(mIt != mEnd);
                UNIT_ASSERT(sIt != sEnd);
                UNIT_ASSERT_EQUAL(*vIt, GenString(pos * 2 + idx));
                UNIT_ASSERT_EQUAL(*sIt, GenString(pos * 2 + idx));
                UNIT_ASSERT_EQUAL(mIt->first, GenString(pos * 2 + idx));
                UNIT_ASSERT_EQUAL(mIt->second, GenInt(pos * 2 + idx));
            }
            UNIT_ASSERT(mIt == mEnd);
            UNIT_ASSERT(sIt == sEnd);

            UNIT_ASSERT_EQUAL(i->m[GenString(pos * 2)], GenInt(pos * 2));
            UNIT_ASSERT_EQUAL(i->m[GenString(pos * 2).c_str()], GenInt(pos * 2));
            UNIT_ASSERT_EQUAL(i->s.count(GenString(pos * 2)), 1);
            UNIT_ASSERT_EQUAL(i->s.count(GenString(pos * 2 + 1)), 1);
            UNIT_ASSERT_EQUAL(i->s.count(GenString(pos * 2 + 2)), 0);
        }
    }

}

template <class A, class B>
void Out(IOutputStream& out, const std::pair<A, B>& value) {
    out << value.Id << ": " << value.Name;
}

Y_UNIT_TEST_SUITE(TMmsDiffTest) {
    Y_UNIT_TEST(DifficultTest) {
        NMms::TVectorType<NMms::TStandalone, TTest<NMms::TStandalone>> v;
        const int count = 20;
        for (int i = 0; i != count; ++i) {
            v.push_back(TTest<NMms::TStandalone>(GenInt(i), GenString(i)));
            TTest<NMms::TStandalone>& t = v.back();
            t.m.insert(std::make_pair(GenString(i * 2),
                                      GenInt(i * 2)));
            t.m.insert(std::make_pair(GenString(i * 2 + 1),
                                      GenInt(i * 2 + 1)));
            t.s.insert(GenString(i * 2));
            t.s.insert(GenString(i * 2 + 1));
            t.v.push_back(GenString(i * 2));
            t.v.push_back(GenString(i * 2 + 1));
        }

        TBufferOutput out;
        NMms::TWriter w(out);
        const size_t ofs = NMms::SafeWrite(w, v);
        const TBuffer& buf = out.Buffer();

        typedef NMms::TVectorType<NMms::TMmapped, TTest<NMms::TMmapped>> MTests;
        CheckTests(NMms::Cast<MTests>(buf.Data(), buf.Size()));
        CheckTests(*reinterpret_cast<const MTests*>(buf.Data() + ofs));
    }
}
