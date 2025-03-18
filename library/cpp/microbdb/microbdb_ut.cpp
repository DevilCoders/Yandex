#include <library/cpp/testing/unittest/registar.h>

#include <functional>
#include <sys/stat.h>
#include <util/random/fast.h>
#include <util/generic/cast.h>
#include <util/system/maxlen.h>

#include "microbdb.h"
#include "safeopen.h"
#include "utility.h"

namespace NMbdbTest0 {
    class TMbdbTest: public TTestBase {
        UNIT_TEST_SUITE(TMbdbTest);
        UNIT_TEST(TestEmpty);
        UNIT_TEST(TestCreate);
        UNIT_TEST(TestSort);
        UNIT_TEST_SUITE_END();

    public:
        void TestEmpty();
        void TestCreate();
        void TestSort();
    };

    UNIT_TEST_SUITE_REGISTRATION(TMbdbTest);

    struct TTestRec {
        ui32 DocId;
        char Name[URL_MAX];

        static const ui32 RecordSig = 0x01010101;

        size_t SizeOf() const {
            return offsetof(TTestRec, Name) + strlen(Name) + 1;
        }
    };

    struct TTestRec2 {
        static const ui32 RecordSig = 0x0101010A;
        ui32 Id;
        TTestRec2(ui32 i)
            : Id(i)
        {
        }
        bool operator<(const TTestRec2& r) const {
            return Id < r.Id;
        }
    };

    template <typename T>
    struct TByDoc;

    template <>
    struct TByDoc<TTestRec> {
        inline bool operator()(const TTestRec* a, const TTestRec* b) const {
            return a->DocId < b->DocId;
        }
    };

    const char* const testdat = "testdat";
    const char* const testdat2 = "testdat2";
    const char* const symb = "/0123456789qwertyuiopasdfghjklzxcvbnmQWERTYUIOPASDFGHJKLZXCVBNM+%";

    void TMbdbTest::TestEmpty() {
        TOutDatFileImpl<TTestRec> TestFile;
        UNIT_ASSERT_EQUAL(TestFile.Open(testdat, 0x1000, 0x10000, 0) == 0, true);
        UNIT_ASSERT_EQUAL(TestFile.GetError() == 0, true);
        UNIT_ASSERT_EQUAL(TestFile.Close() == 0, true);

        TInDatFileImpl<TTestRec> TestFileIn;
        UNIT_ASSERT_EQUAL(TestFileIn.Open(testdat, INT_MAX, 1) == 0, true);
        UNIT_ASSERT_EQUAL(TestFileIn.GetPageSize() == 0x1000, true);
        UNIT_ASSERT_EQUAL(TestFileIn.GetLastPage() == 0, true);
        const TTestRec* u = TestFileIn.Next();
        UNIT_ASSERT_EQUAL(u == nullptr, true);
        UNIT_ASSERT_EQUAL(TestFileIn.IsEof() != 0, true);
        UNIT_ASSERT_EQUAL(TestFileIn.GetError() == 0, true);
        UNIT_ASSERT_EQUAL(TestFileIn.Close() == 0, true);

        UNIT_ASSERT_EQUAL(unlink(testdat), 0);
    }

    void TMbdbTest::TestCreate() {
        TOutDatFileImpl<TTestRec> TestFile;
        TTestRec TestRec;
        UNIT_ASSERT_EQUAL(TestFile.Open(testdat, 0x1000, 0x10000, 0) == 0, true);

        TReallyFastRng32 Rndm(TTestRec::RecordSig);

        ui32 d = 0;
        ui32 r = Rndm.GenRand();

        for (int i = 0; i < 50000; i++) {
            if (r == 0)
                r = Rndm.GenRand();
            d += 1 + (r & 7);
            r = r >> 8;
            TestRec.DocId = d;
            size_t len = 29 + (r & 511);
            r = r >> 16;
            size_t j = 0;
            while (j < len) {
                if (r == 0)
                    r = Rndm.GenRand();
                TestRec.Name[j++] = symb[r & 63];
                r = r >> 8;
            }
            TestRec.Name[len] = 0;
            TestFile.Push(&TestRec);
        }

        UNIT_ASSERT_EQUAL(TestFile.GetError() == 0, true);
        UNIT_ASSERT_EQUAL(TestFile.Close() == 0, true);
    }

    void TMbdbTest::TestSort() {
        TFile ifile(testdat, RdOnly);
        UNIT_ASSERT_EQUAL(ifile.IsOpen(), true);
        TFile ofile(testdat2, WrOnly | CreateAlways | ARW | AWOther);
        UNIT_ASSERT_EQUAL(ofile.IsOpen(), true);

        ui8 buf[METASIZE];
        ui8* ptr = buf;
        size_t size = sizeof(buf);
        size_t ret;
        while (size && (ret = ifile.Read(ptr, size)) > 0) {
            size -= ret;
            ptr += ret;
        }

        UNIT_ASSERT_EQUAL(size == 0, true);
        TDatMetaPage meta = *reinterpret_cast<TDatMetaPage*>(buf);
        UNIT_ASSERT_EQUAL(meta.MetaSig == METASIG, true);
        UNIT_ASSERT_EQUAL(meta.RecordSig == TTestRec::RecordSig, true);
        UNIT_ASSERT_EQUAL(meta.PageSize == 0x1000, true);

        size_t memory = 0x1000;
        SortData<TTestRec, TByDoc>(ifile, ofile, &meta, memory);

        ifile = TFile(testdat, RdOnly);
        ofile = TFile(testdat2, RdOnly);
        UNIT_ASSERT_EQUAL(ifile.IsOpen(), true);
        UNIT_ASSERT_EQUAL(ofile.IsOpen(), true);
        UNIT_ASSERT_EQUAL(ifile.GetLength() > 0, true);
        UNIT_ASSERT_EQUAL(ofile.GetLength() > 0, true);
        UNIT_ASSERT_EQUAL(ifile.GetLength() == ofile.GetLength(), true);
        ifile.Close();
        ofile.Close();

        UNIT_ASSERT_EQUAL(unlink(testdat), 0);
        UNIT_ASSERT_EQUAL(unlink(testdat2), 0);
    }

}

