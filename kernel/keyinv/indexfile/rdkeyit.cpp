#include <cstdlib>

#include <util/generic/list.h>
#include <util/generic/hash_set.h>

#include "rdkeyit.h"

TBufferedHitIterator::TBufferedHitIterator(size_t pageSizeBits)
    : InvFile(nullptr)
    , PageSize(1 << pageSizeBits)
    , PageSizeBits(pageSizeBits)
    , BufferManager(nullptr)
    , PageData(nullptr)
    , Cur(nullptr)
    , Upper(nullptr)
    , End(nullptr)
    , LastStart(0)
    , LastLength(0)
    , LastCount(0)
{
}

void TBufferedHitIterator::InitBuffers(IHitsBufferManager* bufferManager) {
    assert(BufferManagerCookie == nullptr || !BufferManager || !bufferManager || BufferManager == bufferManager);
    BufferManager = bufferManager;
    if (!BufferManager && !PageData) {
        PageData = new char[PageSize];
        *PageData = '\0';
    }
    if (BufferManager && PageData) {
        if (BufferManagerCookie == nullptr) {
            delete[] PageData;
        } else
            BufferManager->ReleasePage(this);
        PageData = nullptr;
    }
}

TBufferedHitIterator::~TBufferedHitIterator() {
    if (BufferManager) {
        if (PageData)
            BufferManager->ReleasePage(this);
    } else {
        delete[] PageData;
    }
}

void TBufferedHitIterator::Init(NIndexerCore::TInputFile& invFile, EHitFormat hitFormat, IHitsBufferManager* bufferManager) {
    InvFile = &invFile;
    Y_ASSERT(InvFile);
    InitBuffers(bufferManager);
    PageEnd = RealPageEnd = PageData + PageSize;
    CurrentPage = EdgeBufferPrevPage = (ui32)-1;
    IsOver = true;
    IsLength64 = false;
    HitDecoder.SetFormat(hitFormat);
}

void TBufferedHitIterator::Restart64(SUPERLONG start, SUPERLONG length, ui32 count) {
    assert(length); //if (!length) return;
    ui32 page = (ui32)(start >> PageSizeBits);
    Cur = PageData + (start & (PageSize - 1));
    if (length < (SUPERLONG)PageSize * 2) {
        End = Cur + (ui32)length;
        IsLength64 = false;
    } else {
        End = Cur + PageSize * 2;
        IsLength64 = true;
    }
    End64 = (uintptr_t)Cur + length;
    CheckPageReal(page);
    IsOver = (Cur >= End);
    HitDecoder.Reset();
    HitDecoder.SetSize(count);
    HitDecoder.ReadHeader(Cur);
    if (count < 8)
        HitDecoder.FallBackDecode(Cur, count);
}


void TBufferedHitIterator::GetPageReal(ui32 page) {
    size_t bytesRead;
    if (BufferManager) {
        char* oldPageData = PageData;
        bytesRead = BufferManager->GetPage(this, InvFile, page, PageSize, PageData);
        ptrdiff_t offset = PageData - oldPageData;
        PageEnd += offset;
        if (IsLength64)
            End64 += offset;
        assert(PageEnd == PageData + PageSize);
        if (!(Cur >= EdgeBuffer && Cur <= EdgeBuffer + PHIT_MAX * 2)) {
            // we are not inside edge buffer, i.e. on the page -> must fix pointers
            Cur   += offset;
            Upper += offset;
            End   += offset;
        }
    } else {
        InvFile->Seek(page * (SUPERLONG)PageSize, SEEK_SET);
        bytesRead = InvFile->Read(PageData, PageSize);
    }
    RealPageEnd = PageData + bytesRead;
    CurrentPage = page;
}

void TBufferedHitIterator::CheckPageReal(ui32 page) {
    assert((Cur >= PageData && Cur <= PageEnd) || (Cur >= EdgeBuffer && Cur <= EdgeBuffer + PHIT_MAX * 2)); // sanity check
    if (Cur >= EdgeBuffer && Cur <= EdgeBuffer + PHIT_MAX * 2) {
        // (1) we are inside EdgeBuffer -> just leave it
        Cur += PageData - (EdgeBuffer + PHIT_MAX);
        End += PageData - (EdgeBuffer + PHIT_MAX);
        if (IsLength64) {
            End64 -= PageSize;
            End += PageSize;
            if (End64 <= (uintptr_t)End) {
                End = (char*)(uintptr_t)End64;
                IsLength64 = false;
            }
        }
        if (CurrentPage != EdgeBufferPrevPage + 1)
            GetPageReal(EdgeBufferPrevPage + 1);
        Upper = End;
        assert(RealPageEnd == PageEnd || Upper <= RealPageEnd); // broken index?
        // later hit may cross page boundary
        if (Upper > PageEnd)
            Upper = PageEnd - PHIT_MAX;
    } else if (Cur >= PageEnd - PHIT_MAX && End > PageEnd) {
        // (2) any hit here can cross page boundary
        Cur += EdgeBuffer - (PageEnd - PHIT_MAX);
        End += EdgeBuffer - (PageEnd - PHIT_MAX);
        if (page != EdgeBufferPrevPage)
            FillEdgeBuffer(page);
        Upper = End;
        if (Upper > EdgeBuffer + PHIT_MAX * 2)
            Upper = EdgeBuffer + PHIT_MAX;
    } else {
        // (3) first hit can't cross page boundary
        if (CurrentPage != page)
            GetPageReal(page);
        Upper = End;
        assert(RealPageEnd == PageEnd || Upper <= RealPageEnd); // broken index?
        // later hit may cross page boundary
        if (Upper > PageEnd)
            Upper = PageEnd - PHIT_MAX;
    }
}

void TBufferedHitIterator::CheckPage() {
    CheckPageReal(CurrentPage);
}

void TBufferedHitIterator::FillEdgeBuffer(ui32 page) {
    assert(page != (ui32)-1);
    if (page == CurrentPage) { // usual case
        assert(PageEnd >= PageData + PHIT_MAX); // might throw exception instead
        memcpy(EdgeBuffer, PageEnd - PHIT_MAX, PHIT_MAX);
        GetPageReal(page + 1);
        memcpy(EdgeBuffer + PHIT_MAX, PageData, PHIT_MAX);
    } else if (page + 1 == CurrentPage) { // strange case, yet may happen
        memcpy(EdgeBuffer + PHIT_MAX, PageData, PHIT_MAX);
        GetPageReal(page);
        memcpy(EdgeBuffer, PageEnd - PHIT_MAX, PHIT_MAX);
    } else { // total miss
        GetPageReal(page);
        memcpy(EdgeBuffer, PageEnd - PHIT_MAX, PHIT_MAX);
        GetPageReal(page + 1);
        memcpy(EdgeBuffer + PHIT_MAX, PageData, PHIT_MAX);
    }
    EdgeBufferPrevPage = page;
}

ui8* TBufferedHitIterator::NextSaved() {
    if (Y_UNLIKELY(Cur >= Upper)) {
        if (Cur >= End) {
            IsOver = true;
            return nullptr;
        }
        CheckPage();
    }
    ui8* prevCur = (ui8*)Cur;
    DoNext();
    return prevCur;
}

