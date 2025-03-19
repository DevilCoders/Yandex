#pragma once

//#include <unistd.h>

#include "lfproc.h"
#include "mtsort.h"

#include <kernel/keyinv/indexfile/indexfile.h>
#include <kernel/keyinv/indexfile/indexwriter.h>
#include <kernel/keyinv/indexfile/memoryportion.h>

#include <library/cpp/sorter/sorter.h>
#include <library/cpp/sorter/filesortst.h>
#include <library/cpp/sorter/buffile.h>
#include <library/cpp/sorter/pipe.h>

#include <util/generic/string.h>
#include <util/memory/segmented_string_pool.h>
#include <util/stream/output.h>
#include <util/system/filemap.h>
#include <util/system/tempfile.h>

struct PacketReadHitIterator {
    typedef SUPERLONG value_type;
    const SUPERLONG *cur, *upper;
    /*bool*/ int upper_is_end;

    TNumFormArray* forms_map; // helper for forms processing

    bool Next() {
        cur++;
        if (Y_UNLIKELY(cur >= upper)) {
            if (upper_is_end)
                return false;
            Draw();
        }
        return true;
    }
    void operator ++() {
        Next();
    }
    bool Valid() const {
        return cur < upper || !upper_is_end;
    }
    inline SUPERLONG Current() const { return *cur; }
    inline SUPERLONG operator *() { return *cur; }

    void Restart() {}

    virtual void Draw() = 0;
    virtual ~PacketReadHitIterator() {}
};

struct PRHI_FileOrMem : public PacketReadHitIterator {
    ui32 start0;

    void RestartMem(SUPERLONG *buf, size_t start, size_t end) {
        cur = buf + start;
        buf_start = (SUPERLONG*)cur;
        upper = buf + end;
        upper_is_end = true;
        f = nullptr;
    }

    FILE *f;
    SUPERLONG *buf_start;
    size_t buf_len;
    size_t f_start, f_end;
    void RestartFile(FILE *ff, SUPERLONG *cbuf, size_t cbuf_len, size_t start, size_t end) {
        start0 = start;
        this->f = ff;
        f_start = start, f_end = end;
        buf_start = cbuf, buf_len = cbuf_len;
        cur = upper = cbuf + cbuf_len;
        Draw();
    }
    void Draw() override;
    void Restart() {
        if (f) {
            f_start = start0;
            cur = upper = buf_start + buf_len;
            Draw();
        }
        else
            cur = buf_start;
    }

    //PRHI_FileOrMem();
};

struct KeyAndWBLIter {
    TNumFormArray forms_map; // helper for forms processing
    const char *key;
    ui32 offset;
    ui32 sorted_count;
//    TVector<ui32> offsets;

//    ui32 form_count[16];

    KeyAndWBLIter(const char *key_, ui32 offset_) : key(key_), offset(offset_), sorted_count(0) {
//        memset(form_count, 0, sizeof(ui32) * 16);
    }
};

template<class T>
class TMappedArrayN : public TMappedAllocation {
public:
    TMappedArrayN(size_t siz = 0)
      : TMappedAllocation(0) {
       if (siz)
          Create(siz);
    }

    T *Create(size_t siz) {
       Y_ASSERT(MappedSize() == 0 && Ptr() == nullptr);
       T* arr = (T*)Alloc((sizeof(T) * siz));
       if (!arr)
         return nullptr;
       Y_ASSERT(MappedSize() == sizeof(T) * siz);
       return arr;
    }

    void Destroy() {
       T* arr = (T*)Ptr();
       if (arr)
          Dealloc();
    }

    T &operator[](size_t pos) {
       return ((T*)Ptr())[pos];
    }

    ~TMappedArrayN() {
       Destroy();
    }
};

struct keyno_and_wpos {
   ui32 keyno;
   SUPERLONG pos;
   keyno_and_wpos() {}
   keyno_and_wpos(ui32 keyno_, SUPERLONG pos_) : keyno(keyno_), pos(pos_) {}
   bool operator <(const keyno_and_wpos &w) const { return keyno < w.keyno || (keyno == w.keyno && pos < w.pos); }
};
/*
class TWPosSorter : public NSorter::TSorter<keyno_and_wpos> {
public:
    TMutex* Mutex;

public:
    TWPosSorter(size_t size)
        : NSorter::TSorter<keyno_and_wpos>(size)
        , Mutex(nullptr)
    {
    }

    ~TWPosSorter() override {
        Restart();
    }

    void Restart() override {
        if (Mutex)
            Mutex->Acquire();
        NSorter::TSorter<keyno_and_wpos>::Restart();
        if (Mutex)
            Mutex->Release();
    }

    void WritePortion() override {
        if (Mutex)
            Mutex->Acquire();
        NSorter::TSorter<keyno_and_wpos>::WritePortion();
        if (Mutex)
            Mutex->Release();
    }
};
*/
template<class OutputIndexFile, class InvKeyWriter>
void* WriteInd(void* arg);