template <>
inline size_t SizeOf<NMbdbTest0::TTestRec>(const NMbdbTest0::TTestRec* rec) {
    return rec->SizeOf();
}

namespace NMbdbTest {
    const size_t DEFAULT_PAGE_SIZE = 32;
    const int FIRST_ID = 3;
    const int ID_STEP = 11;
    const char TEST_FILE_NAME[] = "test";

    struct TTestRec {
        int ID;
        static const ui32 RecordSig;
    };
    const ui32 TTestRec::RecordSig = 1u;

    template <typename T1>
    T1 CastVoidPtr(void* pv) {
        return reinterpret_cast<T1>(pv);
    }

    class TTestPage {
        void* const Buf;

        void InitBuffer() {
            char* p = (char*)Buf + sizeof(TDatPage);
            char* const beg = p;
            char* const end = (char*)Buf + DEFAULT_PAGE_SIZE;

            TTestRec rec;
            int id = FIRST_ID;
            while (p < end) {
                rec.ID = id;
                memcpy(p, &rec, sizeof(TTestRec));
                id += ID_STEP;
                p += sizeof(TTestRec);
            }

            TDatPage page;
            page.PageSig = PAGESIG;
            page.RecNum = p - beg;
            page.Format = MBDB_FORMAT_RAW;
            page.Reserved = 0;
            memcpy(Buf, &page, sizeof(TDatPage));
        }

    public:
        TTestPage()
            : Buf(malloc(DEFAULT_PAGE_SIZE))
        {
            memset(Buf, 0, DEFAULT_PAGE_SIZE);
            InitBuffer();
        }
        ~TTestPage() {
            free(Buf);
        }
        TDatPage* GetDatPage() {
            return reinterpret_cast<TDatPage*>(Buf);
        }
    };

    class TBaseRecordIterator {
    public:
        TBaseRecordIterator()
            : Page(nullptr)
        {
        }
        void SetDatPage(TDatPage* page) {
            Page = page;
        }
        TDatPage* Current() {
            return Page;
        }
        TDatPage* Next() {
            return Page;
        }

    protected:
        TDatPage* Page;
    };

    class TBaseInputRecordIterator: public TBaseRecordIterator {
    public:
        TDatPage* GotoPage(int) {
            return Page;
        }

        int IsFrozen() const {
            return 0;
        }
    };

    class TTestInputRecordIterator: public TInputRecordIterator<TTestRec, TBaseInputRecordIterator> {
        typedef TInputRecordIterator<TTestRec, TBaseInputRecordIterator> TBase;

    public:
        using TBase::GotoPage;
    };

    class TTestInputFileManip {
    public:
        enum {
            PageSize = DEFAULT_PAGE_SIZE,
            NumOfPages = 7,
            BufSize = METASIZE + PageSize * NumOfPages
        };
        explicit TTestInputFileManip(bool input = true)
            : FakeOpened(false)
            , EndOfBuf(Buf + BufSize)
            , Ptr(nullptr)
        {
            memset(Buf, 0, BufSize);
            if (input)
                InitInputBuffer();
        }
        int Open(const char* /*fname*/, bool /*direct*/) {
            if (IsOpen()) {
                Close();
                return MBDB_ALREADY_INITIALIZED;
            }
            FakeOpened = true;
            Ptr = Buf;
            return 0;
        }
        int Init(const TFile& file) {
            if (IsOpen()) {
                Close();
                return MBDB_ALREADY_INITIALIZED;
            }
            File = file;
            Ptr = Buf;
            return 0;
        }
        int FakeInit() {
            if (IsOpen()) {
                Close();
                return MBDB_ALREADY_INITIALIZED;
            }
            FakeOpened = true;
            Ptr = Buf;
            return 0;
        }
        bool IsOpen() const {
            return FakeOpened || File.IsOpen();
        }
        int Close() {
            File.Close();
            FakeOpened = false;
            return 0;
        }
        i64 GetLength() const {
            if (!IsOpen())
                return -1;
            return BufSize;
        }
        int Read(void* buf, unsigned len) { // Write
            if (!IsOpen())
                return -1;
            size_t n = Min(len, (unsigned)(BufSize - (Ptr - Buf)));
            if (n) {
                memcpy(buf, Ptr, n);
                Ptr += n;
            }
            return n;
        }
        i64 Seek(i64 offset, int whence) {
            if (!IsOpen())
                return -1;
            UNIT_ASSERT(whence == SEEK_SET);
            if (offset < 0 || offset >= BufSize)
                return -1;
            Ptr = Buf + offset;
            return offset;
        }
        i64 RealSeek(i64 offset, int whence) {
            return Seek(offset, whence);
        }

        bool IsEof() const {
            return Ptr >= EndOfBuf;
        }

