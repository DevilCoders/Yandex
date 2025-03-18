#pragma once

#include "base_wad_printer.h"

#include <kernel/doom/info/index_format.h>

#include <search/itditp/static_features/library/static_features_io.h>

class TSlimIndexPrinter : public TBaseWadPrinter {
private:
    using TSearcher = NItdItp::TWebItdItpSlimIndexRawIo::TSearcher;
    static constexpr NDoom::EWadIndexType IndexType = NDoom::WebItdItpSlimIndexType;
    using THit = typename std::remove_pointer<NItdItp::TWebItdItpSlimIndexRawIo::TWriter::THit>::type;
public:
    TSlimIndexPrinter(const TIdxPrintOptions& options, NDoom::IWad* wad)
        : TBaseWadPrinter(options, wad)
    {
        Searcher_.Reset(wad);
    }

private:
    void DoPrint(ui32 docId, IOutputStream* out) override {
        typename TSearcher::TIterator Iterator;
        typename TSearcher::THit hit = nullptr;
        if (Searcher_.Find(docId, &Iterator) && Iterator.ReadHit(&hit)) {
            *out << "DocId: " << docId << " {\n";
            *out << "DocumentTimestampEncoded:" << hit->DocumentTimestampEncoded << "\n";
            *out << "HasTurboMobile:" << hit->HasTurboMobile << "\n";
            *out << "IsItdItpPornoDoc:" << hit->IsItdItpPornoDoc << "\n";
            *out << "HasTurboEcom:" << hit->HasTurboEcom;
            *out << "\n}\n";
        }
    }

    const char* Name() override {
        return "SlimItdItp";
    }

private:
   TSearcher Searcher_;
};

void PrintSlimIndex(const TIdxPrintOptions& options) {
    THolder<NDoom::IWad> wad = THolder(NDoom::IWad::Open(options.IndexPath).Release());

    TSlimIndexPrinter printer(options, wad.Get());
    if (options.DocIds.empty() && !options.PrintHits) {
        printer.Print();
    } else if (options.PrintHits) {
        if (options.DocIds.empty()) {
            for (ui32 docId = 0, wadSize = wad->Size(); docId < wadSize; ++docId) {
                printer.Print(docId);
            }
        } else {
            for (const ui32 docId: options.DocIds) {
                printer.Print(docId);
            }
        }
    }
}
