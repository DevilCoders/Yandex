#pragma once

#include <library/cpp/offroad/keyinv/null_model.h>

#include <util/generic/yexception.h>
#include <util/stream/output.h>

namespace NOffroad {
    class TRawSampleStream: public IOutputStream {
    public:
        using TModel = TNullModel;
        using TPosition = ui64;

        TRawSampleStream(TModel*) {
        }

        void Reset(TModel*) {
            Finished_ = false;
        }

        bool IsFinished() const {
            return Finished_;
        }

        ui64 Position() const {
            return 0;
        }

    protected:
        void DoWrite(const void* /*buf*/, size_t /*len*/) override {
        }

        void DoFinish() override {
            Y_ENSURE(!Finished_);
            Finished_ = true;
        }

    private:
        bool Finished_ = false;
    };

    class TRawOutputStream: public IOutputStream {
    public:
        using TModel = TNullModel;
        using TTable = TNullTable;
        using TPosition = ui64;

        TRawOutputStream() = default;

        TRawOutputStream(const TTable* /*table*/, IOutputStream* output)
            : Output_(output)
        {
        }

        void Reset(const TTable* /*table*/, IOutputStream* output) {
            Output_ = output;
            Position_ = 0;
            Finished_ = false;
        }

        bool IsFinished() const {
            return Finished_;
        }

        ui64 Position() const {
            return Position_;
        }

    protected:
        void DoWrite(const void* buf, size_t len) override {
            Y_ENSURE(!Finished_);
            Output_->Write(buf, len);
            Position_ += len;
        }

        void DoFinish() override {
            Y_ENSURE(!Finished_);
            Finished_ = true;
        }

    private:
        IOutputStream* Output_ = nullptr;
        ui64 Position_ = 0;
        bool Finished_ = false;
    };

}
