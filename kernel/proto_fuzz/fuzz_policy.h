#pragma once

#include "fuzzer.h"

#include <util/generic/singleton.h>

namespace NProtoFuzz {
    class TDefaultFuzzPolicy
        : public TFuzzer::IFuzzPolicy
    {
    public:
        using TValues = TFuzzer::TValues;
        using TRng = TFuzzer::TRng;
        using TOptions = TFuzzer::TOptions;

        static const size_t SmallStringLen = 32;
        static const size_t MaxStringLen = 1024;

        ~TDefaultFuzzPolicy() override {}

        void SetRng(TRng& rng) override {
            Rng = &rng;
        }
        TRng* GetRng() const override {
            return Rng;
        }

        void SetFuzzerOptions(const TOptions& opts) override {
            Opts = &opts;
        }
        const TOptions& GetFuzzerOptions() const override {
            return (!!Opts ? *Opts : *Singleton<TOptions>());
        }

        bool IsAccepted(
            const NProtoBuf::FieldDescriptor& /*fd*/) const override
        {
            return true;
        }

        void DoFuzzInt64(
            const NProtoBuf::FieldDescriptor& fd,
            i64& value) override;

        void DoFuzzUInt64(
            const NProtoBuf::FieldDescriptor& fd,
            ui64& value) override;

        void DoFuzzInt32(
            const NProtoBuf::FieldDescriptor& fd,
            i32& value) override;

        void DoFuzzUInt32(
            const NProtoBuf::FieldDescriptor& fd,
            ui32& value) override;

        void DoFuzzDouble(
            const NProtoBuf::FieldDescriptor& fd,
            double& value) override;

        void DoFuzzBool(
            const NProtoBuf::FieldDescriptor& fd,
            bool& value) override;

        void DoFuzzEnum(
            const NProtoBuf::FieldDescriptor& fd,
            const NProtoBuf::EnumValueDescriptor*& value) override;

        void DoFuzzString(
            const NProtoBuf::FieldDescriptor& fd,
            TString& value) override;

    private:
        TRng* Rng = nullptr;
        const TOptions* Opts = nullptr;
    };
} // NProtoFuzz
