#pragma once

#include "fuzzy_values.h"

#include <library/cpp/protobuf/util/traits.h>

#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>

#include <util/generic/string.h>
#include <util/generic/ptr.h>
#include <util/memory/blob.h>
#include <util/random/fast.h>

namespace NProtoFuzz {
    void Fuzz(NProtoBuf::Message& message, ui64 seed = 0);

    // Supported fuzz actions
    enum class EAction {
        // set value for unset field, or
        // add new value to the end of repeated field;
        // new value is always != default
        //
        AddField = 0,

        // same as AddField, but
        // new value is always = default
        //
        AddDefaultField = 1,

        // clear value for set field, or
        // remove element at the end of repeated field
        //
        DeleteField = 2,

        // clear all values of repeated field
        //
        ClearRepeatedField = 3,

        // change value of set field, or
        // change value of repeated field element
        //
        ModifyField = 4
    };

    class TFuzzer {
    public:
        using TRng = TFastRng64;
        using TValues = TFuzzyValues<TRng>;

        class IFuzzPolicy;

        using TFuzzPolicyPtr = THolder<IFuzzPolicy>;

        struct TOptions {
            double FuzzRatio = 0.1;
            size_t MinFuzzDepth = 0;
            size_t MaxFuzzDepth = Max<size_t>();
            double FuzzStructureRatio = 0.0;
            size_t MaxAddedDepth = 1;
            ui64 Seed = 0; // 0 = randomize

            TOptions() = default;
            explicit TOptions(ui64 seed)
                : Seed(seed)
            {}

            TOptions ReSeed(ui64 seed) const {
                TOptions res = *this;
                res.Seed = seed;
                return res;
            }
        };

    public:
        TFuzzer();
        TFuzzer(const TOptions& opts);
        TFuzzer(const TOptions& opts, TFuzzPolicyPtr&& policy);

        void Fuzz(NProtoBuf::Message& message);
        void operator() (NProtoBuf::Message& message) {
            Fuzz(message);
        }

        void FuzzField(NProtoBuf::Message& message, const NProtoBuf::FieldDescriptor& fd);

        TRng& GetRng() {
            return *Rng;
        }

    private:
        bool CanFuzzFields(size_t depth) const {
            return depth >= Opts.MinFuzzDepth && depth <= Opts.MaxFuzzDepth;
        }
        bool CanFuzzMessages(size_t addedDepth) const {
            return addedDepth < Opts.MaxAddedDepth;
        }

        void DoFuzz(
            NProtoBuf::Message& message,
            size_t depth,
            size_t addedDepth);

        void DoFillRequiredFields(
            NProtoBuf::Message& message);

        void DoFillFieldIfRequired(
            NProtoBuf::Message& message,
            const NProtoBuf::FieldDescriptor& fd,
            const NProtoBuf::Reflection& reflect);

        void DoFuzzField(
            NProtoBuf::Message& message,
            const NProtoBuf::FieldDescriptor& fd,
            size_t depth, size_t addedDepth,
            bool canFuzzFields, bool canFuzzMessages);

        void FuzzMessageField(
            NProtoBuf::Message& message,
            const NProtoBuf::FieldDescriptor& fd,
            size_t depth, size_t addedDepth,
            bool canFuzz);

        void FuzzPlainField(
            NProtoBuf::Message& message,
            const NProtoBuf::FieldDescriptor& fd);

        void DoProcessPlainAction(
            NProtoBuf::Message& message,
            const NProtoBuf::FieldDescriptor& fd,
            EAction action);

    private:
        TOptions Opts;
        THolder<TRng> Rng;
        TFuzzPolicyPtr Policy;
    };

    // For simplicity to "fuzz" a value always
    // means that it will be changed, i.e.
    // Fuzz(x) != x
    class TFuzzer::IFuzzPolicy {
    public:
        virtual ~IFuzzPolicy() {}

