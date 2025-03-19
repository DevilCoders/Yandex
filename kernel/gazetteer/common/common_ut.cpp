#include <library/cpp/testing/unittest/registar.h>

#include "compactstorage.h"
#include "serialize.h"

#include <util/stream/buffer.h>
#include <util/ysaveload.h>

class TGazetteerCommonTest : public TTestBase {
private:
    UNIT_TEST_SUITE(TGazetteerCommonTest);
        UNIT_TEST(TestVectorsVector);
        UNIT_TEST(TestCompactStorage);
        UNIT_TEST(TestIndexator);
        UNIT_TEST(TestMemoryAdapter);
    UNIT_TEST_SUITE_END();

public:
    void CheckVectorsVectorContent(const NGzt::TVectorsVector<size_t>& vv, const TVector< TVector<size_t> >& data) {
        size_t totalCount = 0;
        for (size_t i = 0; i < data.size(); ++i)
            totalCount += data[i].size();

        UNIT_ASSERT_EQUAL(vv.Size(), data.size());
        UNIT_ASSERT_EQUAL(vv.TotalSize(), totalCount);
        for (size_t i = 0; i < vv.Size(); ++i) {
            NGzt::TVectorsVector<size_t>::TConstIter it = vv[i];
            UNIT_ASSERT(it.Ok() != data[i].empty());
            size_t j = 0;
            for (; it.Ok(); ++it, ++j)
                UNIT_ASSERT_EQUAL(*it, data[i][j]);
            UNIT_ASSERT_EQUAL(j, data[i].size());
        }
    }

    void TestVectorsVector() {
        {
            NGzt::TVectorsVector<size_t> empty;
            UNIT_ASSERT_EQUAL(empty.Size(), 0);
            UNIT_ASSERT_EQUAL(empty.TotalSize(), 0);
        }
        {
            TVector< TVector<size_t> > data(4);
            data[0].push_back(0);
            data[0].push_back(1);
            data[0].push_back(2);

            for (size_t k = 0; k < 1000; ++k)
                data[1].push_back(k*2);

            // data[2] is empty

            data[3].push_back(2);
            data[3].push_back(1);

            NGzt::TVectorsVector<size_t> filled(data);
            CheckVectorsVectorContent(filled, data);

            TBufferStream buf;
            ::Save(&buf, filled);

            NGzt::TVectorsVector<size_t> loaded;
            ::Load(&buf, loaded);
            CheckVectorsVectorContent(loaded, data);
        }
    }

    template <typename TIter>
    void CheckEmptyIterator(const TIter& it) {
//        if (it.Ok())
//            Cerr << *it << Endl;
        UNIT_ASSERT(!it.Ok());
    }

    template <typename TIter, typename T>
    void CheckIteratorValueAndProceed(TIter& it, T v1) {
//        Cerr << "[" << v1 << "]" ;
        UNIT_ASSERT(it.Ok());
//        Cerr << " == [" << *it << "]" << Endl;
        UNIT_ASSERT_EQUAL(*it, v1);
        ++it;
    }

    template <typename TIter>
    void CheckIterator(const TIter& it) {
        CheckEmptyIterator(it);
    }

    template <typename TIter, typename T>
    void CheckIterator(TIter it, T v1) {
        CheckIteratorValueAndProceed(it, v1);
        CheckEmptyIterator(it);
    }

    template <typename TIter, typename T>
    void CheckIterator(TIter it, T v1, T v2) {
        CheckIteratorValueAndProceed(it, v1);
        CheckIteratorValueAndProceed(it, v2);
        CheckEmptyIterator(it);
    }

    template <typename TIter, typename T>
    void CheckIterator(TIter it, T v1, T v2, T v3) {
        CheckIteratorValueAndProceed(it, v1);
        CheckIteratorValueAndProceed(it, v2);
        CheckIteratorValueAndProceed(it, v3);
        CheckEmptyIterator(it);
    }

    template <typename TIter, typename T>
    void CheckIterator(TIter it, T v1, T v2, T v3, T v4) {
        CheckIteratorValueAndProceed(it, v1);
        CheckIteratorValueAndProceed(it, v2);
        CheckIteratorValueAndProceed(it, v3);
        CheckIteratorValueAndProceed(it, v4);
        CheckEmptyIterator(it);
    }

    typedef NGzt::TCompactStorageBuilder<TString>::TIndex TIndex;

