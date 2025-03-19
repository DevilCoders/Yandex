#include "fuzzer.h"
#include "fuzz_policy.h"

#include <util/random/entropy.h>
#include <util/stream/buffer.h>
#include <util/generic/buffer.h>
#include <util/generic/xrange.h>
#include <util/generic/array_size.h>

namespace {
    using namespace NProtoFuzz;

    inline EAction GetPlainAction(TFuzzer::TRng& rng) {
        return TFuzzer::TValues::GenEnum<EAction>(rng,
            {{EAction::AddDefaultField, 0.025},
            {EAction::AddField, 0.225},
            {EAction::DeleteField, 0.25},
            {EAction::ModifyField, 0.5}});
    }

    inline EAction GetSingleMessageAction(
        TFuzzer::TRng& rng,
        bool isEmpty)
    {
        return TFuzzer::TValues::GenEnum<EAction>(rng,
            {{EAction::AddDefaultField, isEmpty ? 0.1 : 0.0},
            {EAction::AddField, isEmpty ? 0.9 : 0.0},
            {EAction::DeleteField, isEmpty ? 0.0 : 1.0}});
    }

    inline EAction GetRepeatedMessageAction(
        TFuzzer::TRng& rng,
        bool isEmpty)
    {
        return TFuzzer::TValues::GenEnum<EAction>(rng,
            {{EAction::AddDefaultField, isEmpty ? 0.1 : 0.05},
            {EAction::AddField, isEmpty ? 0.9 : 0.45},
            {EAction::DeleteField, isEmpty ? 0.0 : 0.45},
            {EAction::ClearRepeatedField, isEmpty ? 0.0 : 0.05}});
    }

    struct TFuzzByType {
        template <NProtoBuf::FieldDescriptor::CppType Type>
        static void Do(
            TFuzzer::IFuzzPolicy& policy,
            NProtoBuf::Message& message,
            const NProtoBuf::FieldDescriptor& fd, size_t /*index*/)
        {
            policy.DoFuzz<Type, false>(message, fd);
        }
    };

    struct TFuzzRepeatedByType {
        template <NProtoBuf::FieldDescriptor::CppType Type>
        static void Do(
            TFuzzer::IFuzzPolicy& policy,
            NProtoBuf::Message& message,
            const NProtoBuf::FieldDescriptor& fd, size_t index)
        {
            policy.DoFuzz<Type, true>(message, fd, index);
        }
    };

    struct TGenByType {
        template <NProtoBuf::FieldDescriptor::CppType Type>
        static void Do(
            TFuzzer::IFuzzPolicy& policy,
            NProtoBuf::Message& message,
            const NProtoBuf::FieldDescriptor& fd, size_t /*index*/)
        {
            policy.DoGen<Type, false>(message, fd);
        }
    };

    struct TGenRepeatedByType {
        template <NProtoBuf::FieldDescriptor::CppType Type>
        static void Do(
            TFuzzer::IFuzzPolicy& policy,
            NProtoBuf::Message& message,
            const NProtoBuf::FieldDescriptor& fd, size_t index)
        {
            policy.DoGen<Type, true>(message, fd, index);
        }
    };

    struct TResetByType {
        template <NProtoBuf::FieldDescriptor::CppType Type>
        static void Do(
            TFuzzer::IFuzzPolicy& policy,
            NProtoBuf::Message& message,
            const NProtoBuf::FieldDescriptor& fd, size_t /*index*/)
        {
            policy.DoReset<Type, false>(message, fd);
        }
    };

    struct TResetRepeatedByType {
        template <NProtoBuf::FieldDescriptor::CppType Type>
        static void Do(
            TFuzzer::IFuzzPolicy& policy,
            NProtoBuf::Message& message,
            const NProtoBuf::FieldDescriptor& fd, size_t index)
        {
            policy.DoReset<Type, true>(message, fd, index);
        }
    };

    struct TAppendByType {
        template <NProtoBuf::FieldDescriptor::CppType Type>
        static void Do(
            TFuzzer::IFuzzPolicy& policy,
            NProtoBuf::Message& message,
            const NProtoBuf::FieldDescriptor& fd, size_t /*index*/)
        {
            policy.DoAppend<Type>(message, fd);
        }
    };

