#pragma once

#include <yandex/maps/idl/full_type_ref.h>
#include <yandex/maps/idl/idl.h>
#include <yandex/maps/idl/nodes/interface.h>
#include <yandex/maps/idl/type_info.h>

#include <ctemplate/template.h>

#include <set>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace common {

/**
 * Returns type-info for given interface's top-most base interface. If this
 * interface has no base classes, it will be itself.
 */
const TypeInfo* topMostBaseTypeInfo(
    const FullScope& scope,
    const nodes::Interface& i);

/**
 * Same as above, but with interface type-info.
 */
const TypeInfo* topMostBaseTypeInfo(const TypeInfo* typeInfo);

class DocMaker;
class FunctionMaker;
class ImportMaker;
class PodDecider;
class TypeNameMaker;

/**
 * Holds type information about "subscribed" listeners. Needed only to create
 * bindings of interfaces that will have methods to subscribe / unsubscribe
 * listeners.
 */
class SubscribedListenerTypes {
public:
    /**
     * Clears all found listener types.
     */
    void clear();

    /**
     * Returns all found listener types.
     */
    const std::set<TypeInfo>& listenerTypes() const;

    /**
     * Registers type info of given type reference if it refers to a classic
     * (not lambda) listener.
     */
    void addIfListener(const FullTypeRef& typeRef);

private:
    std::set<TypeInfo> listenerTypes_;
};

class InterfaceMaker {
public:
    InterfaceMaker(
        const PodDecider* podDecider,
        const TypeNameMaker* typeNameMaker,
        ImportMaker* importMaker,
        const DocMaker* docMaker,
        FunctionMaker* functionMaker,
        bool isHeader,
        bool generateBaseMethods = false);

    virtual ~InterfaceMaker();

    virtual ctemplate::TemplateDictionary* make(
        ctemplate::TemplateDictionary* parentDict,
        FullScope scope,
        const nodes::Interface& i);

protected:
    virtual ctemplate::TemplateDictionary* createDict(
        ctemplate::TemplateDictionary* parentDict,
        const FullScope& scope,
        const nodes::Interface& i) const;

    virtual ctemplate::TemplateDictionary* makeFunction(
        ctemplate::TemplateDictionary* parentDict,
        const FullScope& scope,
        const nodes::Interface& i,
        const nodes::Function& f,
        bool isBaseItem = false);

    virtual ctemplate::TemplateDictionary* makeProperty(
        ctemplate::TemplateDictionary* parentDict,
        const FullScope& scope,
        const nodes::Property& p,
        bool isBaseItem = false);

    const PodDecider* podDecider_;

    const TypeNameMaker* typeNameMaker_;
    ImportMaker* importMaker_;

    const DocMaker* docMaker_;
    FunctionMaker* functionMaker_;

    const bool isHeader_;
    const bool generateBaseMethods_;

    SubscribedListenerTypes subscribedListenerTypes_;
};

} // namespace common
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
