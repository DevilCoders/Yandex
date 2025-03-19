// Generator of NAME.{cpp,h} from NAME.in

#include <cstdio>
#include <cstdlib>

#include <kernel/struct_codegen/metadata/metadata.pb.h>
#include <kernel/struct_codegen/reflection/reflection.h>

#include <google/protobuf/text_format.h>

#include <util/folder/path.h>
#include <util/generic/hash.h>
#include <util/generic/set.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/stream/file.h>
#include <util/stream/zlib.h>
#include <util/string/builder.h>
#include <util/string/cast.h>
#include <util/string/strip.h>

#include <kernel/proto_codegen/codegen.h>

using NErf::TRecordDescriptor;

TSet<TString> PastAsserts;


void Prologue(TCodegenParams& params, const NErf::TCodeGenInput& input) {
    params.Cpp << params.Hdr.Str();

// @ymake begin tool.add_header.h
    params.Hdr << "#include <cstring>\n"
        << "#include <type_traits>\n"
        << "#include <util/generic/singleton.h>\n"
        << "#include <util/generic/string.h>\n"
        << "#include <util/generic/vector.h>\n"
        << "#include <kernel/struct_codegen/reflection/reflection.h>\n"
        << "#include <kernel/struct_codegen/reflection/floats.h>\n"
        << "\n"
        << "#pragma pack(push, " << (input.GetPackAsOneByte() ? 1 : 4) << ")\n\n";
// @ymake end tool.add_header.h

// @ymake begin tool.add_header.cpp
    params.Cpp << "#include <util/generic/ptr.h>\n"
        << "#include <util/generic/yexception.h>\n"
        << "#include <util/generic/strbuf.h>\n\n";
// @ymake end tool.add_header.cpp
}

void Epilogue(TCodegenParams& params) {
    params.Hdr << "#pragma pack(pop)\n\n";
}

inline TString GenStaticAssert(const TString& condition) {
    return TStringBuilder() << "static_assert(" << condition << ", \"Static assertion failed: " << condition << "\");";
}

void GenStructHeader(TRecordDescriptor& descr, TCodegenParams& params, TRecordDescriptor::TCodeGenOptions& options, const TString& structName);
void GenStructFields(
    TRecordDescriptor& descr,
    TCodegenParams& params,
    TSet<TString>& asserts,
    const TString& structName,
    IOutputStream& staticReflection,
    IOutputStream& dynamicReflection,
    ui32& offset,
    ui32& fieldCount,
    ui32& padWidth,
    bool packAsOneByte);
void GenStructReflection(
    const TRecordDescriptor& descr,
    TCodegenParams& params,
    TRecordDescriptor::TCodeGenOptions& options,
    const TString& staticReflection,
    const TString& dynamicReflection,
    const size_t fieldCount,
    const size_t recordSize);
void GenStructOptions(const TRecordDescriptor& descr, TCodegenParams& params);

