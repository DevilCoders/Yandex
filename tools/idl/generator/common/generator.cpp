#include "common/generator.h"

#include "common/enum_maker.h"
#include "common/interface_maker.h"
#include "common/listener_maker.h"
#include "common/struct_maker.h"
#include "common/variant_maker.h"

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace common {

Generator::Generator(
    ctemplate::TemplateDictionary* parentDict,
    const Makers makers,
    const Idl* idl,
    const AbstractFilter* filter,
    const Scope& scope)
    : parentDict_(parentDict),
      makers_(makers),
      filter_(filter),
      scope_{ idl, scope }
{
}

void Generator::onVisited(const nodes::Enum& e)
{
    onSimpleNode(makers_.enums, e);
}

void Generator::onVisited(const nodes::Interface& i)
{
    onComplexNode(makers_.interfaces, i);
}

void Generator::onVisited(const nodes::Listener& l)
{
    onSimpleNode(makers_.listeners, l);
}

void Generator::onVisited(const nodes::Struct& s)
{
    onComplexNode(makers_.structs, s);
}

void Generator::onVisited(const nodes::Variant& v)
{
    onSimpleNode(makers_.variants, v);
}

template <typename Maker, typename Node>
void Generator::onSimpleNode(Maker* maker, const Node& n)
{
    if (maker) {
        if (filter_ == nullptr || filter_->isUseful(scope_.scope, n)) {
            maker->make(parentDict_, scope_, n);
        }
    }
}

template <typename Maker, typename Node>
void Generator::onComplexNode(Maker* maker, const Node& n)
{
    if (maker) {
        if (filter_ == nullptr || filter_->isUseful(scope_.scope, n)) {
            auto dict = maker->make(parentDict_, scope_, n);

            ScopeGuard guard(&scope_.scope, n.name.original());
            n.nodes.traverse(Generator(dict, makers_, scope_.idl, filter_, scope_.scope));
        }
    } else { // No struct / interface maker simply means traverse down
        traverseWithScope(scope_.scope, n, this);
    }
}

HasPublicUsagesVisitor::HasPublicUsagesVisitor(const TypeInfo& info):
    info_(info),
    used_(false)
{
}

void HasPublicUsagesVisitor::onVisited(const nodes::Function& f)
{
    if (hasInternalDoc(f))
        return;

    if (isSameType(f.result.typeRef))
        used_ = true;

    for (const auto& param : f.parameters) {
        if (isSameType(param.typeRef))
            used_ = true;
    }
}

void HasPublicUsagesVisitor::onVisited(const nodes::Property& p)
{
    if (hasInternalDoc(p))
        return;

    if (isSameType(p.typeRef))
        used_ = true;
}

bool HasPublicUsagesVisitor::isSameType(const nodes::TypeRef& typeRef)
{
    if (typeRef.id == nodes::TypeId::Vector && typeRef.parameters[0].id == nodes::TypeId::Custom)
        return typeRef.parameters[0].name->last() == info_.name.original();

    if (typeRef.id == nodes::TypeId::Dictionary && typeRef.parameters[1].id == nodes::TypeId::Custom)
        return typeRef.parameters[1].name->last() == info_.name.original();

    if (typeRef.id == nodes::TypeId::Custom)
        return typeRef.name->last() == info_.name.original();

    return false;
}

} // namespace common
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
