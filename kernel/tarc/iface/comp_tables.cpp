#include "comp_tables.h"

#include <library/cpp/archive/yarchive.h>

#include <util/generic/ptr.h>
#include <util/generic/singleton.h>
#include <util/stream/input.h>
#include <util/memory/blob.h>

namespace NArc {

static const unsigned char comp_tables_data[] = {
    #include <kernel/tarc/iface/comp_tables.inc>
};

static void LoadCompressorTable(
        const TArchiveReader& archiveReader,
        const char* const path,
        NCompTable::TCompressorTable* table)
{
    TAutoPtr<IInputStream> input(archiveReader.ObjectByKey(path));
    TSerializer<NCompTable::TCompressorTable>::Load(input.Get(), *table);
}

struct TCompressorTableHelper : private TNonCopyable {
    NCompTable::TCompressorTable MarkupZonesTable;
    NCompTable::TCompressorTable SentInfosTable;

    TCompressorTableHelper() {
        TArchiveReader archiveReader(
                    TBlob::NoCopy(comp_tables_data,
                                  sizeof(comp_tables_data)));
        LoadCompressorTable(
                    archiveReader,
                    "/comp_table_markup_zones.bin",
                    &MarkupZonesTable);
        LoadCompressorTable(
                    archiveReader,
                    "/comp_table_sent_infos.bin",
                    &SentInfosTable);
    }
};

class TCompressorsHolder : private TNonCopyable {
public:
    TCompressorsHolder() {
        auto tableHelperHolder = MakeHolder<TCompressorTableHelper>();
        MarkupZonesCompressor.Reset(
                    new NCompTable::TChunkCompressor(true, tableHelperHolder->MarkupZonesTable));
        SentInfosCompressor.Reset(
                    new NCompTable::TChunkCompressor(true, tableHelperHolder->SentInfosTable));
    }

public:
    NCompTable::TChunkCompressor& GetMarkupZonesCompressor() {
        return *MarkupZonesCompressor.Get();
    }

    NCompTable::TChunkCompressor& GetSentInfosCompressor() {
        return *SentInfosCompressor.Get();
    }

private:
    THolder<NCompTable::TChunkCompressor>   MarkupZonesCompressor;
    THolder<NCompTable::TChunkCompressor>   SentInfosCompressor;
};

class TDecompressorsHolder : private TNonCopyable {
public:
    TDecompressorsHolder() {
        auto tableHelperHolder = MakeHolder<TCompressorTableHelper>();
        MarkupZonesDecompressor.Reset(
                    new NCompTable::TChunkDecompressor(true, tableHelperHolder->MarkupZonesTable));
        SentInfosDecompressor.Reset(
                    new NCompTable::TChunkDecompressor(true, tableHelperHolder->SentInfosTable));
    }

public:
    NCompTable::TChunkDecompressor& GetMarkupZonesDecompressor() {
        return *MarkupZonesDecompressor.Get();
    }

    NCompTable::TChunkDecompressor& GetSentInfosDecompressor() {
        return *SentInfosDecompressor.Get();
    }

private:
    THolder<NCompTable::TChunkDecompressor> MarkupZonesDecompressor;
    THolder<NCompTable::TChunkDecompressor> SentInfosDecompressor;
};

NCompTable::TChunkCompressor& GetMarkupZonesCompressor() {
    return Singleton<TCompressorsHolder>()->GetMarkupZonesCompressor();
}

NCompTable::TChunkDecompressor& GetMarkupZonesDecompressor() {
    return Singleton<TDecompressorsHolder>()->GetMarkupZonesDecompressor();
}

NCompTable::TChunkCompressor& GetSentInfosCompressor() {
    return Singleton<TCompressorsHolder>()->GetSentInfosCompressor();
}

NCompTable::TChunkDecompressor& GetSentInfosDecompressor() {
    return Singleton<TDecompressorsHolder>()->GetSentInfosDecompressor();
}

} //NArc
