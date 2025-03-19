#include <util/generic/ylimits.h>
#include <util/memory/blob.h>
#include <util/generic/noncopyable.h>
#include <util/generic/yexception.h>
#include <util/stream/buffer.h>
#include <util/system/filemap.h>
#include <util/system/fs.h>
#include <util/system/tempfile.h>
#include <util/folder/dirut.h>

#include <library/cpp/testing/unittest/registar.h>

#include "positerator.h"
#include <library/cpp/wordpos/wordpos.h>
#include <kernel/keyinv/indexfile/oldindexfile.h>
#include <kernel/keyinv/indexfile/searchfile.h>

#include <util/charset/wide.h>

using CHitDecoderT = DecoderFallBack<CHitDecoder2, false, HIT_FMT_BLK8>;

namespace NPosIteratorUT {

    const ui32 FIRST_HIT = 1;

    //TODO: MakeTempName()
    const char INDEX_FILE_PREFIX[]      = "positerator_ut_index";

    class TKeyData : private TNonCopyable {
    public:
        typedef std::pair<const char*, ui32> TItem;
        typedef TVector<TItem> TContainer;

        TKeyData() {
            Init();
        }
        const TContainer& Get() const {
            return Data;
        }
    private:
        void Init() {
            Data.push_back(TItem("a1", 1));
            Data.push_back(TItem("a2", 2));
            Data.push_back(TItem("a3", 3));

            Data.push_back(TItem("b1", 20));
            Data.push_back(TItem("b2", 21)); // MINIMUM_HITS_COMPRESSION_COUNT
            Data.push_back(TItem("b3", 22));

            Data.push_back(TItem("c1", 63));
            Data.push_back(TItem("c2", 64));
            Data.push_back(TItem("c3", 65));

            Data.push_back(TItem("d1", 127));
            Data.push_back(TItem("d2", 128)); // DEFAULT_SUBINDEX_STEP
            Data.push_back(TItem("d3", 129));

            Data.push_back(TItem("e", 256));
            Data.push_back(TItem("f", 512));

            Data.push_back(TItem("g1", 1023));
            Data.push_back(TItem("g2", 1024));
            Data.push_back(TItem("g3", 1025));

            Data.push_back(TItem("h1", 2047));
            Data.push_back(TItem("h2", 2048));
            Data.push_back(TItem("h3", 2049));

            Data.push_back(TItem("o", 4096));
            Data.push_back(TItem("p", 8192));
            Data.push_back(TItem("q", 16384));
            Data.push_back(TItem("r", 32768));
            Data.push_back(TItem("s", 65536));
            Data.push_back(TItem("t", 131072));

            Data.push_back(TItem("u1", 50));
            Data.push_back(TItem("u2", 101));
            Data.push_back(TItem("u3", 200));
            Data.push_back(TItem("u4", 401));
            Data.push_back(TItem("u5", 700));
            Data.push_back(TItem("u6", 1301));
            Data.push_back(TItem("u7", 1501));
            Data.push_back(TItem("u8", 3000));
            Data.push_back(TItem("u9", 5001));

            Data.push_back(TItem("v1", 13000));
            Data.push_back(TItem("v2", 25001));
            Data.push_back(TItem("v3", 50000));
            Data.push_back(TItem("v4", 140001));
        }
    private:
        TContainer Data;
    };

    void WriteDataToFile(const TKeyData::TContainer& data, NIndexerCore::TOldIndexFile& file) {
        for (const auto& item : data) {
            file.StoreNextKey(item.first);
            for (ui32 hit = FIRST_HIT; hit <= item.second; ++hit) {
                file.StoreNextHit(TWordPosition(hit, hit).SuperLong());
            }
        }
    }

    void WriteFile(const TKeyData::TContainer& data, const char* prefix, ui32 version) {
        // @todo it would be better to make over TYndexFile into a template class in order not to write to a file but write into the memory
        NIndexerCore::TOldIndexFile file(IYndexStorage::FINAL_FORMAT, version); // should be used other formats?
        file.Open(prefix);
        WriteDataToFile(data, file);
        file.CloseEx();
    }

    void RemoveFile(const char* prefix) {
        NFs::Remove(prefix + TString("inv"));
        NFs::Remove(prefix + TString("key"));
    }

