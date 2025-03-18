#include <library/cpp/deprecated/solartrie/indexed_region/idxr_big.h>
#include <library/cpp/deprecated/solartrie/indexed_region/idxr_small.h>
#include <library/cpp/deprecated/solartrie/indexed_region/idxr_misc.h>

#include <library/cpp/codecs/solar_codec.h>
#include <library/cpp/codecs/pfor_codec.h>
#include <library/cpp/codecs/huffman_codec.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/buffer.h>
#include <util/string/util.h>
#include <util/string/hex.h>
#include <library/cpp/string_utils/relaxed_escaper/relaxed_escaper.h>

class TIndexedRegionTest: public TTestBase {
    UNIT_TEST_SUITE(TIndexedRegionTest);
    UNIT_TEST(TestBigIndexedRegion)
    UNIT_TEST(TestSmallIndexedRegion)
    UNIT_TEST_SUITE_END();

private:
    void TestBigIndexedRegion() {
        using namespace NIndexedRegion;

        TVector<TBuffer> d;

        for (ui32 i = 0; i < 256; ++i) {
            d.emplace_back();

            for (i32 j = i; j >= 0; --j) {
                d.back().Append((ui8)j);
            }
        }

        TBuffer buff;
        {
            TBigIndexedRegion v;

            for (const auto& b : d) {
                v.PushBack(TStringBuf{b.data(), b.size()});
            }

            for (size_t i = 0; i < d.size(); ++i) {
                const auto& dd = d[i];
                UNIT_ASSERT_VALUES_EQUAL(v[i], TStringBuf(dd.data(), dd.size()));
            }

            UNIT_ASSERT_VALUES_EQUAL(v.Size(), d.size());
            v.Commit();
            UNIT_ASSERT_VALUES_EQUAL(v.Size(), d.size());
            v.Encode(buff);
        }

        {
            TBigIndexedRegion v(TStringBuf{buff.data(), buff.size()});

            UNIT_ASSERT_VALUES_EQUAL(v.Size(), d.size());

            for (size_t i = 0; i < d.size(); ++i) {
                const auto& dd = d[i];
                UNIT_ASSERT_VALUES_EQUAL(v[i], TStringBuf(dd.data(), dd.size()));
            }

            for (TBigIndexedRegion::TConstIterator it = v.Begin(); it != v.End(); ++it) {
                const auto& dd = d[it - v.Begin()];
                UNIT_ASSERT_EQUAL_C(*it, TStringBuf(dd.data(), dd.size()), ToString(it - v.Begin()));
            }
        }
    }

    template <typename TRegion>
    void DoTestSmallIndexedRegion(const TVector<TBuffer>& d, NCodecs::TCodecPtr ic, NCodecs::TCodecPtr bc) {
        using namespace NIndexedRegion;
        TBuffer buff;
        {
            TRegion v(ic, bc);

            for (auto it = d.begin(); it != d.end(); ++it) {
                UNIT_ASSERT_VALUES_EQUAL(v.Size(), (size_t)(it - d.begin()));
                v.PushBack(TStringBuf{it->data(), it->size()});
            }

            for (size_t i = 0; i < d.size(); ++i) {
                const auto& dd = d[i];
                UNIT_ASSERT_VALUES_EQUAL(v[i], TStringBuf(dd.data(), dd.size()));
            }

            UNIT_ASSERT_VALUES_EQUAL(v.Size(), d.size());
            v.Commit();
            UNIT_ASSERT_VALUES_EQUAL(v.Size(), d.size());
            v.Encode(buff);
        }
        {
            TRegion v(ic, bc);
            v.Decode(TStringBuf{buff.data(), buff.size()});

            UNIT_ASSERT_VALUES_EQUAL(v.Size(), d.size());

            for (typename TRegion::TConstIterator it = v.Begin(); it != v.End(); ++it) {
                const auto& dd = d[it - v.Begin()];
                UNIT_ASSERT_EQUAL_C(*it, TStringBuf(dd.data(), dd.size()), ToString(it - v.Begin()));
            }

            for (size_t i = 0; i < d.size(); ++i) {
                const auto& dd = d[i];
                UNIT_ASSERT_VALUES_EQUAL(v[i], TStringBuf(dd.data(), dd.size()));
            }

            TRegion vv;
            v.CopyTo(vv);

            UNIT_ASSERT_VALUES_EQUAL(vv.Size(), d.size());

            for (typename TRegion::TConstIterator it = vv.Begin(); it != vv.End(); ++it) {
                const auto& dd = d[it - vv.Begin()];
                UNIT_ASSERT_EQUAL_C(*it, TStringBuf(dd.data(), dd.size()), ToString(it - vv.Begin()));
            }

            for (size_t i = 0; i < d.size(); ++i) {
                const auto& dd = d[i];
                UNIT_ASSERT_VALUES_EQUAL(vv[i], TStringBuf(dd.data(), dd.size()));
            }
        }
    }

