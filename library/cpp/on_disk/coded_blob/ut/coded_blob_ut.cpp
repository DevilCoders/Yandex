#include <library/cpp/on_disk/coded_blob/coded_blob_array.h>
#include <library/cpp/on_disk/coded_blob/coded_blob_trie.h>
#include <library/cpp/on_disk/coded_blob/coded_blob_simple_builder.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/random/shuffle.h>

#include <util/string/printf.h>
#include <util/string/vector.h>
#include <util/stream/buffer.h>

class TCodedBlobTest: public NUnitTest::TTestBase {
    UNIT_TEST_SUITE(TCodedBlobTest)
    UNIT_TEST(TestCodedBlob)
    UNIT_TEST(TestCodedBlobArray)
    UNIT_TEST(TestCodedBlobTrie)
    UNIT_TEST_SUITE_END();

    struct TTestData {
        TVector<TString> Data;
        TString Marker;
        TString Codec;
        TString TmpCodec;

        TTestData() {
        }

        TTestData(std::initializer_list<TString> d, const TString& name)
            : Data(d)
            , Marker(name)
        {
        }
    };

    TVector<TTestData> TestData;

    void SetUp() override {
        for (const TString& codec : {"none", "solar-16k:huffman"}) {
            for (const TString& tmpCodec : {"none", "snappy"}) {
                TestData.push_back(TTestData({},
                                             Sprintf("test0/codec=%s/tmp_codec=%s", codec.data(), tmpCodec.data())));
                TestData.push_back(TTestData({},
                                             Sprintf("test1/codec=%s/tmp_codec=%s", codec.data(), tmpCodec.data())));
                TestData.push_back(TTestData({"foo"},
                                             Sprintf("test2/codec=%s/tmp_codec=%s", codec.data(), tmpCodec.data())));
                TestData.push_back(TTestData({"foo", ""},
                                             Sprintf("test3/codec=%s/tmp_codec=%s", codec.data(), tmpCodec.data())));
                TestData.push_back(TTestData({"", "foo"},
                                             Sprintf("test4/codec=%s/tmp_codec=%s", codec.data(), tmpCodec.data())));
                TestData.push_back(TTestData({"foo", "", "bar"},
                                             Sprintf("test5/codec=%s/tmp_codec=%s", codec.data(), tmpCodec.data())));
                TestData.push_back(TTestData({},
                                             Sprintf("test6/codec=%s/tmp_codec=%s", codec.data(), tmpCodec.data())));

                for (ui32 i = 0; i < 100000; ++i) {
                    TestData.back().Data.push_back(Sprintf("%012u", i));
                }

                Shuffle(TestData.back().Data.begin(), TestData.back().Data.end());
            }
        }
    }

    void TestCodedBlob() {
        for (const auto& test : TestData) {
            using namespace NCodedBlob;
            TBufferOutput bout;
            {
                TCodedBlobSimpleBuilder builder(400, test.TmpCodec);
                builder.Init(test.Codec);

                for (ui32 i = 0, sz = test.Data.size(); i < sz; ++i) {
                    builder.Add(test.Data[i]);
                }

                builder.Finish(&bout);
            }

            {
                TCodedBlob blob;
                blob.Init(TBlob::FromBuffer(bout.Buffer()));

                UNIT_ASSERT_VALUES_EQUAL_C(test.Data.size(), blob.Count(), test.Marker);

                TCodedBlob::TOffsetIterator it = blob.OffsetIterator();

                {
                    ui64 idx = 0;
                    while (it.Next()) {
                        UNIT_ASSERT_VALUES_EQUAL_C(test.Data[idx], it.GetCurrentValue(), test.Marker);
                        UNIT_ASSERT_VALUES_EQUAL_C(test.Data[idx], blob.GetByOffset(it.GetCurrentOffset()), test.Marker);
                        ++idx;
                    }

                    UNIT_ASSERT_VALUES_EQUAL_C(test.Data.size(), idx, test.Marker);
                }
                {
                    it = blob.OffsetIterator();
                    ui64 idx = 0;
                    while (it.Next()) {
                        UNIT_ASSERT_VALUES_EQUAL_C(test.Data[idx], it.GetCurrentValue(), test.Marker);
                        UNIT_ASSERT_VALUES_EQUAL_C(test.Data[idx], blob.GetByOffset(it.GetCurrentOffset()), test.Marker);
                        ++idx;
                    }

                    UNIT_ASSERT_VALUES_EQUAL_C(test.Data.size(), idx, test.Marker);
                }
            }
        }
    }

