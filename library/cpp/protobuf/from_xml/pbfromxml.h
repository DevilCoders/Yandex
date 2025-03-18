#pragma once

#include "enum.h"
#include "pb.h"

#include <contrib/libs/pugixml/pugixml.hpp>

#include <util/generic/maybe.h>
#include <util/generic/string.h>

#include <functional>
#include <initializer_list>

namespace NProtobufFromXml {
    class TMapping {
    public:
        TMapping(const TString& fieldName, const TString& xmlPath, void (*mapFunc)(PbMessage&, const pugi::xml_node&));
        TMapping(const TString& fieldName, const TString& xmlPath, TMaybe<TEnum> (*mapFunc)(const TString&));
        TMapping(const TString& fieldName, const TString& xmlPath, TMaybe<bool> (*mapFunc)(const TString&));
        TMapping(const TString& fieldName, const TString& xmlPath);

        void operator()(PbMessage& msg, const pugi::xml_node& node) const;

    private:
        std::function<void(PbMessage&, const pugi::xml_node&)> MapFunc;
    };

    // Useful for bool values represented with empty node
    inline TMaybe<bool> True(const TString&) {
        return true;
    }

    void Fields(PbMessage& msg, const pugi::xml_node& node, const std::initializer_list<TMapping>& mappings);

}
