#include "protoconv/proto_file.h"

#include <yandex/maps/idl/config.h>
#include <yandex/maps/idl/nodes/protobuf.h>
#include <yandex/maps/idl/nodes/type_ref.h>
#include <yandex/maps/idl/scope.h>
#include <yandex/maps/idl/utils/errors.h>

#include <google/protobuf/compiler/importer.h>

#include <boost/algorithm/string.hpp>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace protoconv {

namespace {

namespace gp = google::protobuf;

} // namespace

ProtoFile::ProtoFile(
    const std::string& rootDir,
    const std::string& basePackage,
    const std::string& relativePath)
    : basePackage_(basePackage)
{
    // Protobuf parser objects
    static gp::compiler::DiskSourceTree diskSourceTree;
    static gp::compiler::Importer importer(&diskSourceTree, nullptr);

    diskSourceTree.MapPath("", rootDir.c_str());

    descriptor_ = importer.Import(relativePath.c_str());
    if (!descriptor_) {
        throw utils::UsageError() <<
            "Couldn't parse Protobuf file '" << relativePath << "'";
    }

    if (!descriptor_->package().StartsWith(basePackage)) {
        throw utils::UsageError() << "Protobuf file '" << relativePath <<
            "' has package name that doesn't start with "
            "required \"base\" package '" << basePackage << "'";
    }
}

ProtoFile::ProtoFile(const google::protobuf::FileDescriptor* descriptor)
    : descriptor_(descriptor)
{
}

bool ProtoFile::operator==(const ProtoFile& other) const
{
    // Comparing relative names of files in the source tree is sufficient
    return descriptor_->name() == other.descriptor_->name();
}

bool ProtoFile::operator!=(const ProtoFile& other) const
{
    return ! (*this == other);
}

std::string ProtoFile::relativeNamespacePrefix() const
{
    std::string relativePackage =
        descriptor_->package().substr(basePackage_.size() + 1);
    if (relativePackage.empty()) {
        return "";
    } else {
        return boost::algorithm::replace_all_copy(
            relativePackage, ".", "::") + "::";
    }
}

namespace {

/**
 * Returns given .idl node's .proto type's full name.
 */
template <typename Node>
std::string fullTypeName(const gp::FileDescriptor* descriptor, const Node& n)
{
    return descriptor->package() + '.' +
        n.protoMessage->pathInProto.asString(".");
}

} // namespace

template <typename Node>
const typename ProtoDescriptor<Node>::type* ProtoFile::findProtoTypeDesc(
    const Node& n) const
{
    return descriptor_->pool()->FindMessageTypeByName(
        fullTypeName(descriptor_, n).c_str());
}

/**
 * Specialization for enums.
 */
template <>
const gp::EnumDescriptor* ProtoFile::findProtoTypeDesc(
    const nodes::Enum& e) const
{
    return descriptor_->pool()->FindEnumTypeByName(
        fullTypeName(descriptor_, e).c_str());
}

/**
 * Instantiation for structs.
 */
template
const gp::Descriptor* ProtoFile::findProtoTypeDesc(
    const nodes::Struct& s) const;

/**
 * Instantiation for variants.
 */
template
const gp::Descriptor* ProtoFile::findProtoTypeDesc(
    const nodes::Variant& v) const;

std::vector<std::string> ProtoFile::enumFieldNames(
    const nodes::Enum& e) const
{
    const auto* enumDesc = findProtoTypeDesc(e);

    std::vector<std::string> fieldNames;
    for (int index = 0; index < enumDesc->value_count(); ++index) {
        fieldNames.push_back(enumDesc->value(index)->name());
    }
    return fieldNames;
}

} // namespace protoconv
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
