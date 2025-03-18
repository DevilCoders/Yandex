#pragma once

#include <contrib/libs/pugixml/pugixml.hpp>

#include <util/generic/string.h>
#include <util/generic/yexception.h>

#include <memory>

namespace NProtobufFromXml {
    using XmlDocPtr = std::unique_ptr<pugi::xml_document>;

    inline XmlDocPtr LoadDocument(const TString& data) {
        XmlDocPtr document{new pugi::xml_document};
        Y_VERIFY(document->load_string(data.data(), data.size()));
        return document;
    }

}
