#pragma once

#include <library/cpp/dbg_output/dump.h>

#include "base_wad_printer.h"
#include "erf_printer.h"
#include "key_inv_printer.h"
#include "herf_printer.h"
#include "omni_printer.h"
#include "reg_erf_printer.h"
#include "reg_herf_printer.h"

#include <kernel/doom/chunked_wad/chunked_mega_wad.h>

#include <kernel/doom/offroad_ann_data_wad/ann_data_hit_adaptors.h>
#include <kernel/doom/offroad_ann_data_wad/offroad_ann_data_wad_io.h>
#include <kernel/doom/offroad_doc_wad/offroad_doc_wad_searcher.h>
#include <kernel/doom/offroad_doc_wad/offroad_factor_ann_doc_wad_io.h>
#include <kernel/doom/offroad_key_wad/offroad_key_wad_io.h>
#include <kernel/doom/offroad_sent_wad/offroad_sent_wad_io.h>
#include <kernel/doom/offroad_sent_wad/sent_hit_adaptors.h>
#include <tools/idx_print/wad_printers/doc_wad_printer.h>
#include <tools/idx_print/wad_printers/inv_hash_wad_printer.h>
#include <tools/idx_print/wad_printers/text_archive_printer.h>

class TSmartWadPrinter : public TBaseWadPrinter {
public:
    TSmartWadPrinter(const TIdxPrintOptions& options, NDoom::IWad* wad)
        : TBaseWadPrinter(options, wad)
        , GlobalLumps_(wad->GlobalLumps())
        , DocLumps_(wad->DocLumps())
    {
        Sort(DocLumps_);
        Sort(GlobalLumps_);
        InitSubPrinters();
        CheckRequestedPrinters();
    }

    virtual void DoPrint(ui32 docId, IOutputStream* out) override {
        for (const auto& printer: Printers_) {
            if (CheckPrinterName(printer)) {
                if (Printers_.size() > 1)
                    *out << "\n\n*** Listing " << printer->Name() << Endl;
                printer->Print(docId, out);
            }
        }
    }

    virtual void PrintKeys(IOutputStream* out) override {
        for (const auto& printer: Printers_) {
            if (CheckPrinterName(printer)) {
                if (Printers_.size() > 1)
                    *out << "\n\n*** Listing " << printer->Name() << Endl;
                printer->Print(out);
            }
        }
    }

    virtual const char* Name() override {
        return "SmartWadPrinter";
    }

private:
    bool CheckPrinterName(const THolder<TBaseWadPrinter>& printer) {
        return Options_.WadPrinters.empty() ||
                (Options_.WadPrinters && Find(Options_.WadPrinters.begin(), Options_.WadPrinters.end(), printer->Name()) != Options_.WadPrinters.end());
    }

    void CheckRequestedPrinters() {
        for (const TString& printer : Options_.WadPrinters) {
            Y_ENSURE(Find(AvaliablePrinterNames_.begin(), AvaliablePrinterNames_.end(), printer) != AvaliablePrinterNames_.end(),
                    "No printer " << printer << " avaliable. List is " << DbgDump(AvaliablePrinterNames_));
        }
    }

    template <class Printer>
    bool HasLumpsForPrinter() {
        for (const auto& lump: Printer::UsedGlobalLumps()) {
           if (!Wad_->HasGlobalLump(lump)) {
                return false;
           }
        }
        TVector<NDoom::TWadLumpId> docLumps = Wad_->DocLumps();
        for (const auto& lump: Printer::UsedDocLumps()) {
            if (Find(docLumps.begin(), docLumps.end(), lump) == docLumps.end()) {
                return false;
            }
        }
        return true;
    }

    template <class Printer>
    void SubtractUsedLumps() {
        TLumps restGlobalLumps;
        TLumps restDocLumps;
        std::set_difference(GlobalLumps_.begin(), GlobalLumps_.end(), Printer::UsedGlobalLumps().begin(), Printer::UsedGlobalLumps().end(),
                            std::inserter(restGlobalLumps, restGlobalLumps.begin()));
        std::set_difference(DocLumps_.begin(), DocLumps_.end(), Printer::UsedDocLumps().begin(), Printer::UsedDocLumps().end(),
                        std::inserter(restDocLumps, restDocLumps.begin()));
        GlobalLumps_ = restGlobalLumps;
        DocLumps_ = restDocLumps;
    }

