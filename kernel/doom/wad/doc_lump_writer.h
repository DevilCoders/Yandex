#pragma once

#include "mega_wad_info.h"

#include <kernel/doom/wad/check_sum_doc_lump.h>
#include <kernel/doom/offroad_common/calc_check_sum_stream.h>
#include <kernel/doom/offroad_common/accumulating_output.h>

#include <library/cpp/offroad/flat/flat_writer.h>
#include <library/cpp/offroad/custom/ui32_vectorizer.h>
#include <library/cpp/offroad/custom/null_vectorizer.h>
#include <library/cpp/digest/crc32c/crc32c.h>

#include <util/generic/algorithm.h>

namespace NDoom {

class TDocLumpWriter {
public:
    TDocLumpWriter() = default;

    TDocLumpWriter(TMegaWadInfo* info) {
        Reset(info);
    }

    void Reset(TMegaWadInfo* info) {
        Info_ = info;
        Info_->DocLumps.clear();
        Outputs_.clear();
        DocLumpIndexByType_.clear();
    }

    void RegisterDocLumpType(TWadLumpId id) {
        return RegisterDocLumpType(ToString(id));
    }

    void RegisterDocLumpType(TStringBuf lumpName) {
        Y_ASSERT(DocLumpIndexByType_.empty());

        if (Find(Info_->DocLumps.begin(), Info_->DocLumps.end(), lumpName) != Info_->DocLumps.end())
            return; /* Note that re-registration is OK, but you still cannot write the same lump twice. */

        Info_->DocLumps.emplace_back(lumpName);
    }

    IOutputStream* StartDocLump(TWadLumpId id) {
        return StartDocLump(ToString(id));
    }

    IOutputStream* StartDocLump(TStringBuf lumpName) {
        PrepareDocLumpMapping();

        Y_ENSURE_EX(DocLumpIndexByType_.contains(lumpName), yexception() << "Unexpected document lump type " << lumpName);

        ui32 docLumpIndex = DocLumpIndexByType_[lumpName];
        Y_ENSURE_EX(Outputs_[docLumpIndex].Size() == 0, yexception() << "Only one document lump of type " << lumpName << " is allowed");

        return &Outputs_[docLumpIndex];
    }

    IOutputStream* ResetDocLump(TWadLumpId id) {
        return ResetDocLump(ToString(id));
    }

    IOutputStream* ResetDocLump(TStringBuf lumpName) {
        PrepareDocLumpMapping();

        Y_ENSURE_EX(DocLumpIndexByType_.contains(lumpName), yexception() << "Unexpected document lump type " << lumpName);

        ui32 docLumpIndex = DocLumpIndexByType_[lumpName];
        Outputs_[docLumpIndex].Reset();

        return &Outputs_[docLumpIndex];
    }

    ui32 FinishDoc(IOutputStream* out) {
        PrepareDocLumpMapping();

        return WriteDocumentBuffer(out, /* copyToSteam = */ false);
    }

    TCalcCheckSumStream<TCrcExtendCalcer> CheckSumStream() {
        CalcCheckSumStream_.Reset();
        WriteDocumentBuffer(&CalcCheckSumStream_, /* copyToSteam = */ true);
        return CalcCheckSumStream_;
    }

    TCalcCheckSumStream<TCrcExtendCalcer>::TCheckSum CheckSum() {
        return CheckSumStream().CheckSum();
    }

private:
    void PrepareDocLumpMapping() {
        if (!Outputs_.empty() || Info_->DocLumps.empty())
            return;

        /* Check inner invariants just to feel safe. */
        Y_ASSERT(DocLumpIndexByType_.empty());

        /* We don't want lump order to affect md5. */
        Sort(Info_->DocLumps);

        Outputs_.resize(Info_->DocLumps.size());
        DocLumpIndexByType_.reserve(Info_->DocLumps.size());
        for (size_t i = 0; i < Info_->DocLumps.size(); ++i)
            DocLumpIndexByType_[Info_->DocLumps[i]] = i;
    }

    ui32 WriteDocumentBuffer(IOutputStream* out, bool copyToSteam = false) {
        ui32 result = 0;

        /* Write lengths only if we have to. */
        if (Outputs_.size() > 1) {
            ui32 offset = 0;
            TAccumulatingOutput offsetHolder;
            NOffroad::TFlatWriter<ui32, std::nullptr_t, NOffroad::TUi32Vectorizer, NOffroad::TNullVectorizer> writer(&offsetHolder);
            for (size_t i = 0; i < Outputs_.size() - 1; i++) {
                offset += Outputs_[i].Size();
                writer.Write(offset, nullptr);
            }
            writer.Finish();

            result = offsetHolder.Size();
            offsetHolder.Flush(out);
        }

        /* Write all streams. */
        for (TAccumulatingOutput& output: Outputs_) {
            result += output.Size();
            if (copyToSteam) {
                out->Write(output.Buffer().Data(), output.Buffer().Size());
            } else {
                output.Flush(out);
            }
        }

        return result;
    }

private:
    TMegaWadInfo* Info_ = nullptr;
    THashMap<TString, ui32> DocLumpIndexByType_;
    TVector<TAccumulatingOutput> Outputs_;

    TCalcCheckSumStream<TCrcExtendCalcer> CalcCheckSumStream_;
};

}