    struct TAppendGenByType {
        template <NProtoBuf::FieldDescriptor::CppType Type>
        static void Do(
            TFuzzer::IFuzzPolicy& policy,
            NProtoBuf::Message& message,
            const NProtoBuf::FieldDescriptor& fd, size_t /*index*/)
        {
            policy.DoAppendGen<Type>(message, fd);
        }
    };

    template <typename ByTypeOp>
    inline void ApplyByType(
        TFuzzer::IFuzzPolicy& policy,
        NProtoBuf::Message& message,
        const NProtoBuf::FieldDescriptor& fd,
        size_t index = 0)
    {
        switch (fd.cpp_type()) {
            case NProtoBuf::FieldDescriptor::CPPTYPE_INT32: {
                ByTypeOp::template Do<NProtoBuf::FieldDescriptor::CPPTYPE_INT32>(policy, message, fd, index);
                break;
            }
            case NProtoBuf::FieldDescriptor::CPPTYPE_INT64: {
                ByTypeOp::template Do<NProtoBuf::FieldDescriptor::CPPTYPE_INT64>(policy, message, fd, index);
                break;
            }
            case NProtoBuf::FieldDescriptor::CPPTYPE_UINT32: {
                ByTypeOp::template Do<NProtoBuf::FieldDescriptor::CPPTYPE_UINT32>(policy, message, fd, index);
                break;
            }
            case NProtoBuf::FieldDescriptor::CPPTYPE_UINT64: {
                ByTypeOp::template Do<NProtoBuf::FieldDescriptor::CPPTYPE_UINT64>(policy, message, fd, index);
                break;
            }
            case NProtoBuf::FieldDescriptor::CPPTYPE_DOUBLE: {
                ByTypeOp::template Do<NProtoBuf::FieldDescriptor::CPPTYPE_DOUBLE>(policy, message, fd, index);
                break;
            }
            case NProtoBuf::FieldDescriptor::CPPTYPE_FLOAT: {
                ByTypeOp::template Do<NProtoBuf::FieldDescriptor::CPPTYPE_FLOAT>(policy, message, fd, index);
                break;
            }
            case NProtoBuf::FieldDescriptor::CPPTYPE_BOOL: {
                ByTypeOp::template Do<NProtoBuf::FieldDescriptor::CPPTYPE_BOOL>(policy, message, fd, index);
                break;
            }
            case NProtoBuf::FieldDescriptor::CPPTYPE_ENUM: {
                ByTypeOp::template Do<NProtoBuf::FieldDescriptor::CPPTYPE_ENUM>(policy, message, fd, index);
                break;
            }
            case NProtoBuf::FieldDescriptor::CPPTYPE_STRING: {
                ByTypeOp::template Do<NProtoBuf::FieldDescriptor::CPPTYPE_STRING>(policy, message, fd, index);
                break;
            }
            case NProtoBuf::FieldDescriptor::CPPTYPE_MESSAGE: {
                Y_ASSERT(false);
                break;
            }
        }
    }
} // namespace

namespace NProtoFuzz {
    void Fuzz(NProtoBuf::Message& message, ui64 seed) {
        TFuzzer{TFuzzer::TOptions(seed)}(message);
    }

    TFuzzer::TFuzzer()
        : TFuzzer(TOptions{})
    {}

    TFuzzer::TFuzzer(const TOptions& opts)
        : TFuzzer(opts, MakeHolder<TDefaultFuzzPolicy>())
    {}

    TFuzzer::TFuzzer(const TOptions& opts, TFuzzPolicyPtr&& policy)
        : Opts(opts)
        , Rng(0 == opts.Seed ? new TRng(Seed()) : new TRng(opts.Seed))
        , Policy(std::forward<TFuzzPolicyPtr>(policy))
    {
        Policy->SetRng(*Rng);
        Policy->SetFuzzerOptions(Opts);
    }

    void TFuzzer::Fuzz(NProtoBuf::Message& message) {
        DoFuzz(message, 0, 0);
    }

    void TFuzzer::FuzzField(
        NProtoBuf::Message& message,
        const NProtoBuf::FieldDescriptor& fd)
    {
        DoFuzzField(message, fd,
            0, 0,
            CanFuzzFields(0), CanFuzzMessages(0));
    }