    void FillCompactStorageBuilder(NGzt::TCompactStorageBuilder<TString, true>& builder,
                                   TIndex& single1, TIndex& single2, TIndex& emptySet, TIndex& set42) {
        CheckIterator(builder[builder.EmptySetIndex]);

        single1 = builder.Add("123");
        single2 = builder.Add("aaa");
        TIndex single3 = builder.Add("");

        // GetSingleItem
        UNIT_ASSERT_EQUAL(builder.GetSingleItem(single1), "123");
        UNIT_ASSERT_EQUAL(builder.GetSingleItem(single2), "aaa");
        UNIT_ASSERT_EQUAL(builder.GetSingleItem(single3), "");

        UNIT_ASSERT(builder[single2].Ok());
        CheckIterator(builder[single2], TString("aaa"));

        // HasItemAt
        UNIT_ASSERT(builder.HasItemAt(single1, "123"));
        UNIT_ASSERT(!builder.HasItemAt(single1, "aaa"));
        UNIT_ASSERT(builder.HasItemAt(single2, "aaa"));

        UNIT_ASSERT_EQUAL(builder.Add("123"), single1);
        UNIT_ASSERT_EQUAL(builder.Add(""), single3);


        // AddTo
        UNIT_ASSERT_EQUAL(builder.AddTo(single1, "123"), single1);
        UNIT_ASSERT_EQUAL(builder.AddTo(single2, "aaa"), single2);

        TIndex set1 = builder.AddTo(single1, "456");        // 123, 456
        UNIT_ASSERT(set1 != single1);
        CheckIterator(builder[set1], TString("123"), TString("456"));

        TIndex set12 = builder.AddTo(set1, "678");        // 123, 456, 678
        UNIT_ASSERT_EQUAL(set1, set12);       // re-using set2 index
        CheckIterator(builder[set1], TString("123"), TString("456"), TString("678"));

        TIndex set2 = builder.AddTo(single2, "bbb");
        UNIT_ASSERT(set2 != single2);
        CheckIterator(builder[set2], TString("aaa"), TString("bbb"));

        TIndex set22 = builder.AddTo(set2, "bbb");      // alread in
        UNIT_ASSERT_EQUAL(set2, set22);

        TIndex set23 = builder.AddTo(single2, "bbb");      // gives same set as set2, and increases its refcount
        UNIT_ASSERT_EQUAL(set2, set23);

        TIndex set24 = builder.AddTo(set2, "ccc");          // should give a new set number
        UNIT_ASSERT(set2 != set24);
        CheckIterator(builder[set2], TString("aaa"), TString("bbb"));
        CheckIterator(builder[set24], TString("aaa"), TString("bbb"), TString("ccc"));

        // RemoveFrom
        TIndex set25 = builder.RemoveFrom(set24, "bbb");
        CheckIterator(builder[set25], TString("aaa"), TString("ccc"));
        UNIT_ASSERT(builder.RemoveFrom(set25, "") == set25);
        TIndex set26 = builder.RemoveFrom(set25, "aaa");        // single item "ccc" remains
        CheckIterator(builder[set26], TString("ccc"));
        UNIT_ASSERT_EQUAL(set26, builder.Add("ccc"));
        emptySet = builder.RemoveFrom(set26, "ccc");        // empty set
        CheckIterator(builder[emptySet]);
        UNIT_ASSERT(builder.IsEmptySet(emptySet));

        // AddRange, AddRangeTo
        const char* range1[] = { "123", "456" };
        TIndex set13 = builder.AddRange(range1, range1 + Y_ARRAY_SIZE(range1));
        CheckIterator(builder[set13], TString("123"), TString("456"));
        const char* range2[] = { "678", "911" };
        TIndex set14 = builder.AddRangeTo(set13, range2, range2 + Y_ARRAY_SIZE(range2));
        CheckIterator(builder[set14], TString("123"), TString("456"), TString("678"), TString("911"));

        // Merge
        TIndex set40 = builder.Merge(set14, emptySet);     // set27 is empty
        CheckIterator(builder[set40], TString("123"), TString("456"), TString("678"), TString("911"));
        TIndex set41 = builder.Merge(set2, set25);
        CheckIterator(builder[set41], TString("aaa"), TString("bbb"), TString("ccc"));
        TIndex indexRange[] = { single1, single2, single3 };
        set42 = builder.Merge(indexRange, indexRange + Y_ARRAY_SIZE(indexRange));
        CheckIterator(builder[set42], TString("123"), TString("aaa"), TString(""));
    }

    void CheckCompactStorage(const NGzt::TCompactStorage<TString>& storage, TIndex single1, TIndex single2, TIndex emptySet, TIndex set42) {
        CheckIterator(storage[single1], TString("123"));
        CheckIterator(storage[single2], TString("aaa"));
        UNIT_ASSERT_EQUAL(storage.GetSingleItem(single1), "123");
        UNIT_ASSERT_EQUAL(storage.GetSingleItem(single2), "aaa");
        CheckIterator(storage[emptySet]);
        UNIT_ASSERT(storage.IsEmptySet(emptySet));
        CheckIterator(storage[set42], TString("123"), TString("aaa"), TString(""));
    }

