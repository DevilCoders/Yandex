// #define LENGTH_DUMP
// #define LENGTH_VALIDATE

#include <kernel/doom/wad/wad.h>
#include <kernel/doom/standard_models/standard_models.h>

#include <util/stream/buffer.h>
#include <util/system/filemap.h>

#include <library/cpp/on_disk/chunks/chunked_helpers.h>
#include <library/cpp/sse/sse.h>

#include <util/system/mlock.h>
#include <util/system/fs.h>

#include "sent_lens.h"

const TSentenceLengthsReader::TOffset TSentenceLengthsReader::INVALID_OFFSET((ui64)-1);

bool TSentenceLengthsReader::HasUnderlyingWad(const TString& fileName) {
    TString wadFileName = fileName + ".wad";
    return NFs::Exists(wadFileName);
}

TSentenceLengthsReader::TSentenceLengthsReader(TUseWad,
                                               const TString& fileName,
                                               NDoom::EWadIndexType indexType,
                                               bool lockMem)
    : LockMem(lockMem)
{
    LocalWad = NDoom::IWad::Open(fileName + ".wad", lockMem);
    FileName = fileName;
    InitFromWad(LocalWad.Get(), LocalWad.Get(), indexType, /* usePreloader= */ false);
}

void TSentenceLengthsReader::InitFromWad(const NDoom::IWad* wad, const NDoom::IDocLumpMapper* mapper, NDoom::EWadIndexType indexType, bool usePreloader) {
    switch (indexType) {
    case NDoom::SentIndexType:
        WadSentReader.Reset(new TSentenceLengthsWadReader<NDoom::TOffroadSentWadIo>(wad, mapper, usePreloader));
        break;
    case NDoom::AnnSentIndexType:
        WadSentReader.Reset(new TSentenceLengthsWadReader<NDoom::TOffroadAnnSentWadIo>(wad, mapper, usePreloader));
        break;
    case NDoom::FactorAnnSentIndexType:
        WadSentReader.Reset(new TSentenceLengthsWadReader<NDoom::TOffroadFactorAnnSentWadIo>(wad, mapper, usePreloader));
        break;
    case NDoom::LinkAnnSentIndexType:
        WadSentReader.Reset(new TSentenceLengthsWadReader<NDoom::TOffroadLinkAnnSentWadIo>(wad, mapper, usePreloader));
        break;
    default:
        Y_ENSURE(false, "Cannot handle indexType");
    }
}

TSentenceLengthsReader::TSentenceLengthsReader(const NDoom::IWad* wad, const NDoom::IDocLumpMapper* mapper, NDoom::EWadIndexType indexType) {
    InitFromWad(wad, mapper, indexType, /* usePreloader= */ true);
}

TSentenceLengthsReader::TSentenceLengthsReader(const TString& fileName, bool lockMem)
    : Data(TBlob::PrechargedFromFile(fileName))
    , LockMem(lockMem)
{
    InternalInit();
}

TSentenceLengthsReader::TSentenceLengthsReader(const TMemoryMap& mapping, bool lockMem)
    : Data(TBlob::FromMemoryMap(mapping, 0, mapping.Length()))
    , LockMem(lockMem)
{
    InternalInit();
}

TSentenceLengthsReader::TSentenceLengthsReader(const TBlob& blob, bool lockMem)
    : Data(blob)
    , LockMem(lockMem)
{
    InternalInit();
}

TSentenceLengthsReader::~TSentenceLengthsReader() {
    if (LockMem) {
        UnlockMemory(Data.Data(), Data.Size());
    }
}

void TSentenceLengthsReader::InternalInit() {
    Version = *((ui8*)GetBlock(Data, 0).AsCharPtr()); // TODO: ui8->ui32
    Packed = (const char*)GetBlock(Data, 1).AsCharPtr();
    PackedEnd = Packed + GetBlock(Data, 1).Length();
    Offsets = (const TOffset*)GetBlock(Data, 2).AsCharPtr();
    Size = GetBlock(Data, 2).Length() / sizeof(Offsets[0]);

    if (LockMem) {
        LockMemory(Data.Data(), Data.Size());
    }
}

