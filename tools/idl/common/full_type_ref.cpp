#include <yandex/maps/idl/full_type_ref.h>

#include <yandex/maps/idl/nodes/enum.h>
#include <yandex/maps/idl/nodes/interface.h>
#include <yandex/maps/idl/nodes/listener.h>
#include <yandex/maps/idl/nodes/struct.h>
#include <yandex/maps/idl/nodes/variant.h>

namespace yandex {
namespace maps {
namespace idl {

namespace {

std::vector<FullTypeRef> makeSubRefs(
    const FullScope& fullScope,
    const nodes::TypeRef& parentTypeRef)
{
    std::vector<FullTypeRef> subRefs;
    for (const auto& parameter : parentTypeRef.parameters) {
        subRefs.emplace_back(FullTypeRef(fullScope, parameter));
    }
    return subRefs;
}

} // namespace

FullTypeRef::FullTypeRef(
    const FullScope& fullScope,
    const nodes::TypeRef& typeRef)
    : fullScope_(fullScope),
      id_(typeRef.id),
      isConst_(typeRef.isConst),
      isOptional_(typeRef.isOptional),
      subRefs_(makeSubRefs(fullScope, typeRef)),
      typeInfo_(nullptr)
{
    if (id_ == nodes::TypeId::Custom) {
        typeInfo_ = &fullScope.type(*typeRef.name);
    }
}

FullTypeRef::FullTypeRef(const FullScope& fullScope, nodes::TypeId id)
    : FullTypeRef(
          fullScope,
          nodes::TypeRef{ id, std::nullopt, false, false, { } })
{
}
FullTypeRef::FullTypeRef(
    const FullScope& fullScope,
    const Scope& qualifiedName)
    : FullTypeRef(
          fullScope,
          nodes::TypeRef{ nodes::TypeId::Custom, qualifiedName, false, false, { } })
{
}

const TypeInfo& FullTypeRef::info() const
{
    REQUIRE(id_ == nodes::TypeId::Custom,
        "Treating built-in type as a custom type");

    return *typeInfo_;
}

const FullTypeRef& FullTypeRef::vectorItem() const
{
    REQUIRE(id_ == nodes::TypeId::Vector,
        "Accessing vector item on something other then a vector");
    return subRefs_[0];
}
const FullTypeRef& FullTypeRef::dictKey() const
{
    REQUIRE(id_ == nodes::TypeId::Dictionary,
        "Accessing dictionary key on something other then a dictionary");
    return subRefs_[0];
}
const FullTypeRef& FullTypeRef::dictValue() const
{
    REQUIRE(id_ == nodes::TypeId::Dictionary,
        "Accessing dictionary value on something other then a dictionary");
    return subRefs_[1];
}

template <typename Node>
bool FullTypeRef::is() const
{
    return id_ == nodes::TypeId::Custom &&
        std::holds_alternative<const Node*>(typeInfo_->type);
}
template bool FullTypeRef::is<nodes::Enum>() const;
template bool FullTypeRef::is<nodes::Interface>() const;
template bool FullTypeRef::is<nodes::Listener>() const;
template bool FullTypeRef::is<nodes::Struct>() const;
template bool FullTypeRef::is<nodes::Variant>() const;

FullTypeRef FullTypeRef::asConst(bool isConst) const
{
    auto copy = *this;
    copy.isConst_ = isConst;
    return copy;
}
FullTypeRef FullTypeRef::asOptional(bool isOptional) const
{
    auto copy = *this;
    copy.isOptional_ = isOptional;
    return copy;
}

template <typename Node>
const Node* FullTypeRef::as() const
{
    REQUIRE(id_ == nodes::TypeId::Custom,
        "Treating built-in type as a custom type");

    auto node = std::get_if<const Node*>(&typeInfo_->type);
    if (node) {
        return *node;
    } else {
        return nullptr;
    }
}
template const nodes::Enum* FullTypeRef::as() const;
template const nodes::Interface* FullTypeRef::as() const;
template const nodes::Listener* FullTypeRef::as() const;
template const nodes::Struct* FullTypeRef::as() const;
template const nodes::Variant* FullTypeRef::as() const;

FullTypeRef FullTypeRef::structField(const nodes::TypeRef& typeRef) const
{
    REQUIRE(as<nodes::Struct>(),
        "Accessing struct field on something other then a struct");

    auto fieldScope = typeInfo_->scope.original() + typeInfo_->name.original();
    return FullTypeRef({ typeInfo_->idl, fieldScope }, typeRef);
}
FullTypeRef FullTypeRef::variantField(const nodes::TypeRef& typeRef) const
{
    REQUIRE(as<nodes::Variant>(),
        "Accessing variant field on something other then a variant");

    return FullTypeRef({ typeInfo_->idl, typeInfo_->scope.original() }, typeRef);
}

bool FullTypeRef::isBitfieldEnum() const
{
    if (id_ == nodes::TypeId::Custom) {
        auto e = as<nodes::Enum>();
        return e && e->isBitField;
    } else {
        return false;
    }
}
bool FullTypeRef::isClassicEnum() const
{
    if (id_ == nodes::TypeId::Custom) {
        auto e = as<nodes::Enum>();
        return e && !e->isBitField;
    } else {
        return false;
    }
}

bool FullTypeRef::isBridged() const
{
    if (id_ == nodes::TypeId::Custom) { // Bridged structure?
        auto s = as<nodes::Struct>();
        return s && s->kind == nodes::StructKind::Bridged;
    } else { // Intrinsically bridged?
        return id_ == nodes::TypeId::Vector ||
            id_ == nodes::TypeId::Dictionary ||
            id_ == nodes::TypeId::AnyCollection;
    }
}
bool FullTypeRef::isBridgedStruct() const
{
    if (id_ == nodes::TypeId::Custom) {
        auto s = as<nodes::Struct>();
        return s && s->kind == nodes::StructKind::Bridged;
    } else {
        return false;
    }
}
bool FullTypeRef::isLiteStruct() const
{
    if (id_ == nodes::TypeId::Custom) {
        auto s = as<nodes::Struct>();
        return s && s->kind == nodes::StructKind::Lite;
    } else {
        return false;
    }
}
bool FullTypeRef::isOptionsStruct() const
{
    if (id_ == nodes::TypeId::Custom) {
        auto s = as<nodes::Struct>();
        return s && s->kind == nodes::StructKind::Options;
    } else {
        return false;
    }
}

bool FullTypeRef::isError() const
{
    if (id_ == nodes::TypeId::Custom) {
        return typeInfo_->idl->idlNamespace == Scope("runtime") &&
            typeInfo_->scope.original().isEmpty() &&
            typeInfo_->name.original() == "Error";
    } else {
        return false;
    }
}
bool FullTypeRef::isStaticInterface() const
{
    if (id_ == nodes::TypeId::Custom) {
        auto i = as<nodes::Interface>();
        return i && i->isStatic;
    } else {
        return false;
    }
}
bool FullTypeRef::isClassicInterface() const
{
    if (id_ == nodes::TypeId::Custom) {
        auto i = as<nodes::Interface>();
        return i && !i->isStatic;
    } else {
        return false;
    }
}
bool FullTypeRef::isStrongInterface() const
{
    if (id_ == nodes::TypeId::Custom) {
        auto i = as<nodes::Interface>();
        return i && i->ownership == nodes::Interface::Ownership::Strong;
    } else {
        return false;
    }
}
bool FullTypeRef::isSharedInterface() const
{
    if (id_ == nodes::TypeId::Custom) {
        auto i = as<nodes::Interface>();
        return i && i->ownership == nodes::Interface::Ownership::Shared;
    } else {
        return false;
    }
}
bool FullTypeRef::isWeakInterface() const
{
    if (id_ == nodes::TypeId::Custom) {
        auto i = as<nodes::Interface>();
        return i && i->ownership == nodes::Interface::Ownership::Weak;
    } else {
        return false;
    }
}
bool FullTypeRef::isVirtualInterface() const
{
    if (id_ == nodes::TypeId::Custom) {
        auto i = as<nodes::Interface>();
        return i && i->isVirtual;
    } else {
        return false;
    }
}

bool FullTypeRef::isLambdaListener() const
{
    if (id_ == nodes::TypeId::Custom) {
        auto l = as<nodes::Listener>();
        return l && l->isLambda;
    } else {
        return false;
    }
}
bool FullTypeRef::isClassicListener() const
{
    if (id_ == nodes::TypeId::Custom) {
        auto l = as<nodes::Listener>();
        return l && !l->isLambda;
    } else {
        return false;
    }
}
bool FullTypeRef::isStrongListener() const
{
    if (id_ == nodes::TypeId::Custom) {
        auto l = as<nodes::Listener>();
        return l && !l->isLambda && l->isStrongRef;
    } else {
        return false;
    }
}
bool FullTypeRef::isWeakListener() const
{
    if (id_ == nodes::TypeId::Custom) {
        auto l = as<nodes::Listener>();
        return l && !l->isLambda && !l->isStrongRef;
    } else {
        return false;
    }
}

bool FullTypeRef::isCppNullable() const
{
    if (id_ == nodes::TypeId::Custom) {
        return is<nodes::Listener>() || is<nodes::Interface>();
    } else {
        return id_ == nodes::TypeId::ImageProvider ||
            id_ == nodes::TypeId::AnimatedImageProvider ||
            id_ == nodes::TypeId::ModelProvider ||
            id_ == nodes::TypeId::AnimatedModelProvider ||
            id_ == nodes::TypeId::Any ||
            id_ == nodes::TypeId::PlatformView ||
            id_ == nodes::TypeId::ViewProvider;
    }
}

bool FullTypeRef::isCsNullable() const
{
    if (id_ == nodes::TypeId::Custom) {
        return !is<nodes::Enum>();
    } else {
        return id_ == nodes::TypeId::String ||
            id_ == nodes::TypeId::Bytes ||
            id_ == nodes::TypeId::Bitmap ||
            id_ == nodes::TypeId::ImageProvider ||
            id_ == nodes::TypeId::AnimatedImageProvider ||
            id_ == nodes::TypeId::ModelProvider ||
            id_ == nodes::TypeId::AnimatedModelProvider ||
            id_ == nodes::TypeId::Vector ||
            id_ == nodes::TypeId::Dictionary ||
            id_ == nodes::TypeId::Any ||
            id_ == nodes::TypeId::AnyCollection ||
            id_ == nodes::TypeId::PlatformView ||
            id_ == nodes::TypeId::ViewProvider;
    }
}

bool FullTypeRef::isJavaNullable() const
{
    if (id_ == nodes::TypeId::Custom) {
        return !isBitfieldEnum();
    } else {
        return id_ == nodes::TypeId::String ||
            id_ == nodes::TypeId::Bytes ||
            id_ == nodes::TypeId::Point ||
            id_ == nodes::TypeId::Bitmap ||
            id_ == nodes::TypeId::ImageProvider ||
            id_ == nodes::TypeId::AnimatedImageProvider ||
            id_ == nodes::TypeId::ModelProvider ||
            id_ == nodes::TypeId::AnimatedModelProvider ||
            id_ == nodes::TypeId::Vector ||
            id_ == nodes::TypeId::Dictionary ||
            id_ == nodes::TypeId::Any ||
            id_ == nodes::TypeId::AnyCollection ||
            id_ == nodes::TypeId::PlatformView ||
            id_ == nodes::TypeId::ViewProvider;
    }
}

bool FullTypeRef::isObjCNullable() const
{
    if (id_ == nodes::TypeId::Custom) {
        return !is<nodes::Enum>();
    } else {
        return id_ == nodes::TypeId::String ||
            id_ == nodes::TypeId::AbsTimestamp ||
            id_ == nodes::TypeId::RelTimestamp ||
            id_ == nodes::TypeId::Bytes ||
            id_ == nodes::TypeId::Color ||
            id_ == nodes::TypeId::Bitmap ||
            id_ == nodes::TypeId::ImageProvider ||
            id_ == nodes::TypeId::AnimatedImageProvider ||
            id_ == nodes::TypeId::ModelProvider ||
            id_ == nodes::TypeId::AnimatedModelProvider ||
            id_ == nodes::TypeId::Vector ||
            id_ == nodes::TypeId::Dictionary ||
            id_ == nodes::TypeId::Any ||
            id_ == nodes::TypeId::AnyCollection ||
            id_ == nodes::TypeId::PlatformView ||
            id_ == nodes::TypeId::ViewProvider;
    }
}

bool FullTypeRef::isByReferenceInCpp() const
{
    if (id_ == nodes::TypeId::Custom) {
        auto i = as<nodes::Interface>();
        if (i) {
            return i->ownership == nodes::Interface::Ownership::Shared;
        } else {
            return !is<nodes::Enum>();
        }
    } else {
        return id_ == nodes::TypeId::String ||
            id_ == nodes::TypeId::Bytes ||
            id_ == nodes::TypeId::Point ||
            id_ == nodes::TypeId::Bitmap ||
            id_ == nodes::TypeId::Vector ||
            id_ == nodes::TypeId::Dictionary ||
            id_ == nodes::TypeId::Any ||
            id_ == nodes::TypeId::AnyCollection;
    }
}

bool FullTypeRef::isInternal() const
{
    if (id_ == nodes::TypeId::Custom && info().isInternal)
        return true;

    for (const auto& subRef : subRefs())
        if (subRef.isInternal())
            return true;

    return false;
}

template <typename Node>
bool FullTypeRef::isHolding(const Node& n) const
{
    if (id_ == nodes::TypeId::Vector) {
        return vectorItem().isHolding(n);
    }

    if (id_ == nodes::TypeId::Dictionary) {
        return dictKey().isHolding(n) || dictValue().isHolding(n);
    }

    if (id_ == nodes::TypeId::Custom) {
        if (as<Node>() == &n) {
            return true;
        }

        auto v = as<nodes::Variant>();
        if (v) {
            for (const auto& f : v->fields) {
                if (variantField(f.typeRef).isHolding(n)) {
                    return true;
                }
            }
        }

        auto s = as<nodes::Struct>();
        if (s) {
            bool isFieldHolding = false;
            s->nodes.lambdaTraverse(
                [&](const nodes::StructField& f)
                {
                    if (structField(f.typeRef).isHolding(n)) {
                        isFieldHolding = true;
                    }
                });
            return isFieldHolding;
        }
    }

    return false;
}
template bool FullTypeRef::isHolding(const nodes::Enum&) const;
template bool FullTypeRef::isHolding(const nodes::Struct&) const;
template bool FullTypeRef::isHolding(const nodes::Variant&) const;

} // namespace idl
} // namespace maps
} // namespace yandex
