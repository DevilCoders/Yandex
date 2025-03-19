#include <kernel/querydata/tries/qd_trie.h>

#include <kernel/querydata/tries/qd_codectrie.h>
#include <kernel/querydata/tries/qd_comptrie.h>
#include <kernel/querydata/tries/qd_solartrie.h>
#include <kernel/querydata/tries/qd_metatrie.h>
#include <kernel/querydata/tries/qd_coded_blob_trie.h>
#include <kernel/querydata/tries/qd_categ_mask_comptrie.h>

#include <kernel/querydata/idl/querydata_structs.pb.h>

#include <library/cpp/on_disk/coded_blob/coded_blob_simple_builder.h>
#include <library/cpp/on_disk/codec_trie/triebuilder.h>
#include <library/cpp/on_disk/meta_trie/simple_builder.h>
#include <library/cpp/deprecated/solartrie/triebuilder.h>
#include <library/cpp/infected_masks/masks_comptrie.h>

#include <library/cpp/testing/unittest/registar.h>

class TQDTrieTest : public TTestBase {
    UNIT_TEST_SUITE(TQDTrieTest)
        UNIT_TEST(TestMetatrie)
        UNIT_TEST(TestCompTrie)
        UNIT_TEST(TestSolarTrie)
        UNIT_TEST(TestCodecTrie)
        UNIT_TEST(TestCodedBlobTrie)
        UNIT_TEST(TestCategMaskCompTrie)
    UNIT_TEST_SUITE_END();

    template <size_t N>
    void DoTestTrie(const NQueryData::TQDTrie& trie, TString name, const TStringBuf (&keyvals)[N]) {
        TAutoPtr<NQueryData::TQDTrie::TIterator> it = trie.Iterator();

        size_t i = 0;
        TString key, val;
        NQueryData::TQDTrie::TValue d;

        while (it->Next()) {
            UNIT_ASSERT_C(i < N, name);
            it->Current(key, val);
            UNIT_ASSERT_C(trie.Find(key, d), (key + " " + name));
            UNIT_ASSERT_VALUES_EQUAL_C(key, keyvals[i], (key + " " + name));
            UNIT_ASSERT_VALUES_EQUAL_C(val, keyvals[i + 1], (key + " " + name));
            UNIT_ASSERT_VALUES_EQUAL_C(val, d.Get(), (key + " " + name));
            i += 2;
        }

        UNIT_ASSERT_VALUES_EQUAL_C(i, N, name);
    }

    void TestMetatrie() {
        TStringBuf testdata[] = {
              TStringBuf("foo"), TStringBuf("bar")
            , TStringBuf("key"), TStringBuf("val")
        };

        NMetatrie::TMetatrieBuilderSimple b(NMetatrie::ST_COMPTRIE);
        for (size_t i = 0; i < Y_ARRAY_SIZE(testdata); i += 2) {
            b.Add(testdata[i], testdata[i + 1]);
        }

        TBufferOutput bout;
        b.Finish(bout);

        NQueryData::TQDMetatrie trie;
        trie.Init(TBlob::FromBuffer(bout.Buffer()), NQueryData::LM_RAM);
        DoTestTrie(trie, "_metatrie", testdata);
    }

    void TestCompTrie() {
        TStringBuf testdata[] = {
              TStringBuf("foo"), TStringBuf("bar")
            , TStringBuf("key"), TStringBuf("val")
        };

        TCompactTrieBuilder<char, TStringBuf> b;
        for (size_t i = 0; i < Y_ARRAY_SIZE(testdata); i += 2) {
            b.Add(TString{testdata[i]}, testdata[i + 1]);
        }

        TBufferOutput bout;
        b.Save(bout);
        NQueryData::TQDCompTrie trie;
        trie.Init(TBlob::FromBuffer(bout.Buffer()), NQueryData::LM_RAM);
        DoTestTrie(trie, "_comptrie", testdata);
    }

