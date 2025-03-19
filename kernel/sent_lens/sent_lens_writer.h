#pragma once

#include <util/stream/file.h>
#include <util/generic/hash.h>
#include <util/generic/ptr.h>

#include <library/cpp/on_disk/chunks/chunked_helpers.h>

#include "sent_lens.h"

class ISentenceLengthsWriter {
public:
    virtual ~ISentenceLengthsWriter() {}

    virtual void Add(ui32 docId, const ui8* begin, size_t size) = 0;
    virtual void AddPacked(ui32 docId, const TSentenceLengths& lengthsPacked) = 0;
};

class TSentenceLengthsWriter: public ISentenceLengthsWriter {
private:
    ui32 Version;
    THolder<IOutputStream> FOut;
    IOutputStream* Out;
    TChunkedDataWriter ChunkedOut;

    typedef TVector<TSentenceLengthsReader::TOffset> TOffsets;
    TOffsets Offsets;
    typedef THashMap<ui64, size_t> THash2Index;
    THash2Index Hash2Index;
    THash2Index PackedHash2Index;
    size_t Estimated;
    size_t Blocks;

    void InternalInit();
    void AddVersion2(const ui8* begin, size_t size);

public:
    TSentenceLengthsWriter(IOutputStream* out, ui32 version = TSentenceLengthsReader::SL_VERSION);
    TSentenceLengthsWriter(const TString& name, ui32 version = TSentenceLengthsReader::SL_VERSION);
    ~TSentenceLengthsWriter() override;

    void Add(ui32 docId, const TSentenceLengths& lengths);
    void Add(ui32 docId, const ui8* begin, size_t size) override;
    void AddPacked(ui32 docId, const TSentenceLengths& lengthsPacked) override;
    size_t GetEstimated() const;
    size_t GetBlocks() const;
};