        virtual void SetRng(TRng& rng) = 0;
        virtual TRng* GetRng() const = 0;

        virtual void SetFuzzerOptions(const TFuzzer::TOptions& opts) = 0;
        virtual const TFuzzer::TOptions& GetFuzzerOptions() const = 0;

        virtual bool IsAccepted(const NProtoBuf::FieldDescriptor& fd) const = 0;

        virtual void DoFuzzInt64(const NProtoBuf::FieldDescriptor& fd, i64& value) = 0;
        virtual void DoFuzzUInt64(const NProtoBuf::FieldDescriptor& fd, ui64& value) = 0;
        virtual void DoFuzzInt32(const NProtoBuf::FieldDescriptor& fd, i32& value) = 0;
        virtual void DoFuzzUInt32(const NProtoBuf::FieldDescriptor& fd, ui32& value) = 0;
        virtual void DoFuzzDouble(const NProtoBuf::FieldDescriptor& fd, double& value) = 0;
        virtual void DoFuzzBool(const NProtoBuf::FieldDescriptor& fd, bool& value) = 0;
        virtual void DoFuzzEnum(const NProtoBuf::FieldDescriptor& fd, const NProtoBuf::EnumValueDescriptor*& value) = 0;
        virtual void DoFuzzString(const NProtoBuf::FieldDescriptor& fd, TString& value) = 0;

        using TFieldType = NProtoBuf::FieldDescriptor::CppType;

        template <TFieldType Type>
        using TProtoValue = typename NProtoBuf::TCppTypeTraits<Type>::T;

        template <TFieldType Type>
        void DoFuzzValue(
            const NProtoBuf::FieldDescriptor& fd,
            TProtoValue<Type>& value);

        template <TFieldType Type, bool IsRepeated>
        void DoFuzz(
            NProtoBuf::Message& message,
            const NProtoBuf::FieldDescriptor& fd,
            size_t index = 0)
        {
            using TTraits = NProtoBuf::TFieldTraits<Type, IsRepeated>;

            TProtoValue<Type> value = TTraits::Get(message, &fd, index);
            DoFuzzValue<Type>(fd, value);
            TTraits::Set(message, &fd, value, index);
        }

        template <TFieldType Type, bool IsRepeated>
        void DoGen(
            NProtoBuf::Message& message,
            const NProtoBuf::FieldDescriptor& fd,
            size_t index = 0)
        {
            using TTraits = NProtoBuf::TFieldTraits<Type, IsRepeated>;

            TProtoValue<Type> value = TTraits::GetDefault(&fd);
            DoFuzzValue<Type>(fd, value);
            TTraits::Set(message, &fd, value, index);
        }

        template <TFieldType Type, bool IsRepeated>
        void DoReset(
            NProtoBuf::Message& message,
            const NProtoBuf::FieldDescriptor& fd,
            size_t index = 0)
        {
            using TTraits = NProtoBuf::TFieldTraits<Type, IsRepeated>;

            TProtoValue<Type> value = TTraits::GetDefault(&fd);
            TTraits::Set(message, &fd, value, index);
        }

        template <TFieldType Type>
        void DoAppend(
            NProtoBuf::Message& message,
            const NProtoBuf::FieldDescriptor& fd)
        {
            using TTraits = NProtoBuf::TFieldTraits<Type, true>;

            TProtoValue<Type> value = TTraits::GetDefault(&fd);
            TTraits::Add(message, &fd, value);
        }

        template <TFieldType Type>
        void DoAppendGen(
            NProtoBuf::Message& message,
            const NProtoBuf::FieldDescriptor& fd)
        {
            using TTraits = NProtoBuf::TFieldTraits<Type, true>;

            TProtoValue<Type> value = TTraits::GetDefault(&fd);
            DoFuzzValue<Type>(fd, value);
            TTraits::Add(message, &fd, value);
        }
    };

    class TDefaultFuzzPolicy;
} // NProtoFuzz