    private:
        void InitInputBuffer() {
            TDatMetaPage* meta = reinterpret_cast<TDatMetaPage*>(Buf);
            meta->MetaSig = METASIG;
            meta->RecordSig = TTestRec::RecordSig;
            meta->PageSize = PageSize;

            int id = 1; // one-based ID of record
            // IDs:
            // page #1: 1, 2, 3, 4, 5
            // page #2: 6, 7, 8, 9, 10
            // page #3: 11, 12, 13, 14, 15
            // page #4: 16, 17, 18
            // page #5: 19, 20, 21, 22, 23
            // page #6: 24, 25, 26, 27, 28
            // page #7: 29, 30
            int pageno = 1; // one-based page number
            char* p = Buf + METASIZE;
            Y_ASSERT((PageSize - sizeof(TDatPage)) % sizeof(TTestRec) == 0);
            const ui32 nrecs = (PageSize - sizeof(TDatPage)) / sizeof(TTestRec); // number of records per page
            while (p < EndOfBuf) {
                TDatPage* page = reinterpret_cast<TDatPage*>(p);
                page->RecNum = (pageno == 4 ? 3 : (pageno == 7 ? 2 : nrecs));
                page->PageSig = PAGESIG;
                page->Format = MBDB_FORMAT_RAW;
                page->Reserved = pageno++;
                p += PageSize;

                TTestRec* const rec = reinterpret_cast<TTestRec*>(page + 1);
                for (ui32 i = 0; i < page->RecNum; ++i)
                    rec[i].ID = id++;
            }
            MaxID = id - 1;
        }

    protected:
        TFile File;
        bool FakeOpened;
        char* const EndOfBuf;
        char* Ptr;

    public:
        static char Buf[BufSize];
        static ui32 MaxID;
    };

    char TTestInputFileManip::Buf[TTestInputFileManip::BufSize];
    ui32 TTestInputFileManip::MaxID = 0;

    class TTestOutputFileManip: public TTestInputFileManip {
    public:
        TTestOutputFileManip()
            : TTestInputFileManip(false)
        {
        }
        int Open(const char* /*fname, int mode = WrOnly | CreateAlways | ARW | AWOther*/, bool /*direct*/) {
            if (IsOpen())
                return MBDB_ALREADY_INITIALIZED;
            FakeOpened = true;
            Ptr = Buf;
            return 0;
        }
        int Init(const TFile& file) {
            if (IsOpen())
                return MBDB_ALREADY_INITIALIZED;
            File = file;
            Ptr = Buf;
            return 0;
        }
        int Write(const void* buf, unsigned len) {
            if (!IsOpen())
                return -1;
            UNIT_ASSERT((unsigned)(BufSize - (Ptr - Buf)) >= len);
            memcpy(Ptr, buf, len);
            Ptr += len;
            return len;
        }
    };

    //! forward-only reader intended for testing @c TInputPageIterator
    //! @note a substitute of TInputPageFileImpl<TInputFileManip>
    class TTestReader {
        typedef TTestInputFileManip TFileManip;

    public:
        TTestReader() {
            FileManip.FakeInit();
            FileManip.Seek(METASIZE, SEEK_SET); // skip to the first page
        }
        size_t GetPageSize() const {
            return TFileManip::PageSize;
        }
        bool IsEof() const {
            return FileManip.IsEof();
        }
        int GetLastPage() const {
            return TFileManip::NumOfPages;
        }
        int GotoPage(int pageno) {
            const i64 offset = pageno * TFileManip::PageSize + METASIZE;
            return (FileManip.Seek(offset, SEEK_SET) != offset ? MBDB_BAD_FILE_SIZE : 0);
        }
        int ReadPages(iovec* vec, int nvec, int* pages) {
            *pages = 0;
            if (!IsEof()) {
                int n = Readv(FileManip, vec, nvec);
                *pages += n / TFileManip::PageSize;
            }
            return 0;
        }

    private:
        TFileManip FileManip;
    };

    class TTestInputPageIterator: public TInputPageIterator<TTestReader> {
        typedef TInputPageIterator<TTestReader> TBase;

    public:
        int Init(size_t pages, int pagesOrBytes) {
            return TBase::Init(pages, pagesOrBytes);
        }
        using TBase::GotoPage;
    };

    typedef TInDatFileImpl<NMbdbTest::TTestRec, TInputRecordIterator<
                                                    NMbdbTest::TTestRec, TInputPageIterator<TInputPageFileImpl<TTestInputFileManip>>>>
        TTestInDatFile;

    typedef TOutDatFileImpl<NMbdbTest::TTestRec, TOutputRecordIterator<
                                                     NMbdbTest::TTestRec, TOutputPageIterator<TOutputPageFileImpl<TTestOutputFileManip>>>>
        TTestOutDatFileBase;

    class TTestOutDatFile: public TTestOutDatFileBase {
    public:
        using TTestOutDatFileBase::GotoPage;
    };

    //! it intended for testing THeapIter<>
    //! @note it simulates methods of @c TInDatFileImpl: Next and Current
    class TTestIterator {
    public:
        template <size_t N>
        TTestIterator(const int (&arr)[N])
            : Cur(nullptr) // at first it is NULL
            , First(arr)
            , Last(arr + N)
        {
            Y_ASSERT(First <= Last);
        }
        const int* Current() {
            return (Cur == Last ? nullptr : Cur);
        }
        const int* Next() {
            if (!Cur)
                Cur = First;
            else if (Cur != Last)
                ++Cur;
            return Current();
        }

    private:
        const int* Cur;
        const int* const First;
        const int* const Last;
    };

    struct TTestSieve {
        static int Sieve(TTestRec* a, const TTestRec* b) {
            return (a != b && a->ID == b->ID ? 1 : 0);
        }
    };

    struct TTestRecLess {
        inline bool operator()(const TTestRec* a, const TTestRec* b) const {
            return a->ID < b->ID;
        }
    };

    class TTmpFiles {
        char Templ[FILENAME_MAX];

    public:
        TTmpFiles() {
            MakeSorterTempl(Templ, nullptr);
        }
        ~TTmpFiles() {
            *strrchr(Templ, LOCSLASH_C) = 0;
            RemoveDirWithContents(Templ);
        }
        const char* GetTempl() const {
            return Templ;
        }
    };