    // rewrite in C++17 way https://stackoverflow.com/questions/62343424/detecting-if-a-class-member-exists-with-c17
    template <class Printer, class = void>
    struct HasOptionalDocLumps : std::false_type {};
    template <class Printer>
    struct HasOptionalDocLumps<Printer, std::void_t<decltype(Printer::OptionalDocLumps)>> : std::true_type {};

    template <class Printer>
    void SubtractOptionalDocLumps() {
        if constexpr (HasOptionalDocLumps<Printer>::value) {
            TLumps restDocLumps;
            std::set_difference(DocLumps_.begin(), DocLumps_.end(), Printer::OptionalDocLumps().begin(), Printer::OptionalDocLumps().end(),
                            std::inserter(restDocLumps, restDocLumps.begin()));
            DocLumps_ = restDocLumps;
        }
    }

    void InitSubPrinters() {
        using namespace NDoom;

        AddSubPrinter<TKeyInvWadPrinter<TOffroadFactorAnnKeyWadIo, TOffroadFactorAnnDocWadIo>>();
        AddSubPrinter<TKeyInvWadPrinter<TOffroadKeyInvKeyWadIo, TOffroadKeyInvDocWadIo>>();
        AddSubPrinter<TL1Printer>();

        AddSubPrinter<TErfPrinter>();
        AddSubPrinter<TRegHostErfPrinter>();
        AddSubPrinter<THostErfPrinter>();
        AddSubPrinter<TRegErfPrinter>();
        AddSubPrinter<TInvHashWadPrinter>();
        AddSubPrinter<TExtInfoArcWadPrinter>();
        AddSubPrinter<TLinkAnnArcWadPrinter>();
        AddSubPrinter<TWadTextArchivePrinter>();

        AddSubPrinter<TDocWadPrinter<TOffroadFactorAnnDocWadIo>>();
        AddSubPrinter<TDocWadPrinter<TOffroadKeyInvDocWadIo>>();
        AddSubPrinter<TDocWadPrinter<TOffroadAnnDataDocWadIo>>();
        AddSubPrinter<TDocWadPrinter<TOffroadLinkAnnDataDocWadIo>>();
        AddSubPrinter<TDocWadPrinter<TOffroadFactorAnnDataDocWadIo>>();
        AddSubPrinter<TDocWadPrinter<TOffroadSentWadIo>>();
        AddSubPrinter<TDocWadPrinter<TOffroadAnnSentWadIo>>();
        AddSubPrinter<TDocWadPrinter<TOffroadFactorAnnSentWadIo>>();
        AddSubPrinter<TDocWadPrinter<TOffroadLinkAnnSentWadIo>>();

        AddSubPrinter<TGeneralOmniPrinter<TOmniUrlIo>>();
        AddSubPrinter<TGeneralOmniPrinter<TOmniTitleIo>>();
        AddSubPrinter<TGeneralOmniPrinter<TOmniRelCanonicalTargetIo>>();
        //AddSubPrinter<TGeneralOmniPrinter<TOmniDssmAnnXfDtShowOneSeAmSsHardCompressedEmbeddingRawIo>>();

        if (!TGeneralOmniPrinter<TOmniAnnRegStatsIo>::IsL2Wad(Wad_)) {
            AddSubPrinter<TGeneralOmniPrinter<TOmniAnnRegStatsIo>>();
            AddSubPrinter<TGeneralOmniPrinter<TOmniDssmAggregatedAnnRegEmbeddingIo>>();
            AddSubPrinter<TGeneralOmniPrinter<TOmniDssmAnnCtrCompressedEmbeddingIo>>();

            AddSubPrinter<TGeneralOmniPrinter<TOmniDssmAnnXfDtShowWeightCompressedEmbedding1Io>>();
            AddSubPrinter<TGeneralOmniPrinter<TOmniDssmAnnXfDtShowWeightCompressedEmbedding2Io>>();
            AddSubPrinter<TGeneralOmniPrinter<TOmniDssmAnnXfDtShowWeightCompressedEmbedding3Io>>();
            AddSubPrinter<TGeneralOmniPrinter<TOmniDssmAnnXfDtShowWeightCompressedEmbedding4Io>>();
            AddSubPrinter<TGeneralOmniPrinter<TOmniDssmAnnXfDtShowWeightCompressedEmbedding5Io>>();
            AddSubPrinter<TGeneralOmniPrinter<TOmniDssmAnnXfDtShowOneCompressedEmbeddingIo>>();
            AddSubPrinter<TGeneralOmniPrinter<TOmniDssmAnnXfDtShowOneSeCompressedEmbeddingIo>>();
            AddSubPrinter<TGeneralOmniPrinter<TOmniDssmMainContentKeywordsEmbeddingIo>>();
        } else {
            AddSubPrinter<TGeneralOmniPrinter<TOmniAnnRegStatsRawIo>>();
            AddSubPrinter<TGeneralOmniPrinter<TOmniDssmAggregatedAnnRegEmbeddingRawIo>>();
            AddSubPrinter<TGeneralOmniPrinter<TOmniDssmAnnCtrCompressedEmbeddingRawIo>>();
            AddSubPrinter<TGeneralOmniPrinter<TOmniDssmAnnXfDtShowWeightCompressedEmbeddingsRawIo>>();
            AddSubPrinter<TGeneralOmniPrinter<TOmniDssmAnnXfDtShowOneCompressedEmbeddingRawIo>>();
            AddSubPrinter<TGeneralOmniPrinter<TOmniDssmAnnXfDtShowOneSeCompressedEmbeddingRawIo>>();
            AddSubPrinter<TGeneralOmniPrinter<TOmniDssmMainContentKeywordsEmbeddingRawIo>>();
            AddSubPrinter<TGeneralOmniPrinter<TOmniDssmBertDistillL2EmbeddingRawIo>>();
            AddSubPrinter<TGeneralOmniPrinter<TOmniDssmNavigationL2EmbeddingRawIo>>();
            AddSubPrinter<TGeneralOmniPrinter<TOmniDssmAnnXfDtShowOneSeAmSsHardCompressedEmbeddingRawIo>>();
            AddSubPrinter<TGeneralOmniPrinter<TOmniDssmAnnSerpSimilarityHardSeCompressedEmbeddingRawIo>>();
            AddSubPrinter<TGeneralOmniPrinter<TOmniRecDssmSpyTitleDomainCompressedEmb12RawIo>>();
            AddSubPrinter<TGeneralOmniPrinter<TOmniRecCFSharpDomainRawIo>>();
            AddSubPrinter<TGeneralOmniPrinter<TOmniDssmFullSplitBertEmbeddingRawIo>>();
            AddSubPrinter<TGeneralOmniPrinter<TOmniDssmSinsigL2EmbeddingRawIo>>();
            AddSubPrinter<TGeneralOmniPrinter<TOmniReservedEmbeddingRawIo>>();
        }

        AddSubPrinter<TGeneralOmniPrinter<TOmniAliceWebMusicTrackTitleIo>>();
        AddSubPrinter<TGeneralOmniPrinter<TOmniAliceWebMusicArtistNameIo>>();

        for (NDoom::TWadLumpId lump : GlobalLumps_) {
            Cerr << "Warning: " << ToString(lump) << " printer not defined\n";
        }

        for (NDoom::TWadLumpId lump : DocLumps_) {
            Cerr << "Warning: " << ToString(lump) << " printer not defined\n";
        }
    }

    template<class Printer>
    void AddSubPrinter() {
        if (!HasLumpsForPrinter<Printer>())
            return;

        Printers_.emplace_back(new Printer(Options_, Wad_));
        AvaliablePrinterNames_.push_back(Printers_.back()->Name());
        SubtractUsedLumps<Printer>();
        SubtractOptionalDocLumps<Printer>();
    }

private:
    TVector<THolder<TBaseWadPrinter>> Printers_;
    TVector<TString> AvaliablePrinterNames_;
    TLumps GlobalLumps_;
    TLumps DocLumps_;
};

void PrintWad(const TIdxPrintOptions& options) {
    THolder<NDoom::IWad> wad = THolder(options.Chunked
        ? new NDoom::TChunkedMegaWad(options.IndexPath, false)
        : NDoom::IWad::Open(options.IndexPath).Release());

    TSmartWadPrinter printer(options, wad.Get());
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
