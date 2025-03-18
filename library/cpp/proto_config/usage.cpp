#include "usage.h"

#include <library/cpp/colorizer/colors.h>
#include <library/cpp/proto_config/protos/extensions.pb.h>

#include <util/generic/hash_set.h>
#include <util/string/builder.h>
#include <util/string/cast.h>

namespace NProtoConfig {
    namespace {
        TString BuildConfigFieldName(NColorizer::TColors& colors, const NProtoBuf::FieldDescriptor& descr) {
            TStringBuilder builder;
            builder << colors.GreenColor() << descr.name() << colors.OldColor();
            return builder;
        }

        TString BuildConfigFieldType(NColorizer::TColors& colors, const NProtoBuf::FieldDescriptor& descr) {
            TStringBuilder builder;

            builder << colors.LightBlueColor();
            if (descr.is_repeated()) {
                builder << "repeated ";
            }
            if (descr.type() == NProtoBuf::FieldDescriptor::TYPE_MESSAGE) {
                builder << descr.message_type()->name();
            } else {
                builder << descr.type_name();
            }
            builder << colors.OldColor();

            return builder;
        }

        TString BuildConfigFieldHelp(const NProtoBuf::FieldDescriptor& descr) {
            if (descr.options().HasExtension(Help)) {
                return descr.options().GetExtension(Help);
            }
            return "???";
        }

        TString BuildDefaultValue(const NProtoBuf::FieldDescriptor& descr) {
            Y_VERIFY(descr.has_default_value());

            switch (descr.cpp_type()) {
#define CASE(ProtoType, Get)                    \
    case NProtoBuf::FieldDescriptor::ProtoType: \
        return ToString(descr.Get());

                CASE(CPPTYPE_BOOL, default_value_bool);
                CASE(CPPTYPE_DOUBLE, default_value_double);
                CASE(CPPTYPE_FLOAT, default_value_float);
                CASE(CPPTYPE_INT32, default_value_int32);
                CASE(CPPTYPE_INT64, default_value_int64);
                CASE(CPPTYPE_UINT32, default_value_uint32);
                CASE(CPPTYPE_UINT64, default_value_uint64);

#undef CASE

                case NProtoBuf::FieldDescriptor::CPPTYPE_STRING:
                    return TStringBuilder() << '"' << descr.default_value_string() << '"';

                case NProtoBuf::FieldDescriptor::CPPTYPE_ENUM:
                    return TStringBuilder() << descr.default_value_enum()->name();

                case NProtoBuf::FieldDescriptor::CPPTYPE_MESSAGE:
                    ythrow yexception() << "Can't process default message value yet.";

                default:
                    ythrow yexception() << "Unknown default type.";
            }
        }

        void PrintConfigUsage(IOutputStream& outputStream, NColorizer::TColors& colors, THashSet<TString>& printedSections,
                              const NProtoBuf::Descriptor& descr) {
            for (int field = 0; field < descr.field_count(); ++field) {
                const NProtoBuf::FieldDescriptor& fieldDescr = *descr.field(field);

                outputStream << "    \""
                             << BuildConfigFieldName(colors, fieldDescr) << "\": "
                             << BuildConfigFieldType(colors, fieldDescr) << " - "
                             << BuildConfigFieldHelp(fieldDescr);

                if (fieldDescr.has_default_value()) {
                    outputStream << " (default: " << colors.CyanColor() << BuildDefaultValue(fieldDescr) << colors.OldColor() << ")";
                }

                outputStream << Endl;
            }

            for (int field = 0; field < descr.field_count(); ++field) {
                const NProtoBuf::FieldDescriptor& fieldDescr = *descr.field(field);

                if (fieldDescr.type() == NProtoBuf::FieldDescriptor::TYPE_MESSAGE) {
                    if (!printedSections.insert(fieldDescr.message_type()->name()).second) {
                        continue;
                    }

                    outputStream << Endl;
                    outputStream << colors.BoldColor() << "Config section: " << colors.OldColor()
                                 << BuildConfigFieldType(colors, fieldDescr) << Endl;

                    PrintConfigUsage(outputStream, colors, printedSections, *fieldDescr.message_type());
                }
            }
        }

    }

    void PrintConfigUsage(const NProtoBuf::Message& message, IOutputStream& outputStream, bool colored) {
        const auto& descr = *message.GetDescriptor();

        auto& colors = NColorizer::AutoColors(outputStream);
        if (!colored) {
            colors.Disable();
        }

        outputStream << colors.BoldColor() << "Config options" << colors.OldColor() << Endl;

        THashSet<TString> printedSections;
        PrintConfigUsage(outputStream, colors, printedSections, descr);
    }

}
