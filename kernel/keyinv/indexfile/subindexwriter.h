#pragma once

#include <util/system/defaults.h>
#include <util/system/file.h>
#include <util/generic/vector.h>
#include <util/generic/noncopyable.h>
#include <kernel/keyinv/hitlist/subindex.h>
#include "indexfile.h"

namespace NIndexerCore::NIndexerDetail {

//! @note works with TOutputIndexFileImpl<TFile> only
class TSubIndexWriter : private TNonCopyable {
private:
    // all compiler-generated constructors and operators are ok!
    enum EConst {
        N_BUFFER_SIZE = 128 * 1024
    };
    TVector<YxPerst*> blocks;
    YxPerst *pCurrent, *pEnd;
    i64 Offset;

    size_t GetPerstBlockCount() const {
        return blocks.size();
    }
    void GetPerstData(size_t nBlock, YxPerst **ppStart, YxPerst **ppEnd) {
        *ppStart = blocks[nBlock];
        if (nBlock == blocks.size() - 1)
            *ppEnd = pCurrent;
        else
            *ppEnd = *ppStart + N_BUFFER_SIZE;
    }
public:
    TSubIndexWriter()
      : pCurrent(nullptr)
      , pEnd(nullptr)
      , Offset(0)
    {}
    ~TSubIndexWriter() {
        for (size_t i = 0; i < blocks.size(); ++i)
            delete[] blocks[i];
    }
    void ClearSubIndex() {
        if (blocks.empty())
            return;
        for (size_t i = 1; i < blocks.size(); ++i)
            delete[] blocks[i];
        blocks.resize(1);
        pCurrent = blocks[0];
        pEnd = pCurrent + N_BUFFER_SIZE;
    }
    void AddPerst(SUPERLONG pos, ui32 len) {
        AddPerst(GetPerst(pos, len));
    }
private:
    void AddPerst(const YxPerst &a) {
        if (pCurrent == pEnd)
            AddPerstBlock();
        *pCurrent++ = a;
    }
    void AddPerstBlock() {
        YxPerst *p = new YxPerst[N_BUFFER_SIZE];
        blocks.push_back(p);
        pCurrent = p;
        pEnd = pCurrent + N_BUFFER_SIZE;
    }

public:
    template<class TStream>
    void WriteSubIndex(TOutputIndexFileImpl<TStream>& file, const TSubIndexInfo &Info, SUPERLONG offset) {
        static char align_buffer[sizeof(YxPerst)];
        AddPerst(YxPerst::NullSubIndex);
        size_t forWrite = (unsigned int)perstAlign(offset);
        if (0 != forWrite) {
            file.WriteInvData(align_buffer, forWrite);
        }
        if (sizeof(YxPerst) == Info.nPerstSize) {
            for (size_t nBlk = 0; nBlk < GetPerstBlockCount(); ++nBlk) {
                YxPerst *pStart2, *pEnd2;
                GetPerstData(nBlk, &pStart2, &pEnd2);
                file.WriteInvData((const char *)pStart2, (pEnd2 - pStart2)*sizeof(YxPerst));
            }
        } else {
            assert((int)sizeof(YxPerst) < Info.nPerstSize);
            for (size_t nBlk = 0; nBlk < GetPerstBlockCount(); ++nBlk) {
                YxPerst *pStart3, *pEnd3;
                GetPerstData(nBlk, &pStart3, &pEnd3);
                for (YxPerst *pCurrent3 = pStart3; pCurrent3 < pEnd3; ++pCurrent3) {
                    const YxPerst &perst = *pCurrent3;
                    SUPERLONG Sum = perst.Sum;
                    ui32 Off = perst.Off;
                    file.WriteInvData((const char*)&Sum, sizeof(Sum)); // @todo replace with WriteInvData(i64)
                    file.WriteInvData((const char*)&Off, sizeof(Off)); // @todo replace with WriteInvData(ui32)
                    SUPERLONG junk = 0;
                    int nJunkLength = Info.nPerstSize - sizeof(YxPerst);
                    assert(nJunkLength <= (int)sizeof(junk));
                    file.WriteInvData((const char*)&junk, nJunkLength);
                }
            }
        }
    }
    //! @param count     number of hits
    //! @param length    length of hits
    template<class TStream>
    void WriteSubIndex(TOutputIndexFileImpl<TStream>& file, i64 count, ui32 length, const TSubIndexInfo& subIndexInfo) {
        if (needSubIndex(count, subIndexInfo)) {
            WriteSubIndex(file, subIndexInfo, Offset + length);
            Offset += hitsSize(Offset, length, count, subIndexInfo);
        }
    }
    //! @note for diagnostic purposes only
    i64 GetOffset() const {
        return Offset;
    }
};

class TNoSubIndexWriter {
public:
    void ClearSubIndex() {
    }
    void AddPerst(SUPERLONG, ui32) {
    }
    template <typename TStream>
    void WriteSubIndex(TOutputIndexFileImpl<TStream>&, i64, ui32, const TSubIndexInfo&) {
    }
};

} // namespace NIndexerCore::NIndexerDetail