    template <typename TSorter, typename TRecords>
    void PushRecords(TSorter& sorter, TRecords& recs, const int ids[], int& i, int n) {
        for (; i < n; ++i) {
            NMbdbTest::TTestRec& rec = recs[i];
            rec.ID = ids[i];
            sorter.Push(&rec);
        }
    }

    template <typename TSorter>
    void CheckSortedPortion(TSorter& sorter, int nrecs) {
        int n = 0;
        int prevID = 0;
        UNIT_ASSERT(!sorter.Currentp());
        while (sorter.Nextp()) {
            UNIT_ASSERT(sorter.Currentp());
            UNIT_ASSERT(prevID <= sorter.Currentp()->ID);
            prevID = sorter.Currentp()->ID;
            ++n;
        }
        UNIT_ASSERT_VALUES_EQUAL(n, nrecs);
    }

}

namespace NMbdbTest {
    class TMbdbTest2: public TTestBase {
        UNIT_TEST_SUITE(TMbdbTest2);
        UNIT_TEST(TestInputRecordIterator);
        UNIT_TEST(TestInputRecordIterator2);

        UNIT_TEST(TestInputPageIterator_Next);
        UNIT_TEST(TestInputPageIterator_GotoPage);
        UNIT_TEST(TestInputPageIterator_Freeze);

        UNIT_TEST(TestInDatFile);
        UNIT_TEST(TestInDatFile_WrongDatMetaPage);
        UNIT_TEST(TestInDatFile_WrongDatPage);

        UNIT_TEST(TestOutDatFile_NotOpen);
        UNIT_TEST(TestOutDatFile);
        UNIT_TEST(TestOutDatFile_Freeze);

        UNIT_TEST(TestOutIndexFile);
        UNIT_TEST(TestDirectFile);
        UNIT_TEST(TestMappedFile);

        UNIT_TEST(TestHeapIter);
        UNIT_TEST(TestSorter);
        UNIT_TEST(TestSorter_SortPortion);
        UNIT_TEST_SUITE_END();

    public:
        void TestInputRecordIterator();
        void TestInputRecordIterator2();

        void TestInputPageIterator_Next();
        void TestInputPageIterator_GotoPage();
        void TestInputPageIterator_Freeze();

        void TestInDatFile();
        void TestInDatFile_WrongDatMetaPage();
        void TestInDatFile_WrongDatPage();

        void TestOutDatFile_NotOpen();
        void TestOutDatFile();
        void TestOutDatFile_Freeze();

        void TestOutIndexFile();
        void TestDirectFile();
        void TestMappedFile();

        void TestHeapIter();
        void TestSorter();
        void TestSorter_SortPortion();

    private:
        void CheckHeapIter(THeapIter<int, TTestIterator>& heap, size_t totalCount);
        template <typename TSorter>
        void GeneratePortions(TSorter& sorter);
        template <typename TSorter, typename TCmpOp>
        void CheckSortedRecords(TSorter& sorter, TCmpOp op, size_t nrecs);
        template <typename TSorter, typename TCmpOp>
        void TestSorter(TSorter& sorter, const char* templ, int maxPortions, TCmpOp op, size_t nrecs);
    };

    UNIT_TEST_SUITE_REGISTRATION(TMbdbTest2);

    void TMbdbTest2::TestInputRecordIterator() {
        TTestInputRecordIterator it;
        UNIT_ASSERT(!it.Current());
        UNIT_ASSERT(!it.Next());

        int n = 1;
        UNIT_ASSERT(!it.Skip(n));
        n = 0;
        UNIT_ASSERT(!it.Skip(n));
        n = -1;
        //    UNIT_ASSERT(!it.Skip(n)); if n is negative it gets into an infinite loop

        UNIT_ASSERT(!it.GotoPage(1));
    }

    void TMbdbTest2::TestInputRecordIterator2() {
        TTestPage page;
        TTestInputRecordIterator it;
        it.SetDatPage(page.GetDatPage());
        UNIT_ASSERT(!it.Current());

        const NMbdbTest::TTestRec* rec = it.Next();
        UNIT_ASSERT(rec);
        UNIT_ASSERT_VALUES_EQUAL(rec->ID, FIRST_ID);
        rec = it.Next();
        UNIT_ASSERT(rec);
        UNIT_ASSERT_VALUES_EQUAL(rec->ID, FIRST_ID + ID_STEP);

        int n = 0;
        rec = it.Skip(n);
        UNIT_ASSERT(rec);
        UNIT_ASSERT_VALUES_EQUAL(rec->ID, FIRST_ID + ID_STEP);
        n = 2;
        rec = it.Skip(n);
        UNIT_ASSERT(rec);
        UNIT_ASSERT_VALUES_EQUAL(rec->ID, FIRST_ID + ID_STEP * 3);

        rec = it.GotoPage(1); // it does not matter what value is passed into the method
        UNIT_ASSERT(rec);
        UNIT_ASSERT_VALUES_EQUAL(rec->ID, FIRST_ID);
    }

    void TMbdbTest2::TestInputPageIterator_Next() {
        TTestInputPageIterator it;

        UNIT_ASSERT(!it.Current() && it.IsEof());
        UNIT_ASSERT(!it.Next());
        it.Init(3, 1); // 3 pages for buffer
        UNIT_ASSERT(!it.IsEof());

        ui32 n = 1;
        while (it.Next()) {
            TDatPage* page = it.Current();
            UNIT_ASSERT(page);
            UNIT_ASSERT(!it.IsEof());
            UNIT_ASSERT_VALUES_EQUAL(page->Reserved, n); // in this test RecNum is one-based page index
            UNIT_ASSERT_VALUES_EQUAL(page->PageSig, PAGESIG);
            ++n;
        }

        UNIT_ASSERT_VALUES_EQUAL(n, 8u);
        UNIT_ASSERT(it.IsEof());
    }

