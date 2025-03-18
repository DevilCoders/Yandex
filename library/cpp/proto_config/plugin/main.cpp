#include <google/protobuf/compiler/code_generator.h>
#include <google/protobuf/compiler/command_line_interface.h>
#include <google/protobuf/compiler/plugin.h>
#include <google/protobuf/compiler/plugin.pb.h>
#include <google/protobuf/compiler/cpp/cpp_helpers.h>
#include <google/protobuf/io/zero_copy_stream.h>
#include <google/protobuf/io/printer.h>

#include <library/cpp/proto_config/protos/extensions.pb.h>

#include <util/string/ascii.h>
#include <util/string/builder.h>
#include <util/stream/str.h>
#include <util/generic/vector.h>
#include <util/generic/yexception.h>

using namespace google::protobuf;

namespace {
    TString GetMessageHolderType(const Descriptor* type) {
        if (type->options().HasExtension(NProtoConfig::ConfigClass)) {
            return type->options().GetExtension(NProtoConfig::ConfigClass);
        } else {
            return type->name();
        }
    }

    class TIndent {
    public:
        explicit TIndent(io::Printer& printer)
            : Printer_(printer)
        {
            Printer_.Indent();
        }

        ~TIndent() {
            Printer_.Outdent();
        }
    private:
        io::Printer& Printer_;
    };

    class TPrintStream
        : public TStringBuilder
    {
    public:
        explicit TPrintStream(io::Printer& printer, const std::map<TString, TString>& vars)
            : Printer_(printer)
            , Vars_(vars)
        {
        }

        ~TPrintStream() {
            Printer_.Print(Vars_, c_str());
        }
    private:
        io::Printer& Printer_;
        std::map<TString, TString> Vars_;
    };

    class TContext {
    public:
        TContext(TString protoName, compiler::OutputDirectory& output)
            : HeaderName_(protoName + ".pb.h")
            , SourceName_(protoName + ".pb.cc")
            , Output_(output)
        {
        }

        void GenerateIncludes(const FileDescriptor* file) {
            THolder<io::ZeroCopyOutputStream> res(Output_.OpenForInsert(HeaderName_, "includes"));
            io::Printer printer(res.Get(), '$');

            const auto& includes = file->options().GetRepeatedExtension(NProtoConfig::Include);
            for (int i = 0; i < includes.size(); ++i) {
                TStringStream s;
                s << "#include <" << includes.Get(i) << ">\n";
                printer.Print(s.Str().c_str());
            }

            printer.Print("#include <library/cpp/proto_config/codegen/parse_value.h>\n");
            printer.Print("#include <util/generic/hash.h>\n");
            printer.Print("#include <util/generic/vector.h>\n");
        }

