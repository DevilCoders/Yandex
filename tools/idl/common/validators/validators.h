#pragma once

#include "variable_kind.h"

#include <yandex/maps/idl/idl.h>
#include <yandex/maps/idl/nodes/doc.h>
#include <yandex/maps/idl/nodes/enum.h>
#include <yandex/maps/idl/nodes/function.h>
#include <yandex/maps/idl/nodes/interface.h>
#include <yandex/maps/idl/nodes/listener.h>
#include <yandex/maps/idl/nodes/root.h>
#include <yandex/maps/idl/nodes/struct.h>
#include <yandex/maps/idl/nodes/type_ref.h>
#include <yandex/maps/idl/nodes/variant.h>
#include <yandex/maps/idl/nodes/nodes.h>
#include <yandex/maps/idl/scope.h>
#include <yandex/maps/idl/type_info.h>
#include <yandex/maps/idl/utils/paths.h>

#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

namespace yandex::maps::idl {

/**
 * Represents a single shared state object for all semantic validators.
 */
struct ValidatorsState {
    FullScope scope;
    std::vector<std::string> errors;
};

/**
 * Visits nodes in syntax tree and checks if semantics are OK.
 */
class Validator : public nodes::Visitor {
public:
    Validator(ValidatorsState* state);

    using nodes::Visitor::onVisited;

    virtual void onVisited(const nodes::Enum& e) override;
    virtual void onVisited(const nodes::Interface& i) override;
    virtual void onVisited(const nodes::Listener& l) override;
    virtual void onVisited(const nodes::Struct& s) override;
    virtual void onVisited(const nodes::Variant& v) override;

protected:
    ValidatorsState* state_;

protected:
    /**
     * Adds the error.
     *
     * @param context - more info about where the error was found
     * @param message - error message
     */
    void addError(const std::string& context, const std::string& message);

    /**
     * Adds the error on @internal tag with enable internal checks.
     *
     * @param context - more info about where the error was found
     * @param message - error message
     */
    void addInternalTagError(const std::string& context, const std::string& message);

    /**
     * Adds "constant qualifier not allowed" error.
     */
    void addConstNotAllowedError(const std::string& context);

    /**
     * Adds "optional qualifier not allowed" error.
     */
    void addOptionalNotAllowedError(const std::string& context);

    /**
     * Adds "type not found" error for type with given name.
     */
    void addTypeNotFoundError(
        const std::string& context,
        const Scope& typeName);

    /**
     * Adds "type not allowed" error for given type.
     */
    void addTypeNotAllowedError(
        const std::string& context,
        const nodes::TypeRef& typeRef,
        const std::string& reason);

    /**
     * Checks if enum or variant has duplicate items. Struct fields are
     * checked separately by StructValidator.
     */
    template <typename Node>
    void checkDuplicateItems(const Node& n);

    /**
     * Checks errors applicable to all kinds of functions.
     *
     * @param isNative - see VariableKind about native and platform methods.
     */
    void checkFunction(const nodes::Function& f, bool isNative);

    /**
     * Checks external function's owning type for errors.
     */
    void checkOwningType(
        const std::string& functionName,
        const std::optional<Scope>& owningTypeName);

    /**
     * Checks errors applicable to properties.
     */
    void checkProperty(const nodes::Property& p);

    /**
     * Checks interface's or listener's base type for errors.
     */
    template <typename Node>
    void checkBaseType(const Node& n);

    /**
     * Checks if type ref refers to a valid type.
     *
     * @return true if type ref is valid, false - otherwise
     */
    bool checkTypeRef(
        const std::string& errorContext,
        VariableKind kind,
        const nodes::TypeRef& typeRef,
        bool isConst = false,
        bool isOptional = false);

    /**
     * Checks if given variant has fields with duplicate types.
     */
    void checkDuplicateFieldTypes(const nodes::Variant& v);

    /**
     * These methods check given documentation items for semantic errors.
     */
    void checkDoc(
        const std::string& errorContext,
        const std::optional<nodes::Doc>& doc,
        const std::optional<nodes::Function>& function = std::nullopt);
    void checkDocBlock(
        const std::string& errorContext,
        const nodes::DocBlock& block);
    void checkDocLink(
        const std::string& errorContext,
        const nodes::DocLink& link,
        const TypeInfo& info);

    /**
     * Checks the member give doc link refers to for semantic errors. This
     * method is parameterized by Idl node type: enum, interface, variant, ...
     */
    template <typename Node>
    void checkDocLinkMember(
        const std::string& errorContext,
        const nodes::DocLink& link,
        const TypeInfo& typeInfo);
};

class StructValidator : public Validator {
public:
    StructValidator(ValidatorsState* state, const nodes::Struct* s);

    using Validator::onVisited; // to avoid hidden overloaded functions
    virtual void onVisited(const nodes::StructField& f) override;

    unsigned int numberOfFieldsVisited() const;

private:
    const nodes::Struct* s_;
    std::unordered_set<std::string> visitedFieldNames_;
};

class InterfaceValidator : public Validator {
public:
    InterfaceValidator(ValidatorsState* state, const nodes::Interface* i);

    using Validator::onVisited;
    virtual void onVisited(const nodes::Function& f) override;
    virtual void onVisited(const nodes::Property& p) override;

private:
    const nodes::Interface* interface_;
    const bool isInternal_;
};

/**
 * Validates parsed tree against semantic errors - as opposed to syntax errors
 * that are checked by Flex / Bison during parsing.
 */
std::vector<std::string> validate(const Idl* idl);

} // namespace yandex::maps::idl