    void TestCodedBlobArray() {
        for (const auto& test : TestData) {
            using namespace NCodedBlob;
            TBufferOutput bout;
            {
                TCodedBlobArraySimpleBuilder builder(400, test.TmpCodec);
                builder.Init(test.Codec);

                for (ui32 i = 0, sz = test.Data.size(); i < sz; ++i) {
                    builder.Add(test.Data[i]);
                }

                builder.Finish(&bout);
            }

            {
                TCodedBlobArray blob;
                blob.Init(TBlob::FromBuffer(bout.Buffer()));

                UNIT_ASSERT_VALUES_EQUAL_C(test.Data.size(), blob.Count(), test.Marker);

                for (ui32 i = 0, sz = test.Data.size(); i < sz; ++i) {
                    UNIT_ASSERT_VALUES_EQUAL_C(test.Data[i], blob.GetByIndex(i), test.Marker);
                }

                {
                    TCodedBlobArray::TOffsetIterator oit = blob.OffsetIterator();
                    TCodedBlobArray::TIndexIterator iit = blob.IndexIterator();
                    {
                        ui64 idx = 0;
                        while (oit.Next() && iit.Next()) {
                            UNIT_ASSERT_VALUES_EQUAL_C(test.Data[idx], oit.GetCurrentValue(), test.Marker);
                            UNIT_ASSERT_VALUES_EQUAL_C(test.Data[idx], iit.GetCurrentValue(), test.Marker);
                            UNIT_ASSERT_VALUES_EQUAL_C(idx, iit.GetCurrentIndex(), test.Marker);
                            UNIT_ASSERT_VALUES_EQUAL_C(test.Data[idx], blob.GetByOffset(oit.GetCurrentOffset()), test.Marker);
                            UNIT_ASSERT_VALUES_EQUAL_C(test.Data[idx], blob.GetByOffset(iit.GetCurrentOffset()), test.Marker);
                            ++idx;
                        }

                        UNIT_ASSERT_VALUES_EQUAL_C(test.Data.size(), idx, test.Marker);
                        UNIT_ASSERT_C(!oit.HasNext(), test.Marker);
                        UNIT_ASSERT_C(!iit.HasNext(), test.Marker);
                    }
                    {
                        oit = blob.OffsetIterator();
                        iit = blob.IndexIterator();
                        ui64 idx = 0;
                        while (oit.Next() && iit.Next()) {
                            UNIT_ASSERT_VALUES_EQUAL_C(test.Data[idx], oit.GetCurrentValue(), test.Marker);
                            UNIT_ASSERT_VALUES_EQUAL_C(test.Data[idx], iit.GetCurrentValue(), test.Marker);
                            UNIT_ASSERT_VALUES_EQUAL_C(idx, iit.GetCurrentIndex(), test.Marker);
                            UNIT_ASSERT_VALUES_EQUAL_C(test.Data[idx], blob.GetByOffset(oit.GetCurrentOffset()), test.Marker);
                            UNIT_ASSERT_VALUES_EQUAL_C(test.Data[idx], blob.GetByOffset(iit.GetCurrentOffset()), test.Marker);
                            ++idx;
                        }

                        UNIT_ASSERT_VALUES_EQUAL_C(test.Data.size(), idx, test.Marker);
                        UNIT_ASSERT_C(!oit.HasNext(), test.Marker);
                        UNIT_ASSERT_C(!iit.HasNext(), test.Marker);
                    }
                }
            }
        }
    }

    struct TTrieTestData : TTestData {
        THashMap<TString, TVector<TString>> Multikeys;
        size_t ElementsCount = 0;
        bool Sorted = false;
        bool Multikey = false;

        TTrieTestData() = default;
        TTrieTestData(const TTestData& d)
            : TTestData(d)
            , ElementsCount(d.Data.size())
        {
        }
    };

    void DoTestCodedBlobTrieIterators(NCodedBlob::TCodedBlobTrie::TOffsetIterator& oit,
                                      NCodedBlob::TCodedBlobTrie::TPrimaryKeyIterator& pit,
                                      const NCodedBlob::TCodedBlobTrie& blob,
                                      const TTrieTestData& test) {
        ui64 idx = 0;
        ui64 cnt = 0;
        while (oit.Next() && pit.Next()) {
            const TString& key = test.Data[idx];
            ++idx;
            ++cnt;

            UNIT_ASSERT_VALUES_EQUAL_C(pit.GetCurrentKey(), key, test.Marker);
            UNIT_ASSERT_VALUES_EQUAL_C(pit.GetCurrentValue(), key, test.Marker);
            UNIT_ASSERT_VALUES_EQUAL_C(blob.GetByOffset(pit.GetCurrentOffset()), key, test.Marker);

            if (test.Sorted) {
                UNIT_ASSERT_VALUES_EQUAL_C(oit.GetCurrentValue(), key, test.Marker);
                UNIT_ASSERT_VALUES_EQUAL_C(blob.GetByOffset(oit.GetCurrentOffset()), key, test.Marker);
            }

            if (test.Multikey) {
                const auto& subkeys = test.Multikeys.at(key);
                if (subkeys.size()) {
                    UNIT_ASSERT_C(oit.Next(), test.Marker); // one value for several keys
                    ui64 offset = 0;
                    for (size_t j = 0; j < subkeys.size(); ++j) {
                        UNIT_ASSERT_C(pit.HasNext(), test.Marker);
                        UNIT_ASSERT_C(pit.Next(), test.Marker);
                        if (!j) {
                            offset = pit.GetCurrentOffset();
                        }
                        UNIT_ASSERT_VALUES_EQUAL_C(pit.GetCurrentOffset(), offset, test.Marker);
                        UNIT_ASSERT_VALUES_EQUAL_C(pit.GetCurrentKey(), subkeys[j], test.Marker);
                        UNIT_ASSERT_VALUES_EQUAL_C(pit.GetCurrentValue(), key, test.Marker);
                        UNIT_ASSERT_VALUES_EQUAL_C(blob.GetByOffset(pit.GetCurrentOffset()), key, test.Marker);
                    }
                    ++cnt;
                }
            }
        }

        UNIT_ASSERT_VALUES_EQUAL_C(idx, test.Data.size(), test.Marker);
        UNIT_ASSERT_VALUES_EQUAL_C(cnt, test.ElementsCount, test.Marker);
        UNIT_ASSERT_C(!oit.HasNext(), test.Marker);
        UNIT_ASSERT_C(!pit.HasNext(), test.Marker);
    }

