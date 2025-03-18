#pragma once

#include "wad_writer.h"

#include <util/stream/null.h>
#include <util/system/yassert.h>

namespace NOffroad {
    /**
     * An output stream that writes out data chunks (lumps) in WAD format. To be used with
     * `TTypedWadSearcher`.
     *
     * Internal data layout is as follows:
     * \code
     * [IWAD] - 4 byte file format signature.
     * [type] - user-specified signature for wad contents, also 4 bytes.
     * [lumps] - data lumps.
     * [sub] - flat-encoded subindex for data lumps.
     * [sub_size] - size of the subindex, 8 bytes.
     * \endcode
     *
     * Note that this is more of a low-level building block for the actual wad file
     * format. For a writer that operates on a well-defined format, see `TMegaWadWriter`.
     */
    class TTypedWadWriter: public TWadWriter {
        using TBase = TWadWriter;

    public:
        TTypedWadWriter() {
            Reset(nullptr, TString());
        }

        virtual ~TTypedWadWriter() {
            Finish();
        }

        TTypedWadWriter(IOutputStream* output, const TString& signature) {
            Reset(output, signature);
        }

        void Reset(IOutputStream* output, const TString& signature) {
            Y_VERIFY(signature.size() == 4 || output == nullptr);

            Output_ = output ? output : &Cnull;
            Signature_ = signature;

            /* Write out header directly so that it doesn't affect position. */
            Output_->Write("IWAD");
            Output_->Write(signature);

            TBase::Reset(output, this);
        }

        const TString& Signature() const {
            return Signature_;
        }

    protected:
        virtual void DoFinish() override {
            if (TBase::IsFinished())
                return;

            ui64 subPos = TBase::Position();
            TBase::DoFinish();
            ui64 subSize = TBase::Position() - subPos;

            /* Flat searcher that we use for subindex might 'over-read' up to 7
             * bytes, so spending 8 bytes here, while excessive, is intentional. */
            Write(&subSize, 8);
        }

    private:
        IOutputStream* Output_ = nullptr;
        TString Signature_;
    };

}