        void GenerateConfigClass(const Descriptor& message) {
            if (!message.options().HasExtension(NProtoConfig::ConfigClass)) {
                return;
            }

            const bool proto3 = (message.file()->syntax() == FileDescriptor::SYNTAX_PROTO3);
            const bool plainStruct = message.options().HasExtension(NProtoConfig::PlainStruct) && message.options().GetExtension(NProtoConfig::PlainStruct);

            THolder<io::ZeroCopyOutputStream> res(Output_.OpenForInsert(HeaderName_, "namespace_scope"));
            io::Printer printer(res.Get(), '$');

            {
                TPrintStream s(printer, {
                    {"configClass", message.options().GetExtension(NProtoConfig::ConfigClass)},
                    {"protoClass", message.name()},
                });
                s << (plainStruct ? "struct" : "class") << " $configClass$ {\n"
                << "public:\n"
                << "  using TProto = $protoClass$;\n";

                if (plainStruct) {
                  s << "  static $configClass$ FromProto(const $protoClass$& v) {\n"
                    << "    $configClass$ res;\n"
                    << "    res.Fill(v);\n"
                    << "    return res;\n"
                    << "  }\n";
                } else {
                  s << "  $configClass$() {\n"
                    << "    Fill($protoClass$::default_instance());\n"
                    << "  }\n"
                    << "  $configClass$(const $protoClass$& v) {\n"
                    << "    Fill(v);\n"
                    << "  }\n";
                }
            }

            TStringStream f;

            for (int fieldId = 0; fieldId < message.field_count(); ++fieldId) {
                const FieldDescriptor* field = message.field(fieldId);
                const FieldDescriptor* mapField = nullptr;

                TString defaultInitializer;
                if (field->options().HasExtension(NProtoConfig::DefaultValue)) {
                    Y_ENSURE(!field->is_repeated() && !field->has_default_value() && !proto3,
                        "Unexpected DefaultValue for field " << field->name());

                    defaultInitializer = field->options().GetExtension(NProtoConfig::DefaultValue);
                } else if (field->has_default_value()) {
                    defaultInitializer = compiler::cpp::DefaultValue(field);
                }

                const TString holderName = plainStruct ? field->name() : compiler::cpp::FieldName(field) + "_";
                const bool underscored = IsAsciiLower(field->name()[0]);

                if (field->is_map()) {
                    // A workaround for map reflection suggested in
                    // https://groups.google.com/d/msg/protobuf/nNZ_ItflbLE/x7hLZ1GtAAAJ
                    Y_ENSURE(field->message_type()->FindFieldByNumber(1)->cpp_type() == FieldDescriptor::CPPTYPE_STRING,
                        "Map key type must be string, found "
                            << field->message_type()->FindFieldByNumber(2)->type_name() << " in field " << field->name());

                    mapField = field;
                    field = mapField->message_type()->field(1);
                }

                TString holderType;
                bool byRef = false;
                bool mutableGetter = false;
                bool isProtoCfgPlainStructField = false;

                if (field->options().HasExtension(NProtoConfig::Type)) {
                    holderType = field->options().GetExtension(NProtoConfig::Type);
                    byRef = true;

                    if (defaultInitializer) {
                        defaultInitializer = TStringBuilder() << "NProtoConfig::ParseConfigValue<"
                            << holderType << ">(" << defaultInitializer << ")";
                    }
                } else if (field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
                    const Descriptor* type = field->message_type();
                    const FileDescriptor* file = type->file();

                    isProtoCfgPlainStructField = type->options().HasExtension(NProtoConfig::PlainStruct) && type->options().GetExtension(NProtoConfig::PlainStruct);

                    holderType = GetMessageHolderType(type);
                    while (type = type->containing_type()) {
                        holderType = GetMessageHolderType(type) + "::" + holderType;
                    }

                    // WARN: type is nullptr here. Do not attempt to dereference it.
                    holderType = compiler::cpp::QualifiedFileLevelSymbol(file, holderType, compiler::cpp::Options());

                    byRef = true;
                    mutableGetter = true;
                } else if (field->cpp_type() == FieldDescriptor::CPPTYPE_ENUM) {
                    holderType = compiler::cpp::QualifiedClassName(field->enum_type());
                } else {
                    holderType = compiler::cpp::PrimitiveTypeName(field->cpp_type());
                    if (field->cpp_type() == FieldDescriptor::CPPTYPE_STRING) {
                        byRef = true;
                    }
                }

                TString repeatedType = holderType;
                if (mapField) {
                    holderType = "THashMap<TString, " + holderType + ">";
                    byRef = true;
                    mutableGetter = true;
                } else if (field->is_repeated()) {
                    holderType = "TVector<" + holderType + ">";
                    byRef = true;
                    mutableGetter = true;
                }

                if (!defaultInitializer) {
                    defaultInitializer = "{}";
                }

                const TString paramType = byRef ? "const " + holderType + "&" : holderType;
                const bool hasHas = !mapField && !field->is_repeated() && !proto3;
                const bool generateHas = hasHas && !plainStruct;


                const std::map<TString, TString> vars = {
                    {"paramType", paramType},
                    {"fieldName", (mapField ? mapField : field)->name()},
                    {"holderName", holderName},
                    {"holderType", holderType},
                    {"default", defaultInitializer},
                };

                if (!plainStruct) {
                    printer.Print("public:\n");

                    {
                        TIndent _(printer);
                        //getters

                        TPrintStream(printer, vars)
                        << "$paramType$ $fieldName$() const {\n"
                        << "  return $holderName$;\n"
                        << "}\n";

                        if (mutableGetter) {
                            TPrintStream(printer, vars)
                            << "$holderType$& $fieldName$() {\n"
                            << "  return $holderName$;\n"
                            << "}\n";
                        }

                        //has + reset
                        if (generateHas) {
                            TPrintStream(printer, vars)
                            << "bool " << (underscored ? "has_" : "Has") << "$fieldName$() const {\n"
                            << "  return has_$holderName$;\n"
                            << "}\n"
                            << "void " << (underscored ? "reset_" : "Reset") << "$fieldName$() {\n"
                            << "  $holderName$ = $default$;\n"
                            << "  has_$holderName$ = false;\n"
                            << "}\n";
                        }

                        //setter
                        TPrintStream(printer, vars)
                        << "void " << (underscored ? "set_" : "Set") << "$fieldName$($paramType$ val) {\n"
                        << "  $holderName$ = val;\n"
                        << (generateHas ? "  has_$holderName$ = true;\n": "")
                        << "}\n";
                    }

                    //holder
                    TPrintStream(printer, vars)
                    << "private:\n"
                    << "  $holderType$ $holderName$ = $default$;\n"
                    << (generateHas ? "  bool has_$holderName$ = false;\n" : "");
                } else {
                    TPrintStream(printer, vars)
                    << "public:\n"
                    << "  $holderType$ $holderName$ = $default$;\n";
                }

                printer.Print("\n");

                //fill from proto, will be injected into Fill method
                TString construct = isProtoCfgPlainStructField ? "::FromProto" : "";
                if (mapField) {
                    f << holderName << ".clear();\n";
                    f << "for (const auto &p : proto." << mapField->lowercase_name() << "()) {\n";
                    f << "    " << holderName << ".emplace(TString(p.first), " << repeatedType << construct << "(p.second));\n";
                    f << "}\n";
                } else if (field->is_repeated()) {
                    f << holderName << ".clear();\n";
                    f << "for (const auto &p : proto." << field->lowercase_name() << "()) {\n";
                    f << "    " << holderName << ".emplace_back(" << repeatedType << construct << "(p));\n";
                    f << "}\n";
                } else {
                    if (generateHas) {
                        f << "has_" << holderName << " = proto.has_" << field->lowercase_name() << "();\n";
                    }

                    if (field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
                        f << holderName << " =  " << holderType << construct << "(proto." << field->lowercase_name() << "());\n";
                    } else {
                        f << "NProtoConfig::ParseConfigValue(proto." << field->lowercase_name() << "(), " << holderName << ");\n";
                    }

                    if (hasHas) {
                        f << "if (!proto.has_" << field->lowercase_name() << "()) {\n";
                        f << "  " << holderName << " = " << defaultInitializer << ";\n";
                        f << "}\n";
                    }
                }

                f << "\n";
            }

            {
                TIndent _(printer);

                printer.Print({{"messageType", message.name()}},
                    "void Fill(const $messageType$& proto) {\n"
                );

                {
                    TIndent _(printer);
                    if (f.Str()) {
                        printer.Print(f.Str().c_str());
                    } else {
                        printer.Print("  Y_UNUSED(proto);\n");
                    }
                }

                printer.Print("}\n");
            }

            printer.Print("};\n");

            Y_VERIFY(!printer.failed());
        }
    private:
        TString HeaderName_;
        TString SourceName_;
        compiler::OutputDirectory& Output_;
    };
}


class TGenerator : public compiler::CodeGenerator {
public:
    bool Generate(const FileDescriptor* file,
        const TProtoStringType& /*parameter*/,
        compiler::OutputDirectory* outputDirectory,
        TProtoStringType* error) const override
    {
        try {
            TString protoName = compiler::cpp::StripProto(file->name());

            TContext ctx(protoName, *outputDirectory);

            ctx.GenerateIncludes(file);

            for (int m = 0; m < file->message_type_count(); ++m) {
                const auto& messageType = *file->message_type(m);
                ctx.GenerateConfigClass(messageType);
            }
        } catch (...) {
            *error = CurrentExceptionMessage();
            return false;
        }

        return true;
    }
};


int main(int argc, char* argv[]) {
    TGenerator generator;
    return compiler::PluginMain(argc, argv, &generator);
}

