#pragma once

#include "base_wad_printer.h"

#include <kernel/doom/wad/wad.h>
#include <kernel/doom/offroad_doc_wad/offroad_doc_wad_io.h>
#include <kernel/doom/offroad_struct_wad/struct_type.h>

template <class Io, bool Struct = false>
class TDocWadPrinter : public TBaseWadPrinter {
    using TSearcher = typename Io::TSearcher;
    using TIterator = typename TSearcher::TIterator;
    using THit = typename TIterator::THit;

public:
    TDocWadPrinter(const TIdxPrintOptions& options, NDoom::IWad* wad)
        : TBaseWadPrinter(options, wad)
    {
        Searcher_.Reset(wad);
    }

    static const TLumpSet& UsedDocLumps() {
        // TODO: HitSub currently ignored =(
        static TLumpSet lumps = Struct ? TLumpSet {NDoom::TWadLumpId(Io::IndexType, NDoom::EWadLumpRole::Struct)}
                                       : TLumpSet {NDoom::TWadLumpId(Io::IndexType, NDoom::EWadLumpRole::Hits)};
        return lumps;
    }

    static const TLumpSet& UsedGlobalLumps() {
        return UsedGlobalLumps(std::integral_constant<bool, Struct>());
    }

private:

    static const TLumpSet& UsedGlobalLumps(std::true_type) {
        static TLumpSet lumps = ( Io::StructType == NDoom::FixedSizeStructType ?
                           TLumpSet{ NDoom::TWadLumpId(Io::IndexType, NDoom::EWadLumpRole::StructModel),
                                     NDoom::TWadLumpId(Io::IndexType, NDoom::EWadLumpRole::StructSize) }
                         : TLumpSet{ NDoom::TWadLumpId(Io::IndexType, NDoom::EWadLumpRole::StructModel)} );
        return lumps;
    }

    static const TLumpSet& UsedGlobalLumps(std::false_type)  {
        static TLumpSet lumps = TLumpSet{ NDoom::TWadLumpId(Io::IndexType, NDoom::EWadLumpRole::HitsModel)} ;
        return lumps;
    }

    virtual void DoPrint(ui32 docId, IOutputStream* out) override {
        TIterator iterator;
        if (!Searcher_.Find(docId, &iterator))
            return;

        THit hit;
        CombineHit(hit, docId, std::is_integral<THit>());
        while (iterator.ReadHit(&hit))
            *out << "\t" << hit << "\n";
    }

    virtual const char* Name() override {
        if (Name_.empty()) {
            NDoom::EWadIndexType type = Io::IndexType;
            Name_ = ToString(type);
        }
        return Name_.c_str();
    }

    void CombineHit(THit& hit, ui32 docId, std::false_type) {
        hit.SetDocId(docId);
    }

    void CombineHit(THit& /*hit*/, ui32 /*docId*/, std::true_type) {
        //DoNothing
    }

private:
    TSearcher Searcher_;
    TString Name_;
};
