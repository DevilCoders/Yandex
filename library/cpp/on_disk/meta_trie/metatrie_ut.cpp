#include "metatrie.h"
#include "index_digest.h"
#include "subtrie_compacttrie.h"
#include "subtrie_solartrie.h"
#include "subtrie_codectrie.h"
#include "simple_builder.h"

#include <library/cpp/testing/unittest/registar.h>

class TMetatrieTest: public TTestBase {
    UNIT_TEST_SUITE(TMetatrieTest)
    UNIT_TEST(TestComptrie)
    UNIT_TEST(TestCodectrie)
    UNIT_TEST(TestSolartrie)
    UNIT_TEST_SUITE_END();

private:
    typedef TVector<TStringBuf> TStringBufs;

    struct TKeyIdx {
        TString Meta;
        TStringBuf Key;
        TStringBuf Val;

        TKeyIdx(const TString& m, TStringBuf k, TStringBuf v)
            : Meta(m)
            , Key(k)
            , Val(v)
        {
        }

        friend bool operator<(const TKeyIdx& a, const TKeyIdx& b) {
            return a.Meta < b.Meta || a.Meta == b.Meta && a.Key < b.Key;
        }

        bool SameTrie(const TKeyIdx* prev) const {
            return prev->Meta == Meta;
        }

        TString ToString() const {
            return Sprintf("IDX %s->%s : %s", Key.data(), Val.data(), Meta.data());
        }
    };

    struct TDataHolder {
        typedef TVector<std::pair<TString, TString>> TItems;
        TItems Data;

        TDataHolder() {
        }

        void Add(const TString& a) {
            Add(a, a);
        }

        void Add(const TString& a, const TString& b) {
            Data.push_back(std::make_pair(a, b));
        }

        TStringBuf GetKey(ui32 i) const {
            return Data[i].first;
        }

        TStringBuf GetVal(ui32 i) const {
            return Data[i].second;
        }

        ui32 Size() const {
            return Data.size();
        }
    };

    TBlob BuildTrie(const NMetatrie::TMetatrieConf& conf, const TDataHolder& dh) {
        using namespace NMetatrie;
        TBufferOutput out;
        {
            TIndexBuilderPtr ib = CreateIndexBuilder(conf, Min<ui32>(dh.Size(), 10));
            TMetatrieBuilder builder(out, ib);

            TVector<TKeyIdx> sorter;

            for (ui32 i = 0, sz = dh.Size(); i < sz; ++i) {
                TStringBuf k = dh.GetKey(i);
                TStringBuf v = dh.GetVal(i);
                sorter.push_back(TKeyIdx(ib->GenerateMetaKey(k), k, v));
            }

            Sort(sorter.begin(), sorter.end());

            const TKeyIdx* last = sorter.begin();
            TSubtrieBuilderPtr bb = CreateSubtrieBuilder(conf);
            for (TVector<TKeyIdx>::const_iterator it = sorter.begin(); it != sorter.end(); ++it) {
                if (!it->SameTrie(last)) {
                    builder.AddSubtrie(*bb);
                    bb = CreateSubtrieBuilder(conf);
                }

                last = it;
                bb->Add(it->Key, it->Val);
            }

            if (!bb->Empty())
                builder.AddSubtrie(*bb);
        }

        return TBlob::FromBuffer(out.Buffer());
    }

    TBlob BuildTrieSimple(const NMetatrie::TMetatrieConf& conf, const TDataHolder& dh) {
        using namespace NMetatrie;
        TBufferOutput out;
        {
            TMetatrieBuilderSimple builder(conf);

            TVector<TKeyIdx> sorter;

            for (ui32 i = 0, sz = dh.Size(); i < sz; ++i) {
                TStringBuf k = dh.GetKey(i);
                TStringBuf v = dh.GetVal(i);
                sorter.push_back(TKeyIdx("", k, v));
            }

            Sort(sorter.begin(), sorter.end());

            for (const auto& it : sorter) {
                builder.Add(it.Key, it.Val);
            }

            builder.Finish(out);
        }

        return TBlob::FromBuffer(out.Buffer());
    }

    void DoTestTrie(const TDataHolder& str, const TStringBufs& exc, TBlob b) {
        using namespace NMetatrie;
        TMetatrie t;
        t.Init(b);
        TVal val1;

        TMetatrie::TConstIterator it = t.Iterator();
        while (it.Next()) {
            TKey key;
            TVal val2;
            UNIT_ASSERT(it.Current(key, val2));
        }

        for (ui32 i = 0, sz = str.Size(); i < sz; ++i) {
            TStringBuf k = str.GetKey(i);
            TStringBuf v = str.GetVal(i);
            UNIT_ASSERT_C(t.Get(k, val1), k);
            UNIT_ASSERT_VALUES_EQUAL_C(v, val1.Get(), k);
        }

        for (ui32 i = 0; i < exc.size(); ++i) {
            UNIT_ASSERT_C(!t.Get(exc[i], val1), exc[i]);
        }
    }