    void TFuzzer::DoFuzzField(
        NProtoBuf::Message& message,
        const NProtoBuf::FieldDescriptor& fd,
        size_t depth,
        size_t addedDepth,
        bool canFuzzFields,
        bool canFuzzMessages)
    {
        if (!Policy->IsAccepted(fd)) {
            return;
        }

        if (fd.cpp_type() == NProtoBuf::FieldDescriptor::CPPTYPE_MESSAGE) {
            FuzzMessageField(message, fd, depth, addedDepth, canFuzzFields && canFuzzMessages);
        } else if (canFuzzFields) {
            FuzzPlainField(message, fd);
        }
    }

    void TFuzzer::DoFuzz(
        NProtoBuf::Message& message,
        size_t depth,
        size_t addedDepth)
    {
        Y_ASSERT(addedDepth <= depth);

        const NProtoBuf::Descriptor* descr = message.GetDescriptor();
        const NProtoBuf::Reflection* reflect = message.GetReflection();

        if (Y_UNLIKELY(!descr || !reflect)) {
            Y_ASSERT(false);
            return;
        }

        const bool canFuzzFields = CanFuzzFields(depth);
        const bool canFuzzMessages = CanFuzzMessages(addedDepth);

        for (int i : xrange(descr->field_count())) {
            const NProtoBuf::FieldDescriptor* fd = descr->field(i);
            Y_ASSERT(!!fd);

            DoFuzzField(
                message, *fd,
                depth, addedDepth,
                canFuzzFields, canFuzzMessages);
        }
    }

    void TFuzzer::DoFillRequiredFields(
        NProtoBuf::Message& message)
    {
        const NProtoBuf::Descriptor* descr = message.GetDescriptor();
        const NProtoBuf::Reflection* reflect = message.GetReflection();

        if (Y_UNLIKELY(!descr || !reflect)) {
            Y_ASSERT(false);
            return;
        }

        for (int i : xrange(descr->field_count())) {
            const NProtoBuf::FieldDescriptor* fd = descr->field(i);
            Y_ASSERT(!!fd);

            if (Y_UNLIKELY(!fd)) {
                Y_ASSERT(false);
                continue;
            }

            DoFillFieldIfRequired(message, *fd, *reflect);
        }
    }

    void TFuzzer::DoFillFieldIfRequired(
        NProtoBuf::Message& message,
        const NProtoBuf::FieldDescriptor& fd,
        const NProtoBuf::Reflection& reflect)
    {
        if (fd.is_required() && !reflect.HasField(message, &fd)) {
            if (fd.cpp_type() == NProtoBuf::FieldDescriptor::CPPTYPE_MESSAGE) {
                NProtoBuf::Message* fieldMessage = reflect.MutableMessage(&message, &fd);
                if (Y_LIKELY(!!fieldMessage)) {
                    DoFillRequiredFields(*fieldMessage);
                } else {
                    Y_ASSERT(false);
                }
            } else {
                ApplyByType<TResetByType>(*Policy, message, fd);
            }
        }
    }

