#pragma once

#include <util/generic/buffer.h>
#include <util/generic/hash.h>
#include <util/generic/strbuf.h>
#include <util/generic/vector.h>
#include <util/stream/file.h>
#include <util/system/filemap.h>
#include <util/generic/noncopyable.h>

// TArrayWithHead2D files are composed of blocks indexed by ui32.
// Each block contains a variable number (possibly zero) of fixed-size records,
// corresponding to different versions of the same structure.
// Records within a block are referenced by an arbitrary 64-bit key.
//
// Typically a block will correspond to a document or host and is indexed by
// search docId or hostId, and records in a block are indexed by geo/geoa category.
//
// File structure:
//   1) header (TArrayWithHead2DBase::THeader) - 1024 bytes.
//   2) blocks. Each block starts with a subindex, listing the keys of all
//      records in this blocks, then padding to a 32-bit boundary, and an array
//      of records (of template type TArrayEl). Keys in the subindex are sorted.
//      In simple cases (block with one record or when the keys are 0, 1, 2, ...)
//      subindex is not used, and the block stores just the records.
//   3) array of 64-bit TDescriptor structs, encoding for each block its file
//      offset, length and type of subindex used. Offset is stored in a 32-bit
//      integer and is divided by 4, so max file size is 16G. A zero-filled
//      TDescriptor indicates a missing block.
//   4) Optionally, an array of indirect keys (64-bit integers), see below.
//
// Each block's subindex may have a different format. TArrayWithHead2DWriter
// automatically selects a format for each block to store the keys more compactly.
// Block's format is encoded in block's TDescriptor::Mode, and the possible
// formats currently are:
//   Mode=0: no subindex
//     Used when the keys are consecutive integers 0, 1, ..., number of records - 1.
//     Block contains just an array of records, and record with key i is stored
//     at the i-th position.
//   Mode=1: no subindex, because block has a single record.
//     In this case the key of the record is stored directly in TDescriptor::Length.
//   Mode=2: subindex with direct keys
//     Each block has a subindex, which is simply an array of all keys of records
//     in sorted order.
//   Mode=3: subindex with indirect keys
//     Same as Mode=2, except that the subindex contains indices into the
//     indirect keys array. This allows to more efficiently handle the case
//     when the number of possible keys is small, but the keys themselves can be
//     very large, as is the case with geo/geoa categories.
//
// Subindexes with indirect keys are used only when you pass useIndirectKeys=true
// to the writer.
//

class TArrayWithHead2DBase {
public:
    enum { N_HEAD_SIZE = 1024 };

#pragma pack(push, 4)

    // File header - the first 1024 bytes.
    struct THeader {
        char Magic[16];     // TArrayWithHead2DBase::MagicValue, identifies format
        ui32 RecordSize;    // sizeof(TArrayEl)
        ui32 RecordVersion; // TArrayEl::Version
        ui32 NumDocs;
        ui64 NumRecords;
        ui64 DescriptorsOffset; // offset to array with TDescriptor's
        ui64 IndirectKeysOffset;
        ui32 IndirectKeysCount;
    };
    static_assert(sizeof(THeader) <= N_HEAD_SIZE, "expect sizeof(THeader) <= N_HEAD_SIZE");

    // Block descriptor.
    // A missing block will have zero-filled descriptor.
    struct TDescriptor {
        enum { OFFSET_MUL = 4,
               LENGTH_SIZE = 28 };
        ui32 Offset : 32;            // file offset divided by OFFSET_MUL
        ui32 Mode : 2;               // 0=no subindex, 1=block with a single key (in the field Length)
                                     // 2=subindex with direct keys, 3=subindex with indirect keys
        ui32 SubindexKeyLogSize : 2; // log_2(size of subindex key in bytes). Ignored if mode=0 or 1.
        ui32 Length : LENGTH_SIZE;   // number of records in the block. If mode=1, this field stores the key.
    };
    static_assert(sizeof(TDescriptor) == 8, "expect sizeof(TDescriptor) == 8");

#pragma pack(pop)

    enum {
        MODE_NO_SUBINDEX = 0,
        MODE_SINGLE_KEY = 1,
        MODE_DIRECT_KEYS = 2,
        MODE_INDIRECT_KEYS = 3
    };

    static const char MagicValue[]; // first 16 bytes of the file, identifying its format.
};