    void TMbdbTest2::TestInputPageIterator_GotoPage() {
        TTestInputPageIterator it;

        UNIT_ASSERT(!it.GotoPage(-3));
        UNIT_ASSERT(!it.GotoPage(0));
        UNIT_ASSERT(!it.GotoPage(5));
        it.Init(3, 1); // 3 pages for buffer

        TDatPage* page = it.GotoPage(1); // page index is zero based here
        UNIT_ASSERT(page);
        UNIT_ASSERT(!it.IsEof());
        UNIT_ASSERT_VALUES_EQUAL(page->Reserved, 2u); // in this test RecNum is one-based page index
        UNIT_ASSERT_VALUES_EQUAL(page->PageSig, PAGESIG);

        page = it.GotoPage(6);
        UNIT_ASSERT(page);
        UNIT_ASSERT(!it.IsEof());
        UNIT_ASSERT_VALUES_EQUAL(page->Reserved, 7u);
        UNIT_ASSERT_VALUES_EQUAL(page->PageSig, PAGESIG);

        //    page = it.GotoPage(-3);
        //    UNIT_ASSERT(!page); // it should be NULL
        //    UNIT_ASSERT(!it.IsEof());
        //    UNIT_ASSERT_VALUES_EQUAL(it.Current()->Reserved, 7u);
        //    UNIT_ASSERT_VALUES_EQUAL(it.Current()->PageSig, PAGESIG);

        page = it.GotoPage(10000);
        UNIT_ASSERT(!page);
        //    UNIT_ASSERT(!it.IsEof()); actually TInputPageFileImpl<>::GetError() should be an error
        //    UNIT_ASSERT_VALUES_EQUAL(it.Current()->Reserved, 7u);
        //    UNIT_ASSERT_VALUES_EQUAL(it.Current()->PageSig, PAGESIG);
    }

    void TMbdbTest2::TestInputPageIterator_Freeze() {
        TTestInputPageIterator it;
        TDatPage* page = nullptr;

        UNIT_ASSERT(!it.IsFrozen());
        it.Init(3, 1);
        UNIT_ASSERT(!it.IsFrozen());
        it.Freeze();
        UNIT_ASSERT(it.IsFrozen());

        page = it.Next();
        UNIT_ASSERT(page);
        UNIT_ASSERT(!it.IsEof());
        UNIT_ASSERT_VALUES_EQUAL(page->Reserved, 1u);
        UNIT_ASSERT_VALUES_EQUAL(page->PageSig, PAGESIG);

        page = it.GotoPage(4); // does not move to page #4
        UNIT_ASSERT(!page);
        UNIT_ASSERT(!it.IsEof());
        UNIT_ASSERT_VALUES_EQUAL(it.Current()->Reserved, 1u);
        UNIT_ASSERT_VALUES_EQUAL(it.Current()->PageSig, PAGESIG);

        it.Unfreeze();
        UNIT_ASSERT(!it.IsFrozen());

        page = it.GotoPage(4); // moves to page #4
        UNIT_ASSERT(page);
        UNIT_ASSERT(!it.IsEof());
        UNIT_ASSERT_VALUES_EQUAL(page->Reserved, 5u);
        UNIT_ASSERT_VALUES_EQUAL(page->PageSig, PAGESIG);

        it.Freeze();
        page = it.Next();
        UNIT_ASSERT(page);
        UNIT_ASSERT(!it.IsEof());
        UNIT_ASSERT_VALUES_EQUAL(page->Reserved, 6u);
        UNIT_ASSERT_VALUES_EQUAL(page->PageSig, PAGESIG);
        page = it.Next();
        UNIT_ASSERT(page);
        UNIT_ASSERT(!it.IsEof());
        UNIT_ASSERT_VALUES_EQUAL(page->Reserved, 7u);
        UNIT_ASSERT_VALUES_EQUAL(page->PageSig, PAGESIG);
        page = it.Next();
        UNIT_ASSERT(!page);
        UNIT_ASSERT(it.IsEof()); // it was the last page

        it.Unfreeze();

        page = it.GotoPage(1);
        UNIT_ASSERT(page);
        UNIT_ASSERT_VALUES_EQUAL(page->Reserved, 2u);
        UNIT_ASSERT_VALUES_EQUAL(page->PageSig, PAGESIG);

        it.Freeze();

        page = it.Next();
        UNIT_ASSERT(!it.IsEof());
        UNIT_ASSERT_VALUES_EQUAL(page->Reserved, 3u);
        UNIT_ASSERT_VALUES_EQUAL(page->PageSig, PAGESIG);
        page = it.Next();
        UNIT_ASSERT(!it.IsEof());
        UNIT_ASSERT_VALUES_EQUAL(page->Reserved, 4u);
        UNIT_ASSERT_VALUES_EQUAL(page->PageSig, PAGESIG);
        page = it.Next(); // it does not go to the next page
        UNIT_ASSERT(!page);
        UNIT_ASSERT(!it.IsEof()); // it was not the last page

        it.Unfreeze();

        page = it.GotoPage(0);
        ui32 n = 1;
        while (page) {
            UNIT_ASSERT(page);
            UNIT_ASSERT_VALUES_EQUAL(page->Reserved, n);
            UNIT_ASSERT_VALUES_EQUAL(page->PageSig, PAGESIG);
            ++n;

            it.Freeze(); // it is not very well that Freeze updates Frozed to the current page number
            page = it.Next();
        }
        UNIT_ASSERT_VALUES_EQUAL(n, 8u);
    }