    void TFuzzer::FuzzMessageField(
        NProtoBuf::Message& message,
        const NProtoBuf::FieldDescriptor& fd,
        size_t depth, size_t addedDepth, bool canFuzz)
    {
        const NProtoBuf::Reflection* reflect = message.GetReflection();

        if (Y_UNLIKELY(!reflect)) {
            Y_ASSERT(false);
            return;
        }

        const double fuzzStructureRoll = Rng->GenRandReal1();
        const double fuzzDepthModifier = 1.0 / Log2(2.0 + addedDepth);
        const double fuzzStructureThreshold = Opts.FuzzStructureRatio * fuzzDepthModifier;

        if (fd.label() != NProtoBuf::FieldDescriptor::LABEL_REPEATED) {
            const bool isEmpty = !reflect->HasField(message, &fd);

            if (!isEmpty) {
                NProtoBuf::Message* curMessage = reflect->MutableMessage(&message, &fd);
                if (Y_LIKELY(!!curMessage)) {
                    DoFuzz(*curMessage, depth + 1, addedDepth);
                } else {
                    Y_ASSERT(false);
                }
            }

            if (canFuzz && fuzzStructureRoll < fuzzStructureThreshold) {
                EAction action = GetSingleMessageAction(*Rng, isEmpty);

                if (EAction::DeleteField == action) {
                    reflect->ClearField(&message, &fd);
                } else {
                    Y_ASSERT(EAction::AddField == action || EAction::AddDefaultField == action);

                    NProtoBuf::Message* newMessage = reflect->MutableMessage(&message, &fd);
                    if (Y_LIKELY(!!newMessage)) {
                        if (EAction::AddField == action) {
                            DoFuzz(*newMessage, depth + 1, isEmpty ? addedDepth + 1 : addedDepth);
                        } else {
                            DoFillRequiredFields(*newMessage);
                        }
                    } else {
                        Y_ASSERT(false);
                    }
                }
            }

            DoFillFieldIfRequired(message, fd, *reflect);
        } else {
            const bool isEmpty = (0 == reflect->FieldSize(message, &fd));

            for (int j : xrange(reflect->FieldSize(message, &fd))) {
                DoFuzz(*reflect->MutableRepeatedMessage(&message, &fd, j), depth + 1, addedDepth);
            }

            if (canFuzz && fuzzStructureRoll < fuzzStructureThreshold) {
                EAction action = GetRepeatedMessageAction(*Rng, isEmpty);

                switch (action) {
                    case EAction::AddDefaultField:
                    case EAction::AddField: {
                        NProtoBuf::Message* newMessage = reflect->AddMessage(&message, &fd);
                        if (Y_LIKELY(!!newMessage)) {
                            if (EAction::AddField == action) {
                                DoFuzz(*newMessage, depth + 1, addedDepth + 1);
                            } else {
                                DoFillRequiredFields(*newMessage);
                            }
                        } else {
                            Y_ASSERT(false);
                        }
                        break;
                    }
                    case EAction::DeleteField: {
                        if (!isEmpty) {
                            reflect->RemoveLast(&message, &fd);
                        }
                        break;
                    }
                    case EAction::ClearRepeatedField: {
                        reflect->ClearField(&message, &fd);
                        break;
                    }
                    default: {
                        Y_ASSERT(false);
                        break;
                    }
                }
            }
        }
    }

    void TFuzzer::FuzzPlainField(
        NProtoBuf::Message& message,
        const NProtoBuf::FieldDescriptor& fd)
    {
        if (Rng->GenRandReal1() < Opts.FuzzRatio) {
            EAction action = GetPlainAction(*Rng);
            DoProcessPlainAction(message, fd, action);
        } else {
            const NProtoBuf::Reflection* reflect = message.GetReflection();

            if (Y_UNLIKELY(!reflect)) {
                Y_ASSERT(false);
                return;
            }

            if (fd.is_required() && !reflect->HasField(message, &fd)) {
                ApplyByType<TResetByType>(*Policy, message, fd);
            }
        }
    }

    void TFuzzer::DoProcessPlainAction(
        NProtoBuf::Message& message,
        const NProtoBuf::FieldDescriptor& fd,
        EAction action)
     {
        switch(action) {
            case EAction::AddDefaultField:
            case EAction::AddField: {
                switch(fd.label()) {
                    case NProtoBuf::FieldDescriptor::LABEL_OPTIONAL:
                    case NProtoBuf::FieldDescriptor::LABEL_REQUIRED: {
                        if (!message.GetReflection()->HasField(message, &fd)) {
                            if (EAction::AddDefaultField == action) {
                                ApplyByType<TResetByType>(*Policy, message, fd);
                            } else {
                                ApplyByType<TFuzzByType>(*Policy, message, fd);
                            }
                        } else {
                            DoProcessPlainAction(message, fd,
                                TValues::GenEnum<EAction>(*Rng, {EAction::DeleteField, EAction::ModifyField}));
                        }
                        break;
                    }
                    case NProtoBuf::FieldDescriptor::LABEL_REPEATED: {
                        if (EAction::AddDefaultField == action) {
                            ApplyByType<TAppendByType>(*Policy, message, fd);
                        } else {
                            ApplyByType<TAppendGenByType>(*Policy, message, fd);
                        }
                        break;
                    }
                }
                break;
            }
            case EAction::DeleteField: {
                switch(fd.label()) {
                    case NProtoBuf::FieldDescriptor::LABEL_OPTIONAL: {
                        if (message.GetReflection()->HasField(message, &fd)) {
                            message.GetReflection()->ClearField(&message, &fd);
                        } else {
                            DoProcessPlainAction(message, fd,
                                TValues::GenEnum<EAction>(*Rng, {EAction::AddField, EAction::ModifyField}));
                        }
                        break;
                    }
                    case NProtoBuf::FieldDescriptor::LABEL_REQUIRED: {
                        ApplyByType<TResetByType>(*Policy, message, fd);
                        break;
                    }
                    case NProtoBuf::FieldDescriptor::LABEL_REPEATED: {
                        if (message.GetReflection()->FieldSize(message, &fd) > 0) {
                            message.GetReflection()->RemoveLast(&message, &fd);
                        } else {
                            DoProcessPlainAction(message, fd,
                                TValues::GenEnum<EAction>(*Rng, {EAction::AddField, EAction::ModifyField}));
                        }
                        break;
                    }
                }
                break;
            }
            case EAction::ModifyField: {
                switch(fd.label()) {
                    case NProtoBuf::FieldDescriptor::LABEL_OPTIONAL:
                    case NProtoBuf::FieldDescriptor::LABEL_REQUIRED: {
                        ApplyByType<TFuzzByType>(*Policy, message, fd);
                        break;
                    }
                    case NProtoBuf::FieldDescriptor::LABEL_REPEATED: {
                        const NProtoBuf::Reflection* reflect = message.GetReflection();
                        Y_ASSERT(!!reflect);
                        if (reflect->FieldSize(message, &fd) > 0) {
                            for (int i : xrange(reflect->FieldSize(message, &fd))) {
                                ApplyByType<TFuzzRepeatedByType>(*Policy, message, fd, i);
                            }
                        } else {
                            DoProcessPlainAction(message, fd, EAction::AddField);
                        }
                        break;
                    }
                }
                break;
            }
            default: {
                Y_ASSERT(false);
                break;
            }
        }
    }

