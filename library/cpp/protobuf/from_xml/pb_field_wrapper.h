#pragma once

#include "enum.h"
#include "pb.h"

#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/generic/yexception.h>
#include <util/system/yassert.h>

namespace NProtobufFromXml {
    inline yexception UnknownField(const TString& fieldName, const PbDescriptor* descriptor) {
        yexception ex;
        ex << "Field \"" << descriptor->full_name() << "::" << fieldName << "\" not found.\n";

        ex << "Known fields are:\n";
        for (int i = 0; i < descriptor->field_count(); ++i) {
            ex << "\t" << descriptor->field(i)->full_name() << "\n";
        }

        ex << "Known extensions are:\n";
        TVector<const PbFieldDescriptor*> extensions;
        descriptor->file()->pool()->FindAllExtensions(descriptor, &extensions);
        for (const auto& extension : extensions) {
            ex << "\t" << extension->full_name() << "\n";
        }
        return ex;
    }

    class TPbFieldWrapper {
    public:
        TPbFieldWrapper(PbMessage& msg, const TString& fieldName)
            : Message(msg)
            , Reflection(msg.GetReflection())
            , Descriptor(Message.GetDescriptor())
            , FieldDescriptor(Descriptor->FindFieldByName(fieldName))
        {
            if (!FieldDescriptor) {
                FieldDescriptor = Reflection->FindKnownExtensionByName(fieldName);
            }
            Y_ENSURE_EX(FieldDescriptor, UnknownField(fieldName, Descriptor));
        }

        PbFieldDescriptor::Type Type() const {
            return FieldDescriptor->type();
        }

        void RequireType(PbFieldDescriptor::Type type) const {
            Y_ENSURE(
                type == Type(),
                "Types mismatch in \"" << ParentName() << "::" << Name() << "\"");
        }

        TString Name() const {
            return FieldDescriptor->name();
        }

        TString ParentName() const {
            return Descriptor->full_name();
        }

        bool IsRepeated() const {
            return FieldDescriptor->is_repeated();
        }

        bool IsOptional() const {
            return FieldDescriptor->is_optional();
        }

        PbMessage& MutableMessage() {
            return *Reflection->MutableMessage(&Message, FieldDescriptor);
        }

        PbMessage& AddMessage() {
            return *Reflection->AddMessage(&Message, FieldDescriptor);
        }

        void Set(const TEnum& value) {
            const auto* enumValueDescr = FieldDescriptor
                                             ->enum_type()
                                             ->FindValueByName(value.Value);
            Y_ASSERT(enumValueDescr);
            Reflection->SetEnum(&Message, FieldDescriptor, enumValueDescr);
        }

        void Add(const TEnum& value) {
            const auto* enumValueDescr = FieldDescriptor
                                             ->enum_type()
                                             ->FindValueByName(value.Value);
            Y_ASSERT(enumValueDescr);
            Reflection->AddEnum(&Message, FieldDescriptor, enumValueDescr);
        }

#define FIELD_IMPL_SETTERS(CPP_TYPE, PB_TYPE)                       \
    void Set(const CPP_TYPE& value) {                               \
        Reflection->Set##PB_TYPE(&Message, FieldDescriptor, value); \
    }                                                               \
    void Add(const CPP_TYPE& value) {                               \
        Reflection->Add##PB_TYPE(&Message, FieldDescriptor, value); \
    }

        FIELD_IMPL_SETTERS(i32, Int32);
        FIELD_IMPL_SETTERS(ui32, UInt32);
        FIELD_IMPL_SETTERS(i64, Int64);
        FIELD_IMPL_SETTERS(ui64, UInt64);
        FIELD_IMPL_SETTERS(double, Double);
        FIELD_IMPL_SETTERS(float, Float);
        FIELD_IMPL_SETTERS(bool, Bool);
        FIELD_IMPL_SETTERS(TString, String);

#undef FIELD_IMPL_SETTERS

    private:
        PbMessage& Message;
        const PbReflection* Reflection;
        const PbDescriptor* Descriptor;
        const PbFieldDescriptor* FieldDescriptor;
    };

}
