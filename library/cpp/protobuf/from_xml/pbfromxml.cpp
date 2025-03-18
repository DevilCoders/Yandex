#include "pbfromxml.h"

#include "enum.h"
#include "field_mapping.h"
#include "map_field.h"
#include "pb.h"
#include "pb_field_wrapper.h"

#include <util/generic/map.h>
#include <util/generic/vector.h>
#include <util/generic/yexception.h>

#include <memory>

namespace pb = google::protobuf;

namespace NProtobufFromXml {
    TMapping::TMapping(const TString& fieldName, const TString& xmlPath, void (*mapFunc)(PbMessage&, const pugi::xml_node&)) {
        MapFunc = [=](PbMessage& msg, const pugi::xml_node& node) {
            TPbFieldWrapper field(msg, fieldName);
            field.RequireType(PbFieldDescriptor::TYPE_MESSAGE);
            MapField(
                field,
                node.select_nodes(xmlPath.data()),
                TMessageFieldMapping(mapFunc));
        };
    }

    TMapping::TMapping(const TString& fieldName, const TString& xmlPath, TMaybe<TEnum> (*mapFunc)(const TString&)) {
        MapFunc = [=](PbMessage& msg, const pugi::xml_node& node) {
            TPbFieldWrapper field(msg, fieldName);
            field.RequireType(PbFieldDescriptor::TYPE_ENUM);
            MapField(
                field,
                node.select_nodes(xmlPath.data()),
                TPrimitiveFieldMapping<TEnum>(mapFunc));
        };
    }

    TMapping::TMapping(const TString& fieldName, const TString& xmlPath, TMaybe<bool> (*mapFunc)(const TString&)) {
        MapFunc = [=](PbMessage& msg, const pugi::xml_node& node) {
            TPbFieldWrapper field(msg, fieldName);
            field.RequireType(PbFieldDescriptor::TYPE_BOOL);
            MapField(
                field,
                node.select_nodes(xmlPath.data()),
                TPrimitiveFieldMapping<bool>(mapFunc));
        };
    }

    TMapping::TMapping(const TString& fieldName, const TString& xmlPath) {
        using TMappers = TMap<int, std::unique_ptr<IPbFieldMapping>>;
        static const auto mappers = [] {
            TMappers result;
            result[PbFieldDescriptor::TYPE_DOUBLE].reset(new TPrimitiveFieldMapping<double>());
            result[PbFieldDescriptor::TYPE_FLOAT].reset(new TPrimitiveFieldMapping<float>());
            result[PbFieldDescriptor::TYPE_INT32].reset(new TPrimitiveFieldMapping<i32>());
            result[PbFieldDescriptor::TYPE_SINT32].reset(new TPrimitiveFieldMapping<i32>());
            result[PbFieldDescriptor::TYPE_UINT32].reset(new TPrimitiveFieldMapping<ui32>());
            result[PbFieldDescriptor::TYPE_INT64].reset(new TPrimitiveFieldMapping<i64>());
            result[PbFieldDescriptor::TYPE_SINT64].reset(new TPrimitiveFieldMapping<i64>());
            result[PbFieldDescriptor::TYPE_UINT64].reset(new TPrimitiveFieldMapping<ui64>());
            result[PbFieldDescriptor::TYPE_BOOL].reset(new TPrimitiveFieldMapping<bool>());
            result[PbFieldDescriptor::TYPE_STRING].reset(new TPrimitiveFieldMapping<TString>());
            return result;
        }();

        MapFunc = [=](PbMessage& msg, const pugi::xml_node& node) {
            TPbFieldWrapper field(msg, fieldName);
            MapField(field, node.select_nodes(xmlPath.data()), *mappers.at(field.Type()));
        };
    }

    void TMapping::operator()(PbMessage& msg, const pugi::xml_node& node) const {
        MapFunc(msg, node);
    }

    void Fields(PbMessage& msg, const pugi::xml_node& node, const std::initializer_list<TMapping>& mappings) {
        for (const auto& mapping : mappings) {
            mapping(msg, node);
        }
    }

}