    void TestTrie(const TDataHolder& str, const TStringBufs& exc, const NMetatrie::TMetatrieConf& ibt) {
        using namespace NMetatrie;
        DoTestTrie(str, exc, BuildTrie(ibt, str));
        //        DoTestTrie(str, exc, BuildTrieSimple(ibt, str));
    }

    void TestTrie(const NMetatrie::TMetatrieConf& ibt) {
        using namespace NMetatrie;
        {
            TDataHolder t1;

            TStringBufs x1;
            x1.push_back("");

            TestTrie(t1, x1, ibt);
        }

        {
            TDataHolder t1;
            t1.Add("", "test");

            TStringBufs x1;

            TestTrie(t1, x1, ibt);
        }

        {
            TDataHolder t1;
            t1.Add("key1");
            t1.Add("key3");

            TStringBufs x1;
            x1.push_back("");
            x1.push_back("key0");
            x1.push_back("key2");
            x1.push_back("key4");

            TestTrie(t1, x1, ibt);
        }

        {
            TDataHolder t1;
            t1.Add("key1");
            t1.Add("key2");
            t1.Add("key4");
            t1.Add("key5");

            TStringBufs x1;
            x1.push_back("");
            x1.push_back("key0");
            x1.push_back("key3");
            x1.push_back("key6");

            TestTrie(t1, x1, ibt);
        }

        {
            TDataHolder t1;
            t1.Add("key1");
            t1.Add("key2");
            t1.Add("key3");
            t1.Add("key5");
            t1.Add("key6");
            t1.Add("key7");

            TStringBufs x1;
            x1.push_back("");
            x1.push_back("key0");
            x1.push_back("key4");
            x1.push_back("key8");

            TestTrie(t1, x1, ibt);
        }
    }

    void DoTestTrieLong(const TDataHolder& data, TBlob b) {
        using namespace NMetatrie;
        TMetatrie t;
        t.Init(b);
        THashMap<TStringBuf, TStringBuf> dict;

        for (ui32 i = 0, sz = data.Size(); i < sz; ++i) {
            dict[data.GetKey(i)] = data.GetVal(i);
        }

        TMetatrie::TConstIterator it = t.Iterator();
        while (it.Next()) {
            TKey key;
            TVal val;

            UNIT_ASSERT(it.Current(key, val));
            UNIT_ASSERT(dict.contains(key.Get()));
        }

        TVal val;
        for (ui32 i = 0, sz = data.Size(); i < sz; ++i) {
            TStringBuf k = data.GetKey(i);
            TStringBuf v = data.GetVal(i);
            UNIT_ASSERT_C(t.Get(k, val), k);
            UNIT_ASSERT_VALUES_EQUAL_C(v, val.Get(), k);
        }
    }

    void TestTrieLong(const NMetatrie::TMetatrieConf& ibt) {
        using namespace NMetatrie;
        TDataHolder data;

        TString key;
        TString val;
        for (ui32 i = 0; i < 10; ++i) {
            for (ui32 j = 1; j < 100; ++j) {
                key.clear();
                val.clear();

                for (ui32 k = 0; k < j; ++k)
                    key.append((char)('0' + i));

                val.append(ToString(i)).append(ToString(j));
                data.Add(key, val);
            }
        }

        DoTestTrieLong(data, BuildTrie(ibt, data));
        //        DoTestTrieLong(data, BuildTrieSimple(ibt, data));
    }

    void TestComptrie() {
        using namespace NMetatrie;
        TestTrie(TMetatrieConf(ST_COMPTRIE, IT_DIGEST));
        TestTrieLong(TMetatrieConf(ST_COMPTRIE, IT_DIGEST));
    }

    void TestCodectrie() {
        using namespace NMetatrie;
        TestTrie(TMetatrieConf(ST_CODECTRIE, IT_DIGEST));
        TestTrieLong(TMetatrieConf(ST_CODECTRIE, IT_DIGEST));
    }

    void TestSolartrie() {
        using namespace NMetatrie;
        TestTrie(TMetatrieConf(ST_SOLARTRIE, IT_DIGEST));
        TestTrieLong(TMetatrieConf(ST_SOLARTRIE, IT_DIGEST));
    }
};

UNIT_TEST_SUITE_REGISTRATION(TMetatrieTest)
