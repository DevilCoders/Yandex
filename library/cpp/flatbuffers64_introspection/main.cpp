#include <contrib/libs/flatbuffers64/include/flatbuffers/reflection.h>
#include <contrib/libs/flatbuffers64/include/flatbuffers/reflection_generated.h>

#include <library/cpp/resource/resource.h>
#include <library/cpp/getopt/last_getopt.h>

#include <util/generic/maybe.h>
#include <util/generic/ptr.h>
#include <util/generic/stack.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/generic/yexception.h>
#include <util/memory/blob.h>
#include <util/stream/output.h>
#include <util/string/cast.h>
#include <util/string/printf.h>

namespace {
    class TSizeReport {
    public:
        TSizeReport(
            size_t size,
            size_t overheadSize,
            TString name,
            TMaybe<TString> value,
            TVector<TSizeReport> children)
            : Size_(size)
            , OverheadSize_(overheadSize)
            , Name_(std::move(name))
            , Value_(std::move(value))
            , Children_(std::move(children))
        {
        }

        void PrettyPrint(IOutputStream* out, bool verbose) const {
            PrettyPrint(TotalSize(), TotalSize(), 0, out, verbose);
        }

        size_t Size() const {
            return Size_;
        }

        size_t OverheadSize() const {
            return OverheadSize_;
        }

        size_t TotalSize() const {
            return Size_ + OverheadSize_;
        }

    private:
        void PrettyPrint(
            size_t fileSize,
            size_t parentSize,
            int depth,
            IOutputStream* out,
            bool verbose) const {
            doIndent(depth, out);
            *out << Name_;

            if (Value_) {
                *out << " = " << *Value_;
            }
            if (verbose) {
                *out << ": " << Sprintf("[%2.02f%% total, %2.02f%% relative, %s; efficiency %2.02f%%]", 100.0 * TotalSize() / Max<size_t>(fileSize, 1), 100.0 * TotalSize() / Max<size_t>(parentSize, 1), PrettySize(TotalSize()).data(), 100.0 * Efficiency());
            } else {
                *out << ": " << PrettySize(TotalSize());
            }

            if (!Children_.empty()) {
                *out << " {\n";
                for (const auto& report : Children_) {
                    report.PrettyPrint(fileSize, TotalSize(), depth + 1, out, verbose);
                }
                doIndent(depth, out);
                *out << "};\n";
            } else {
                *out << ";\n";
            }
        }

        double Efficiency() const {
            return Size_ + OverheadSize_ == 0 ? 1.0 : Size_ / double(Size_ + OverheadSize_);
        }

        static TString PrettySize(size_t size) {
            double sizeFormat = size;
            TString mod = "b";
            const char* mods[] = {"Kb", "Mb", "Gb", "Tb", "Pb", "Eb"};
            TString numFormat = "%.0f";

            for (const char* nextMod : mods) {
                if (sizeFormat > 1024) {
                    sizeFormat /= 1024;
                    mod = nextMod;
                    numFormat = "%.02f";
                } else {
                    break;
                }
            }

            return Sprintf((numFormat + " %s").data(), sizeFormat, mod.data());
        }

        static void doIndent(int indent, IOutputStream* out) {
            indent *= 4;
            const int MAX_INDENT = 128;
            static const TString filler(MAX_INDENT, ' ');
            Y_ENSURE(indent <= MAX_INDENT);

            *out << filler.substr(0, indent);
        }

        size_t Size_;
        size_t OverheadSize_;
        TString Name_;
        TMaybe<TString> Value_;
        TVector<TSizeReport> Children_;
    };

    class TSizeReportBuilder {
        struct Node {
            explicit Node(TString name)
                : Name(std::move(name))
                , Size(0)
                , OverheadSize(0)
                , ChildrenNumber(0)
            {
            }

            Node& node(size_t offset, const char* name) {
                if (offset >= Children.size()) {
                    Children.resize(offset + 1);
                }
                auto& child = Children[offset];
                if (!child) {
                    child = MakeHolder<Node>(name);
                    ++ChildrenNumber;
                }

                return *child;
            }

            TString Name;
            size_t Size;
            size_t OverheadSize;
            TMaybe<TString> Value;
            TVector<THolder<Node>> Children;
            size_t ChildrenNumber;
        };

    public:
        TSizeReportBuilder()
            : Root("")
        {
            Nodes_.push(&Root);
        }

        void AddOverhead(size_t size) {
            Top()->OverheadSize += size;
        }

        void AddSize(size_t size) {
            Top()->Size += size;
        }

        void AddSize(const char* name, size_t offset, size_t size, const TMaybe<TString>& value = {}) {
            auto& node = Top()->node(offset, name);
            node.Size += size;
            if (value) {
                node.Value = value;
            }
        }

        void PushContext(const char* name, size_t offset) {
            Nodes_.push(&Top()->node(offset, name));
        }

        void PopContext() {
            Nodes_.pop();
        }

        TSizeReport Build(size_t size) const {
            Y_ENSURE(Root.Children.size() == 1);
            return Build(Root.Children.front().Get(), size);
        }

