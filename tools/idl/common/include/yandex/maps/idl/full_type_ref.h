#pragma once

#include <yandex/maps/idl/idl.h>
#include <yandex/maps/idl/nodes/type_ref.h>
#include <yandex/maps/idl/type_info.h>

#include <cstddef>
#include <vector>

namespace yandex {
namespace maps {
namespace idl {

class FullTypeRef {
public:
    //
    // Constructors
    //

    FullTypeRef(const FullScope& fullScope, const nodes::TypeRef& typeRef);

    FullTypeRef(const FullScope& fullScope, nodes::TypeId id);
    FullTypeRef(const FullScope& fullScope, const Scope& qualifiedName);

    FullTypeRef(FullScope fullScope, const nodes::Name& name)
        : FullTypeRef(fullScope, Scope(name.original()))
    {
    }

    FullTypeRef(const TypeInfo& info)
        : FullTypeRef(FullScope{ info.idl, info.scope.original() }, info.name)
    {
    }

    //
    // Field getters
    //

    const FullScope& fullScope() const { return fullScope_; }

    const Idl* idl() const { return fullScope_.idl; }
    const Environment* env() const { return fullScope_.idl->env; }

    nodes::TypeId id() const { return id_; }
    bool isConst() const { return isConst_; }
    bool isOptional() const { return isOptional_; }
    const std::vector<FullTypeRef>& subRefs() const { return subRefs_; }

    const TypeInfo& info() const;

    const FullTypeRef& vectorItem() const;
    const FullTypeRef& dictKey() const;
    const FullTypeRef& dictValue() const;

    //
    // General methods
    //

    template <typename Node>
    bool is() const; // Returns false if type is not a custom type

    FullTypeRef asConst(bool isConst = true) const;
    FullTypeRef asOptional(bool isOptional = true) const;

    template <typename Node>
    const Node* as() const; // Type must be a custom type

    FullTypeRef structField(const nodes::TypeRef& typeRef) const;
    FullTypeRef variantField(const nodes::TypeRef& typeRef) const;

    bool isBitfieldEnum() const;
    bool isClassicEnum() const;

    bool isBridged() const;
    bool isBridgedStruct() const;
    bool isLiteStruct() const;
    bool isOptionsStruct() const;

    bool isError() const;
    bool isStaticInterface() const;
    bool isClassicInterface() const;
    bool isStrongInterface() const;
    bool isSharedInterface() const;
    bool isWeakInterface() const;
    bool isVirtualInterface() const;

    bool isLambdaListener() const;
    bool isClassicListener() const;
    bool isStrongListener() const;
    bool isWeakListener() const;

    bool canBeRequired() const { return !isCppNullable(); }
    bool isCppNullable() const;
    bool isCsNullable() const;
    bool isJavaNullable() const;
    bool isObjCNullable() const;

    bool isByReferenceInCpp() const; // Passed by const-ref in C++?

    bool isInternal() const;

    /**
     * @return - true if a type that this reference refers to is a
     *           "collection" (vector, dictionary, struct or variant), and it
     *           holds given node (as an item / field)
     */
    template <typename Node>
    bool isHolding(const Node& n) const;

private:
    FullScope fullScope_;

    nodes::TypeId id_;
    bool isConst_;
    bool isOptional_;

    std::vector<FullTypeRef> subRefs_; // If generic type

    const TypeInfo* typeInfo_; // If custom type
};

} // namespace idl
} // namespace maps
} // namespace yandex
