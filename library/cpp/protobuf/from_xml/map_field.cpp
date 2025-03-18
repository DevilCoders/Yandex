#include "map_field.h"

#include "pb_field_wrapper.h"
#include "field_mapping.h"

#include <contrib/libs/pugixml/pugixml.hpp>

#include <util/generic/yexception.h>

namespace NProtobufFromXml {
    void MapField(TPbFieldWrapper& field, const pugi::xpath_node_set& nodes, const IPbFieldMapping& fieldMapping) {
        if (field.IsRepeated()) {
            for (const auto& node : nodes) {
                fieldMapping.Add(field, node);
            }
        } else if (field.IsOptional()) {
            if (nodes.size() == 1) {
                fieldMapping.Set(field, nodes.first());
            } else {
                Y_ENSURE(nodes.empty(), "optional field \"" << field.Name() << "\" is set more than once");
            }
        } else {
            Y_ENSURE(
                nodes.size() == 1,
                "No xml node or multiple xml nodes found for required field \"" << field.ParentName() << "::" << field.Name() << "\"");
            fieldMapping.Set(field, nodes.first());
        }
    }

}