class TWPosSorter : public CFileSortST<keyno_and_wpos, CBufferedReadFile<keyno_and_wpos> > {
public:
    virtual void WritePiece(TFile& f) {
        CFileSortST<keyno_and_wpos, CBufferedReadFile<keyno_and_wpos> >::WritePiece(f);
        unlink(f.GetName().data());
    }
};

template<class OutputIndexFile = NIndexerCore::TOutputIndexFile, class InvKeyWriter = NIndexerCore::TInvKeyWriter>
class YxFileWBLTmpl {
protected:
    enum { POS_BUF_SZ = 1024 * 1024 };

    TString keyname, invname;
    segmented_string_pool kpool;
    TVector<KeyAndWBLIter> keys;
    TVector<PRHI_FileOrMem> iters4heap;
    TVector<PacketReadHitIterator*> iters;
    TVector<ui32> offsets;
    const char *curkey;
    TNumFormArray cur_froms_map;
    ui64 FormsTotal;
    ui64 FormsNZ;
    ui64 ExternalSortHits;
    ui32 ExternalSortTime;
    ui32 ExternalSorts;
    ui32 FormCounts[N_MAX_FORMS_PER_KISHKA];
    TWPosSorter srt;
//    CFileSortST<keyno_and_wpos, CBufferedReadFile<keyno_and_wpos> > srt;
    TMultPipeTmpl<SUPERLONG> Pipe;
    THolder<TThread> Thread;
    bool Thr;
    bool no_forms;
    ui32 cur_offset;
    ui32 cur_off;
    ui32 buf_offs;
    ui32 buf_start_key;
    ui32 sorted_pieces;
//    ui32* cur_forms_count;

    TVector<SUPERLONG> pos_buffer_a;
    TMappedArrayN<SUPERLONG> pos_buffer_m;
    SUPERLONG *pos_buffer;
    size_t pos_buffer_size;
    size_t pos_buffer_size_realloc;

    FILE *f_buf_tmp;      // Single tmp file for all data
    TString tmp_file_name; // Name of the above file
    const bool need_sort;       // We get non-sorted hits on input, sort them internally
    const bool form_pos_uniq;   // For most key types, remove duplicate hits
    bool form_pos_uniq_eff;

    NIndexerCore::TLemmaAndFormsProcessor lem;
    TVector<int> remapv;

    int verbose;

    OutputIndexFile IdxFile;
    InvKeyWriter Writer;

    float Weight = 0.;
    float CurWeight = 0.;
    THolder<IOutputStream> WeightOutput;