    //! checks hits moving from first to last
    template <typename TDecoder>
    void CheckHits(TPosIterator<TDecoder>& it, ui32 lastHit) {
        ui32 hit = FIRST_HIT;
        for (; it.Valid(); ++it, ++hit) {
            UNIT_ASSERT(it.Valid());
            UNIT_ASSERT_VALUES_EQUAL(*it, TWordPosition(hit, hit).SuperLong());
        }
        UNIT_ASSERT_VALUES_EQUAL(hit - 1, lastHit);
    }

    //! checks hits sequentially in upward order
    template <typename TDecoder>
    class THitChecker1 {
    public:
        void Check(TPosIterator<TDecoder>& it, ui32 lastHit) {
            UNIT_ASSERT(FIRST_HIT <= lastHit);

            CheckHits(it, lastHit);
        }
    };

    //! checks hits using the @c SkipTo function
    template <typename TDecoder>
    class THitChecker2 {
    public:
        //! @note this function uses the same test cases as @c THitChecker3::Check
        void Check(TPosIterator<TDecoder>& it, ui32 lastHit) {
            UNIT_ASSERT(FIRST_HIT <= lastHit);

            // skip upward: from FIRST_HIT to lastHit
            it.Restart();
            for (ui32 hit = FIRST_HIT; hit <= lastHit; ++hit) {
                const TWordPosition pos(hit, hit);
                it.SkipTo(pos.SuperLong());
                UNIT_ASSERT(it.Valid());
                UNIT_ASSERT_VALUES_EQUAL(*it, pos.SuperLong());
            }

            const TWordPosition firstPos(FIRST_HIT, FIRST_HIT);
            const TWordPosition lastPos(lastHit, lastHit);

            // skip to first
            {
                it.Restart();
                it.SkipTo(firstPos.SuperLong());
                UNIT_ASSERT(it.Valid());
                UNIT_ASSERT(*it == firstPos.SuperLong());
            }

            // skip beyond the last (it must get invalid)
            {
                it.Restart();
                const TWordPosition pos(lastHit + 1, lastHit + 1);
                it.SkipTo(pos.SuperLong());
                UNIT_ASSERT(!it.Valid());
            }

            if (lastHit > FIRST_HIT) {
                const ui32 middleHit = lastHit / 2;
                const TWordPosition middlePos(middleHit, middleHit);

                // skip to the middle
                {
                    it.Restart();
                    it.SkipTo(middlePos.SuperLong());
                    UNIT_ASSERT(it.Valid());
                    UNIT_ASSERT(*it == middlePos.SuperLong());
                }

                // not found in the middle
                {
                    it.Restart();
                    const TWordPosition pos(middleHit, 0);
                    it.SkipTo(middlePos.SuperLong());
                    UNIT_ASSERT(it.Valid());
                    UNIT_ASSERT(*it == middlePos.SuperLong());
                }

                // skip to last
                {
                    it.Restart();
                    it.SkipTo(lastPos.SuperLong());
                    UNIT_ASSERT(it.Valid());
                    UNIT_ASSERT(*it == lastPos.SuperLong());
                }

                // skip back (it must keep the current position)
                {
                    // don't restart
                    it.SkipTo(firstPos.SuperLong());
                    UNIT_ASSERT(it.Valid());
                    UNIT_ASSERT(*it == lastPos.SuperLong());
                }

                // not found (it must find greater or equal position)
                {
                    it.Restart();
                    const TWordPosition pos(lastHit, 0);
                    it.SkipTo(pos.SuperLong());
                    UNIT_ASSERT(it.Valid());
                    UNIT_ASSERT(*it == lastPos.SuperLong());
                }
            }
        }
    };