    void TMbdbTest2::TestInDatFile() {
        const int recordsPerPage = ((TTestInputFileManip::PageSize - sizeof(TDatPage)) / sizeof(NMbdbTest::TTestRec));
        const NMbdbTest::TTestRec* rec = nullptr;
        TTestInDatFile file; // it mostly tests TInputPageFileImpl<>

        UNIT_ASSERT(!file.IsOpen());
        UNIT_ASSERT(file.Open(TEST_FILE_NAME, 3, 1) == 0);
        UNIT_ASSERT(file.IsOpen());
        UNIT_ASSERT(!file.Current());

        int id = 1;
        while (rec = file.Next()) {
            UNIT_ASSERT(!file.GetError());
            UNIT_ASSERT(rec);
            UNIT_ASSERT_VALUES_EQUAL(rec->ID, id++);
        }
        UNIT_ASSERT(file.IsEof());
        UNIT_ASSERT_VALUES_EQUAL(ui32(id), TTestInputFileManip::MaxID + 1);

        id = 24;
        rec = file.GotoPage(5);
        UNIT_ASSERT(!file.IsEof());
        do {
            UNIT_ASSERT(!file.GetError());
            UNIT_ASSERT(rec);
            UNIT_ASSERT_VALUES_EQUAL(rec->ID, id++);
        } while (rec = file.Next());
        UNIT_ASSERT(file.IsEof());
        UNIT_ASSERT_VALUES_EQUAL(ui32(id), TTestInputFileManip::MaxID + 1);

        int i = 0;
        while (rec = file.GotoPage(i)) {
            UNIT_ASSERT(!file.IsEof());
            UNIT_ASSERT(!file.GetError());
            UNIT_ASSERT(rec);
            UNIT_ASSERT_VALUES_EQUAL(rec->ID, (i < 4 ? recordsPerPage * i + 1 : (i == 4 ? 19 : (i == 5 ? 24 : 29))));
            ++i;
        }
        UNIT_ASSERT(!file.IsEof());
        UNIT_ASSERT(file.GetError() == MBDB_BAD_FILE_SIZE);

        //    {
        //        TTestInDatFile file2;
        //        file2.Open(TEST_FILE_NAME, 3, 1);

        //        rec = file2.GotoPage(0);
        //        UNIT_ASSERT(rec);
        //        UNIT_ASSERT_VALUES_EQUAL(rec->ID, 1);
        //        rec = file2.GotoPage(-3);
        //        UNIT_ASSERT(!rec);
        //        UNIT_ASSERT(file2.GetError() != MBDB_BAD_RECORDSIG); // actually TInputPageFileImpl<TFileManip>::ReadPages should return 0 (no pages were read)
        //    }
    }

    void TMbdbTest2::TestInDatFile_WrongDatMetaPage() {
        TTestInDatFile file;
        TDatMetaPage mp;

        mp.MetaSig = 0; // bad metasig
        mp.PageSize = TTestInputFileManip::PageSize;
        mp.RecordSig = NMbdbTest::TTestRec::RecordSig;
        memcpy(TTestInputFileManip::Buf, &mp, sizeof(mp));

        UNIT_ASSERT(file.Open(TEST_FILE_NAME) == MBDB_BAD_METAPAGE);
        UNIT_ASSERT(file.GetError() == MBDB_BAD_METAPAGE);

        mp.MetaSig = METASIG;
        mp.RecordSig = 0; // bad recordsig
        memcpy(TTestInputFileManip::Buf, &mp, sizeof(mp));

        UNIT_ASSERT(file.Open(TEST_FILE_NAME) == MBDB_BAD_RECORDSIG);
        UNIT_ASSERT(file.GetError() == MBDB_BAD_RECORDSIG);

        mp.PageSize = 0x10000000; // bad pagesize
        mp.RecordSig = NMbdbTest::TTestRec::RecordSig;
        memcpy(TTestInputFileManip::Buf, &mp, sizeof(mp));

        UNIT_ASSERT(file.Open(TEST_FILE_NAME) == MBDB_BAD_FILE_SIZE);
        UNIT_ASSERT(file.GetError() == MBDB_BAD_FILE_SIZE);
    }

    void TMbdbTest2::TestInDatFile_WrongDatPage() {
        TTestInDatFile file;
        TDatPage p;

        p.RecNum = 1;
        p.PageSig = 0; // bad pagesig
        p.Reserved = 0;
        memcpy(TTestInputFileManip::Buf + METASIZE + TTestInputFileManip::PageSize, &p, sizeof(p));

        file.Open(TEST_FILE_NAME);
        file.Next();
        UNIT_ASSERT(file.GetError() == MBDB_BAD_PAGESIG);
    }

    void TMbdbTest2::TestOutDatFile_NotOpen() {
        TTestOutDatFile file;
        UNIT_ASSERT(file.IsEof());

        UNIT_ASSERT(file.Reserve(sizeof(NMbdbTest::TTestRec)) == nullptr);
        UNIT_ASSERT(file.IsEof());
        UNIT_ASSERT(!file.GetError());

        NMbdbTest::TTestRec rec;
        rec.ID = 1;
        UNIT_ASSERT(file.Push(&rec) == nullptr);
        UNIT_ASSERT(file.IsEof());
        UNIT_ASSERT(!file.GetError());

        UNIT_ASSERT(file.GotoPage(1) == MBDB_BAD_FILE_SIZE);
    }