void GenStruct(TRecordDescriptor& descr, TCodegenParams& params, bool packAsOneByte) {
    TRecordDescriptor::TCodeGenOptions options;
    if (descr.HasCodeGenOptions())
        options = descr.GetCodeGenOptions();

    if (!descr.HasName())
        ythrow TGenError() << "struct without a name";

    const TString& structName = descr.GetName();
    size_t structStart = params.Hdr.Size();

    GenStructHeader(descr, params, options, structName);

    ui32 offset = 0, fieldCount = 0, padWidth = 0;
    TSet<TString> asserts;
    TStringStream staticReflection;
    TStringStream dynamicReflection;

    GenStructFields(descr, params, asserts, structName, staticReflection, dynamicReflection, offset, fieldCount, padWidth, packAsOneByte);
    GenStructOptions(descr, params);

    params.Hdr <<
        "};\n\n";

    GenStructReflection(descr, params, options, staticReflection.Str(), dynamicReflection.Str(), fieldCount, offset / 8);

    if (fieldCount == 0)
        ythrow TGenError() << "struct " << structName << " has no fields";

    ui32 neededOffsetDivisor = packAsOneByte ? 8 : 32;
    if (offset % neededOffsetDivisor != 0) {
        Cerr << "[" << offset << " " << (offset/32) << " " << (offset % 32) << "]\n";
        ythrow TGenError() << "size of struct " << structName << " (" << offset << " bits)"
                           << " is not a multiple of " << neededOffsetDivisor << " bits";
    }

    if (!descr.HasSize())
        descr.SetSize(offset / 8);
    else if (descr.GetSize() != offset / 8)
        ythrow TGenError() << "actual size of " << structName << " (" << (offset/8) << " bytes)"
                           << " does not match its declared size of " << descr.GetSize() << " bytes";

    TStringStream comment;
    comment << "// " << fieldCount << " fields, " << (offset / 8) << " bytes. ";
    comment << "Unused: " << padWidth << " bit" << (padWidth == 1 ? "" : "s") << ".\n";
    params.Hdr.Str() = params.Hdr.Str().substr(0, structStart) + comment.Str() + params.Hdr.Str().substr(structStart);

    for (TSet<TString>::const_iterator it = asserts.begin(); it != asserts.end(); ++it)
        if (PastAsserts.count(*it) == 0)
            params.Hdr << GenStaticAssert(*it) << "\n";
    PastAsserts.insert(asserts.begin(), asserts.end());

    params.Hdr << GenStaticAssert(TStringBuilder() << "sizeof(" << structName << ") == " << descr.GetSize()) << '\n';
    params.Hdr << GenStaticAssert(TStringBuilder() << "std::is_trivially_copyable_v<" << structName << ">") << "\n\n";
}

void GenStructHeader(TRecordDescriptor& descr, TCodegenParams& params, TRecordDescriptor::TCodeGenOptions& options, const TString& structName) {
    params.Hdr << "class " << structName;
    if (options.MixinSize() != 0) {
        params.Hdr << " : public ";
        for (size_t i = 0; i < options.MixinSize(); i++)
            params.Hdr << (i == 0 ? "" : ", ") << options.GetMixin(i) << "<" << structName << ">";
    }
    params.Hdr << " {\n";

    params.Hdr << "public:\n    static constexpr ui32 Version = " << descr.GetVersion() << ";\n\n";

    if (options.GetNeedConstructor()) {
        params.Hdr
            << "    " << structName << "() noexcept {\n"
            << "        Zero(*this);\n"
            << "    }\n\n";
    }

    params.Hdr
        << "    void Clear() {\n"
        << "        Zero(*this);\n"
        << "    }\n\n";
}

inline TString ToTitle(const TString& type) {
    TString pref = type;
    char* p = pref.begin();
    *p = (char)toupper((int)*p);
    return pref;
}

