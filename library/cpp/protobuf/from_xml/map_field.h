#pragma once

#include "pb_field_wrapper.h"
#include "field_mapping.h"

#include <contrib/libs/pugixml/pugixml.hpp>

namespace NProtobufFromXml {
    void MapField(TPbFieldWrapper& field, const pugi::xpath_node_set& nodes, const IPbFieldMapping& fieldMapping);
}