    private:
        TSizeReport Build(const Node* node, TMaybe<size_t> sizeOverride = {}) const {
            size_t size = node->Size;
            size_t overheadSize = node->OverheadSize;
            TVector<TSizeReport> children;
            TString name = node->Name;

            // Collapse tables that have only 1 field.
            // Instead of
            // a : 18 {
            //     b : 18 {
            //          c: 18
            //      }
            // }
            // get
            // a.b.c: 18
            while (node->ChildrenNumber == 1) {
                for (const auto& child : node->Children) {
                    if (!child) {
                        continue;
                    }
                    node = child.Get();
                    name += ".";
                    name += node->Name;

                    size += node->Size;
                    overheadSize += node->OverheadSize;
                    break;
                }
            }

            for (const auto& childNode : node->Children) {
                if (!childNode) {
                    continue;
                }
                TSizeReport child = Build(childNode.Get());
                size += child.Size();
                overheadSize += child.OverheadSize();
                children.push_back(child);
            }

            return TSizeReport(
                size,
                sizeOverride ? *sizeOverride - size : overheadSize,
                name,
                node->Value,
                children);
        }

        Node* Top() {
            return Nodes_.top();
        }

        Node Root;
        TStack<Node*> Nodes_;
    };

    size_t StringSize(
        const flatbuffers64::Table& table, const reflection64::Field& field) {
        if (const auto str = flatbuffers64::GetFieldS(table, field)) {
            return str->size();
        } else {
            return 0;
        }
    }

    size_t VectorSize(
        const flatbuffers64::Table& table, const reflection64::Field& field) {
        if (const auto vec = flatbuffers64::GetFieldAnyV(table, field)) {
            return vec->size();
        } else {
            return 0;
        }
    }

    bool IsPrintSafe(TStringBuf str) {
        if (str.size() > 100) {
            return false;
        }
        for (const char c: str) {
            if (!isalnum(c) && !isblank(c) && !ispunct(c)) {
                return false;
            }
        }

        return true;
    }

    class TIntrospector {
    public:
        TIntrospector(TSizeReportBuilder* sizeReportBuilder, const reflection64::Schema* schema)
            : SizeReportBuilder_(sizeReportBuilder)
            , Schema_(schema)
            , InsideVector_(false)
        {
        }

        void introspect(const flatbuffers64::Table& table) {
            SizeReportBuilder_->PushContext(Schema_->root_table()->name()->c_str(), 0);
            IntrospectTable(*Schema_->root_table(), &table);
        }

    private:
        void IntrospectTable(
            const reflection64::Object& schema, const flatbuffers64::Table* table) {
            Y_ENSURE(!schema.is_struct());

            if (!table) {
                return;
            }

            for (const auto& field : *schema.fields()) {
                IntrospectField(schema, *field, *table);
            }
        }

        void IntrospectField(
            const reflection64::Object& schema, const reflection64::Field& field, const flatbuffers64::Table& table) {
            const char* fieldName = field.name()->c_str();
            const auto& type = *field.type();
            if (type.base_type() == reflection64::Obj || type.base_type() == reflection64::Union) {
                const auto& object = type.base_type() == reflection64::Obj ?
                    *Schema_->objects()->Get(type.index()) :
                    flatbuffers64::GetUnionType(*Schema_, schema, field, table);
                if (!object.is_struct()) {
                    SizeReportBuilder_->PushContext(fieldName, field.offset());
                    if (type.base_type() == reflection64::Union) {
                        SizeReportBuilder_->PushContext(Sprintf("[%s]", object.name()->c_str()).data(), 0);
                    }
                    IntrospectTable(object, flatbuffers64::GetFieldT(table, field));
                    if (type.base_type() == reflection64::Union) {
                        SizeReportBuilder_->PopContext();
                    }
                    SizeReportBuilder_->PopContext();
                    return;
                }
            }

            if (const auto size = MaybeFixedSizeOf(type.base_type(), type.index())) {
                SizeReportBuilder_->AddSize(fieldName, field.offset(), *size, InsideVector_ ? Nothing() : ValueOf(field, table));
            } else if (type.base_type() == reflection64::String) {
                SizeReportBuilder_->AddSize(fieldName, field.offset(), StringSize(table, field), InsideVector_ ? Nothing() : ValueOf(field, table));
            } else if (type.base_type() == reflection64::Vector) {
                if (!VectorSize(table, field)) {
                    return;
                }
                SizeReportBuilder_->PushContext(fieldName, field.offset());
                SizeReportBuilder_->AddOverhead(16); // Pointer to data + size

                const auto elementSize = MaybeFixedSizeOf(
                    type.element(), type.index());
                if (elementSize) {
                    SizeReportBuilder_->AddSize(*elementSize * VectorSize(table, field));
                } else {
                    if (type.element() == reflection64::String) {
                        const auto vector = flatbuffers64::GetFieldV<flatbuffers64::Offset<flatbuffers64::String>>(table, field);
                        for (size_t i = 0; i != vector->size(); ++i) {
                            SizeReportBuilder_->AddOverhead(16); // Pointer to data + size
                            SizeReportBuilder_->AddSize(vector->Get(i)->size());
                        }
                    } else {
                        const auto vector = flatbuffers64::GetFieldV<flatbuffers64::Offset<flatbuffers64::Table>>(table, field);
                        const auto& elementSchema = *Schema_->objects()->Get(type.index());
                        SizeReportBuilder_->AddOverhead(8 * vector->size()); // Pointers to tables
                        const bool oldInsideVector = InsideVector_;
                        InsideVector_ = true;
                        for (size_t i = 0; i != vector->size(); ++i) {
                            IntrospectTable(elementSchema, vector->Get(i));
                        }
                        InsideVector_ = oldInsideVector;
                    }
                }
                SizeReportBuilder_->PopContext();
            } else {
                ythrow yexception() << "Unknown field type in IntrospectField: " << (int)type.base_type();
            }
        }

