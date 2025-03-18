#include "head_ar_2d.h"

#include <util/generic/algorithm.h>
#include <util/string/printf.h>

#include <limits>

const char TArrayWithHead2DBase::MagicValue[] = "ArrayWithHead2D\0";

TArrayWithHead2DImpl::TArrayWithHead2DImpl() {
}

void TArrayWithHead2DImpl::LoadImpl(const TMemoryMap& mapping, size_t recordSize, bool isPolite, bool quiet, bool lazyLoad) {
    if (Map.Get() != nullptr)
        ythrow yexception() << "Load() has already been called";

    Map.Reset(new TFileMap(mapping));
    DoLoad(recordSize, isPolite, quiet, lazyLoad);
}

void TArrayWithHead2DImpl::LoadImpl(const TString& fileName, size_t recordSize, bool isPolite, bool quiet, bool lazyLoad) {
    if (Map.Get() != nullptr)
        ythrow yexception() << "Load() has already been called";

    Map.Reset(new TFileMap(fileName));
    DoLoad(recordSize, isPolite, quiet, lazyLoad);
}

void TArrayWithHead2DImpl::DoLoad(size_t recordSize, bool isPolite, bool quiet, bool lazyLoad) {
    Map->Map(0, Map->Length());
    if (!lazyLoad)
        Map->Precharge();
    DoLoad(recordSize, isPolite, quiet, Map->Length(), Map->Ptr(), Map->GetFile().GetName().Quote());
}

void TArrayWithHead2DImpl::DoLoad(size_t recordSize, bool isPolite, bool quiet, const size_t fileSize, void* ptr, const TString& fileNameQuoted) {
    if (fileSize < N_HEAD_SIZE)
        ythrow yexception() << fileNameQuoted << " is truncated";
    Header = (const THeader*)ptr;
    if (memcmp(Header->Magic, MagicValue, sizeof(Header->Magic)) != 0)
        ythrow yexception() << fileNameQuoted << " is not a valid TArrayWithHead2D file or has an incompatible format version";
    if (fileSize < Header->DescriptorsOffset + Header->NumDocs * sizeof(TDescriptor))
        ythrow yexception() << fileNameQuoted << " is truncated";
    if (fileSize < Header->IndirectKeysOffset + Header->IndirectKeysCount * sizeof(ui8))
        ythrow yexception() << fileNameQuoted << " is truncated";

    Data = (const char*)ptr;
    Descriptors = (const TDescriptor*)(Data + Header->DescriptorsOffset);
    IndirectKeys = (const ui64*)(Data + Header->IndirectKeysOffset);

    if (Header->RecordSize < recordSize) {
        // on-disk records are smaller than needed. we'll expand them in-memory unless polite is false.

        if (!isPolite) {
            ythrow yexception() << "Can't read file " << fileNameQuoted << ":"
                                << " on-disk record size " << Header->RecordSize
                                << " is less than in-memory record size " << recordSize
                                << " and polite mode is disabled.";
        }

        size_t dataSize = fileSize - N_HEAD_SIZE - Header->NumDocs * sizeof(TDescriptor) - Header->IndirectKeysCount * sizeof(ui64);
        dataSize += Header->NumRecords * (recordSize - Header->RecordSize);

        if (!quiet)
            Cerr << "[WARNING] Requested polite mode will waste " << Sprintf("%.1f", dataSize / 1048576.0) << " Mb to load " << fileNameQuoted << "." << Endl;

        MemData.Reset(new char[dataSize]);
        MemDescriptors.assign(Descriptors, Descriptors + Header->NumDocs);

        char* dst = MemData.Get();
        memset(dst, 0, dataSize);

        for (ui32 docId = 0; docId < Header->NumDocs; docId++) {
            TDescriptor desc = Descriptors[docId];
            MemDescriptors[docId].Offset = (dst - MemData.Get()) / TDescriptor::OFFSET_MUL;

            // copy subindex
            const char* src = Data + TDescriptor::OFFSET_MUL * (size_t)desc.Offset;
            size_t subindexSize = ((ui64)desc.Length) << desc.SubindexKeyLogSize;
            if (desc.Mode == MODE_SINGLE_KEY || desc.Mode == MODE_NO_SUBINDEX)
                subindexSize = 0;                    // subindex is not used in these two modes
            subindexSize = (subindexSize + 3) & ~3U; // align to 32-bit boundary
            memcpy(dst, src, subindexSize);
            dst += subindexSize;
            src += subindexSize;

            // copy and expand records
            size_t len = desc.Mode == MODE_SINGLE_KEY ? 1 : desc.Length;
            for (size_t i = 0; i < len; i++) {
                memcpy(dst, src, Header->RecordSize);
                dst += recordSize;
                src += Header->RecordSize;
            }
        }

        MemRecordSize = recordSize;
        Data = MemData.Get();
        Descriptors = MemDescriptors.size() == 0 ? nullptr : &MemDescriptors[0];
    } else {
        MemRecordSize = Header->RecordSize;
    }

    for (ui32 i = 0; i < Header->IndirectKeysCount; i++)
        IndirectHash[IndirectKeys[i]] = i;
}