    void TestSolarTrie() {
        TStringBuf testdata[] = {
              TStringBuf("key"), TStringBuf("val")
            , TStringBuf("foo"), TStringBuf("bar")
        };

        NSolarTrie::TSolarTrieBuilder b;
        for (size_t i = 0; i < Y_ARRAY_SIZE(testdata); i += 2) {
            b.Add(testdata[i], testdata[i + 1]);
        }

        NQueryData::TQDSolarTrie trie;
        trie.Init(b.Compact(), NQueryData::LM_RAM);
        DoTestTrie(trie, "_solartrie", testdata);
    }

    void TestCodecTrie() {
        TStringBuf testdata[] = {
              TStringBuf("key"), TStringBuf("val")
            , TStringBuf("foo"), TStringBuf("bar")
        };

        NCodecTrie::TCodecTrieBuilder<TStringBuf> b;
        for (size_t i = 0; i < Y_ARRAY_SIZE(testdata); i += 2) {
            b.Add(testdata[i], testdata[i + 1]);
        }

        NQueryData::TQDCodecTrie trie;
        trie.Init(b.Compact(), NQueryData::LM_RAM);
        DoTestTrie(trie, "_codectrie", testdata);
    }

    void TestCodedBlobTrie() {
        TStringBuf testdata[] = {
              TStringBuf("foo"), TStringBuf("bar")
            , TStringBuf("key"), TStringBuf("val")
        };

        NCodedBlob::TCodedBlobTrieSimpleBuilder b;
        for (size_t i = 0; i < Y_ARRAY_SIZE(testdata); i += 2) {
            b.Add(testdata[i], testdata[i + 1]);
        }

        NQueryData::TQDCodedBlobTrie trie;
        trie.Init(b.Finish(), NQueryData::LM_RAM);
        DoTestTrie(trie, "_coded_blob_trie", testdata);
    }

    void TestCategMaskCompTrie() {
        NQueryData::TRawQueryData qd;

        qd.SetJson("{\"antispam\":\"1\"}");
        TString value = qd.SerializeAsString();

        qd.SetJson("{\"antispam\":\"1\", \"url_key\":\"${url}\", \"sub_url_key\":\"url_key\"}");
        TString value1 = qd.SerializeAsString();

        qd.SetJson("{\"trash\":\"3\"}");
        TString value2 = qd.SerializeAsString();

        qd.SetJson("{\"yellow\":\"2\", \"trash\":\"12\"}");
        TString value3 = qd.SerializeAsString();

        TCompactTrieBuilder<char, TStringBuf> b;
        b.Add("example3.com\tcom.example3/there/*", value);
        b.Add("example4.com\tcom.example4/*", value1);
        b.Add("example5.com\tcom.example5/path/*", value2);
        b.Add("example5.com\tcom.example5/path/more/*", value3);

        TBufferOutput bout;
        b.Save(bout);
        NQueryData::TQDCategMaskCompTrie trie;
        trie.Init(TBlob::FromBuffer(bout.Buffer()), NQueryData::LM_RAM);

        NQueryData::TQDTrie::TValue d;

        trie.Find("example3.com http://sub.sub.example3.com/there/it/is?a=s", d);
        Y_PROTOBUF_SUPPRESS_NODISCARD qd.ParseFromArray(d.Value.data(), d.Value.size());
        UNIT_ASSERT_VALUES_EQUAL(qd.GetJson(), "[{\"antispam\":\"1\"}]");

        trie.Find("example4.com http://sub.sub.example4.com/there", d);
        Y_PROTOBUF_SUPPRESS_NODISCARD qd.ParseFromArray(d.Value.data(), d.Value.size());
        UNIT_ASSERT_VALUES_EQUAL(qd.GetJson(), "[{\"antispam\":\"1\",\"url_key\":\"http%3A%2F%2Fsub.sub.example4.com%2Fthere\"}]");

        trie.Find("example5.com http://sub.example5.com/path/more/path/page.html", d);
        Y_PROTOBUF_SUPPRESS_NODISCARD qd.ParseFromArray(d.Value.data(), d.Value.size());
        UNIT_ASSERT_VALUES_EQUAL(qd.GetJson(), "[{\"trash\":\"3\"},{\"trash\":\"12\",\"yellow\":\"2\"}]");
    }
};

UNIT_TEST_SUITE_REGISTRATION(TQDTrieTest)