        TMaybe<size_t> MaybeFixedSizeOf(const reflection64::BaseType type, int index) const {
            switch (type) {
                case reflection64::UType:
                case reflection64::Bool:
                case reflection64::Byte:
                case reflection64::UByte:
                    return 1;

                case reflection64::Short:
                case reflection64::UShort:
                    return 2;

                case reflection64::Int:
                case reflection64::UInt:
                case reflection64::Float:
                    return 4;

                case reflection64::Long:
                case reflection64::ULong:
                case reflection64::Double:
                    return 8;

                case reflection64::Obj: {
                    const auto& object = *Schema_->objects()->Get(index);
                    if (object.is_struct()) {
                        return object.bytesize();
                    }
                } break;

                default:
                    return {};
            }
            return {};
        }

        TMaybe<TString> ValueOf(const reflection64::Field& field, const flatbuffers64::Table& table) const {
            const auto& type = *field.type();
            switch (type.base_type()) {
                case reflection64::Byte:
                    return ToString(flatbuffers64::GetFieldI<i8>(table, field));
                case reflection64::UType:
                case reflection64::UByte:
                    return ToString(flatbuffers64::GetFieldI<ui8>(table, field));
                case reflection64::Short:
                    return ToString(flatbuffers64::GetFieldI<i16>(table, field));
                case reflection64::UShort:
                    return ToString(flatbuffers64::GetFieldI<ui16>(table, field));
                case reflection64::Int:
                    return ToString(flatbuffers64::GetFieldI<i32>(table, field));
                case reflection64::UInt:
                    return ToString(flatbuffers64::GetFieldI<ui32>(table, field));
                case reflection64::Long:
                    return ToString(flatbuffers64::GetFieldI<i64>(table, field));
                case reflection64::ULong:
                    return ToString(flatbuffers64::GetFieldI<ui64>(table, field));
                case reflection64::String:
                    if (const auto* str = flatbuffers64::GetFieldS(table, field); str && IsPrintSafe({str->c_str(), str->size()})) {
                        return Sprintf("\"%s\"", str->c_str());
                    } else {
                        return {};
                    }
                default:
                    return {};
            }

        }

        TSizeReportBuilder* SizeReportBuilder_;
        const reflection64::Schema* Schema_;
        bool InsideVector_;
    };
}

int main(int argc, char** argv) {
    try {
        TString schemaString;
        Y_ENSURE(
            NResource::FindExact(
                "/library/cpp/flatbuffers64_introspection/schema", &schemaString),
            "Cannot load schema");
        flatbuffers64::Verifier schemaVerifier((const ui8*)schemaString.data(), schemaString.size());
        Y_ENSURE(
            reflection64::VerifySchemaBuffer(schemaVerifier),
            "Incorrect flatbuffer schema");
        const auto& schema = *reflection64::GetSchema(schemaString.data());

        bool verbose = false;

        NLastGetopt::TOpts opts;

        opts.SetTitle("Look inside a file with flatbuffer data and figure out how much space every field occupies");
        opts.SetFreeArgsNum(1);
        opts.SetFreeArgTitle(0, "FILE", "File with flatbuffer data");
        opts.AddHelpOption('h');
        opts.AddLongOption('v', "verbose", "Show more info")
            .NoArgument()
            .StoreResult(&verbose, true);

        NLastGetopt::TOptsParseResult parsedOpts{&opts, argc, argv};

        TBlob blob = TBlob::FromFile(parsedOpts.GetFreeArgs().front());
        const auto ptr = static_cast<const ui8*>(blob.Data());
        if (!flatbuffers64::Verify(schema, *schema.root_table(), ptr, blob.Length())) {
            Cerr << "Warning: data verification has failed\n";
        }

        const auto& root = *flatbuffers64::GetAnyRoot(ptr);

        TSizeReportBuilder builder;
        TIntrospector{&builder, &schema}.introspect(root);
        builder.Build(blob.Length()).PrettyPrint(&Cout, verbose);

        return 0;
    } catch (...) {
        Cerr << CurrentExceptionMessage() << Endl;
        return 1;
    }
}
