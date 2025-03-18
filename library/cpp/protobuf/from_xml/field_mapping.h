#pragma once

#include "field_mapping.h"
#include "node_text.h"
#include "pb.h"
#include "pb_field_wrapper.h"

#include <google/protobuf/message.h>
#include <contrib/libs/pugixml/pugixml.hpp>

#include <util/generic/maybe.h>
#include <util/generic/string.h>
#include <util/generic/yexception.h>
#include <util/stream/str.h>
#include <util/string/cast.h>

#include <functional>

namespace NProtobufFromXml {
    class IPbFieldMapping {
    public:
        virtual ~IPbFieldMapping() {
        }

        virtual void Set(TPbFieldWrapper& field, const pugi::xpath_node& node) const = 0;
        virtual void Add(TPbFieldWrapper& field, const pugi::xpath_node& node) const = 0;
    };

    class TMessageFieldMapping: public IPbFieldMapping {
    public:
        using TMapFunc = std::function<void(PbMessage&, const pugi::xml_node&)>;

        explicit TMessageFieldMapping(TMapFunc mapFunc)
            : MapFunc(mapFunc)
        {
        }

        void Set(TPbFieldWrapper& field, const pugi::xpath_node& node) const override {
            MapFunc(field.MutableMessage(), node.node());
        }

        void Add(TPbFieldWrapper& field, const pugi::xpath_node& node) const override {
            MapFunc(field.AddMessage(), node.node());
        }

    private:
        TMapFunc MapFunc;
    };

    template <typename T>
    class TPrimitiveFieldMapping: public IPbFieldMapping {
    public:
        using TMapFunc = std::function<TMaybe<T>(const TString&)>;

        TPrimitiveFieldMapping()
            : MapFunc(&Read)
        {
        }

        explicit TPrimitiveFieldMapping(TMapFunc mapFunc)
            : MapFunc(mapFunc)
        {
        }

        void Set(TPbFieldWrapper& field, const pugi::xpath_node& node) const override {
            if (const auto value = MapFunc(NodeText(node))) {
                field.Set(*value);
            }
        }

        void Add(TPbFieldWrapper& field, const pugi::xpath_node& node) const override {
            if (const auto value = MapFunc(NodeText(node))) {
                field.Add(*value);
            }
        }

    private:
        static T Read(const TString& s) {
            return FromString(s);
        }

    private:
        TMapFunc MapFunc;
    };

}