template<typename T>
void TSentenceLengthsReader::Get_(ui32 docId, T& integrator) const {
    Y_ASSERT(docId < Size);
    if (Offsets[docId] != INVALID_OFFSET) {
#ifdef LENGTH_DUMP
        Cerr << "Offset: " << Offsets[docId].Offset << Endl;

        Cerr << "Compressed: ";
        for (const char* ch = begin; ch != end; ++ch)
            Cerr << Bin((int)((ui8)(*ch))) << " ";
        Cerr << Endl;
#endif
        const ui8* const begin0 = (const ui8*)Packed + Offsets[docId].Offset;
        ui16 len = *((ui16*)begin0);

#ifdef LENGTH_DUMP
        Cerr << "Len: " << len << Endl;
#endif
        if (2 == Version) {
            integrator.Fill((const ui16*)begin0 + 1, len, &NDataVersion2::PackedOffsets.Offsets[0], NDataVersion2::LENGTHS_SEQUENCES);
        } else {
            ythrow yexception() << "unknown version: " << Version;
        }
        Y_ASSERT(integrator.Size() == len);
    }
}

inline void IntegrateSIMD(ui32* data, const ui8* lengths, size_t size) {
    __m128i accum0 = _mm_setzero_si128();
    __m128i accum1 = _mm_setzero_si128();

    for (size_t i = 0; i < size; i += 8) {
        const __m128i v8x8 = _mm_loadl_epi64((const __m128i *)(lengths + i));
        const __m128i v8 = _mm_unpacklo_epi8(v8x8, _mm_setzero_si128());
        const __m128i s0 = _mm_add_epi16(v8, _mm_slli_si128(v8, 8));
        const __m128i s1 = _mm_add_epi16(s0, _mm_slli_si128(s0, 4));
        const __m128i s2 = _mm_add_epi16(s1, _mm_slli_si128(s1, 2));

        accum1 = _mm_shuffle_epi32(accum1, _MM_SHUFFLE(3, 3, 3, 3));
        accum0 = accum1;

        accum0 = _mm_add_epi32(_mm_unpacklo_epi16(s2, _mm_setzero_si128()), accum0);
        accum1 = _mm_add_epi32(_mm_unpackhi_epi16(s2, _mm_setzero_si128()), accum1);


        _mm_storeu_si128((__m128i *)(data + i + 0), accum0);
        _mm_storeu_si128((__m128i *)(data + i + 4), accum1);
    }
}

inline ui8* CopySingle(ui8* to, const ui8* from, size_t size) {
    ui8* beg = to;
    const __m128i data = _mm_loadu_si128((const __m128i *)(from));
    _mm_storeu_si128((__m128i *)beg, data);
    return to + size;
}

inline ui8* CopyMult(ui8* to, const ui8* from, size_t size) {
    ui8* beg = to;
    const ui8* end = from + size;
    do {
        const __m128i data = _mm_loadu_si128((const __m128i *)(from));
        _mm_storeu_si128((__m128i *)beg, data);
        from += 16;
        beg += 16;
    } while (from < end);

    return to + size;
}

inline ui8* Decompress(ui8* out,
                        const ui16* compressed,
                        size_t decompressedSize,
                        const ui32* dictInfo,
                        const ui8* dictionary) {
    size_t i = 0;
    ui8* end = out + decompressedSize;

    while (out < end) {
        for (; out < end; ++i) {
            ui16 code = compressed[i];
            ui32 lengthOffset = dictInfo[code];
            ui32 length = lengthOffset & 0xff;
            ui32 offset = lengthOffset >> 8;
            if (length < 17)
                out = CopySingle(out, dictionary + offset, length);
            else
                break;
        }

        for (; out < end; ++i) {
            ui16 code = compressed[i];
            ui32 lengthOffset = dictInfo[code];
            ui32 length = lengthOffset & 0xff;
            ui32 offset = lengthOffset >> 8;
            if (length < 17)
                break;
            else
                out = CopyMult(out, dictionary + offset, length);
        }
    }

    return out;
}

class TLengthIntegrator : public TNonCopyable {
private:
    TSentenceLengths& Result;

public:
    TLengthIntegrator(TSentenceLengths& result)
        : Result(result)
    {
    }

    void Fill(const ui16 *stream, ui16 length, const ui32* dictInfo, const ui8* dictionary) {
        Result.Reserve(length + 256);
        Decompress(&Result[0], stream, length, dictInfo, dictionary);
        Result.SetInitializedSize(length);
    }

    size_t Size() const {
        return +Result;
    }
};

THolder<ISentenceLengthsPreLoader> TSentenceLengthsReader::CreatePreLoader() const {
    if (WadSentReader) {
        return WadSentReader->CreatePreLoader();
    }

    return nullptr;
}

void TSentenceLengthsReader::Get(ui32 docId, TSentenceLengths* result, ISentenceLengthsPreLoader* preLoader) const {
    if (WadSentReader) {
        WadSentReader->GetLengths(docId, result, preLoader);
        return;
    }

    result->SetInitializedSize(0);
    TLengthIntegrator integrator(*result);
    Get_(docId, integrator);
}