    void TestCodedBlobTrie() {
        TVector<TTrieTestData> trieTestData;

        int iterTrueFalse[]{0, 1};

        for (int sorted : iterTrueFalse) {
            for (int multikey : iterTrueFalse) {
                for (const auto& test : TestData) {
                    trieTestData.push_back(test);

                    TTrieTestData& trieTest = trieTestData.back();

                    if (trieTest.Data.size() > 1000) {
                        trieTest.Data.resize(1000);
                        trieTest.ElementsCount = 1000;
                    }

                    if (sorted) {
                        Sort(trieTest.Data.begin(), trieTest.Data.end());
                    }

                    if (multikey) {
                        for (size_t i = 0; i < trieTest.Data.size(); ++i) {
                            const TString& key = trieTest.Data[i];
                            TVector<TString>& subkeys = trieTest.Multikeys[key];

                            if (const size_t subkeysCount = (i + 5) % 10) {
                                subkeys.resize(subkeysCount, TString());
                                trieTest.ElementsCount += 1;

                                for (size_t j = 0; j < subkeysCount; ++j) {
                                    subkeys[j] = key + ToString(j);
                                }
                            }
                        }
                    }

                    trieTestData.back().Sorted = sorted;
                    trieTestData.back().Multikey = multikey;
                    trieTestData.back().Marker += Sprintf("/sorted=%d/multikey=%d", sorted, multikey);
                }
            }
        }

        for (auto& test : trieTestData) {
            using namespace NCodedBlob;
            TBufferOutput bout;
            {
                TCodedBlobTrieSimpleBuilder builder(400, !test.Sorted, test.TmpCodec);
                builder.Init(test.Codec);

                for (const auto& key : test.Data) {
                    builder.Add(key, key);

                    if (test.Multikey) {
                        const TVector<TString>& subkeys = test.Multikeys.at(key);
                        if (subkeys.size()) {
                            builder.Add(subkeys.begin(), subkeys.end(), key);
                        }
                    }
                }

                builder.Finish(&bout);
            }

            {
                TCodedBlobTrie blob;
                blob.Init(TBlob::FromBuffer(bout.Buffer()));

                UNIT_ASSERT_VALUES_EQUAL_C(test.ElementsCount, blob.Count(), test.Marker);

                for (ui32 i = 0, sz = test.Data.size(); i < sz; ++i) {
                    const TString& key = test.Data[i];
                    TStringBuf res;

                    UNIT_ASSERT_C(blob.GetByPrimaryKey(key, res), test.Marker);
                    UNIT_ASSERT_VALUES_EQUAL_C(key, res, test.Marker);

                    if (test.Multikey) {
                        const TVector<TString>& subkeys = test.Multikeys.at(key);
                        for (const auto& subkey : subkeys) {
                            UNIT_ASSERT_C(blob.GetByPrimaryKey(subkey, res), test.Marker + " (" + subkey + ")");
                            UNIT_ASSERT_VALUES_EQUAL_C(key, res, test.Marker);
                        }
                    }
                }

                if (!test.Sorted) {
                    Sort(test.Data.begin(), test.Data.end());
                }

                {
                    TCodedBlobTrie::TOffsetIterator oit = blob.OffsetIterator();
                    TCodedBlobTrie::TPrimaryKeyIterator pit = blob.PrimaryKeyIterator();
                    DoTestCodedBlobTrieIterators(oit, pit, blob, test);
                    oit = blob.OffsetIterator();
                    pit = blob.PrimaryKeyIterator();
                    DoTestCodedBlobTrieIterators(oit, pit, blob, test);
                }
            }
        }
    }
};

UNIT_TEST_SUITE_REGISTRATION(TCodedBlobTest);
