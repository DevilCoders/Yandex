#pragma once

#include "base_wad_printer.h"
#include <kernel/doom/info/index_format.h>

#include <kernel/struct_codegen/print/struct_print.h>

#include <ysite/yandex/erf_format/erf_format.h>

template <class RegErfIo>
class TGenericRegErfPrinter : public TBaseWadPrinter {
private:
    using TReader = typename RegErfIo::TReader;
    static constexpr NDoom::EWadIndexType IndexType = RegErfIo::IndexType;
    using TData = typename std::remove_pointer<typename RegErfIo::TWriter::TData>::type;
public:
    TGenericRegErfPrinter(const TIdxPrintOptions& options, NDoom::IWad* wad)
        : TBaseWadPrinter(options, wad)
        , Reader_(wad)
    {
    }

    static const TLumpSet& UsedGlobalLumps()  {

        static TLumpSet lumps = {
            NDoom::TWadLumpId(IndexType, NDoom::EWadLumpRole::StructModel),
            NDoom::TWadLumpId(IndexType, NDoom::EWadLumpRole::StructSize),
            NDoom::TWadLumpId(IndexType, NDoom::EWadLumpRole::Struct),
        };
        return lumps;
    }

    static const TLumpSet& UsedDocLumps() {

        static TLumpSet lumps = {
        };
        return lumps;
    }

private:

    void PrintKeys(IOutputStream* out) override {
        using TKey = typename RegErfIo::TKey;
        using TData = typename RegErfIo::TData;
        TKey key;
        const TData* data;
        while (Reader_.Next(&key, &data)) {
            ui32 docId = key.DocId();
            ui32 region = key.Region();
            *out << "(DocId, region): (" << docId << ", " << region << ") {\n";
            *out << TCuteStrStructPrinter<TData>::PrintFields(*data, typename TData::TFieldMask(true));
            *out << "\n}\n";
        }
    }

    void DoPrint(ui32 docId, IOutputStream* out) override {
        Y_UNUSED(docId);
        *out << "Not supported\n";
    }

    const char* Name() override {
        return "RegHostErf";
    }

private:
    TReader Reader_;
};
