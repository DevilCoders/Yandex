#pragma once

#include "base_wad_printer.h"
#include <kernel/doom/info/index_format.h>

#include <kernel/struct_codegen/print/struct_print.h>

#include <ysite/yandex/erf_format/erf_format.h>

template <class ErfIo>
class TGenericErfPrinter : public TBaseWadPrinter {
private:
    using TSearcher = typename ErfIo::TSearcher;
    static constexpr NDoom::EWadIndexType IndexType = ErfIo::IndexType;
    using THit = typename std::remove_pointer<typename ErfIo::TWriter::THit>::type;
public:
    TGenericErfPrinter(const TIdxPrintOptions& options, NDoom::IWad* wad)
        : TBaseWadPrinter(options, wad)
        , Searcher_(wad)
    {
    }

    static const TLumpSet& UsedGlobalLumps()  {

        static TLumpSet lumps = {
            NDoom::TWadLumpId(IndexType, NDoom::EWadLumpRole::StructModel),
            NDoom::TWadLumpId(IndexType, NDoom::EWadLumpRole::StructSize),
        };
        return lumps;
    }

    static const TLumpSet& UsedDocLumps() {

        static TLumpSet lumps = {
            NDoom::TWadLumpId(IndexType, NDoom::EWadLumpRole::Struct),
        };
        return lumps;
    }

private:

    void DoPrint(ui32 docId, IOutputStream* out) override {
        typename TSearcher::TIterator Iterator;
        typename TSearcher::THit hit = nullptr;
        if (Searcher_.Find(docId, &Iterator) && Iterator.ReadHit(&hit)) {
            *out << "DocId: " << docId << " {\n";
            *out << TCuteStrStructPrinter<THit>::PrintFields(*hit, typename THit::TFieldMask(true));
            *out << "\n}\n";
        }
    }

    const char* Name() override = 0;

private:
   TSearcher Searcher_;
};