    TMutex* TmpMutex;
    TSortThreadList<SUPERLONG*> SortThreadList;
    bool MtSort;

public:
    enum {
        UseMmap = 1,
        NeedSort = 2,
        FormPosUnic = 4,
        NoSubIndex = 8,
        Verbose = 16,
        StripKeys = 32,
        RawKeys = 64
    };
    YxFileWBLTmpl(size_t bufsize = POS_BUF_SZ, int flags = 0, ui32 version = YNDEX_VERSION_FINAL_DEFAULT, size_t bufsizer = 0, bool useHash = false, bool thr = false)
        : kpool(16 * 1024)
        , curkey(nullptr)
        , FormsTotal(0)
        , FormsNZ(0)
        , ExternalSortHits(0)
        , ExternalSortTime(0)
        , ExternalSorts(0)
        , Pipe(0x100000, 4)
        , Thr(thr)
        , no_forms(true)
        , cur_offset(0)
        , cur_off(0)
        , buf_offs(0)
        , buf_start_key(0)
        , sorted_pieces(0)
        , pos_buffer_a(flags & UseMmap ? 0 : bufsize)
        , pos_buffer_m(flags & UseMmap ? bufsize : 0)
        , f_buf_tmp(nullptr)
        , need_sort((flags & NeedSort) != 0)
        , form_pos_uniq((flags & FormPosUnic) != 0)
        , lem((flags & RawKeys) != 0 || (version & YNDEX_VERSION_MASK) == YNDEX_VERSION_RAW64_HITS, (flags & StripKeys) != 0, useHash)
        , verbose((flags & Verbose) != 0)
        , IdxFile(IYndexStorage::FINAL_FORMAT, version)
        , Writer(IdxFile, (flags & NoSubIndex) == 0)
        , TmpMutex(nullptr)
        , MtSort(false)
    {
        pos_buffer = flags & UseMmap ? &pos_buffer_m[0] : pos_buffer_a.data();
        pos_buffer_size = bufsize;
        pos_buffer_size_realloc = bufsizer == 0 ? bufsize : bufsizer;
        form_pos_uniq_eff = false;
        memset(cur_froms_map, 0, sizeof(TNumFormArray));
        memset(FormCounts, 0, sizeof(FormCounts));
        if (thr) {
            Pipe.Init();
            Thread.Reset(new TThread(::WriteInd<OutputIndexFile, InvKeyWriter>, this));
            Thread->Start();
        }
//        srt.Mutex = TmpMutex;
    }
    ~YxFileWBLTmpl() {
        if (f_buf_tmp)
            Close();
        if (MtSort)
            SortThreadList.StopThreads();
        CloseWeight();
    }
    void SetTmpMutex(TMutex* mutex) {
        TmpMutex = mutex;
    }
    void SetWriterMutex(TMutex* mutex) {
        IdxFile.SetMutex(mutex);
    }
    void InitSortThreads(int count) {
        SortThreadList.InitThreads(count);
        MtSort = true;
    }
    void SortMt(SUPERLONG* start, SUPERLONG* finish) {
        if (!MtSort || (finish - start < 0x40000))
            Sort(start, finish);
        else {
            SortCtx<SUPERLONG*>* ctx = new SortCtx<SUPERLONG*>;
            ctx->first = start;
            ctx->last = finish;
            ctx->depth = 0;
            ctx->Next = nullptr;
            SortThreadList.RunTask(ctx);
            SortThreadList.Ev.Wait();
        }
    }
    void Open(const char *prefix, const char *suffix, const char *tmpdir, bool directIO = false, int key_buf_size = NIndexerCore::INIT_FILE_BUF_SIZE, int inv_buf_size = NIndexerCore::INIT_FILE_BUF_SIZE) {//, bool SubIndex) {
        keyname = TString(prefix) + NIndexerCore::KEY_SUFFIX;
        invname = TString(prefix) + NIndexerCore::INV_SUFFIX;
        IdxFile.Open(keyname.data(), invname.data(), key_buf_size, inv_buf_size, directIO);
        curkey = nullptr;
        cur_off = 0, cur_offset = 0, buf_offs = 0, sorted_pieces = 0;
        if (tmpdir && *tmpdir) {
            tmp_file_name = MakeTempName(tmpdir, "mergetmp-");
        } else {
            tmp_file_name = TString(prefix) + "-mergetmp" + suffix;
        }
        f_buf_tmp = fopen(tmp_file_name.data(), "wb+");
        if (!f_buf_tmp)
            err(1, "%s", tmp_file_name.data());
        unlink(tmp_file_name.data());
        buf_start_key = 0;
        TString tmp_file_name2 = tmp_file_name + "-2pass";
        srt.Open(tmp_file_name2.data(), 0x100000);
    }

    void OpenWeight(const TString& weightFileName) {
        WeightOutput = THolder<IOutputStream>(new TFileOutput(weightFileName));
    }