void GenStructFields(
    TRecordDescriptor& descr,
    TCodegenParams& params,
    TSet<TString>& asserts,
    const TString& structName,
    IOutputStream& staticReflection,
    IOutputStream& dynamicReflection,
    ui32& offset,
    ui32& fieldCount,
    ui32& padWidth,
    bool packAsOneByte
) {
    int neededFieldSize = (packAsOneByte ? 8 : 32);
    TSet<TString> reservedNames;
    reservedNames.insert("VERSION");
    reservedNames.insert("SIGNATURE");

    ui32 padCount = 0;
    bool prevBitField32 = false;
    bool wasPriv = false;
    TStringStream acc;
    acc << "public:\n";
    for (size_t i = 0; i < descr.FieldSize(); i++) {
        const NErf::TFieldDescriptor& field = descr.GetField(i);

        if (offset % 32 == 0 && offset > 0)
            params.Hdr << "    // dword border\n";

        if (!field.HasWidth() || field.GetWidth() == 0)
            ythrow TGenError() << structName << " has a field with unspecified or zero width";

        if (!field.HasName()) {
            // padding, not a real field
            for (int width = field.GetWidth(); width > 0; width -= neededFieldSize)
                params.Hdr << "    " << "ui" << neededFieldSize << " _Unused" << ++padCount << ":" << Min(neededFieldSize, width) << ";\n";
            padWidth += field.GetWidth();
            offset += field.GetWidth();
            continue;
        }

        if (reservedNames.count(field.GetName()) != 0 || field.GetName().StartsWith("_Unused"))
            ythrow TGenError() << "struct " << structName << " has a field with a reserved name " << field.GetName();

        size_t lineStart = params.Hdr.Size();
        TString inlineWarning = "";
        ui32 width = field.GetWidth();

        if (field.HasType()) {
            if (packAsOneByte && field.GetType() != "ui8") { 
                ythrow TGenError() << "field " << structName << "::" << field.GetName() << " has type "
                                   << field.GetType() << " which is not supported when PackAsOneByte is set";
            }
            if (width % 8 != 0)
                ythrow TGenError() << "width of field " << structName << "::" << field.GetName()
                                   << " (" << width << " bits) is not a multiple of 8";

            if (offset % 8 != 0)
                ythrow TGenError() << "field " << structName << "::" << field.GetName()
                                   << " is not aligned on a byte boundary" << " (current bit offset: " << offset << ")";

            const TString& type = field.GetType();
            if (type == "sf16" || type == "uf16") { // TODO: process all types here
                TString pref = ToTitle(type);
                if (!wasPriv) {
                    wasPriv = true;
                    params.Hdr << "private:\n";
                }
                acc << "    float Get" << field.GetName() << "() const {\n        return GetFloatFrom" << pref << "(" << field.GetName() << ");\n    }\n";
                acc << "    void Set" << field.GetName() << "(float f) {\n        " << field.GetName() << " = Get" << pref << "FromFloat(f);\n    }\n";
                params.Hdr << "    ui32 " << field.GetName() << ":" << field.GetWidth() << ";";
                prevBitField32 = true;
            } else {
                params.Hdr << "    " << type << " " << field.GetName() << ";";
                prevBitField32 = false;
            }

            if (type != "sf16" && type != "uf16")
                asserts.insert("sizeof(" + field.GetType() + ") == " + ToString(width / 8));
        } else {  // bitfield
            if (width > 32)
                ythrow TGenError() << "width of bitfield " << structName << "::" << field.GetName()
                                   << " (" << width << " bits) exceeds 32 bits";
            if (wasPriv) {
                wasPriv = false;
                params.Hdr << "public:\n";
            }
            if (packAsOneByte || (offset % 32 != 0 && field.GetWidth() < 8 && !prevBitField32))
                params.Hdr << "    ui8 " << field.GetName() << ":" << field.GetWidth() << ";";
            else {
                params.Hdr << "    ui32 " << field.GetName() << ":" << field.GetWidth() << ";";
                prevBitField32 = true;
            }
        }

        if (field.HasName()) {
            TStringStream fieldReflection;
            fieldReflection
                << "/* name = */ \"" << field.GetName() << "\", /* type = */ "
                    << (field.HasType() ? "\"" + field.GetType() + "\"" : "NULL")
                    << ", /* bitOffset = */ " << offset << ", /* bitWidth = */ " << width;

            TString joinedPrevNames = "NULL";
            if (field.PrevNameSize() > 0) {
                joinedPrevNames = "\"";
                const char* sep = "";
                for (size_t nameNo = 0; nameNo < field.PrevNameSize(); ++nameNo) {
                    joinedPrevNames += sep;
                    joinedPrevNames += field.GetPrevName(nameNo);
                    sep = ",";
                }
                joinedPrevNames += "\"";

            }
            fieldReflection << ", /* prevNames = */ " << joinedPrevNames;

            staticReflection
                << "        static constexpr TStaticFieldDescriptor " << field.GetName() << "() {\n"
                << "            return { " << fieldReflection.Str() << " };\n"
                << "        };\n";

            dynamicReflection
                << "    { " << fieldReflection.Str() << " },\n";
        }

        if ((width == 32 || width == 16 || width == 8) && offset % width != 0) {
            inlineWarning = "Warning: not " + ToString(width) + "-bit aligned";
#if 0
            // see SEARCH-2004
            params.Warnings << "Warning: " << width << "-bit field "
                     << structName << "::" << field.GetName() << " is not " << width << "-bit aligned\n";
#endif
        }

        if (!inlineWarning.empty()) {
            while (params.Hdr.Size() < lineStart + 50)
                params.Hdr << " ";
            params.Hdr << "  // " << inlineWarning;
        }

        params.Hdr << "\n";

        offset += field.GetWidth();
        fieldCount++;
    }

    params.Hdr << "\n";
    const TString s = acc.Str();
    if (s != "public:\n") {
        params.Hdr << s << "\n";
    }
}