    //! checks hits using the @c SkipAndCount function
    template <typename TDecoder>
    class THitChecker3 {
    public:
        //! @note this function uses the same test cases as @c THitChecker2::Check
        void Check(TPosIterator<TDecoder>& it, ui32 lastHit) {
            UNIT_ASSERT(FIRST_HIT <= lastHit);

            // skip upward: from FIRST_HIT to lastHit
            it.Restart();
            i64 count = 0;
            for (ui32 hit = FIRST_HIT; hit <= lastHit; ++hit) {
                const TWordPosition pos(hit, hit);
                it.SkipAndCount(pos.SuperLong(), count);
                UNIT_ASSERT(it.Valid());
                UNIT_ASSERT_VALUES_EQUAL(*it, pos.SuperLong());
                if (hit == FIRST_HIT) {
                    UNIT_ASSERT_VALUES_EQUAL(count, 0);
                } else {
                    UNIT_ASSERT_VALUES_EQUAL(static_cast<ui32>(count), hit - 1);
                }
            }

            const TWordPosition firstPos(FIRST_HIT, FIRST_HIT);
            const TWordPosition lastPos(lastHit, lastHit);

            // skip to first
            {
                it.Restart();
                i64 count = 0;
                it.SkipAndCount(firstPos.SuperLong(), count);
                UNIT_ASSERT(it.Valid());
                UNIT_ASSERT(*it == firstPos.SuperLong());
                UNIT_ASSERT_VALUES_EQUAL(count, 0);
            }

            // skip beyond the last (it must get invalid)
            {
                it.Restart();
                const TWordPosition pos(lastHit + 1, lastHit + 1);
                i64 count = 0;
                it.SkipAndCount(pos.SuperLong(), count);
                UNIT_ASSERT(!it.Valid());
            }

            if (lastHit > FIRST_HIT) {
                const ui32 middleHit = lastHit / 2;
                const TWordPosition middlePos(middleHit, middleHit);

                // skip to the middle
                {
                    it.Restart();
                    i64 count = 0;
                    it.SkipAndCount(middlePos.SuperLong(), count);
                    UNIT_ASSERT(it.Valid());
                    UNIT_ASSERT(*it == middlePos.SuperLong());
                    UNIT_ASSERT_VALUES_EQUAL(count + 1, static_cast<i64>(middleHit));
                }

                // not found in the middle
                {
                    it.Restart();
                    const TWordPosition pos(middleHit, 0);
                    i64 count = 0;
                    it.SkipAndCount(middlePos.SuperLong(), count);
                    UNIT_ASSERT(it.Valid());
                    UNIT_ASSERT(*it == middlePos.SuperLong());
                    UNIT_ASSERT_VALUES_EQUAL(count + 1, static_cast<ui32>(middleHit));
                }

                // skip to last
                {
                    it.Restart();
                    i64 count = 0;
                    it.SkipAndCount(lastPos.SuperLong(), count);
                    UNIT_ASSERT(it.Valid());
                    UNIT_ASSERT(*it == lastPos.SuperLong());
                    UNIT_ASSERT_VALUES_EQUAL(count + 1, static_cast<i64>(lastHit));
                }

                // skip back (it must keep the current position)
                {
                    // don't restart
                    i64 count = 0;
                    it.SkipAndCount(firstPos.SuperLong(), count);
                    UNIT_ASSERT(it.Valid());
                    UNIT_ASSERT(*it == lastPos.SuperLong());
                    UNIT_ASSERT_VALUES_EQUAL(count, 0);
                }

                // not found (it must find greater or equal position)
                {
                    it.Restart();
                    const TWordPosition pos(lastHit, 0);
                    i64 count = 0;
                    it.SkipAndCount(pos.SuperLong(), count);
                    UNIT_ASSERT(it.Valid());
                    UNIT_ASSERT(*it == lastPos.SuperLong());
                    UNIT_ASSERT_VALUES_EQUAL(count + 1, static_cast<i64>(lastHit));
                }
            }
        }
    };

