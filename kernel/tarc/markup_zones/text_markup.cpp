#include "text_markup.h"

#include <kernel/tarc/iface/dtiterate.h>
#include <kernel/tarc/iface/tarcface.h>

#include <util/charset/utf8.h>
#include <util/charset/wide.h>
#include <util/stream/zlib.h>

inline size_t GetUTF8ByteSize(const TWtringBuf& text) {
    size_t byteSize = 0;
    size_t runeLen;
    const wchar16* const last = text.data() + text.size();
    unsigned char buff[8];
    for (const wchar16* cur = text.data(); cur != last;) {
        WriteUTF8Char(ReadSymbolAndAdvance(cur, last), runeLen, buff);
        byteSize += runeLen;
    }
    return byteSize;
}

class TSentOffsetCalc
{
private:
    const TVector<TArchiveSent>& Sents;
    TVector<ui64> CharOffsets;
    TVector<ui64> ByteOffsets;

public:
    explicit TSentOffsetCalc(const TVector<TArchiveSent>& s)
        : Sents(s)
    {
        ui64 curCharOffset = 0;
        ui64 curByteOffset = 0;
        for (const auto& iSent : s) {
            CharOffsets.push_back(curCharOffset);
            ByteOffsets.push_back(curByteOffset);

            // Plus one character (byte) for \n at the end of each line
            curCharOffset += iSent.OnlyText.size() + 1;
            curByteOffset += GetUTF8ByteSize(iSent.OnlyText) + 1;
        }
    }

    ui64 GetCharOffset(ui16 sentNum, ui16 offset) const {
        return CharOffsets[sentNum - 1] + offset;
    }

    ui64 GetByteOffset(ui16 sentNum, ui16 offset) const {
        return ByteOffsets[sentNum - 1] + (offset ? GetUTF8ByteSize(TWtringBuf(Sents[sentNum - 1].OnlyText.data(), offset)) : 0);
    }

    void TransformSpans(const TArchiveZone& az, TTextZone& tz) const {
        if (!az.Spans.empty()) {
            for (size_t s = 0; s < az.Spans.size(); ++s) {
                const TArchiveZoneSpan& aSpan = az.Spans[s];
                Y_ASSERT(aSpan.SentBeg > 0 && aSpan.SentBeg <= Sents.size()
                    && aSpan.SentEnd > 0 && aSpan.SentEnd <= Sents.size());

                tz.Spans.emplace_back();
                TTextZoneSpan& tSpan = tz.Spans.back();
                tSpan.CharBeg = GetCharOffset(aSpan.SentBeg, aSpan.OffsetBeg);
                tSpan.CharEnd = GetCharOffset(aSpan.SentEnd, aSpan.OffsetEnd);
                tSpan.ByteBeg = GetByteOffset(aSpan.SentBeg, aSpan.OffsetBeg);
                tSpan.ByteEnd = GetByteOffset(aSpan.SentEnd, aSpan.OffsetEnd);
            }
        }
    }
};

void GetTextMarkupZones(const ui8* data, TTextMarkupZones& zones, bool wZone/* = false*/) {
    TArchiveMarkupZones mZones;
    GetArchiveMarkupZones(data, &mZones);

    TVector<int> sentNumbers;
    TVector<TArchiveSent> outSents;
    TArchiveWeightZones wZones;
    GetSentencesByNumbers(data, sentNumbers, &outSents, wZone ? &wZones : nullptr, false);

    TSentOffsetCalc offsets(outSents);

    for (ui32 id = 0; id < AZ_COUNT; id++) {
        const TArchiveZone& az = mZones.GetZone(id);
        if (!az.Spans.empty()) {
            TTextZone& tz = zones.GetZone(id);
            offsets.TransformSpans(az, tz);
        }
    }

    if (wZone) {
        if (!wZones.LowZone.Spans.empty()) {
            TTextZone& tz = zones.GetZone(AZ_COUNT);
            offsets.TransformSpans(wZones.LowZone, tz);
        }
        if (!wZones.HighZone.Spans.empty()) {
            TTextZone& tz = zones.GetZone(AZ_COUNT + 1);
            offsets.TransformSpans(wZones.HighZone, tz);
        }
        if (!wZones.BestZone.Spans.empty()) {
            TTextZone& tz = zones.GetZone(AZ_COUNT + 2);
            offsets.TransformSpans(wZones.BestZone, tz);
        }
    }
}

void UnpackMarkupZones(const void* markupInfo, size_t markupInfoLen, TArchiveMarkupZones* mZonesOut) {
    if (markupInfoLen) {
        TBufferOutput markupInfoUnpacked;
        {
            TMemoryInput in(markupInfo, markupInfoLen);
            TZLibDecompress decompressor(&in);
            TransferData(&decompressor, &markupInfoUnpacked);
        }
        TMemoryInput in(markupInfoUnpacked.Buffer().Data(), markupInfoUnpacked.Buffer().Size());
        mZonesOut->Load(&in);
    }
}

void GetArchiveMarkupZones(const ui8* data, TArchiveMarkupZones* mZones) {
    if (!data)
        return;
    TArchiveTextHeader* hdr = (TArchiveTextHeader*)data;
    data += sizeof(TArchiveTextHeader);
    UnpackMarkupZones(data, hdr->InfoLen, mZones);
}

class TGetSentencesHandler
{
private:
    TVector<TArchiveSent>* OutSents;
    TArchiveWeightZones* WeightZones;