    void TMbdbTest2::TestOutDatFile() {
        {
            TTestOutDatFile file;
            UNIT_ASSERT(file.IsEof());
            UNIT_ASSERT(file.Open(TEST_FILE_NAME, DEFAULT_PAGE_SIZE, 2, 1) == 0);
            UNIT_ASSERT(!file.IsEof());
            ui32 id = 1;
            while (id <= 12) {
                NMbdbTest::TTestRec* rec = file.Reserve(sizeof(NMbdbTest::TTestRec));
                UNIT_ASSERT(rec);
                rec->ID = id++;
                UNIT_ASSERT(!file.IsEof());
                UNIT_ASSERT(!file.GetError());
            }
            while (id <= 24) {
                NMbdbTest::TTestRec rec;
                rec.ID = id++;
                const NMbdbTest::TTestRec* crec = file.Push(&rec);
                UNIT_ASSERT(crec);
                UNIT_ASSERT_VALUES_EQUAL(crec->ID, rec.ID);
                UNIT_ASSERT(!file.IsEof());
                UNIT_ASSERT(!file.GetError());
            }
            file.GotoPage(2);
            while (id <= 32) {
                NMbdbTest::TTestRec rec;
                rec.ID = id++;
                file.Push(&rec);
            }
        }
        {
            const char* p = TTestOutputFileManip::Buf;
            const TDatMetaPage* meta = reinterpret_cast<const TDatMetaPage*>(p);
            UNIT_ASSERT_VALUES_EQUAL(meta->MetaSig, METASIG);
            UNIT_ASSERT_VALUES_EQUAL(meta->RecordSig, NMbdbTest::TTestRec::RecordSig);
            UNIT_ASSERT_VALUES_EQUAL(meta->PageSize, (ui32)TTestOutputFileManip::PageSize);
            p += METASIZE;

            const ui32 recordsPerPage = 5;
            const int ids[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 25, 26, 27, 28, 29, 30, 31, 32, 0, 0, 21, 22, 23, 24, 0};
            ui32 ipage = 0;
            while (ipage < 5) {
                const TDatPage* page = reinterpret_cast<const TDatPage*>(p + TTestOutputFileManip::PageSize * ipage);
                UNIT_ASSERT_VALUES_EQUAL(page->PageSig, PAGESIG);
                ui32 nrecs = (ipage == 3 ? 3 : (ipage == 4 ? 4 : recordsPerPage));
                UNIT_ASSERT_VALUES_EQUAL(page->RecNum, nrecs);

                const NMbdbTest::TTestRec* rec = reinterpret_cast<const NMbdbTest::TTestRec*>(page + 1);
                int i = ipage * recordsPerPage;
                while (nrecs) {
                    UNIT_ASSERT_VALUES_EQUAL(rec->ID, ids[i]);
                    ++i;
                    ++rec;
                    --nrecs;
                }

                ++ipage;
            }
        }
    }

    void TMbdbTest2::TestOutDatFile_Freeze() {
    }

    void TMbdbTest2::TestOutIndexFile() {
    }

    void TMbdbTest2::TestDirectFile() {
    }

    void TMbdbTest2::TestMappedFile() {
    }

    void TMbdbTest2::CheckHeapIter(THeapIter<int, TTestIterator>& heap, size_t totalCount) {
        UNIT_ASSERT(!heap.Current());
        ui32 n = 0;
        int prev = 0;
        const int* p = nullptr;
        while (p = heap.Next()) {
            UNIT_ASSERT(prev <= *p);
            prev = *p;
            ++n;
        }
        UNIT_ASSERT_VALUES_EQUAL(n, totalCount);
    }

    void TMbdbTest2::TestHeapIter() {
        const int arr1[] = {1, 3, 6, 10, 14, 21, 33};
        const int arr2[] = {2, 3, 8, 9, 12, 14, 19, 23, 51, 97};
        const int arr3[] = {5, 7, 8, 11, 16, 19, 21, 31, 43, 51, 69, 77, 81, 91, 95};
        const int arr4[] = {1, 128};
        const int arr5[] = {2, 4, 8, 16, 32, 64, 128, 256};
        {
            THeapIter<int, TTestIterator> heap;
            UNIT_ASSERT(heap.Init((TTestIterator**)nullptr, 0) == 0);
            //UNIT_ASSERT(!heap.Current()); there is no iterators! it will try to dereference unassigned pointer to TIterator
            //UNIT_ASSERT(!heap.Next());
        }
        {
            TTestIterator it(arr1);
            TTestIterator* iters[] = {&it};
            THeapIter<int, TTestIterator> heap;
            UNIT_ASSERT(heap.Init(iters, 1) == 0);
            CheckHeapIter(heap, Y_ARRAY_SIZE(arr1));
        }
        {
            TTestIterator it1(arr1), it2(arr2);
            THeapIter<int, TTestIterator> heap(&it1, &it2);
            CheckHeapIter(heap, Y_ARRAY_SIZE(arr1) + Y_ARRAY_SIZE(arr2));
        }
        {
            TTestIterator it1(arr1), it2(arr2), it3(arr3), it4(arr4), it5(arr5);
            TTestIterator* iters[] = {&it1, &it2, &it3, &it4, &it5};
            THeapIter<int, TTestIterator> heap;
            UNIT_ASSERT(heap.Init(iters, 5) == 0);
            CheckHeapIter(heap, Y_ARRAY_SIZE(arr1) + Y_ARRAY_SIZE(arr2) + Y_ARRAY_SIZE(arr3) + Y_ARRAY_SIZE(arr4) + Y_ARRAY_SIZE(arr5));
        }
    }

