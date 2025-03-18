#pragma once

#include <contrib/libs/pugixml/pugixml.hpp>
#include <util/generic/string.h>

namespace NProtobufFromXml {
    inline TString NodeText(const pugi::xpath_node& node) {
        if (!node.attribute().empty()) {
            return node.attribute().value();
        }
        TString result;
        for (const auto& child : node.node().children()) {
            result += child.text().get();
        }
        return result;
    }

}