    //! checks hits using the @c SkipToCount function
    template <typename TDecoder>
    class THitChecker4 {
    public:
        void Check(TPosIterator<TDecoder>& it, ui32 lastHit) {
            UNIT_ASSERT(FIRST_HIT <= lastHit);

            const TWordPosition firstPos(FIRST_HIT, FIRST_HIT);
            const TWordPosition lastPos(lastHit, lastHit);

            // skip all hits by one by one
            it.Restart();
            i64 count = 0;
            for (ui32 hit = FIRST_HIT; hit <= lastHit; ++hit, ++count) {
                it.SkipToCount(count, count + 1);
                if (hit == lastHit) {
                    UNIT_ASSERT(!it.Valid());
                } else {
                    UNIT_ASSERT(it.Valid());
                    const TWordPosition pos(hit + 1, hit + 1);
                    UNIT_ASSERT_VALUES_EQUAL(*it, pos.SuperLong());
                }
            }

            // skip from the first to the first
            it.Restart();
            it.SkipToCount(0, 0);
            UNIT_ASSERT(it.Valid());
            UNIT_ASSERT_VALUES_EQUAL(*it, firstPos.SuperLong());

            // skip from the first to the last
            it.Restart();
            it.SkipToCount(0, lastHit - 1);
            UNIT_ASSERT(it.Valid());
            UNIT_ASSERT_VALUES_EQUAL(*it, lastPos.SuperLong());

            if (lastHit > FIRST_HIT) {
                const ui32 middleHit = lastHit / 2;
                const TWordPosition middlePos(middleHit, middleHit);

                // skip from the first to the middle
                it.Restart();
                it.SkipToCount(0, middleHit - 1);
                UNIT_ASSERT(it.Valid());
                UNIT_ASSERT_VALUES_EQUAL(*it, middlePos.SuperLong());

                // skip from the middle to the last being at the middle
                it.SkipToCount(middleHit - 1, lastHit - 1);
                UNIT_ASSERT(it.Valid());
                UNIT_ASSERT_VALUES_EQUAL(*it, lastPos.SuperLong());
            }

            // all invalid cases must be ignored and iterator must keep at the current position - the first position
            it.Restart();
            UNIT_ASSERT(it.Valid());
            it.SkipToCount(0, -1);
            UNIT_ASSERT(!it.Valid());

            it.Restart();
            UNIT_ASSERT(it.Valid());
            it.SkipToCount(-2, -1);
            UNIT_ASSERT(!it.Valid());

            it.Restart();
            UNIT_ASSERT(it.Valid());
            it.SkipToCount(2, -1);
            UNIT_ASSERT(!it.Valid());

            it.Restart();
            UNIT_ASSERT(it.Valid());
            it.SkipToCount(0, Max<i64>());
            UNIT_ASSERT(!it.Valid());
        }
    };

    //! sequentially checks all keys and hits in the file
    template <typename TDecoder, typename THitChecker>
    void CheckFile(const TKeyData::TContainer& data, NIndexerCore::TIndexReader& file) {
        ui32 recIndex = 0;
        THitChecker hitChecker;

        while (file.ReadKey() && recIndex < data.size()) {
            UNIT_ASSERT(recIndex < data.size());
            const TKeyData::TItem& item = data[recIndex];
            UNIT_ASSERT_STRINGS_EQUAL(file.GetKeyText(), item.first);

            TPosIterator<TDecoder> it;
            file.InitPosIterator(it);
            hitChecker.Check(it, item.second);

            ++recIndex;
        }

        UNIT_ASSERT_VALUES_EQUAL(recIndex, data.size());
    }

    void ReadTest(const TKeyData::TContainer& data, const char* prefix) {
        NIndexerCore::TIndexReader file(prefix);
        CheckFile< CHitDecoderT, THitChecker1<CHitDecoderT> >(data, file);
    }

    void SearchTest(const TKeyData::TContainer& data, const char* prefix) {
        TYndex4Searching index;
        index.InitSearch(prefix);
        TPosIterator<> it;
        const READ_HITS_TYPE readType = RH_DEFAULT; // read using MemoryMap

        // all keys must be found
        for (TKeyData::TContainer::const_iterator item = data.begin(); item != data.end(); ++item) {
            UNIT_ASSERT(it.Init(index, item->first, readType));
            CheckHits(it, item->second);
        }

        // the key must not be found
        UNIT_ASSERT(!it.Init(index, "zzz", readType));
    }

    void SkipToTest(const TKeyData::TContainer& data, const char* prefix) {
        NIndexerCore::TIndexReader file(prefix);
        CheckFile< CHitDecoderT, THitChecker2<CHitDecoderT> >(data, file);
    }

    void SkipAndCountTest(const TKeyData::TContainer& data, const char* prefix) {
        NIndexerCore::TIndexReader file(prefix);
        CheckFile< CHitDecoderT, THitChecker3<CHitDecoderT> >(data, file);
    }

    void SkipToCountTest(const TKeyData::TContainer& data, const char* prefix) {
        NIndexerCore::TIndexReader file(prefix);
        CheckFile< CHitDecoderT, THitChecker4<CHitDecoderT> >(data, file);
    }

