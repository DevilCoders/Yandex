#pragma once

#include <yandex/maps/idl/idl.h>
#include <yandex/maps/idl/nodes/enum.h>
#include <yandex/maps/idl/nodes/interface.h>
#include <yandex/maps/idl/nodes/listener.h>
#include <yandex/maps/idl/nodes/nodes.h>
#include <yandex/maps/idl/nodes/struct.h>
#include <yandex/maps/idl/nodes/variant.h>
#include <yandex/maps/idl/scope.h>

#include <ctemplate/template.h>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace common {

class EnumMaker;
class InterfaceMaker;
class ListenerMaker;
class StructMaker;
class VariantMaker;

/**
 * Groups .idl node "maker" objects, so they can be handled as a group (mostly
 * to make parameter passing shorter when recursively creating generators).
 */
struct Makers {
    EnumMaker* enums;
    InterfaceMaker* interfaces;
    ListenerMaker* listeners;
    StructMaker* structs;
    VariantMaker* variants;
};

/**
 * Allows to filter out some nodes during generation, so that they are not
 * handled in any way.
 */
class AbstractFilter {
public:
    virtual ~AbstractFilter() { }

    virtual bool isUseful(
        const Scope& scope, const nodes::Enum& e) const = 0;
    virtual bool isUseful(
        const Scope& scope, const nodes::Interface& i) const = 0;
    virtual bool isUseful(
        const Scope& scope, const nodes::Listener& l) const = 0;
    virtual bool isUseful(
        const Scope& scope, const nodes::Struct& s) const = 0;
    virtual bool isUseful(
        const Scope& scope, const nodes::Variant& v) const = 0;
};

/**
 * Template version of the above that has default return value.
 */
template <bool defaultValue>
class Filter : public AbstractFilter {
public:
    virtual bool isUseful(
        const Scope&, const nodes::Enum&) const override
    {
        return defaultValue;
    }
    virtual bool isUseful(
        const Scope&, const nodes::Interface&) const override
    {
        return defaultValue;
    }
    virtual bool isUseful(
        const Scope&, const nodes::Listener&) const override
    {
        return defaultValue;
    }
    virtual bool isUseful(
        const Scope&, const nodes::Struct&) const override
    {
        return defaultValue;
    }
    virtual bool isUseful(
        const Scope&, const nodes::Variant&) const override
    {
        return defaultValue;
    }
};

/**
 * Traverses nodes and fills their data into given dictionary.
 */
class Generator : public nodes::Visitor {
public:
    Generator(
        ctemplate::TemplateDictionary* parentDict,
        const Makers makers,
        const Idl* idl,
        const AbstractFilter* filter = nullptr,
        const Scope& scope = { });

    virtual void onVisited(const nodes::Enum& e) override;
    virtual void onVisited(const nodes::Interface& i) override;
    virtual void onVisited(const nodes::Listener& l) override;
    virtual void onVisited(const nodes::Struct& s) override;
    virtual void onVisited(const nodes::Variant& v) override;

private:
    template <typename Maker, typename Node>
    void onSimpleNode(Maker* maker, const Node& n);

    template <typename Maker, typename Node>
    void onComplexNode(Maker* maker, const Node& n);

protected:
    ctemplate::TemplateDictionary* parentDict_;

    const Makers makers_;
    const AbstractFilter* filter_;

    FullScope scope_;
};

class HasPublicUsagesVisitor : public nodes::Visitor {
public:
    explicit HasPublicUsagesVisitor(const TypeInfo& info);

    void onVisited(const nodes::Function& f) override;
    void onVisited(const nodes::Property& p) override;

    bool isUsed() const
    {
        return used_;
    }

private:
    bool isSameType(const nodes::TypeRef& typeRef);

    const TypeInfo& info_;
    bool used_;
};

} // namespace common
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