class TArrayWithHead2DImpl: protected  TArrayWithHead2DBase, TNonCopyable {
private:
    const TDescriptor* Descriptors = nullptr;
    const char* Data = nullptr;
    size_t MemRecordSize = 0; // in-memory record size, >= sizeof(TArrayEl)
    const THeader* Header = nullptr;
    THolder<TFileMap> Map;
    TVector<TDescriptor> MemDescriptors;
    TArrayHolder<char> MemData;
    THashMap<ui64, ui32> IndirectHash; // maps IndirectKeys[i] -> i
    const ui64* IndirectKeys = nullptr;

    template <typename TSubindexKey, int Mode>
    const void* FindImpl2(TDescriptor desc, ui64 key) const noexcept;

protected:
    // Searches for (docId, key) record; returns NULL if it can't be found
    const void* FindImpl(ui32 docId, ui64 key) const noexcept;

    void LoadImpl(const TString& filename, size_t structSize, bool isPolite, bool quiet, bool lazyLoad = false);
    void LoadImpl(const TMemoryMap& mapping, size_t structSize, bool isPolite, bool quiet, bool lazyLoad = false);
    /// Actually performs load actions after LoadImpl preparations
    void DoLoad(size_t recordSize, bool isPolite, bool quiet, bool lazyLoad = false);

public:
    TArrayWithHead2DImpl();

    void DoLoad(size_t recordSize, bool isPolite, bool quiet, const size_t fileSize, void* ptr, const TString& fileNameQuoted);

    size_t GetSize() const noexcept {
        return Header->NumDocs;
    }

    // Returns on-disk record size
    ui32 GetRecordSize() const noexcept {
        return Header->RecordSize;
    }

    // Returns TArrayEl::Version as it was written on disk
    ui32 GetVersion() const noexcept {
        return Header->RecordVersion;
    }

    // Returns a list of all keys for a given docID.
    void GetKeys(TVector<ui64>& result, ui32 docId) const;

    size_t GetRowLength(ui32 row) const;

    void SetSequential() {
        Map->SetSequential();
    }

    void Evict() {
        Map->Evict();
    }
};

template <typename TArrayEl>
class TArrayWithHead2D: public TArrayWithHead2DImpl {
    TArrayEl Dummy;

public:
    TArrayWithHead2D(const TString& filename = TString(), bool isPolite = false, bool quiet = false) {
        Zero(Dummy);
        if (!filename.empty()) {
            Load(filename, isPolite, quiet);
        }
    }

    TArrayWithHead2D(const TMemoryMap& mapping, bool isPolite = false, bool quiet = false) {
        Zero(Dummy);
        Load(mapping, isPolite, quiet);
    }

    // Loads file.
    // If on-disk records are smaller than in-memory record, will resize them in-memory if polite is true, else abort.
    void Load(const TString& filename, bool isPolite = false, bool quiet = false, bool lazyLoad = false) {
        LoadImpl(filename, sizeof(TArrayEl), isPolite, quiet, lazyLoad);
    }

    void Load(const TMemoryMap& mapping, bool isPolite = false, bool quiet = false, bool lazyLoad = false) {
        LoadImpl(mapping, sizeof(TArrayEl), isPolite, quiet, lazyLoad);
    }

    void Load(bool isPolite, bool quiet, const size_t fileSize, void* ptr, const TString& fileNameQuoted) {
        DoLoad(sizeof(TArrayEl), isPolite, quiet, fileSize, ptr, fileNameQuoted);
    }

    // Searches for (docId, key) record; returns NULL if it can't be found
    const TArrayEl* Find(ui32 docId, ui64 key) const noexcept {
        return (const TArrayEl*)FindImpl(docId, key);
    }

    // Searches for (docId, key) record; returns a zero-filled dummy if it can't be found.
    const TArrayEl& Get(ui32 docId, ui64 key) const noexcept {
        const TArrayEl* el = Find(docId, key);
        return el == nullptr ? Dummy : *el;
    }
};

template <typename TArrayEl>
class TFileMappedArrayWithHead2DUpdatable: public TMemoryMap, public TArrayWithHead2D<TArrayEl> {
private:
    using TBase = TArrayWithHead2D<TArrayEl>;

public:
    explicit TFileMappedArrayWithHead2DUpdatable(const TString& fileName, bool quiet = false)
        : TMemoryMap(fileName, TMemoryMapCommon::oRdWr)
        , TBase(*this, /*isPolite=*/false, quiet)
    {
    }

    // Does not support new (docId, key) values, can only update already known (docId, key) pairs.
    // If (docId, key) is unknown, returns false
    bool Set(ui32 docId, ui64 key, const TArrayEl& newValue) {
        TArrayEl* value = const_cast<TArrayEl*>(TBase::Find(docId, key));
        if (!value)
            return false;
        *value = newValue;
        return true;
    }
};