class TOffsetsIntegrator: TNonCopyable {
private:
    TSentenceOffsets& Result;

public:
    TOffsetsIntegrator(TSentenceOffsets& result)
        : Result(result)
    {
        Result.Reserve(64);
        Result[0] = 0;
        Result.SetInitializedSize(1);
    }

    void Fill(const ui16 *stream, ui16 length, const ui32 *dictInfo, const ui8 *dictionary) {
        Result.Reserve(FastClp2(length + 256 + 1));
        ui8* out = (ui8 *)alloca(length + 256);
        Decompress(out, stream, length, dictInfo, dictionary);
        // clear tail not to calculate garbage in IntegrateSIMD
        memset(out + length, 0, sizeof(ui64));
        IntegrateSIMD(&Result[1], out, length);
        Result.SetInitializedSize(length + 1);
    }

    size_t Size() const {
        return +Result - 1;
    }
};

void TSentenceLengthsReader::GetOffsets(ui32 docId, TSentenceOffsets* result, ISentenceLengthsPreLoader* preLoader) const {
    if (WadSentReader) {
        WadSentReader->GetOffsets(docId, result, preLoader);
        return;
    }

    result->SetInitializedSize(0);
    TOffsetsIntegrator integrator(*result);
    Get_(docId, integrator);
}

void TSentenceLengthsReader::GetPackedVersion2(const ui8*& begin, size_t& size) {
    Y_ASSERT(2 == TSentenceLengthsCoderData::GetNumBytesForHashVersion2());
    ui16 offset = *((ui16*)begin);
    begin += 2;
    size += NDataVersion2::LENGTHS_OFFSETS[offset].Len;
}

void TSentenceLengthsReader::GetPacked(ui32 docId, TSentenceLengths* result) const {
    result->SetInitializedSize(0);
    if (Offsets[docId] != INVALID_OFFSET) {
        const ui8* const begin0 = (const ui8*)Packed + Offsets[docId].Offset;

        ui64 len = *((ui16*)(begin0));
        size_t size = 0;
        if (2 == Version) {
            const ui8* begin = begin0 + 2;
            while (size < len) {
                GetPackedVersion2(begin, size);
            }
            result->Append(begin0, begin);
        } else {
            ythrow yexception() << "unknown version: " << Version;
        }
        Y_ASSERT(size == len);
    }
}

ui32 TSentenceLengthsReader::GetSize() const {
    if (WadSentReader)
        return WadSentReader->Size();

    return Size;
}

ui32 TSentenceLengthsReader::GetVersion() const {
    return Version;
}

TRealtimeSentenceLengthReader::TRealtimeSentenceLengthReader(size_t maxDocs) {
    Container.resize(maxDocs);
}

THolder<ISentenceLengthsPreLoader> TRealtimeSentenceLengthReader::CreatePreLoader() const {
    return nullptr;
}

void TRealtimeSentenceLengthReader::GetOffsets(ui32 docId, TSentenceOffsets* result, ISentenceLengthsPreLoader* preLoader) const {
    Y_ASSERT(docId < Container.size());
    Y_ASSERT(preLoader == nullptr);

    const TString& curLengths = Container[docId];
    result->Reserve(curLengths.size() + 1);
    ui32 offset = 0;
    for (size_t i = 0; i < curLengths.size(); ++i) {
        (*result)[i] = offset;                     // write offsets into container, 0 element alway zero
        offset += (ui8)curLengths[i];              // other elements are summa of earlier elemnts
    }
    (*result)[curLengths.size()] = offset;
    result->SetInitializedSize(curLengths.size() + 1);
}

void TRealtimeSentenceLengthReader::Get(ui32 docId, TSentenceLengths* result, ISentenceLengthsPreLoader* preLoader) const {
    Y_ASSERT(docId < Container.size());
    Y_ASSERT(preLoader == nullptr);

    const TString& curLengths = Container[docId];
    result->Reserve(curLengths.size() + 1);
    (*result)[0] = (ui8)0;                         // first sentence is url, it is not passed in SetSentencesLength
    for (size_t i = 0; i < curLengths.size(); ++i) {
        (*result)[i+1] = (ui8)curLengths[i];
    }
    result->SetInitializedSize(curLengths.size() + 1);
}

void TRealtimeSentenceLengthReader::SetSentencesLength(ui32 docId, const TString& sentencesLength) {
    if (!sentencesLength)
        return;
    Container[docId] = sentencesLength;
}