    void Close() {
        if (keys.size()) {
            UpdateForms();
            FlushLemma();
        }
        if (Thr) {
            Pipe.PutPtr() = (SUPERLONG)-1;
            Pipe.Switch();
            Thread->Join();
            Thread.Destroy();
        }
        IdxFile.CloseEx();
        fclose(f_buf_tmp);
        f_buf_tmp = nullptr;
        if (TmpMutex)
            TmpMutex->Acquire();
        remove(tmp_file_name.data());
        if (TmpMutex)
            TmpMutex->Release();
    }
    void CloseWeight() {
        if (WeightOutput) {
            WeightOutput->Flush();
            WeightOutput.Destroy();
        }
    }
    // Be aware! This method trusts that data to which pointer key points will not change until
    // YxFileWBL object is alive.
    void SetKey(const char *key) {
        curkey = key;
    }
    void SetWeight(float weight) {
        CurWeight = weight;
    }
    void AddHit(SUPERLONG HitPos) {
        if (curkey)
            StartKey();
        if (!no_forms) {
            int form;
//            int dst_form = cur_froms_map[form = TWordPosition::Form(HitPos)];
            FormCounts[form = TWordPosition::Form(HitPos)]++;
//            Y_ASSERT(dst_form >= 0 && dst_form < (int)lem.GetTotalFormCount());
//            lem.IncreaseFormCount(dst_form);
//            cur_forms_count[form]++;
        }
        pos_buffer[buf_offs++] = HitPos;
        if (buf_offs == pos_buffer_size) {
            if (need_sort)
                UpdateForms();
            DumpBuf();
        }
    }
    void AddHits(SUPERLONG* HitPos, ui32 count) {
        do {
            if (curkey)
                StartKey();
            ui32 left = pos_buffer_size - buf_offs;
            ui32 cnt = std::min(count, left);
            memcpy(&pos_buffer[buf_offs], HitPos, cnt * sizeof(SUPERLONG));
            buf_offs += cnt;
            if (!no_forms) {
                for (ui32 i = 0; i < cnt; i++)
                    FormCounts[TWordPosition::Form(HitPos[i])]++;
            }
            if (buf_offs == pos_buffer_size) {
                if (need_sort)
                    UpdateForms();
                DumpBuf();
            }
            HitPos += cnt;
            count -= cnt;
        } while (count > 0);
    }

    void AddSortedHits(SUPERLONG* HitPos, ui32 count) {
        do {
            if (curkey)
                StartKey();
            ui32 left = pos_buffer_size - buf_offs;
            ui32 cnt = std::min(count, left);
            memcpy(&pos_buffer[buf_offs], HitPos, cnt * sizeof(SUPERLONG));
            buf_offs += cnt;
            if (!no_forms) {
                for (ui32 i = 0; i < cnt; i++)
                    FormCounts[TWordPosition::Form(HitPos[i])]++;
            }
            offsets.push_back(cur_offset + buf_offs);
            keys.back().sorted_count++;
            sorted_pieces++;
            if (buf_offs == pos_buffer_size) {
                if (need_sort)
                    UpdateForms();
                DumpBuf();
            }
            HitPos += cnt;
            count -= cnt;
        } while (count > 0);
    }

    void WriteInd() {
        while (true) {
            SUPERLONG* buf;
            SUPERLONG* ebuf;
            Pipe.Wait();
            ui32 avail = Pipe.GetBufs(buf);
            ebuf = buf + avail;
            while (buf < ebuf) {
                if (Y_LIKELY((ui64)*buf < (ui64)-2)) {
                    Writer.WriteHit(*buf++);
                }
                else {
                    if (Y_UNLIKELY((ui64)*buf == (ui64)-1)) {
                        return;
                    }
                    ++buf;
                    SUPERLONG len = *buf++;
                    Writer.WriteKey((const char*)buf);
                    buf += len;
                }
            }
        }
    }

private:
    void UpdateForms() {
        TNumFormArray& map = keys.back().forms_map;
        ui32 formCount = lem.GetCurFormCount();
        FormsTotal += formCount;
        for (ui32 i = 0; i < formCount; i++) {
            if (FormCounts[i]) {
                FormsNZ++;
                lem.IncreaseFormCount(map[i] = lem.FindFormIndex(i), FormCounts[i]);
                FormCounts[i] = 0;
            }
            else
                map[i] = 0;
        }
        for (ui32 i = formCount; i < N_MAX_FORMS_PER_KISHKA; i++) {
            Y_VERIFY(FormCounts[i] == 0);
        }
    }

public:
    void StartKey() {
        if (!keys.empty())
            UpdateForms();
        if (lem.ProcessNextKey(curkey))
            FlushLemma();
//        lem.FindFormIndexes(cur_froms_map);
        no_forms = lem.GetCurFormCount() == 0;
        if (!keys.empty() && keys.back().offset == cur_offset + buf_offs)
            keys.back().key = kpool.append(curkey);
        else {
            keys.push_back(KeyAndWBLIter(kpool.append(curkey), cur_offset + buf_offs));
//            cur_forms_count = keys.back().form_count;
        }
        curkey = nullptr;
        Weight = Max<float>(CurWeight, Weight);
        CurWeight = 0.;
    }

private:
    void SaveFormPosCollision(SUPERLONG prev_hit, TVector<int> &dforms);
    void FlushLemma();
    void DumpBuf();
    void DumpPrev(ui32& prevOff, ui32 offset);
    void DumpSorted(TVector<ui32>::iterator& offset, ui32 prevOff, ui32 offset_count);
    inline void WriteHitThr(SUPERLONG hit);
    inline void WriteKeyThr(const char* key);
};

