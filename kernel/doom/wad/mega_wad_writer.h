#pragma once

#include <kernel/doom/offroad_common/accumulating_output.h>

#include <library/cpp/offroad/wad/typed_wad_writer.h>
#include <library/cpp/offroad/flat/flat_writer.h>
#include <library/cpp/offroad/custom/ui32_vectorizer.h>
#include <library/cpp/offroad/custom/null_vectorizer.h>

#include <util/generic/algorithm.h>
#include <util/string/cast.h>
#include <util/stream/file.h>
#include <util/stream/direct_io.h>

#include "mega_wad_info.h"
#include "wad_writer.h"
#include "doc_lump_writer.h"

namespace NDoom {

/**
 * Universal writer for megawad. You can write just about anything into this wad.
 *
 * Usage:
 *
 * \code
 * IOutputStream* out = wad.StartGlobalLump(Type1);
 * out->Write(...);
 * wad.FinishGlobal();
 *
 * // ...
 *
 * for (auto doc : documents) {
 *     IOutputStream* out1 = wad.StartDocLump(doc.Id, Type1);
 *     IOutputStream* out2 = wad.StartDocLump(doc.Id, Type2);
 *     WriteFirstLump(out1);
 *     WriteSecondLump(out2);
 * }
 * wad.FinishDoc();
 *
 * wad.Finish();
 * \endcode
 */
class TMegaWadWriter: public IWadWriter {
public:
    TMegaWadWriter()
        : DocLumpsWriter_(&Info_)
    {
    }

    ~TMegaWadWriter() override {
        Finish();
    }

    TMegaWadWriter(const TString& path, ui32 bufferSize = 1 << 17)
        : DocLumpsWriter_(&Info_)
    {
        Reset(path, bufferSize);
    }

    TMegaWadWriter(IOutputStream *output)
        : DocLumpsWriter_(&Info_)
    {
        Reset(output);
    }

    void Reset(const TString& path, ui32 bufferSize = 1 << 17) {
        DirectIo_ = MakeHolder<TDirectIOBufferedFile>(path, WrOnly | Direct | Seq | CreateAlways, bufferSize);
        LocalOutput_ = MakeHolder<TRandomAccessFileOutput>(*DirectIo_);
        Reset(LocalOutput_.Get());
    }

    void Reset(IOutputStream *output) {
        Wad_.Reset(output, "MWAD");

        Info_.GlobalLumps.clear();
        Info_.FirstDoc = 0;
        Info_.DocCount = 0;
        DocLumpsWriter_.Reset(&Info_);
        CurrentLump_ = 0;
        State_ = Started;
    }

    void RegisterDocLumpType(TWadLumpId id) override {
        return RegisterDocLumpType(ToString(id));
    }

    void RegisterDocLumpType(TStringBuf id) override {
        Y_VERIFY(State_ == Started || State_ == WritingLeadingGlobal, "Cannot register doc lump types once document output has started.");
        DocLumpsWriter_.RegisterDocLumpType(id);
    }

    IOutputStream* StartGlobalLump(TWadLumpId id) override {
        return StartGlobalLump(ToString(id));
    }

    IOutputStream* StartGlobalLump(TStringBuf lumpName) override {
        Y_ENSURE_EX(!Info_.GlobalLumps.contains(lumpName), yexception() << "Only one global lump of type " << lumpName << " is allowed");

        Transition(State_ == Started || State_ == WritingLeadingGlobal ? WritingLeadingGlobal : WritingTrailingGlobal);
        Info_.GlobalLumps[lumpName] = CurrentLump_;

        return &Wad_;
    }

    IOutputStream* StartDocLump(ui32 docId, TWadLumpId id) override {
        return StartDocLump(docId, ToString(id));
    }

    IOutputStream* StartDocLump(ui32 docId, TStringBuf id) override {
        if (State_ != WritingDoc)
            Transition(WritingDoc);

        while (CurrentLump_ - Info_.FirstDoc < docId)
            Transition(WritingDoc);

        Y_ENSURE_EX(CurrentLump_ - Info_.FirstDoc == docId, yexception() << "DocId sequence must be sorted");

        return DocLumpsWriter_.StartDocLump(id);
    }

    bool IsFinished() const override {
        return State_ == Finished;
    }

    void Finish() override {
        if (IsFinished())
            return;

        Transition(Finished);

        if (LocalOutput_) {
            LocalOutput_->Finish();
            LocalOutput_.Reset();
            if (DirectIo_) {
                DirectIo_.Reset();
            }
        }
    }

private:
    enum EState {
        Started,
        WritingLeadingGlobal,
        WritingDoc,
        WritingTrailingGlobal,
        Finished
    };

    void Transition(EState newState) {
        if (State_ == WritingDoc) {
            FinishDoc();
            if (newState != WritingDoc)
                Info_.DocCount = CurrentLump_ - Info_.FirstDoc;
        } else if (State_ == WritingLeadingGlobal || State_ == WritingTrailingGlobal) {
            FinishGlobal();
        }

        if (newState == Finished) {
            SaveMegaWadInfo(&Wad_, Info_);
            Wad_.FinishLump();
            Wad_.Finish();
        } else if (newState == WritingDoc) {
            Y_VERIFY(State_ == Started || State_ == WritingDoc || State_ == WritingLeadingGlobal);

            if (State_ == Started || State_ == WritingLeadingGlobal)
                Info_.FirstDoc = CurrentLump_;
        } else if (newState == WritingLeadingGlobal) {
            Y_VERIFY(State_ == Started || State_ == WritingLeadingGlobal);

            /* Do nothing. */
        } else if (newState == WritingTrailingGlobal) {
            Y_VERIFY(State_ == WritingDoc || State_ == WritingTrailingGlobal);

            /* Do nothing. */
        }

        State_ = newState;
    }

    void FinishDoc() {
        Y_ASSERT(State_ == WritingDoc);

        DocLumpsWriter_.FinishDoc(&Wad_);

        Wad_.FinishLump();
        CurrentLump_++;
    }

    void FinishGlobal() {
        Y_ASSERT(State_ == WritingLeadingGlobal || State_ == WritingTrailingGlobal);

        Wad_.FinishLump();
        CurrentLump_++;
    }

private:
    THolder<TDirectIOBufferedFile> DirectIo_;
    THolder<IOutputStream> LocalOutput_;
    NOffroad::TTypedWadWriter Wad_;

    TMegaWadInfo Info_;

    TDocLumpWriter DocLumpsWriter_;

    ui32 CurrentLump_ = 0;
    EState State_ = Started;
};

} // namespace NDoom