    template <typename TSorter>
    void TMbdbTest2::GeneratePortions(TSorter& sorter) {
        static const int ids[140] = {
            18, 20, 22, 50, 17, 39, 44, 60, 36, 90, 31, 22, 35, 39, 27, 44, 20, 33, 82, 64,
            16, 39, 29, 23, 75, 84, 55, 46, 25, 22, 21, 34, 72, 24, 17, 45, 71, 27, 25, 72,
            56, 41, 14, 18, 12, 35, 27, 45, 15, 42, 88, 48, 76, 27, 14, 32, 29, 43, 49, 26,
            20, 33, 71, 23, 16, 44, 70, 26, 24, 71, 47, 38, 13, 31, 25, 91, 99, 20, 41, 32,
            87, 47, 75, 26, 13, 31, 28, 42, 48, 25, 19, 21, 23, 50, 18, 29, 45, 61, 37, 91,
            30, 21, 34, 38, 26, 43, 19, 32, 81, 63, 17, 40, 30, 24, 76, 85, 56, 47, 65, 23,
            46, 37, 12, 30, 24, 90, 99, 19, 40, 31, 57, 42, 15, 19, 12, 36, 28, 46, 16, 43};
        const int portionSize = 7;
        int i = 0;
        for (int portions = 20; portions; --portions) {
            NMbdbTest::TTestRec recs[portionSize]; // all records of the current portion must be in the memory
            for (auto& rec : recs) {
                rec.ID = ids[i++];
                sorter.Push(&rec);
            }
            sorter.NextPortion();
        }
    }

    template <typename TSorter, typename TCmpOp>
    void TMbdbTest2::CheckSortedRecords(TSorter& sorter, TCmpOp op, size_t nrecs) {
        size_t n = 0;
        const NMbdbTest::TTestRec* rec;
        int prevID = 0;
        while (rec = sorter.Next()) {
            UNIT_ASSERT(!sorter.GetError());
            UNIT_ASSERT(op(prevID, rec->ID));
            prevID = rec->ID;
            ++n;
        }
        UNIT_ASSERT_VALUES_EQUAL(n, nrecs);
    }

    template <typename TSorter, typename TCmpOp>
    void TMbdbTest2::TestSorter(TSorter& sorter, const char* templ, int maxPortions, TCmpOp op, size_t nrecs) {
        UNIT_ASSERT(sorter.Open(templ, TTestOutputFileManip::PageSize, 3, 1) == 0);
        GeneratePortions(sorter);
        UNIT_ASSERT(sorter.Sort(TTestOutputFileManip::PageSize * maxPortions, maxPortions) == 0);
        CheckSortedRecords(sorter, op, nrecs);
        UNIT_ASSERT(sorter.Close() == 0);
    }

    void TMbdbTest2::TestSorter() {
        TTmpFiles tmpFiles;

        {
            typedef TDatSorterImpl<NMbdbTest::TTestRec, TTestRecLess, TFakeCompression,
                                   TFakeSieve<NMbdbTest::TTestRec>, TOutputPageFileImpl<TTestOutputFileManip>>
                TSorter;
            TSorter sorter;

            TestSorter(sorter, tmpFiles.GetTempl(), 2, std::less_equal<int>(), 140);
            TestSorter(sorter, tmpFiles.GetTempl(), 22, std::less_equal<int>(), 140);
            TestSorter(sorter, tmpFiles.GetTempl(), 7, std::less_equal<int>(), 140);
            TestSorter(sorter, tmpFiles.GetTempl(), 4, std::less_equal<int>(), 140);
        }
        {
            typedef TDatSorterImpl<NMbdbTest::TTestRec, TTestRecLess, TFakeCompression, TTestSieve, TOutputPageFileImpl<TTestOutputFileManip>> TSorter;
            TSorter sorter;

            TestSorter(sorter, tmpFiles.GetTempl(), 2, std::less<int>(), 61);
            TestSorter(sorter, tmpFiles.GetTempl(), 22, std::less<int>(), 61);
            TestSorter(sorter, tmpFiles.GetTempl(), 7, std::less<int>(), 61);
            TestSorter(sorter, tmpFiles.GetTempl(), 4, std::less<int>(), 61);
        }
    }

    void TMbdbTest2::TestSorter_SortPortion() {
        TTmpFiles tmpFiles;

        const int ids[21] = {
            4, 2, 9, 5, 9, 1, 8,
            1, 6, 7, 6, 1, 3, 8,
            2, 3, 5, 4, 7, 2, 3};

        typedef TDatSorterImpl<NMbdbTest::TTestRec, TTestRecLess, TFakeCompression, TFakeSieve<NMbdbTest::TTestRec>, //TTestSieve,
                               TOutputPageFileImpl<TTestOutputFileManip>>
            TSorter;
        TSorter sorter;
        sorter.Open(tmpFiles.GetTempl(), TTestOutputFileManip::PageSize, 3, 1);

        {
            TVector<NMbdbTest::TTestRec> recs(21);

            int i = 0;
            PushRecords(sorter, recs, ids, i, 7);
            UNIT_ASSERT(sorter.SortPortion() == 0);
            CheckSortedPortion(sorter, 7);

            PushRecords(sorter, recs, ids, i, 14);
            UNIT_ASSERT(sorter.NextPortion() == 0);

            // this portion will be removed by call to Closep()
            PushRecords(sorter, recs, ids, i, 21);
            UNIT_ASSERT(sorter.SortPortion() == 0);

            CheckSortedPortion(sorter, 7);

            sorter.SortPortion();
            UNIT_ASSERT(sorter.Nextp());
            sorter.Closep();
            UNIT_ASSERT(!sorter.Nextp());
            UNIT_ASSERT(!sorter.Currentp());

            i = 14;
            PushRecords(sorter, recs, ids, i, 21);

            UNIT_ASSERT(sorter.Sort(TTestOutputFileManip::PageSize * 3, 3) == 0);

            int n = 0;
            int prevID = 0;
            UNIT_ASSERT(!sorter.Current());
            while (sorter.Next()) {
                const NMbdbTest::TTestRec* rec = sorter.Current();
                UNIT_ASSERT(rec);
                UNIT_ASSERT(prevID <= rec->ID);
                prevID = rec->ID;
                ++n;
            }
            UNIT_ASSERT_VALUES_EQUAL(n, 21);

            UNIT_ASSERT(sorter.Close() == 0);
            UNIT_ASSERT(!sorter.Current());
        }
    }

}