static void GenerateVisitorDriverBody(const TRecordDescriptor& descr, IOutputStream& out, bool checkFieldMask = false) {
    static const TString nothing;

    for (size_t i = 0; i < descr.FieldSize(); i++) {
        const NErf::TFieldDescriptor& field = descr.GetField(i);
        if (field.HasName()) {
            bool hasType = field.HasType();
            if (checkFieldMask) {
                out << "    if (mask." << field.GetName() << ") {\n    ";
            }
            TString accessorName = field.GetName();
            const TString& type = field.GetType();
            if (type == "sf16" || type == "uf16") {
                out << "    float conv" << accessorName << " = obj.Get" << accessorName << "();\n";
                if (checkFieldMask) {
                    out << "    ";
                }

                accessorName = "conv" + accessorName;
                hasType = false;
            } else {
                accessorName = "obj." + accessorName;
            }
            out
                << "    visitor.Visit" << (hasType ? "CppField" : "") << "("
                << (hasType ? "\"" + field.GetType() + "\", " : nothing)
                << ("\"" + field.GetName() + "\", ")
                << (hasType ? "&" : "") << accessorName
                << (hasType ? ", " + ToString(field.GetWidth() / 8) : nothing)
                << ");\n";

            if (checkFieldMask) {
                out << "    }\n";
            }
        }
    }

    out
        << "}\n\n";
}

void GenStructReflection(
    const TRecordDescriptor& descr,
    TCodegenParams& params,
    TRecordDescriptor::TCodeGenOptions& options,
    const TString& staticReflection,
    const TString& dynamicReflection,
    const size_t fieldCount,
    const size_t recordSize)
{
    const TString& structName = descr.GetName();

    params.Hdr
        << "\n"
        << "namespace NErf {\n"
        << "\n"
        << "template<>\n"
        << "struct TStaticReflection<" << structName << "> {\n"
        << "    static constexpr ui32 SIGNATURE = " << NErf::GetRecordSignature(structName) << "; // == NErf::GetRecordSignature(\"" << structName << "\")\n"
        << "\n"
        <<      staticReflection
        << "};\n"
        << "\n"
        << "template<>\n"
        << "class TDynamicReflection<" << structName << "> : public NErf::TStaticRecordDescriptor {\n"
        << "public:\n"
        << "    static const TRecordReflection* Reflection() {\n"
        << "        return Singleton< TDynamicReflection<" << structName << "> >()->ReflectionHolder.Get();\n"
        << "    }\n"
        << "\n"
        << "    TDynamicReflection<" << structName << ">()\n"
        << "        : NErf::TStaticRecordDescriptor(/* name = */ \"" << descr.GetName() << "\", /* version = */ " << descr.GetVersion() << ", "
            << "/* size = */ " << recordSize << ", FIELDS, /* fieldCount = */ " << fieldCount << ")\n"
        << "        , ReflectionHolder(new TRecordReflection(*this))\n"
        << "    {\n"
        << "    }\n"
        << "\n"
        << "private:\n"
        << "    static const NErf::TStaticFieldDescriptor FIELDS[" << fieldCount << "];\n"
        << "\n"
        << "    THolder<TRecordReflection> ReflectionHolder;\n"
        << "};\n"
        << "\n"
        << "} // namespace NErf\n\n";

    params.Cpp
        << "const NErf::TStaticFieldDescriptor NErf::TDynamicReflection<" << structName << ">::FIELDS[] = {\n"
        <<     dynamicReflection
        << "};\n\n";

    if (options.GetNeedVisitor()) {
        params.Cpp
            << "namespace NErf {\n\n"
            << "template<>\n"
            << "void VisitFields<" << structName << ">(const " << structName << "& obj, " << structName << "::IFieldsVisitor& visitor) {\n";

        GenerateVisitorDriverBody(descr, params.Cpp);


        if (options.GetNeedMask()) {
            params.Cpp
                << "template<>\n"
                << "void VisitFields<" << structName << ">(const " << structName << "& obj, " << structName
                    << "::IFieldsVisitor& visitor, const " << structName << "::TFieldMask& mask) {\n";

            GenerateVisitorDriverBody(descr, params.Cpp, true);
        }

        params.Cpp
            << "} // namespace NErf\n\n";
    }
}

