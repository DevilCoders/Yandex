#include "parser.h"
#include "to_enum.h"
#include "base_types.h"

#include <library/cpp/protobuf/parser/options.pb.h>

#include <google/protobuf/message.h>
#include <google/protobuf/descriptor.h>
#include <util/string/cast.h>

namespace NProtoParser {
    namespace {
    //
    // AddConvertedValue functions
    //

#define ADD_CONVERTED_VALUE(type, argument)                                                        \
    void AddConvertedValue(TMessageParser& message, const FieldDescriptor* desc, argument value) { \
        Message* const msg = &message.GetMessage();                                                \
        const Reflection* reflection = msg->GetReflection();                                       \
        if (desc->is_repeated())                                                                   \
            reflection->Add##type(msg, desc, value);                                               \
        else                                                                                       \
            reflection->Set##type(msg, desc, value);                                               \
    }

        ADD_CONVERTED_VALUE(Int32, i32)
        ADD_CONVERTED_VALUE(Int64, i64)
        ADD_CONVERTED_VALUE(UInt32, ui32)
        ADD_CONVERTED_VALUE(UInt64, ui64)
        ADD_CONVERTED_VALUE(Float, float)
        ADD_CONVERTED_VALUE(Double, double)
        ADD_CONVERTED_VALUE(Bool, bool)
        ADD_CONVERTED_VALUE(Enum, const EnumValueDescriptor*)
        ADD_CONVERTED_VALUE(String, TString)

#undef ADD_CONVERTED_VALUE
    }

    template <class TValue>
    void TMessageParser::MergeValue(const FieldDescriptor* desc, const TValue& value) {
        using namespace NBaseTypesConvertor;
        using namespace NEnumConvertor;
        if (!desc) {
            if (IsSilent() || IsIgnoreUnknown())
                return;
            ythrow yexception() << "No field descriptor";
        }
        try {
            const FieldDescriptor::CppType cppType = desc->cpp_type();
            switch (cppType) {
                case FieldDescriptor::CPPTYPE_INT32:
                    AddConvertedValue(*this, desc, ToInteger<i32>(value));
                    break;
                case FieldDescriptor::CPPTYPE_INT64:
                    AddConvertedValue(*this, desc, ToInteger<i64>(value));
                    break;
                case FieldDescriptor::CPPTYPE_UINT32:
                    AddConvertedValue(*this, desc, ToInteger<ui32>(value));
                    break;
                case FieldDescriptor::CPPTYPE_UINT64:
                    AddConvertedValue(*this, desc, ToInteger<ui64>(value));
                    break;
                case FieldDescriptor::CPPTYPE_DOUBLE:
                    AddConvertedValue(*this, desc, ToFloat<double>(value));
                    break;
                case FieldDescriptor::CPPTYPE_FLOAT:
                    AddConvertedValue(*this, desc, ToFloat<float>(value));
                    break;
                case FieldDescriptor::CPPTYPE_BOOL:
                    AddConvertedValue(*this, desc, ToBool(value));
                    break;
                case FieldDescriptor::CPPTYPE_ENUM:
                    AddConvertedValue(*this, desc, ToEnum(value, desc));
                    break;
                case FieldDescriptor::CPPTYPE_STRING:
                    AddConvertedValue(*this, desc, ToString(value));
                    break;
                case FieldDescriptor::CPPTYPE_MESSAGE:
                    ythrow yexception() << "Submessage " << desc->name() << " is not supported";
                    break;
                default:
                    ythrow yexception() << "cpp type " << static_cast<int>(cppType) << " is invalid";
            }
        } catch (const TBadCastException&) {
            if (!IsSilent())
                throw;
        }
    }

#define MERGE_DEFINITION(type)                                             \
    void TMessageParser::Merge(const FieldDescriptor* field, type value) { \
        MergeValue(field, value);                                          \
    }

    //MERGE_DEFINITION(TStringBuf)
    //MERGE_DEFINITION(TString)
    MERGE_DEFINITION(i64)
    MERGE_DEFINITION(ui64)
    MERGE_DEFINITION(i32)
    MERGE_DEFINITION(ui32)
    MERGE_DEFINITION(bool)
    MERGE_DEFINITION(float)
    MERGE_DEFINITION(double)

#undef MERGE_DEFINITION

    void TMessageParser::Merge(const FieldDescriptor* field, const char* value) {
        Merge(field, TStringBuf(value));
    }

    void TMessageParser::MergeWithDelimiter(const FieldDescriptor* field, TStringBuf value, char delim) {
        if (value.empty())
            return MergeValue(field, value);

        while (!value.empty()) {
            const TStringBuf single = value.NextTok(delim);
            MergeValue(field, single);
        }
    }

    static char TryGetDelimiter(const FieldDescriptor* field) {
        if (!field || !field->is_repeated())
            return 0;
        if (field->options().HasExtension(delimiter)) {
            const TString delim = field->options().GetExtension(delimiter);
            if (delim.size() != 1)
                ythrow yexception() << "Delimiter size must be equal to 1";
            return delim[0];
        }
        return 0;
    }

    void TMessageParser::Merge(const FieldDescriptor* field, TStringBuf value) {
        if (char delim = TryGetDelimiter(field))
            return MergeWithDelimiter(field, value, delim);
        MergeValue(field, value);
    }

    void TMessageParser::Merge(const FieldDescriptor* field, TString value) {
        if (char delim = TryGetDelimiter(field))
            return MergeWithDelimiter(field, value, delim);
        MergeValue(field, value);
    }

    bool TMessageParser::IsSubMessage(const FieldDescriptor* field) {
        return field->type() == FieldDescriptor::TYPE_MESSAGE;
    }

    const FieldDescriptor* TMessageParser::FindFieldByName(const TStringBuf& field) {
        Buffer.assign(field.data(), field.size());
        return FindFieldByName(Buffer);
    }

    const FieldDescriptor* TMessageParser::FindFieldByName(const TString& field) {
        if (const FieldDescriptor* const fieldDescriptor = GetMessage().GetDescriptor()->FindFieldByName(field))
            return fieldDescriptor;

        // Not found
        if (IsSilent() || IsIgnoreUnknown())
            return nullptr;
        ythrow yexception() << "Message has no field '" << field << "'";
    }

    TMessageParser::TSubMessageHolder::TSubMessageHolder(TMessageParser* parent, const FieldDescriptor* desc)
        : Parent(parent)
        , Desc(desc)
        , SubMsg()
    {
        const Reflection* const reflection = Parent->GetMessage().GetReflection();
        if (Desc->is_repeated()) {
            SubMsg = reflection->AddMessage(&Parent->GetMessage(), Desc);
        } else {
            const bool needCopy = Parent->IsAtomic() && reflection->HasField(Parent->GetMessage(), Desc);
            SubMsg = reflection->MutableMessage(&Parent->GetMessage(), Desc);
            if (needCopy) { // make copy of submessage
                Copy.Reset(SubMsg->New());
                Copy->CopyFrom(*SubMsg);
            }
        }
    }

    TMessageParser::TSubMessageHolder::~TSubMessageHolder() {
        if (Parent) {
            const Reflection* const reflection = Parent->GetMessage().GetReflection();
            if (Desc->is_repeated()) {
                reflection->RemoveLast(&Parent->GetMessage(), Desc);
            } else {
                if (Copy.Get())
                    SubMsg->GetReflection()->Swap(SubMsg, Copy.Get());
                else
                    reflection->ClearField(&Parent->GetMessage(), Desc);
            }
        }
    }

}
