#include "protoconv/cpp_generator.h"

#include "common/import_maker.h"
#include "cpp/common.h"
#include "cpp/type_name_maker.h"
#include "protoconv/common.h"
#include "protoconv/proto_file.h"
#include "tpl/tpl.h"

#include <yandex/maps/idl/config.h>
#include <yandex/maps/idl/full_type_ref.h>
#include <yandex/maps/idl/scope.h>
#include <yandex/maps/idl/targets.h>
#include <yandex/maps/idl/utils/common.h>

#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <boost/variant/get.hpp>

#include <cctype>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace protoconv {

CppGenerator::CppGenerator(const Idl* idl, const utils::Path& prefixPath)
    : BaseGenerator(
          idl,
          common::ImportMaker(idl, nullptr, { "yandex", "boost", "" }, 1)),
      prefixPath_(prefixPath)
{
    mainDict_.ShowSection("CPP");
}

OutputFile CppGenerator::generate()
{
    tpl::DirGuard guard("protoconv");

    idl_->root.nodes.traverse(this);

    auto path = filePath(idl_, true);
    mainDict_.SetValue("OWN_HEADER_INCLUDE_PATH", path.asString());
    importMaker_.fill(path.inAngles(), &mainDict_);

    return {
        prefixPath_, filePath(idl_, false),
        BaseGenerator::expand(),
        importMaker_.getImportPaths()
    };
}

void CppGenerator::onVisited(const nodes::Enum& e)
{
    addNode<nodes::Enum>(e, "enum.tpl",
        [this](
            const nodes::Enum& e,
            ctemplate::TemplateDictionary* dict)
        {
            const auto& config = idl_->env->config;
            ProtoFile protoFile(config.inProtoRoot, config.baseProtoPackage,
                e.protoMessage->pathToProto);

            auto protoScope = e.protoMessage->pathInProto;
            dict->SetValue("PB_ENUM_NAME", protoScope.last());

            --protoScope;
            auto pbFieldScopeString = protoFile.relativeNamespacePrefix() +
                protoScope.asPrefix("::");

            const auto& typeInfo = idl_->type(scope_, Scope(e.name.original()));

            // Generate switch cases
            for (const auto& protoFieldName : protoFile.enumFieldNames(e)) {
                auto idlFieldName = utils::toCamelCase(protoFieldName, true);

                auto fieldDict = dict->AddSectionDictionary("CASE");
                fieldDict->SetValue(
                    "PB_FIELD_NAME", pbFieldScopeString + protoFieldName);
                fieldDict->SetValue("IDL_FIELD_NAME",
                    cpp::fullName(typeInfo.fullNameAsScope[CPP]) + "::" + idlFieldName);
            }

            dict->SetValue("RUNTIME_NAMESPACE_PREFIX",
                "::" + idl_->env->runtimeFramework()->cppNamespace.asPrefix("::"));
            importMaker_.addCppRuntimeImportPath("exception.h");
        });
}

void CppGenerator::onVisited(const nodes::Struct& s)
{
    addNode<nodes::Struct>(s, "struct.tpl",
        [this](
            const nodes::Struct& s,
            ctemplate::TemplateDictionary* dict)
        {
            const auto& typeInfo =
                idl_->type(scope_, Scope(s.name.original()));
            dict->SetValue(
                "CPP_TYPE_FULL_NAME", cpp::fullName(typeInfo.fullNameAsScope[CPP]));

            std::string idlStructObjectName = s.name.original();
            idlStructObjectName[0] = std::tolower(idlStructObjectName[0]);
            dict->SetValue("IDL_OBJECT_NAME", idlStructObjectName);

            structDataStack_.push_back(StructInfo{&s, dict});

            dict->ShowSection(isDecoderEmpty(s) ? "HAS_NO_BODY" : "HAS_BODY");
        });

    if (!s.protoMessage || s.customCodeLink.protoconvHeader) {
        structDataStack_.push_back(StructInfo{&s, nullptr});
    }

    traverseWithScope(scope_, s, this);
    structDataStack_.pop_back();
}

namespace {

/**
 * Converts Protobuf name to C++ format. Basically a workaround that adds _ to
 * C++ keywords. Only field names of .proto messages need such conversion.
 */
std::string protoFieldNameToCpp(const std::string& protoName)
{
    if (protoName == "class") {
        return "class_";
    }
    return boost::algorithm::to_lower_copy(protoName);
}

} // namespace

void CppGenerator::onVisited(const nodes::StructField& f)
{
    if (f.protoField) {
        StructInfo& info = structDataStack_.back();
        if (!info.bodyDictionary) {
            return; // parent was not auto-generated
        }

        ctemplate::TemplateDictionary* fieldDict =
            info.bodyDictionary->AddSectionDictionary("FIELD");

        fieldDict->SetValue("IDL_FIELD_TYPE", typeRefToString(f.typeRef));
        fieldDict->SetValue("IDL_FIELD_NAME", f.name);

        if (f.typeRef.isOptional) {
            fieldDict->ShowSection("OPTIONAL");
        }

        std::string pbTypeName = info.s->protoMessage->pathInProto.last();
        std::string pbFieldName = protoFieldNameToCpp(*f.protoField);
        fillFieldConverterSection(f, pbTypeName, pbFieldName, fieldDict);
    }
}