    void TestCompactStorage() {
        NGzt::TCompactStorageBuilder<TString, true> builder;
        TIndex single1 = 0, single2 = 0, emptySet = 0, set42 = 0;
        FillCompactStorageBuilder(builder, single1, single2, emptySet, set42);

        TBufferStream buf;
        ::Save(&buf, builder);

        NGzt::TCompactStorage<TString> storage;
        ::Load(&buf, storage);

        CheckCompactStorage(storage, single1, single2, emptySet, set42);
    }

    void TestIndexator() {
        NGzt::TIndexator<TString, true> in;
        UNIT_ASSERT_EQUAL(in.Size(), 0);

        TString s123 = "123";
        size_t i = in.Add(s123);
        UNIT_ASSERT_EQUAL(in.Size(), 1);
        UNIT_ASSERT_EQUAL(i, 0);
        UNIT_ASSERT(s123.empty());      // adding is done via Swap

        TString s456 = "456";
        i = in.Add(s456);
        UNIT_ASSERT_EQUAL(in.Size(), 2);
        UNIT_ASSERT_EQUAL(i, 1);

        s123 = "123";
        i = in.Add(s123);
        UNIT_ASSERT_EQUAL(in.Size(), 2);
        UNIT_ASSERT_EQUAL(i, 0);

        TString saaa = "aaa";
        i = in.Add(saaa);
        UNIT_ASSERT_EQUAL(in.Size(), 3);
        UNIT_ASSERT_EQUAL(i, 2);

        s456 = "456";
        i = in.Add(s456);
        UNIT_ASSERT_EQUAL(in.Size(), 3);
        UNIT_ASSERT_EQUAL(i, 1);

        saaa = "aaa";
        i = in.Add(saaa);
        UNIT_ASSERT_EQUAL(in.Size(), 3);
        UNIT_ASSERT_EQUAL(i, 2);

        UNIT_ASSERT_EQUAL(in[0], "123");
        UNIT_ASSERT_EQUAL(in[1], "456");
        UNIT_ASSERT_EQUAL(in[2], "aaa");
    }

    void TestMemoryAdapter() {
        TString data = "1234567890";
        const void* buf = nullptr;
        int buflen = 0;
        {

            int datalen = data.size();
            TMemoryInput mem(data.data(), data.size());
            NGzt::TMemoryInputStreamAdaptor adaptor(&mem);
            UNIT_ASSERT_EQUAL(adaptor.ByteCount(), 0);

            UNIT_ASSERT(adaptor.Next(&buf, &buflen));
            UNIT_ASSERT_EQUAL(buf, data.data());
            UNIT_ASSERT_EQUAL(buflen, datalen);
            UNIT_ASSERT_EQUAL(adaptor.ByteCount(), datalen);

            adaptor.BackUp(1);
            UNIT_ASSERT_EQUAL(adaptor.ByteCount(), datalen - 1);
            adaptor.BackUp(1);
            UNIT_ASSERT_EQUAL(adaptor.ByteCount(), datalen - 2);
            adaptor.BackUp(3);
            UNIT_ASSERT_EQUAL(adaptor.ByteCount(), datalen - 5);

            UNIT_ASSERT(adaptor.Next(&buf, &buflen));
            UNIT_ASSERT_EQUAL(buf, data.data() + 5);
            UNIT_ASSERT_EQUAL(buflen, 5);
            UNIT_ASSERT_EQUAL(adaptor.ByteCount(), datalen);

            UNIT_ASSERT(!adaptor.Next(&buf, &buflen));
        }

        {
            TMemoryInput mem(data.data(), data.size());
            {
                NGzt::TMemoryInputStreamAdaptor adaptor(&mem);
                adaptor.Next(&buf, &buflen);
                adaptor.BackUp(7);
            }
            // returns unread memory
            UNIT_ASSERT_EQUAL(mem.Avail(), 7);
            UNIT_ASSERT_EQUAL(TStringBuf(mem.Buf(), mem.Avail()), TStringBuf("4567890"));
        }

        {
            TMemoryInput bigFake(&data, (size_t)Max<int>() + 100);
            NGzt::TMemoryInputStreamAdaptor adaptor(&bigFake);

            UNIT_ASSERT(adaptor.Next(&buf, &buflen));
            UNIT_ASSERT_EQUAL(buflen, Max<int>());
            UNIT_ASSERT_EQUAL(adaptor.ByteCount(), Max<int>());

            UNIT_ASSERT(adaptor.Next(&buf, &buflen));
            UNIT_ASSERT_EQUAL(buflen, 100);
            UNIT_ASSERT_EQUAL(adaptor.ByteCount(), (i64)Max<int>() + 100);
        }
    }
};

UNIT_TEST_SUITE_REGISTRATION(TGazetteerCommonTest);

