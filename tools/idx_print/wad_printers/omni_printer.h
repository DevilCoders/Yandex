#pragma once

#include "base_wad_printer.h"
#include <kernel/doom/info/index_format.h>
#include <kernel/indexdoc/omnidoc.cpp>
#include <util/generic/strbuf.h>
#include <util/string/hex.h>
#include <utility>


inline TString HexEncode(const TArrayRef<const char>& h) {
    return HexEncode(h.data(), h.size());
}

class TL1Printer: public TBaseWadPrinter {
public:
    TL1Printer(const TIdxPrintOptions& options, NDoom::IWad* wad)
        : TBaseWadPrinter(options, wad)
        , Reader_(
            !wad->HasGlobalLump(NDoom::TWadLumpId(NDoom::OmniUrlType, NDoom::EWadLumpRole::StructModel)) ? nullptr : wad,
            !wad->HasGlobalLump(NDoom::TWadLumpId(NDoom::OmniUrlType, NDoom::EWadLumpRole::StructModel)) ? wad : nullptr,
              nullptr, TString(), true)
    {
        DssmLogDwellTimeBigramsEmbeddingAccessor_.Reset(TOmniAccessorFactory::NewAccessor<NDoom::TOmniDssmLogDwellTimeBigramsEmbeddingRawIo>(&Reader_) );
        DssmPantherTermsEmbeddingAccessor_.Reset(TOmniAccessorFactory::NewAccessor<NDoom::TOmniDssmPantherTermsEmbeddingRawIo>(&Reader_) );
    }

    static const TLumpSet& UsedGlobalLumps()  {
        static TLumpSet lumps = {
            NDoom::TWadLumpId(NDoom::OmniDssmEmbeddingType, NDoom::EWadLumpRole::StructModel),
            NDoom::TWadLumpId(NDoom::OmniDssmEmbeddingType, NDoom::EWadLumpRole::StructSize)
        };
        return lumps;
    }

    static const TLumpSet& UsedDocLumps() {
        static TLumpSet lumps = {
            NDoom::TWadLumpId(NDoom::OmniDssmEmbeddingType, NDoom::EWadLumpRole::Struct)
        };
        return lumps;
    }

private:
    void DoPrint(ui32 docId, IOutputStream* out) override {
        if (!Wad_->HasGlobalLump(NDoom::TWadLumpId(NDoom::OmniUrlType, NDoom::EWadLumpRole::StructModel))) {
            *out << "DocId: " << docId << " {\n";
            *out << "      " << "DssmEmbedding: " << HexEncode(DssmLogDwellTimeBigramsEmbeddingAccessor_->GetHit(docId)) << "\n";
            if (DssmPantherTermsEmbeddingAccessor_->IsInited()) {
                *out << "      " << "PantherTermsEmbedding: " << HexEncode(DssmPantherTermsEmbeddingAccessor_->GetHit(docId)) << "\n";
            }
            *out << "}\n";
        }
    }

    const char* Name() override {
        return "L1";
    }


private:
    using TOmniDssmPantherTermsEmbeddingRawAccessor = TDocOmniIndexAccessor<NDoom::TOmniDssmPantherTermsEmbeddingRawIo>;

    TDocOmniWadIndex Reader_;
    THolder<TOmniDssmPantherTermsEmbeddingRawAccessor> DssmPantherTermsEmbeddingAccessor_;
    THolder<TOmniDssmLogDwellTimeBigramsEmbeddingRawAccessor> DssmLogDwellTimeBigramsEmbeddingAccessor_;
};

namespace NPrivate {

template<class Hit>
void PrintHit(const Hit& hit, IOutputStream* out) {
    *out << "    " << hit;
}

template<>
void PrintHit(const TArrayRef<const char>& hit, IOutputStream* out) {
    *out << "    " << HexEncode(hit);
}

}

template <class Io>
class TGeneralOmniPrinter: public TBaseWadPrinter {
    using TAccessor = typename std::remove_pointer<decltype(TOmniAccessorFactory::NewAccessor<Io>(nullptr).Get())>::type;

public:
    TGeneralOmniPrinter(const TIdxPrintOptions& options, NDoom::IWad* wad)
        : TBaseWadPrinter(options, wad)
        , Reader_(
            IsL2Wad(wad) ? nullptr : wad,
            nullptr,
            IsL2Wad(wad) ? wad : nullptr,
              TString(), true)
        , Accessor_(TOmniAccessorFactory::NewAccessor<Io>(&Reader_))
    {
        NDoom::EWadIndexType indexType = Io::IndexType;
        DataName_ = ToString(indexType);
    }

    static bool IsL2Wad(NDoom::IWad* wad) {
        return wad->HasGlobalLump(NDoom::TWadLumpId(NDoom::OmniDssmAnnXfDtShowWeightCompressedEmbeddingsType, NDoom::EWadLumpRole::StructModel));
    }

    static const TLumpSet& UsedGlobalLumps()  {
        static TLumpSet lumps = TAccessor::IsFixedSizeStruct ?
        TLumpSet {
            NDoom::TWadLumpId(Io::IndexType, NDoom::EWadLumpRole::StructModel),
            NDoom::TWadLumpId(Io::IndexType, NDoom::EWadLumpRole::StructSize)
        } :
        TLumpSet {
            NDoom::TWadLumpId(Io::IndexType, NDoom::EWadLumpRole::StructModel)
        };

        return lumps;
    }

    static const TLumpSet& UsedDocLumps() {
        NDoom::EWadIndexType indexType = Io::IndexType;
        static TLumpSet lumps = {
            NDoom::TWadLumpId(indexType, NDoom::EWadLumpRole::Struct)
        };
        return lumps;
    }

    void DoPrint(ui32 docId, IOutputStream* out) override {
        *out << "DocId: " << docId << " {\n";
        *out << "    ";
        if (TAccessor::IsFixedSizeStruct)
            NPrivate::PrintHit(HexEncode(Accessor_->GetHit(docId)), out);
        else
            NPrivate::PrintHit(Accessor_->GetHit(docId), out);

        *out << "\n}";
    }

    const char* Name() override {
        return DataName_.c_str();
    }

private:
    TDocOmniWadIndex Reader_;
    THolder<TAccessor> Accessor_;
    TString DataName_;
};
