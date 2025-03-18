#pragma once

#include <yandex/maps/idl/idl.h>
#include <yandex/maps/idl/nodes/function.h>
#include <yandex/maps/idl/nodes/listener.h>
#include <yandex/maps/idl/scope.h>

#include <ctemplate/template.h>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace common {

class DocMaker;
class FunctionMaker;
class TypeNameMaker;

class ListenerMaker {
public:
    ListenerMaker(
        const TypeNameMaker* typeNameMaker,
        const DocMaker* docMaker,
        FunctionMaker* functionMaker,
        bool isLambdaSupported);

    virtual ~ListenerMaker() { }

    virtual ctemplate::TemplateDictionary* make(
        ctemplate::TemplateDictionary* parentDict,
        FullScope scope,
        const nodes::Listener& l);

protected:
    /**
     * Creates listener's dictionary.
     */
    virtual ctemplate::TemplateDictionary* createDict(
        ctemplate::TemplateDictionary* parentDict,
        const FullScope& scope,
        const nodes::Listener& l) const;

    virtual ctemplate::TemplateDictionary* makeFunction(
        ctemplate::TemplateDictionary* parentDict,
        const FullScope& scope,
        const nodes::Listener& l,
        const nodes::Function& f);

    const TypeNameMaker* typeNameMaker_;
    const DocMaker* docMaker_;
    FunctionMaker* functionMaker_;

    bool isLambdaSupported_;
};

} // namespace common
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