class YxFileWBL : public YxFileWBLTmpl<NIndexerCore::TOutputIndexFile, NIndexerCore::TInvKeyWriter>
{
public:
    YxFileWBL(size_t bufsize = POS_BUF_SZ, int flags = 0, ui32 version = YNDEX_VERSION_FINAL_DEFAULT, size_t bufsizer = 0, bool useHash = false, bool thr = false)
        : YxFileWBLTmpl<NIndexerCore::TOutputIndexFile, NIndexerCore::TInvKeyWriter>(bufsize, flags, version, bufsizer, useHash, thr)
    {
    }
};

class TStreams {
public:
    NIndexerCore::NIndexerDetail::TOutputMemoryStream KeyStreamX;
    NIndexerCore::NIndexerDetail::TOutputMemoryStream InvStreamX;
};

class TMemoryOutputIndexFile : public TStreams, public NIndexerCore::TOutputIndexFileImpl<NIndexerCore::NIndexerDetail::TOutputMemoryStream> {
public:
    TMemoryOutputIndexFile(IYndexStorage::FORMAT format, ui32 version = YNDEX_VERSION_CURRENT)
        : NIndexerCore::TOutputIndexFileImpl<NIndexerCore::NIndexerDetail::TOutputMemoryStream>(KeyStreamX, InvStreamX, format, version)
    {
    }

    void CloseEx() {
        Flush();
        NIndexerCore::WriteVersionData(InvStreamX, Version);
        NIndexerCore::WriteIndexStat(InvStreamX, true, 0, 0, 1);
        Close();
    }
};

using TMemoryInvKeyWriter = NIndexerCore::TInvKeyWriterImpl<
    NIndexerCore::TOutputIndexFileImpl<NIndexerCore::NIndexerDetail::TOutputMemoryStream>, NIndexerCore::THitWriterImpl<CHitCoder, NIndexerCore::NIndexerDetail::TOutputMemoryStream>,
    NIndexerCore::NIndexerDetail::TSubIndexWriter, NIndexerCore::NIndexerDetail::TFastAccessTableWriter>;

class YxFileWBLMemory : public YxFileWBLTmpl<TMemoryOutputIndexFile, TMemoryInvKeyWriter> {
public:
    YxFileWBLMemory(size_t bufsize = POS_BUF_SZ, int flags = 0, ui32 version = YNDEX_VERSION_FINAL_DEFAULT, size_t bufsizer = 0, bool useHash = false, bool thr = false)
        : YxFileWBLTmpl<TMemoryOutputIndexFile, TMemoryInvKeyWriter>(bufsize, flags, version, bufsizer, useHash, thr)
    {
    }

    void Open(ui32 pass2size = 0x1000000) {
        curkey = nullptr;
        cur_off = 0, cur_offset = 0, buf_offs = 0, sorted_pieces = 0;
        TString suffix = ToString(getpid());
        tmp_file_name = "mergetmp" + suffix;
        f_buf_tmp = fopen(tmp_file_name.data(), "wb+");
        if (!f_buf_tmp)
            err(1, "%s", tmp_file_name.data());
        unlink(tmp_file_name.data());
        buf_start_key = 0;
        TString tmp_file_name2 = tmp_file_name + "-2pass";
        srt.Open(tmp_file_name2.data(), pass2size);
    }

    const TBuffer& GetKeyBuffer() {
        return IdxFile.KeyStreamX.GetBuffer();
    }

    const TBuffer& GetInvBuffer() {
        return IdxFile.InvStreamX.GetBuffer();
    }
};

template<class OutputIndexFile, class InvKeyWriter>
inline void* WriteInd(void* arg) {
    TThread::SetCurrentThreadName("Index writer");
    YxFileWBLTmpl<OutputIndexFile, InvKeyWriter>* yx = (YxFileWBLTmpl<OutputIndexFile, InvKeyWriter>*)arg;
    yx->WriteInd();
    return nullptr;
}
