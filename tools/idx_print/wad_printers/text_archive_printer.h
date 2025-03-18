#pragma once

#include <kernel/tarc/disk/wad_text_archive.h>
#include <kernel/tarc/markup_zones/arcreader.h>
#include <kernel/tarc/markup_zones/view_sent.h>
#include <ysite/yandex/srchmngr/arcmgr.h>

#include <util/memory/blob.h>

class TWadTextArchivePrinterBase: public TBaseWadPrinter {
public:
    TWadTextArchivePrinterBase(
        const TIdxPrintOptions& options,
        NDoom::IWad* wad,
        NDoom::TWadLumpId lump,
        EDataType dataType,
        TArrayRef<const NDoom::TWadLumpId> optionalLumps = {})
        : TBaseWadPrinter(options, wad)
    {
        WadPrinter = TWadTextArchiveManager(wad, dataType, lump, optionalLumps);
    }

    ui32 GetSize() const {
        return WadPrinter.GetDocCount();
    }

    static const TLumpSet& UsedGlobalLumps()  {
        static TLumpSet lumps = {
        };
        return lumps;
    }

    void PrintKeys(IOutputStream *out) override {
        for (ui32 docId = 0; docId < WadPrinter.GetDocCount(); ++docId) {
            *out << docId << Endl;
        }
    }
protected:
    TWadTextArchiveManager WadPrinter;
};

class TWadTextArchivePrinter: public TWadTextArchivePrinterBase {
public:
    TWadTextArchivePrinter(const TIdxPrintOptions& options, NDoom::IWad* wad)
        : TWadTextArchivePrinterBase(options, wad, TextArchiveLump, EDataType::DocText)
    {
    }

    static const TLumpSet& UsedDocLumps() {
        static TLumpSet lumps = {
            TextArchiveLump
        };
        return lumps;
    }

private:
    static constexpr NDoom::TWadLumpId TextArchiveLump = NDoom::TWadLumpId(NDoom::ArcIndexType, NDoom::EWadLumpRole::Struct);

    void DoPrint(ui32 docId, IOutputStream* out) override {
        TBlob localBlob = WadPrinter.GetDocText(docId)->UncompressBlob();
        if (!localBlob.Empty()) {
            try {
                *out << "DocId=" << docId << Endl;
                PrintDocText(*out, localBlob);
                *out << Endl;
            } catch(...) {
            };
        }
    }

    const char* Name() override {
        return "TextArchiveWad";
    }
};

class TExtInfoArcWadPrinter : public TWadTextArchivePrinterBase {
public:
    TExtInfoArcWadPrinter(const TIdxPrintOptions& options, NDoom::IWad* wad)
        : TWadTextArchivePrinterBase(options, wad, ExtInfoLump, EDataType::ExtInfo, BertLumps)
    {
    }

    static const TLumpSet& UsedDocLumps() {
        static TLumpSet lumps = {
            ExtInfoLump
        };
        return lumps;
    }

    static const TLumpSet& OptionalDocLumps() {
        static TLumpSet lumps(BertLumps.begin(), BertLumps.end());
        return lumps;
    }

private:
    static constexpr NDoom::TWadLumpId ExtInfoLump = NDoom::TWadLumpId(NDoom::ExtInfoArcIndexType, NDoom::EWadLumpRole::Struct);
    static constexpr std::array BertLumps = {
        NDoom::TWadLumpId(NDoom::EWadIndexType::BertEmbeddingIndexType, NDoom::EWadLumpRole::Struct),
        NDoom::TWadLumpId(NDoom::EWadIndexType::BertEmbeddingV2IndexType, NDoom::EWadLumpRole::Struct)
    };

    void DoPrint(ui32 docId, IOutputStream* out) override {
        TBlob localBlob = WadPrinter.GetExtInfo(docId)->UncompressBlob();
        if (!localBlob.Empty()) {
            try {
                *out << "DocId=" << docId << Endl;
                PrintExtInfo(*out, WadPrinter.GetExtInfo(docId)->UncompressBlob());
            } catch(...) {
            };
        }

        TBlob bertBlob = WadPrinter.GetBertEmbedding(docId)->UncompressBlob();
        if (!bertBlob.Empty()) {
            *out << "BertEmbedding=" << HexEncode(bertBlob.AsCharPtr(), bertBlob.Size()) << Endl;
        }

        bertBlob = WadPrinter.GetBertEmbeddingV2(docId)->UncompressBlob();
        if (!bertBlob.Empty()) {
            *out << "BertEmbeddingV2=" << HexEncode(bertBlob.AsCharPtr(), bertBlob.Size()) << Endl;
        }

        *out << Endl;
    }

    const char* Name() override {
        return "ExtInfoArcWad";
    }
};

class TLinkAnnArcWadPrinter : public TWadTextArchivePrinterBase {
public:
    TLinkAnnArcWadPrinter(const TIdxPrintOptions& options, NDoom::IWad* wad)
        : TWadTextArchivePrinterBase(options, wad, LinkAnnLump, EDataType::DocText)
    {
    }

    static const TLumpSet& UsedDocLumps() {
        static TLumpSet lumps = {
            LinkAnnLump
        };
        return lumps;
    }

private:
    static const NDoom::TWadLumpId LinkAnnLump;

    void DoPrint(ui32 docId, IOutputStream* out) override {
        TBlob localBlob = WadPrinter.GetDocText(docId)->UncompressBlob();
        if (!localBlob.Empty()) {
            try {
                *out << docId << Endl;
                PrintDocText(*out, localBlob);
                *out << Endl;
            } catch(...) {
            };
        }
    }

    const char* Name() override {
        return "LinkAnnArcWad";
    }
};

const NDoom::TWadLumpId TLinkAnnArcWadPrinter::LinkAnnLump = NDoom::TWadLumpId(NDoom::LinkAnnArcIndexType, NDoom::EWadLumpRole::Struct);