    void DoTestCompoundIndexedRegion(const TVector<TBuffer>& d, size_t sz, NCodecs::TCodecPtr ic, NCodecs::TCodecPtr bc, bool ra) {
        using namespace NIndexedRegion;
        TCompoundIndexedRegion v(ra, sz, ic, bc);

        for (auto it = d.begin(); it != d.end(); ++it) {
            UNIT_ASSERT_VALUES_EQUAL(v.Size(), (size_t)(it - d.begin()));
            v.PushBack(TStringBuf(it->data(), it->size()));
        }

        UNIT_ASSERT_VALUES_EQUAL(v.Size(), d.size());
        v.Commit();
        UNIT_ASSERT_VALUES_EQUAL(v.Size(), d.size());

        for (TCompoundIndexedRegion::TConstIterator it = v.Begin(); it != v.End(); ++it) {
            const auto& dd = d[it - v.Begin()];
            UNIT_ASSERT_EQUAL_C(*it, TStringBuf(dd.data(), dd.size()), ToString(it - v.Begin()));
        }

        for (size_t i = 0; i < d.size(); ++i) {
            const auto& dd = d[i];
            UNIT_ASSERT_VALUES_EQUAL_C(v[i], TStringBuf(dd.data(), dd.size()), i);
        }

        // this duplication is intended
        UNIT_ASSERT_VALUES_EQUAL(v[0], TStringBuf(d[0].data(), d[0].size()));
        UNIT_ASSERT_VALUES_EQUAL(v[v.Size() - 1], TStringBuf(d[v.Size() - 1].data(), d[v.Size() - 1].size()));
        UNIT_ASSERT_VALUES_EQUAL(v[0], TStringBuf(d[0].data(), d[0].size()));
        UNIT_ASSERT_VALUES_EQUAL(v[v.Size() - 1], TStringBuf(d[v.Size() - 1].data(), d[v.Size() - 1].size()));
        UNIT_ASSERT_VALUES_EQUAL(v[0], TStringBuf(d[0].data(), d[0].size()));
    }

    template <typename T>
    void AppendTo(TBuffer& b, T t) {
        b.Append((const char*)&t, sizeof(t));
    }

    void TestSmallIndexedRegion() {
        using namespace NIndexedRegion;

        TVector<TBuffer> d;
        TVector<TBuffer> ld32;
        TVector<TBuffer> ld64;

        for (ui32 i = 0; i < 256; ++i) {
            d.emplace_back();
            ld32.emplace_back();
            ld64.emplace_back();

            for (i32 j = i; j >= 0; --j) {
                d.back().Append((ui8)j);
            }

            AppendTo(ld32.back(), (ui32)i);
            AppendTo(ld64.back(), (ui64)i);
        }

        TCodecPtr bc = new TPipelineCodec(new TSolarCodec(512, 8), new THuffmanCodec);
        TCodecPtr lc32 = new TPipelineCodec(new TPForCodec<ui32, true>, new THuffmanCodec);
        TCodecPtr lc64 = new TPipelineCodec(new TPForCodec<ui64, true>, new THuffmanCodec);

        bc->Learn(d.begin(), d.end());
        lc32->Learn(ld32.begin(), ld32.end());
        lc64->Learn(ld64.begin(), ld64.end());

        DoTestSmallIndexedRegion<TSmallIndexedRegion<ui32>>(d, lc32, bc);
        DoTestSmallIndexedRegion<TSmallIndexedRegion<ui32>>(d, lc32, nullptr);
        DoTestSmallIndexedRegion<TSmallIndexedRegion<ui32>>(d, nullptr, lc32);
        DoTestSmallIndexedRegion<TSmallIndexedRegion<ui32>>(d, nullptr, nullptr);
        DoTestSmallIndexedRegion<TSmallIndexedRegion<ui64>>(d, lc64, bc);
        DoTestSmallIndexedRegion<TSmallIndexedRegion<ui64>>(d, lc64, nullptr);
        DoTestSmallIndexedRegion<TSmallIndexedRegion<ui64>>(d, nullptr, bc);
        DoTestSmallIndexedRegion<TSmallIndexedRegion<ui64>>(d, nullptr, nullptr);
        DoTestSmallIndexedRegion<TAdaptiveSmallRegion>(d, lc32, bc);
        DoTestSmallIndexedRegion<TAdaptiveSmallRegion>(d, lc64, bc);
        DoTestSmallIndexedRegion<TAdaptiveSmallRegion>(d, lc32, nullptr);
        DoTestSmallIndexedRegion<TAdaptiveSmallRegion>(d, lc64, nullptr);
        DoTestSmallIndexedRegion<TAdaptiveSmallRegion>(d, nullptr, bc);
        DoTestSmallIndexedRegion<TAdaptiveSmallRegion>(d, nullptr, nullptr);
        DoTestCompoundIndexedRegion(d, 10, ICodec::GetInstance("pfor-delta32-sorted"), ICodec::GetInstance("zlib"), true);
        DoTestCompoundIndexedRegion(d, 10, ICodec::GetInstance("pfor-delta32-sorted"), ICodec::GetInstance("zlib"), false);
    }
};

UNIT_TEST_SUITE_REGISTRATION(TIndexedRegionTest)