void TArrayWithHead2DImpl::GetKeys(TVector<ui64>& result, ui32 docId) const {
    result.clear();
    if (docId >= Header->NumDocs)
        return;

    TDescriptor desc = Descriptors[docId];
    if (desc.Mode == MODE_NO_SUBINDEX) {
        result.yresize(desc.Length);
        for (ui32 i = 0; i < desc.Length; i++)
            result[i] = i;
    } else if (desc.Mode == MODE_SINGLE_KEY) {
        result.push_back(desc.Length);
    } else {
        result.yresize(desc.Length);
        ui64* dst = (desc.Length == 0 ? nullptr : &result[0]);

        const char* sub = Data + TDescriptor::OFFSET_MUL * (size_t)desc.Offset;
        for (ui32 i = 0; i < desc.Length; i++) {
            ui64 key = 0;
            switch (desc.SubindexKeyLogSize) {
                case 0:
                    key = *(ui8*)sub;
                    break;
                case 1:
                    key = *(ui16*)sub;
                    break;
                case 2:
                    key = *(ui32*)sub;
                    break;
                case 3:
                    key = *(ui64*)sub;
                    break;
            }
            sub += size_t(1) << desc.SubindexKeyLogSize;

            if (desc.Mode == MODE_INDIRECT_KEYS) {
                if (IndirectKeys == nullptr || key >= Header->IndirectKeysCount)
                    ythrow yexception() << "Block " << docId << " of TArrayWithHead2D is corrupted";
                key = IndirectKeys[key];
            }

            *dst++ = key;
        }
        // it may be unsorted if (IndirectHash.count(key) != 0) in WriteBlockImpl()
        if (desc.Mode == MODE_INDIRECT_KEYS)
            Sort(result.begin(), result.end());
    }
}

size_t TArrayWithHead2DImpl::GetRowLength(ui32 row) const {
    if (row >= Header->NumDocs)
        return 0;

    TDescriptor desc = Descriptors[row];
    if (desc.Mode == MODE_SINGLE_KEY)
        return 1;
    return desc.Length;
}

const void* TArrayWithHead2DImpl::FindImpl(ui32 docId, ui64 key) const noexcept {
    if (Y_UNLIKELY(!Header || (docId >= Header->NumDocs)))
        return nullptr;

    TDescriptor desc = Descriptors[docId];
    switch (4 * desc.Mode + desc.SubindexKeyLogSize) {
    // gcc should optimize this switch to a jump table even in -O0 mode
#define C(mode, typeLogSize, type) \
    case (4 * mode + typeLogSize): \
        return FindImpl2<type, mode>(desc, key)
        C(0, 0, ui8);
        C(0, 1, ui16);
        C(0, 2, ui32);
        C(0, 3, ui64);
        C(1, 0, ui8);
        C(1, 1, ui16);
        C(1, 2, ui32);
        C(1, 3, ui64);
        C(2, 0, ui8);
        C(2, 1, ui16);
        C(2, 2, ui32);
        C(2, 3, ui64);
        C(3, 0, ui8);
        C(3, 1, ui16);
        C(3, 2, ui32);
        C(3, 3, ui64);
#undef C
    };
    return nullptr;
}