    TVector<int>::const_iterator SBeg;
    TVector<int>::const_iterator SEnd;
    bool NeedAllSents;
    bool NeedExtendedSents;
public:
    TGetSentencesHandler(const TVector<int>& sentNumbers, TVector<TArchiveSent>* outSents,
                         TArchiveWeightZones* wZones, bool needExtended = false)
        : OutSents(outSents)
        , WeightZones(wZones)
        , NeedExtendedSents(needExtended)
    {
        SBeg = sentNumbers.begin();
        SEnd = sentNumbers.end();
        NeedAllSents = (SBeg == SEnd);
    }

    bool OnHeader(const TArchiveTextHeader* /*hdr*/) {
        return true;
    }

    bool OnMarkupInfo(const void* /*markupInfo*/, size_t /*markupInfoLen*/) {
        return true;
    }

    bool OnBeginExtendedBlock(const TArchiveTextBlockInfo& /*b*/) {
        return NeedExtendedSents;
    }

    bool OnEndExtendedBlock() {
        return NeedExtendedSents;
    }

    bool OnBeginBlock(ui16 prevSentCount, const TArchiveTextBlockInfo& b) {
        return NeedAllSents || *SBeg <= prevSentCount + b.SentCount;
    }

    bool OnWeightZones(TMemoryInput* str) {
        if (WeightZones)
            WeightZones->Load(str);
        return true;
    }

    bool OnSent(size_t sentNum, ui16 sentFlag, const void* sentBytes, size_t sentBytesLen) {
        if (NeedAllSents || int(sentNum) == *SBeg) {
            OutSents->resize(OutSents->size() + 1);
            TArchiveSent& as = OutSents->back();

            if (sentFlag & SENT_HAS_EXTSYMBOLS)
                as.Text.assign((const wchar16*)sentBytes, sentBytesLen/sizeof(wchar16));
            else
                as.Text = CharToWide((const char*)sentBytes, sentBytesLen, csYandex);

            if (sentFlag & SENT_HAS_ATTRS) {
                size_t pos = as.Text.find('\t');
                as.OnlyText = as.Text.substr(0, pos);
                as.OnlyAttrs = TUtf16String::npos == pos ? TUtf16String() : as.Text.substr(pos);
            } else {
                as.OnlyText = as.Text;
            }

            as.Number = ui16(sentNum);
            as.Flag = sentFlag;
            if (!NeedAllSents && ++SBeg == SEnd)
                return false;
        }
        return true;
    }

    bool OnEndBlock() {
        return NeedAllSents || SBeg != SEnd;
    }

    void OnEnd() {
    }
};

void GetSentencesByNumbers(const ui8* data, const TVector<int>& sentNumbers,
                           TVector<TArchiveSent>* outSents, TArchiveWeightZones* wZones, bool needExtended) {

    TGetSentencesHandler handler(sentNumbers, outSents, wZones, needExtended);
    IterateArchiveDocText(data, handler);
}

class TGetAllTextHandler
{
private:
    TBuffer* Out;
    size_t TitleSentBeg;
    size_t TitleSentEnd;
public:
    explicit TGetAllTextHandler(TBuffer* out)
        : Out(out)
        , TitleSentBeg(0)
        , TitleSentEnd(0)
    {
        Out->Clear();
    }

    bool OnHeader(const TArchiveTextHeader* hdr) {
        if (hdr && hdr->BlockCount) {
            Out->Reserve(32768 * hdr->BlockCount + 4096);
            return true;
        }
        return false;
    }

    bool OnMarkupInfo(const void* markupInfo, size_t markupInfoLen) {
        TArchiveMarkupZones mZones;
        UnpackMarkupZones(markupInfo, markupInfoLen, &mZones);
        TArchiveZone& zTitle = mZones.GetZone(AZ_TITLE);
        if (!zTitle.Spans.empty()) {
            const TArchiveZoneSpan& tSpan = zTitle.Spans[0];
            TitleSentBeg = tSpan.SentBeg;
            TitleSentEnd = tSpan.SentEnd;
        }
        return true;
    }

    bool OnBeginExtendedBlock(const TArchiveTextBlockInfo& /*b*/) {
        return false;
    }

    bool OnEndExtendedBlock() {
        return true;
    }

    bool OnBeginBlock(ui16 /*prevSentCount*/, const TArchiveTextBlockInfo& /*b*/) {
        return true;
    }

    bool OnWeightZones(TMemoryInput* /*str*/) {
        return true;
    }

    bool OnSent(size_t sentNum, ui16 sentFlag, const void* sentBytes, size_t sentBytesLen) {
        if (sentNum < TitleSentBeg || sentNum > TitleSentEnd) {
            if (sentFlag & SENT_HAS_EXTSYMBOLS) {
                TString s = WideToChar((const wchar16*)sentBytes, sentBytesLen/sizeof(wchar16), CODES_YANDEX);
                Out->Append(s.data(), s.size());
            } else
                Out->Append((const char*)sentBytes, sentBytesLen);
            Out->Append('\n');
        }
        return true;
    }

    bool OnEndBlock() {
        return true;
    }

    void OnEnd() {
        Out->Append(0);
    }
};

void GetAllText(const ui8* data, TBuffer* out) {
    TGetAllTextHandler handler(out);
    IterateArchiveDocText(data, handler);
}

