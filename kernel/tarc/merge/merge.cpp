#include "merge.h"

#include <util/generic/noncopyable.h>
#include <kernel/tarc/iface/tarcio.h>
#include <kernel/tarc/iface/arcface.h>
#include <util/system/defaults.h>
#include <util/stream/file.h>
#include <util/draft/holder_vector.h>

#include <library/cpp/deprecated/mbitmap/mbitmap.h>
#include <library/cpp/containers/mh_heap/mh_heap.h>

class TArchiveIteratorForMerge : TNonCopyable {
private:
    MERGE_MODE Defmode;
    const bitmap_2* AddDelDocuments;
    ui32 Delta;
    TArchiveIterator *ArchIn;
    int NumArch;
    int OutputDoc;
    TArchiveHeader* CurHdr;
public:
    TArchiveIteratorForMerge(TArchiveIterator* in, int num, MERGE_MODE defmode, const bitmap_2* addDelDocuments, ui32 delta)
        : Defmode(defmode)
        , AddDelDocuments(addDelDocuments)
        , Delta(delta)
        , ArchIn(in)
        , NumArch(num)
        , OutputDoc(-1)
        , CurHdr(nullptr)
    {
    }

    ~TArchiveIteratorForMerge() {
        delete ArchIn;
    }

    typedef int value_type;

    TArchiveHeader* GetArcHdr() {
        return CurHdr;
    }

    const TArchiveHeader* GetArcHdr() const {
        return CurHdr;
    }

    void Next() {
        do {
            CurHdr = ArchIn->NextAuto();
            if (!Valid())
                break;
            OutputDoc = CurHdr->DocId;
            if (NumArch > 0)
                OutputDoc = CurHdr->DocId + Delta;

            if (AddDelDocuments != nullptr) {
                MERGE_MODE mm = (MERGE_MODE)AddDelDocuments->test(CurHdr->DocId);
                if (mm == MM_DEFAULT)
                    mm = Defmode;
                if (mm == MM_DELETE || mm == MM_REPLACE && NumArch == 0)
                    continue; // skip
            }
            break;
        } while (1);
   }

    bool Valid() const {
        return CurHdr != nullptr;
    }

    int Current() const {
        return OutputDoc;
    }

    void operator ++() {
        Next();
    }

    void Restart() const {
        return;
    }

    void ReplaceDoc() {
        Y_ASSERT(Valid());
        CurHdr->DocId = OutputDoc;
    }
};

struct TArcLess {
    bool operator()(const TArchiveIteratorForMerge* x, const TArchiveIteratorForMerge* y) const {
        return x->GetArcHdr()->DocId < y->GetArcHdr()->DocId;
    }
};

void MergeArchives(const char* suffix, const TVector<TString>& newNames, const TString& oldName, const TString& outName,
                   MERGE_MODE  defmode, const bitmap_2* addDelDocuments, ui32 delta, TArchiveVersion archiveVersion,
                   const ui32* remap, ui32 remapSize)
{
    TFixedBufferFileOutput archOut(outName + suffix);
    WriteTextArchiveHeader(archOut, archiveVersion);

    THolderVector<TArchiveIteratorForMerge> inputs;

    THolder<TArchiveIterator> iterHolder;

    iterHolder.Reset(new TArchiveIterator);
    if (!!oldName) {
        iterHolder->Open((oldName + suffix).data());
        VerifyArchiveVersion(*iterHolder, archiveVersion);
    }

    inputs.PushBack(new TArchiveIteratorForMerge(iterHolder.Get(), 0, defmode, addDelDocuments, delta));
    Y_UNUSED(iterHolder.Release());

    for (size_t i = 0; i < newNames.size(); i++) {
        iterHolder.Reset(new TArchiveIterator);
        iterHolder->Open((newNames[i] + suffix).data());
        VerifyArchiveVersion(*iterHolder, archiveVersion);
        inputs.PushBack(new TArchiveIteratorForMerge(iterHolder.Get(), int(i+1), defmode, addDelDocuments, delta));
        Y_UNUSED(iterHolder.Release());
    }

    for (TVector<TArchiveIteratorForMerge*>::iterator i = inputs.begin(); i != inputs.end(); ++i) {
        (*i)->Next();
    }
    MultHitHeap<TArchiveIteratorForMerge, TArcLess> it(inputs.data(), (ui32)inputs.size());
    for (it.Restart(); it.Valid(); ++it) {
        TArchiveIteratorForMerge* top = it.TopIter();
        top->ReplaceDoc();
        TArchiveHeader* hdr = top->GetArcHdr();
        if (remap && hdr->DocId < remapSize) {
            const ui32 newDocId = remap[hdr->DocId];
            if (newDocId == (ui32)-1) // means deleted document
                continue;
            hdr->DocId = newDocId;
        }
        archOut.Write(hdr, hdr->DocLen);
    }

}