template <typename TSubindexKey, int Mode>
const void* TArrayWithHead2DImpl::FindImpl2(TDescriptor desc, ui64 key) const noexcept {
    const char* block = Data + TDescriptor::OFFSET_MUL * (size_t)desc.Offset;

    if (Mode == MODE_NO_SUBINDEX) {
        if (key >= desc.Length)
            return nullptr;
        return block + key * MemRecordSize;
    } else if (Mode == MODE_SINGLE_KEY) {
        if (key != (ui64)desc.Length)
            return nullptr;
        return block;
    }

    if (Mode == MODE_INDIRECT_KEYS) {
        THashMap<ui64, ui32>::const_iterator it = IndirectHash.find(key);
        if (it == IndirectHash.end())
            return nullptr;
        key = it->second;
    }

    if (key > std::numeric_limits<TSubindexKey>::max())
        return nullptr;

    const TSubindexKey* sub = (const TSubindexKey*)block;
    int a = 0, b = (int)desc.Length - 1;

    // binary search for key in sub[a..b]
    while (a <= b) {
        int c = (a + b) >> 1;
        if (sub[c] == key) {
            int subindexSize = (desc.Length * sizeof(TSubindexKey) + 3) & ~3U; // align to 32-bit boundary
            return block + subindexSize + c * MemRecordSize;
        } else if (sub[c] < key) {
            a = c + 1;
        } else {
            b = c - 1;
        }
    }

    return nullptr;
}

// Writer

TArrayWithHead2DWriterImpl::TArrayWithHead2DWriterImpl(const TString& filename, size_t recordSize, ui32 recordVersion, bool useIndirectKeys)
    : File(filename, WrOnly | CreateAlways)
    , Stream(File)
    , UseIndirectKeys(useIndirectKeys)
{
    char buf[N_HEAD_SIZE];
    memset(buf, 0, sizeof(buf));
    Stream.Write(buf, N_HEAD_SIZE); // reserve space for header, which will be written in FinishImpl

    Zero(Header);
    memcpy(Header.Magic, MagicValue, sizeof(Header.Magic));
    Header.RecordSize = recordSize;
    Header.RecordVersion = recordVersion;
}

// Finishes output: writes descriptors arrays, indirect keys, header, and closes the file.
void TArrayWithHead2DWriterImpl::FinishImpl() {
    if (!File.IsOpen())
        return;

    Header.DescriptorsOffset = Offset;
    if (Descriptors.size() != 0) {
        Stream.Write(&Descriptors[0], Descriptors.size() * sizeof(TDescriptor));
        Offset += Descriptors.size() * sizeof(TDescriptor);
    }

    Header.IndirectKeysOffset = Offset;
    Header.IndirectKeysCount = IndirectKeys.size();
    if (IndirectKeys.size() != 0)
        Stream.Write(&IndirectKeys[0], IndirectKeys.size() * sizeof(ui64));

    Stream.Finish();
    File.Pwrite(&Header, sizeof(Header), 0);
    File.Close();
}