class TArrayWithHead2DWriterImpl: private TArrayWithHead2DBase, TNonCopyable {
private:
    TFile File;
    TFileOutput Stream;
    THeader Header;
    ui64 Offset = N_HEAD_SIZE;
    TVector<TDescriptor> Descriptors;

    bool UseIndirectKeys = false;
    TVector<ui64> IndirectKeys;
    THashMap<ui64, ui32> IndirectHash;

protected:
    typedef std::pair<ui64, size_t> TKeyIndexPair;

    TArrayWithHead2DWriterImpl(const TString& filename, size_t recordSize, ui32 recordVersion, bool useIndirectKeys);
    void WriteBlockImpl(ui32 docId, TVector<TKeyIndexPair>& block, const void* records);
    void FinishImpl();
};

template <typename TArrayEl>
class TArrayWithHead2DWriter: public TArrayWithHead2DWriterImpl {
    TVector<TArrayEl> BlockData;
    TVector<TKeyIndexPair> Block; // (key, index in BlockData) pairs
    ui32 LastDocId = 0;

    void WriteBlock() {
        if (Block.size() != 0) {
            WriteBlockImpl(LastDocId, Block, &BlockData[0]);
            Block.clear();
            BlockData.clear();
        }
    }

public:
    TArrayWithHead2DWriter(const TString& filename, bool useIndirectKeys = false)
        : TArrayWithHead2DWriterImpl(filename, sizeof(TArrayEl), TArrayEl::Version, useIndirectKeys)
    {
    }

    ~TArrayWithHead2DWriter() {
        Finish();
    }

    // Note: docId's must not decrease in subsequent calls.
    // Subindex keys' may go in any order, though.
    void Put(ui32 docId, ui64 key, const TArrayEl& el) {
        if (docId < LastDocId)
            ythrow yexception() << "Can't put record with docID " << docId << " which is less than docID " << LastDocId << " of last record";
        if (docId > LastDocId) {
            WriteBlock();
            LastDocId = docId;
        }
        Block.push_back(TKeyIndexPair(key, BlockData.size()));
        BlockData.push_back(el);
    }

    // Finishes writing the output.
    void Finish() {
        WriteBlock();
        FinishImpl();
    }
};

template <>
class TArrayWithHead2DWriter<TStringBuf>: public TArrayWithHead2DWriterImpl {
public:
    TArrayWithHead2DWriter(
        const TString& fileName, size_t recordSize,
        ui32 recordVersion, bool useIndirectKeys = false);

    ~TArrayWithHead2DWriter();

    void Put(ui32 docId, ui64 key, TStringBuf data);

    void Finish();

private:
    size_t GetElementsInBlockCount() const {
        return BlockData.Size() / RecordSize;
    }

    void WriteBlock();

private:
    TBuffer BlockData;
    TVector<TKeyIndexPair> Block; // (key, index in BlockData) pairs
    const size_t RecordSize;
    ui32 LastDocId = 0;
};

template <class TArrayEl>
class TArrayWithHead2DSerialWriter: public TArrayWithHead2DWriterImpl {
private:
    TVector<TArrayEl> BlockData;
    TVector<TKeyIndexPair> Block;
    ui64 CurrentCol = 0;
    ui32 CurrentRow = 0;
    bool Finished = false;

public:
    TArrayWithHead2DSerialWriter(const TString& filename)
        : TArrayWithHead2DWriterImpl(filename, sizeof(TArrayEl), TArrayEl::Version, false)
    {
    }

    ~TArrayWithHead2DSerialWriter() {
        Finish();
    }

    // Create a new row of the array (after object creation first row already exists)
    void NewLine() {
        WriteBlockImpl(CurrentRow, Block, Block.size() == 0 ? nullptr : &BlockData[0]);
        Block.clear();
        BlockData.clear();
        ++CurrentRow;
        CurrentCol = 0;
    }

    // Write element to the next column of current row
    void Write(const TArrayEl& el) {
        Block.push_back(TKeyIndexPair(CurrentCol++, BlockData.size()));
        BlockData.push_back(el);
    }

    // Save file. After calling this function you can't write to file any more.
    void Finish() {
        if (Finished)
            return;
        WriteBlockImpl(CurrentRow, Block, Block.size() == 0 ? nullptr : &BlockData[0]);
        FinishImpl();
        Finished = true;
    }
};