    template<>
    void TFuzzer::IFuzzPolicy::DoFuzzValue<NProtoBuf::FieldDescriptor::CPPTYPE_UINT64>(
        const NProtoBuf::FieldDescriptor& fd, ui64& value)
    {
        DoFuzzUInt64(fd, value);
    }

    template<>
    void TFuzzer::IFuzzPolicy::DoFuzzValue<NProtoBuf::FieldDescriptor::CPPTYPE_UINT32>(
        const NProtoBuf::FieldDescriptor& fd, ui32& value)
    {
        DoFuzzUInt32(fd, value);
    }

    template<>
    void TFuzzer::IFuzzPolicy::DoFuzzValue<NProtoBuf::FieldDescriptor::CPPTYPE_INT64>(
        const NProtoBuf::FieldDescriptor& fd, i64& value)
    {
        DoFuzzInt64(fd, value);
    }

    template<>
    void TFuzzer::IFuzzPolicy::DoFuzzValue<NProtoBuf::FieldDescriptor::CPPTYPE_INT32>(
        const NProtoBuf::FieldDescriptor& fd, i32& value)
    {
        DoFuzzInt32(fd, value);
    }

    template<>
    void TFuzzer::IFuzzPolicy::DoFuzzValue<NProtoBuf::FieldDescriptor::CPPTYPE_DOUBLE>(
        const NProtoBuf::FieldDescriptor& fd, double& value)
    {
        DoFuzzDouble(fd, value);
    }

    template<>
    void TFuzzer::IFuzzPolicy::DoFuzzValue<NProtoBuf::FieldDescriptor::CPPTYPE_FLOAT>(
        const NProtoBuf::FieldDescriptor& fd, float& value)
    {
        double valueDouble = value;
        DoFuzzDouble(fd, valueDouble);
        value = static_cast<float>(valueDouble);
    }

    template<>
    void TFuzzer::IFuzzPolicy::DoFuzzValue<NProtoBuf::FieldDescriptor::CPPTYPE_BOOL>(
        const NProtoBuf::FieldDescriptor& fd, bool& value)
    {
        DoFuzzBool(fd, value);
    }

    template<>
    void TFuzzer::IFuzzPolicy::DoFuzzValue<NProtoBuf::FieldDescriptor::CPPTYPE_ENUM>(
        const NProtoBuf::FieldDescriptor& fd, const NProtoBuf::EnumValueDescriptor*& value)
    {
        DoFuzzEnum(fd, value);
    }

    template<>
    void TFuzzer::IFuzzPolicy::DoFuzzValue<NProtoBuf::FieldDescriptor::CPPTYPE_STRING>(
        const NProtoBuf::FieldDescriptor& fd, TString& value)
    {
        DoFuzzString(fd, value);
    }
} // NProtoFuzz
