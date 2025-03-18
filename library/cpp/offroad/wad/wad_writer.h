#pragma once

#include <util/stream/output.h>
#include <util/stream/null.h>

#include <library/cpp/offroad/flat/flat_writer.h>
#include <library/cpp/offroad/custom/ui64_vectorizer.h>
#include <library/cpp/offroad/custom/null_vectorizer.h>

namespace NOffroad {
    /**
     * An output stream that also writes subindex for lumps of data that are
     * written into it, so that it can be later used with `TWadSearcher`.
     *
     * Naming is homage to DOOM, which stored all its data in WAD files.
     *
     * Example usage:
     * \code
     * TWadWriter writer(wadOutput, subOutput);
     * for(size_t i = 0; i < count; i++) {
     *     WriteSomethingIntoStream(wadOutput);
     *     writer.FinishLump();
     * }
     * writer.Finish();
     * \endcode
     *
     * Note that this is more of a low-level building block for the actual wad file
     * format. For a writer that operates on a well-defined format, see `TMegaWadWriter`.
     */
    class TWadWriter: public IOutputStream {
        using TSubWriter = TFlatWriter<ui64, std::nullptr_t, TUi64Vectorizer, TNullVectorizer>;

    public:
        TWadWriter() {
            Reset(nullptr, nullptr);
        }

        virtual ~TWadWriter() {
            Finish();
        }

        TWadWriter(IOutputStream* lumpOutput, IOutputStream* subOutput) {
            Reset(lumpOutput, subOutput);
        }

        void Reset(IOutputStream* lumpOutput, IOutputStream* subOutput) {
            SubWriter_.Reset(subOutput);
            LumpOutput_ = lumpOutput ? lumpOutput : &Cnull;
            Position_ = 0;
        }

        ui64 Position() const {
            return Position_;
        }

        void FinishLump() {
            SubWriter_.Write(Position_, nullptr);
        }

        using IOutputStream::Finish;

        bool IsFinished() const {
            return SubWriter_.IsFinished();
        }

    protected:
        virtual void DoWrite(const void* buf, size_t len) override {
            LumpOutput_->Write(buf, len);
            Position_ += len;
        }

        virtual void DoWriteV(const TPart* parts, size_t count) override {
            LumpOutput_->Write(parts, count);

            for (size_t i = 0; i < count; i++)
                Position_ += parts[i].len;
        }

        virtual void DoFinish() override {
            if (IsFinished())
                return;

            SubWriter_.Finish();
        }

    private:
        ui64 Position_ = 0;
        IOutputStream* LumpOutput_ = nullptr;
        TSubWriter SubWriter_;
    };

}