void CppGenerator::onVisited(const nodes::Variant& v)
{
    addNode<nodes::Variant>(v, "variant.tpl",
        [this](
            const nodes::Variant& v,
            ctemplate::TemplateDictionary* dict)
        {
            // Generate field convertions
            for (const auto& field : v.fields) {
                if (field.protoField) {
                    auto fieldDict = dict->AddSectionDictionary("FIELD");

                    fieldDict->SetValue("IDL_FIELD_TYPE",
                        typeRefToString(field.typeRef));
                    fieldDict->SetValue("IDL_FIELD_NAME", field.name);

                    auto pbTypeName = v.protoMessage->pathInProto.last();
                    auto pbFieldName = protoFieldNameToCpp(*field.protoField);
                    fillFieldConverterSection(field, pbTypeName, pbFieldName,
                        fieldDict);
                }
            }

            dict->SetValue("RUNTIME_NAMESPACE_PREFIX",
                "::" + idl_->env->runtimeFramework()->cppNamespace.asPrefix("::"));
            importMaker_.addCppRuntimeImportPath("exception.h");
        });
}

template <typename Node>
void CppGenerator::addNode(
    const Node& node,
    const std::string& tplName,
    std::function<void(
        const Node& node,
        ctemplate::TemplateDictionary* dict)> dictBuilder)
{
    if (node.protoMessage) {
        if (node.customCodeLink.protoconvHeader) {
            return;
        }

        if (node.customCodeLink.baseHeader) {
            importMaker_.addImportPath(
                '<' + *node.customCodeLink.baseHeader + '>');
        }

        auto dict = BaseGenerator::addFunctionDict(node);
        dict->ShowSection("DEF");

        dictBuilder(node, tpl::addInclude(dict, "BODY", tplName));
    }
}

namespace {

/**
 * Includes header of the core type of struct's or variant's field.
 */
template <typename Node>
void includeFieldTypeHeader(
    const Node& node,
    const TypeInfo& typeInfo,
    common::ImportMaker& importMaker)
{
    if (node.customCodeLink.baseHeader) {
        importMaker.addImportPath(
            '<' + *node.customCodeLink.baseHeader + '>');
    }

    if (node.customCodeLink.protoconvHeader) {
        importMaker.addImportPath(
            '<' + *node.customCodeLink.protoconvHeader + '>');
    } else {
        importMaker.addImportPath(filePath(typeInfo, true).inAngles());
    }
}

/**
 * Returns Protobuf enum's fully qualified name.
 */
std::string pbEnumName(const Idl* idl, const nodes::Enum& e)
{
    const nodes::ProtoMessage& message = *e.protoMessage;
    const auto& config = idl->env->config;
    ProtoFile protoFile(config.inProtoRoot,
        config.baseProtoPackage, message.pathToProto);

    return protoFile.relativeNamespacePrefix() +
        message.pathInProto.asString("::");
}

} // namespace

template <typename Field>
void CppGenerator::fillFieldConverterSection(
    const Field& field,
    const std::string& pbTypeName,
    const std::string& pbFieldName,
    ctemplate::TemplateDictionary* fieldDict)
{
    fieldDict->SetValue("PB_TYPE_NAME", pbTypeName);
    fieldDict->SetValue("PB_FIELD_NAME", pbFieldName);
    std::string pbItemName = "proto" + pbTypeName + '.' + pbFieldName + "()";

    if (FullTypeRef({ idl_, scope_ }, field.typeRef).isBridged()) {
        fieldDict->ShowSection("BRIDGED_FIELD");
    } else {
        fieldDict->ShowSection("LITE_FIELD");
    }

    bool fieldIsVector = field.typeRef.id == nodes::TypeId::Vector;
    if (fieldIsVector) {
        importMaker_.addImportPath("<vector>");
    }

    if (fieldIsVector) {
        const nodes::TypeRef& typeRef = field.typeRef.parameters[0];
        if (FullTypeRef({ idl_, scope_ }, typeRef).isBridged()) {
            fieldDict->SetValue(
                "IDL_FIELD_ITEM_TYPE", typeRefToString(typeRef));
        }
    }

    fieldDict->ShowSection(fieldIsVector ? "VECTOR" : "SIMPLE");

    auto fieldConvDict = tpl::addInclude(fieldDict, "FIELD_DECODE",
        fieldIsVector ? "vector_field.tpl" : "field.tpl");
    fieldConvDict->SetValue("PB_TYPE_NAME", pbTypeName);
    fieldConvDict->SetValue("PB_FIELD_NAME", pbFieldName);

    // Vector fields have different "core" type:
    const nodes::TypeRef& coreTypeRef =
        fieldIsVector ? field.typeRef.parameters[0] : field.typeRef;

    if (coreTypeRef.id == nodes::TypeId::Custom) {
        fieldConvDict->ShowSection("CUSTOM_TYPE");

        const auto& typeInfo = idl_->type(scope_, *coreTypeRef.name);

        auto idlEnum = std::get_if<const nodes::Enum*>(&typeInfo.type);
        if (idlEnum) {
            includeFieldTypeHeader(**idlEnum, typeInfo, importMaker_);
            if (fieldIsVector) {
                fieldConvDict->SetValueAndShowSection("PB_ENUM_NAME",
                    pbEnumName(idl_, **idlEnum), "ENUM_FIELD");
            }
        } else {
            auto idlStruct = std::get_if<const nodes::Struct*>(&typeInfo.type);
            if (idlStruct) {
                includeFieldTypeHeader(**idlStruct, typeInfo, importMaker_);
            } else {
                // Because of validator, we are sure that this is a variant
                includeFieldTypeHeader(
                    *std::get<const nodes::Variant*>(typeInfo.type),
                    typeInfo, importMaker_);
            }
        }
    }
}

std::string CppGenerator::typeRefToString(
    const nodes::TypeRef& typeRef) const
{
    return cpp::TypeNameMaker().makeRef(FullTypeRef({ idl_, scope_ }, typeRef));
}

} // namespace protoconv
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