    void SaveLoadHitsTest(const TKeyData::TContainer& data) {
        for (const auto& item : data) {
            const ui32 bufLen = sizeof(SUPERLONG) * item.second;
            const TArrayHolder<char> buf(new char[bufLen]);
            const EHitFormat hitFormat = HIT_FMT_RAW_I64;
            const ui32 hitBufLen = sizeof(SUPERLONG) * 2; // two times greater than SUPERLONG just in case
            char hitBuf[hitBufLen];

            CHitCoder hitCoder(hitFormat);
            ui32 len = 0;

            for (ui32 hit = FIRST_HIT; hit <= item.second; ++hit) {
                const TWordPosition pos(hit, hit);
                UNIT_ASSERT(len + sizeof(SUPERLONG) <= bufLen);
                // @todo it would be better to have a member function CHitCoder::OutputSize() that would return size of hit output
                ui32 hitSize = hitCoder.Output(pos.SuperLong(), hitBuf);
                UNIT_ASSERT(hitSize <= hitBufLen);
                UNIT_ASSERT(len + hitSize <= bufLen);
                memcpy(buf.Get() + len, hitBuf, hitSize);
                len += hitSize;
            }

            THitsForRead hitsToSave(TBlob::NoCopy(buf.Get(), len));

            TBufferStream ios;
            hitsToSave.SaveHits(&ios);

            THitsForRead loadedHits;
            loadedHits.LoadHits(&ios, false);

            UNIT_ASSERT(loadedHits.GetHitFormat() == HIT_FMT_RAW_I64);
            UNIT_ASSERT(loadedHits.GetFirstPerst() == &YxPerst::NullSubIndex);
            UNIT_ASSERT(loadedHits.GetLength() == len);
            UNIT_ASSERT(loadedHits.GetCount() == 0);

            // TSubIndexInfo must be constructed by default
            UNIT_ASSERT(hitsToSave.GetSubIndexInfo().nSubIndexStep == loadedHits.GetSubIndexInfo().nSubIndexStep);
            UNIT_ASSERT(hitsToSave.GetSubIndexInfo().nPerstSize == loadedHits.GetSubIndexInfo().nPerstSize);
            UNIT_ASSERT(hitsToSave.GetSubIndexInfo().nMinNeedSize == loadedHits.GetSubIndexInfo().nMinNeedSize);
            UNIT_ASSERT(hitsToSave.GetSubIndexInfo().hasSubIndex == loadedHits.GetSubIndexInfo().hasSubIndex);

            TPosIterator<> it;
            it.Init(loadedHits);
            CheckHits(it, item.second);
        }

    }
}

using namespace NPosIteratorUT;

class TPosIteratorTest : public TTestBase {
private:
    THolder<TKeyData> DataPtr;

private:
    UNIT_TEST_SUITE(TPosIteratorTest);
        UNIT_TEST(TestRead);
        UNIT_TEST(TestSearch);
        UNIT_TEST(TestSkipTo);
        UNIT_TEST(TestSkipAndCount);
        UNIT_TEST(TestSkipToCount);
        UNIT_TEST(TestSaveLoadHits);
    UNIT_TEST_SUITE_END();

public:
    TPosIteratorTest() = default;
    ~TPosIteratorTest() override {
        RemoveFile(INDEX_FILE_PREFIX);
    }

    inline const TKeyData::TContainer& GetData() {
        if (!DataPtr) {
            DataPtr.Reset(new TKeyData);
            Init();
        }

        return DataPtr->Get();
    }

    inline void Init() {
        WriteFile(GetData(), INDEX_FILE_PREFIX, YNDEX_VERSION_BLK8);
    }

    void TestRead();
    void TestSearch();
    void TestSkipTo();
    void TestSkipAndCount();
    void TestSkipToCount();
    void TestSaveLoadHits();
};

UNIT_TEST_SUITE_REGISTRATION(TPosIteratorTest);

void TPosIteratorTest::TestRead() {
    ReadTest(GetData(), INDEX_FILE_PREFIX);
}

void TPosIteratorTest::TestSearch() {
    SearchTest(GetData(), INDEX_FILE_PREFIX);
}

void TPosIteratorTest::TestSkipTo() {
    SkipToTest(GetData(), INDEX_FILE_PREFIX);
}

void TPosIteratorTest::TestSkipAndCount() {
    SkipAndCountTest(GetData(), INDEX_FILE_PREFIX);
}

void TPosIteratorTest::TestSkipToCount() {
    SkipToCountTest(GetData(), INDEX_FILE_PREFIX);
}

void TPosIteratorTest::TestSaveLoadHits() {
    SaveLoadHitsTest(GetData());
}