// Generates mask and visitor classes, visit and merge functions
void GenStructOptions(const TRecordDescriptor& descr, TCodegenParams& params) {
    const TString& structName = descr.GetName();

    if (!descr.HasCodeGenOptions())
        return;

    const TRecordDescriptor::TCodeGenOptions& options = descr.GetCodeGenOptions();

    if (!options.GetNeedVisitor() && !options.GetNeedMask())
        return;

    if (!options.HasShortName())
        ythrow TGenError() << "please specify short struct name in CodeGenOptions of " << structName;

    TSet<TString> fieldTypes;
    ui32 fieldCount = 0;

    for (size_t i = 0; i < descr.FieldSize(); i++) {
        const NErf::TFieldDescriptor& field = descr.GetField(i);
        if (field.HasName()) {
            fieldCount++;
            if (field.HasType())
                fieldTypes.insert(field.GetType());
        }
    }

    params.Hdr
        << "    typedef NErf::TRecordReflection TReflection;\n\n";

    TString visitorClass, visitorClassCppName, visitFunc, visitFuncCppName;
    if (options.GetNeedVisitor()) {
        visitorClass = "IFieldsVisitor";
        visitorClassCppName = structName + "::" + visitorClass;

        visitFunc = "VisitFields";
        visitFuncCppName = structName + "::" + visitFunc;

        params.Hdr << "    class " << visitorClass << " : public IErfFieldsVisitor {\n"
            << "    public:\n"
            << "        virtual ~" << visitorClass << "();\n";
        params.Hdr << "        virtual void Visit(const char* name, const ui32& value) = 0;\n";

        if (!fieldTypes.empty()) {
            params.Hdr << "        virtual void VisitCppField(const char* type, const char* name, const void* value, size_t size);\n";
            bool wasFloat = false;
            for (TSet<TString>::const_iterator it = fieldTypes.begin(); it != fieldTypes.end(); ++it) {
                if (!wasFloat && (*it == "sf16" || *it == "uf16" || *it == "float")) {
                    params.Hdr << "        virtual void Visit(const char* name, const float& value) = 0;\n";
                    wasFloat = true;
                } else if (*it != "ui32") {
                    params.Hdr << "        virtual void Visit(const char* name, const " << *it << "& value) = 0;\n";
                }
            }
        }
        params.Hdr << "    };\n\n";

        params.Cpp << visitorClassCppName << "::~" << visitorClass << "() {\n}\n\n";

        if (!fieldTypes.empty()) {
            params.Cpp << "void " << visitorClassCppName << "::VisitCppField(const char* type, const char* name, const void* value, size_t size) {\n";
            for (TSet<TString>::const_iterator it = fieldTypes.begin(); it != fieldTypes.end(); ++it) {
                params.Cpp << "    if (strcmp(type, \"" << *it << "\") == 0) {\n";
                if (*it != "sf16" && *it != "uf16") {
                    params.Cpp << "        Y_ASSERT(size == sizeof(" << *it << "));\n"
                               << "        this->Visit(name, *static_cast<const " << *it << "*>(value));\n";
                } else {
                    TString pref = ToTitle(*it);
                    params.Cpp << "        Y_ASSERT(size == 2);\n"
                               << "        this->Visit(name, GetFloatFrom" << pref << "(*static_cast<const ui16*>(value)));\n";
                }
                params.Cpp << "        return;\n"
                           << "    }\n";
            }
            params.Cpp << "    ythrow yexception() << \"Unknown type \" << type;\n"
                << "}\n\n";
        }

        params.Cpp
            << "namespace NErf {\n"
            << "template<> void " << visitFunc << "<" << structName << ">(const " << structName << "& obj, "
            << visitorClassCppName << "& visitor);\n"
            << "}\n\n";

        params.Hdr << "    static void " << visitFunc << "(" << visitorClass << "& visitor);\n\n";
        params.Cpp << "void " << visitFuncCppName << "(" << visitorClassCppName << "& visitor) {\n"
            << "    " << structName << " tmp;\n";

        if (!options.GetNeedConstructor()) {
            params.Cpp << "    tmp.Clear();\n";
        }

        params.Cpp
            << "    NErf::" << visitFunc << "(tmp, visitor);\n"
            << "}\n\n";
    }

    TString maskClass, maskClassCppName;
    if (options.GetNeedMask()) {
        maskClass = "TFieldMask";
        maskClassCppName = structName + "::" + maskClass;

        params.Hdr << "    struct " << maskClass << " {\n";
        for (size_t i = 0; i < descr.FieldSize(); i++) {
            if (descr.GetField(i).HasName())
                params.Hdr << "        ui32 " << descr.GetField(i).GetName() << ":1;\n";
        }
        if (fieldCount % 32 != 0)
            params.Hdr << "        ui32 _Unused:" << (32 - fieldCount % 32) << ";\n";

        params.Hdr << "\n"
            << "        " << maskClass << "(bool bSetAll = false);\n"
            << "\n"
            << "        int GetField(ui32 index) const;\n"
            << "        int GetField(const char* name) const;\n"
            << "        void SetField(ui32 index, bool value);\n"
            << "        void SetField(const TStringBuf& name, bool value);\n"
            << "\n"
            << "        static " << maskClass << " GetMask(const TVector<TString>& fieldStrs);\n"
            << "        static " << maskClass << " Parse(const TStringBuf& mask);\n"
            << "    };\n\n"
            << "    " << GenStaticAssert(TStringBuilder() << "sizeof(" << maskClass << ") == " << ((fieldCount + (32 - fieldCount % 32) % 32) / 8)) << "\n\n";

        params.Cpp << maskClassCppName << "::" << maskClass << "(bool bSetAll) {\n"
            << "    if (bSetAll) {\n"
            << "        memset(this, 255, sizeof(" << maskClassCppName << "));\n"
            << "    } else {\n"
            << "        memset(this, 0, sizeof(" << maskClassCppName << "));\n"
            << "    }\n"
            << "}\n\n";

        params.Cpp << "int " << maskClassCppName << "::GetField(ui32 index) const {\n"
            << "    Y_ASSERT(index < " << fieldCount << ");\n"
            << "    return (((const ui32*)this)[index >> 5] >> (index & 31)) & 1;\n"
            << "}\n\n";

        params.Cpp << "int " << maskClassCppName << "::GetField(const char* name) const {\n"
            << "    int index = NErf::TDynamicReflection<" << structName << ">::Reflection()->GetFieldIndex(name);\n"
            << "    if (index < 0)\n"
            << "        ythrow yexception() << \"incorrect field name: \" << name;\n"
            << "    return GetField(index);\n"
            << "}\n\n";

        params.Cpp << "void " << maskClassCppName << "::SetField(ui32 index, bool value) {\n"
            << "    Y_ASSERT(index < " << fieldCount << ");\n"
            << "    ui32& dword = ((ui32*)this)[index >> 5];\n"
            << "    index &= 31;\n"
            << "    dword = (dword & ~(1U << index)) | (ui32(value) << index);\n"
            << "}\n\n";

        params.Cpp << "void " << maskClassCppName << "::SetField(const TStringBuf& name, bool value) {\n"
            << "    int index = NErf::TDynamicReflection<" << structName << ">::Reflection()->GetFieldIndex(name);\n"
            << "    if (index < 0)\n"
            << "        ythrow yexception() << \"incorrect field name: \" << name;\n"
            << "    return SetField(index, value);\n"
            << "}\n\n";

        params.Cpp << maskClassCppName << " " << maskClassCppName << "::GetMask(const TVector<TString>& fieldStrs) {\n"
            << "    " << maskClassCppName << " result;\n\n"
            << "    for (TVector<TString>::const_iterator it = fieldStrs.begin(); it != fieldStrs.end(); ++it)\n"
            << "        result.SetField(*it, true);\n"
            << "    return result;\n"
            << "}\n\n";

        params.Cpp << maskClassCppName << " " << maskClassCppName << "::Parse(const TStringBuf& fields) {\n"
            << "    " << maskClassCppName << " mask;\n\n"
            << "    TStringBuf fieldsLeft(fields);\n"
            << "    bool positive = true;\n"
            << "    if (fields[0] == '!' || fields[0] == '-') {\n"
            << "        positive = false;\n"
            << "        fieldsLeft.Skip(1);\n"
            << "    }\n"
            << "    mask = " << maskClassCppName << "(!positive);\n\n"
            << "    while (!fieldsLeft.empty()) {\n"
            << "        mask.SetField(fieldsLeft.NextTok(','), positive);\n\n"
            << "    }\n"
            << "    return mask;\n"
            << "};\n\n";
    }

    if (options.GetNeedVisitor() && options.GetNeedMask()) {
        TStringStream proto;

        params.Cpp
            << "namespace NErf {\n"
            << "template<> void " << visitFunc << "<" << structName << ">(const " << structName << "& obj, "
            << visitorClassCppName << "& visitor, const " << maskClassCppName << "& mask);\n"
            << "}\n\n";

        params.Hdr << "    static void " << visitFunc << "(" << visitorClass << "& visitor, const " << maskClass << "& mask);\n\n";
        params.Cpp << "void " << visitFuncCppName << "(" << visitorClassCppName << "& visitor, const " << maskClassCppName << "& mask) {\n"
            << "    " << structName << " tmp;\n";

        if (!options.GetNeedConstructor()) {
            params.Cpp << "    tmp.Clear();\n";
        }

        params.Cpp
            << "    NErf::" << visitFunc << "(tmp, visitor, mask);\n"
            << "}\n\n";
    }

    if (options.GetNeedMergeFunction()) {
        if (!options.GetNeedMask())
            ythrow TGenError() << "please enable generation of field masks for struct " << structName << " if you want a merge function";

        TString mergeFuncName = "DoMerge";

        params.Hdr << "    // Merges dst into src.\n"
            << "    // If overwrite is true: for each field f, if f is present in src, then set dst.f to src.f.\n"
            << "    // If overwrite is false: for each field f, if f is present in src and missing in dst, then set dst.f to src.f.\n"
            << "    static void " << mergeFuncName << "(" << structName << "& dst, " << maskClass << "& dstMask, "
            << "const " << structName << "& src, const " << maskClass << "& srcMask, bool overwrite)" << ";\n";

        params.Cpp << "void " << structName << "::" << mergeFuncName << "(" << structName << "& dst, " << maskClassCppName << "& dstMask, "
            << "const " << structName << "& src, const " << maskClassCppName << "& srcMask, bool overwrite)" << " {\n";

        for (size_t i = 0; i < descr.FieldSize(); ++i) {
            const TString& fieldName = descr.GetField(i).GetName();
            if (!fieldName.empty()) {
                params.Cpp
                    << "    if (srcMask." << fieldName << " && (overwrite || !dstMask." << fieldName << "))\n"
                    << "        dst." << fieldName << " = src." << fieldName << ";\n";
            }
        }

        params.Cpp
            << "}\n\n";
    }
}

void GenCode(NErf::TCodeGenInput& input, TCodegenParams& params) {
    Prologue(params, input);

    for (size_t i = 0; i < input.StructSize(); i++) {
        TRecordDescriptor* descr = input.MutableStruct(i);
        GenStruct(*descr, params, input.GetPackAsOneByte());
    }

    Epilogue(params);

    StripInPlace(params.Hdr.Str());
    params.Hdr << "\n";
    StripInPlace(params.Cpp.Str());
    params.Cpp << "\n";
}

int main(int argc, const char **argv) {
    return MainImpl<NErf::TCodeGenInput>(argc, argv);
}