// Writes another block. block contains list of pairs (key, index in records[] array)
void TArrayWithHead2DWriterImpl::WriteBlockImpl(ui32 docId, TVector<TKeyIndexPair>& block, const void* records) {
    Y_ASSERT(File.IsOpen());
    Y_ASSERT(Descriptors.size() <= docId);

    // add descriptors for missing blocks
    TDescriptor desc;
    Zero(desc);
    Descriptors.resize(docId, desc);

    Y_VERIFY(((Offset / TDescriptor::OFFSET_MUL) >> 32) == 0, "file size limit exceeded");
    Y_VERIFY((block.size() >> TDescriptor::LENGTH_SIZE) == 0, "block length limit exceeded");
    desc.Offset = Offset / TDescriptor::OFFSET_MUL;
    desc.Length = block.size();

    Sort(block.begin(), block.end());

    bool identityIndexed = true; // are keys consecutive integers starting from zero?
    for (size_t i = 0; i < block.size() && identityIndexed; i++)
        identityIndexed &= (block[i].first == i);

    // choose best storage mode and write subindex if needed
    if (block.size() == 1 && (block[0].first >> TDescriptor::LENGTH_SIZE) == 0) {
        // mode 1: block with a single key in field TDescriptor::Length
        desc.Mode = MODE_SINGLE_KEY;
        desc.Length = (ui32)block[0].first;

    } else if (identityIndexed) {
        // mode 0: no subindex needed, i-th record's key equals i.
        desc.Mode = MODE_NO_SUBINDEX;

    } else {
        // modes 2 and 3: subindex with either keys or indices in IndirectKeys

        // don't use indirect keys when largest key fits in 8-bit integer
        bool indirect = UseIndirectKeys && block.back().first >= 256;

        if (indirect) {
            // replace keys by indices in IndirectKeys
            for (size_t i = 0; i < block.size(); i++) {
                ui64 key = block[i].first;
                if (IndirectHash.count(key) == 0) {
                    IndirectHash[key] = IndirectKeys.size();
                    IndirectKeys.push_back(key);
                }
                block[i].first = IndirectHash[key];
            }
            Sort(block.begin(), block.end());
        }

        desc.Mode = indirect ? MODE_INDIRECT_KEYS : MODE_DIRECT_KEYS;

        // choose best size of subindex keys and write subindex to the file
#define C(type, typeLogSize)                                      \
    if (block.back().first <= std::numeric_limits<type>::max()) { \
        desc.SubindexKeyLogSize = typeLogSize;                    \
        for (size_t i = 0; i < block.size(); i++) {               \
            type key = block[i].first;                            \
            Stream.Write(&key, sizeof(key));                      \
        }                                                         \
        Offset += sizeof(type) * block.size();                    \
        break;                                                    \
    }
        do {
            C(ui8, 0);
            C(ui16, 1);
            C(ui32, 2);
            C(ui64, 3);
        } while (0);
#undef C

        // align to 32-bit boundary
        if (Offset % 4 != 0) {
            ui32 zero = 0;
            Stream.Write(&zero, 4 - Offset % 4);
            Offset += 4 - Offset % 4;
        }
    }

    // write the records
    for (size_t i = 0; i < block.size(); i++)
        Stream.Write((const char*)records + block[i].second * Header.RecordSize, Header.RecordSize);
    Offset += block.size() * Header.RecordSize;

    Descriptors.push_back(desc);
    Header.NumRecords += block.size();
    Header.NumDocs = Descriptors.size();
}

TArrayWithHead2DWriter<TStringBuf>::TArrayWithHead2DWriter(
    const TString& fileName, size_t recordSize,
    ui32 recordVersion, bool useIndirectKeys)
    : TArrayWithHead2DWriterImpl(fileName, recordSize, recordVersion, useIndirectKeys)
    , RecordSize(recordSize)
{
    if (RecordSize == 0) {
        ythrow yexception() << "RecordSize must not be 0";
    }
}

TArrayWithHead2DWriter<TStringBuf>::~TArrayWithHead2DWriter() {
    Finish();
}

void TArrayWithHead2DWriter<TStringBuf>::Put(ui32 docId, ui64 key, TStringBuf data) {
    if (docId < LastDocId)
        ythrow yexception() << "Can't put record with docID " << docId
                            << " which is less than docID " << LastDocId << " of last record";
    if (docId > LastDocId) {
        WriteBlock();
        LastDocId = docId;
    }
    if (data.length() != RecordSize) {
        ythrow yexception() << "Can't put record, record size: " << data.length()
                            << "; expected record size: " << RecordSize << '.';
    }
    Block.push_back(TKeyIndexPair(key, GetElementsInBlockCount()));
    BlockData.Append(data.data(), data.length());
}

void TArrayWithHead2DWriter<TStringBuf>::Finish() {
    WriteBlock();
    FinishImpl();
}

void TArrayWithHead2DWriter<TStringBuf>::WriteBlock() {
    if (Block.size() != 0) {
        WriteBlockImpl(LastDocId, Block, BlockData.Data());
        Block.clear();
        BlockData.Clear();
    }
}
