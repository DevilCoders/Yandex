#include "validators.h"

#include <yandex/maps/idl/full_type_ref.h>
#include <yandex/maps/idl/functions.h>
#include <yandex/maps/idl/targets.h>
#include <yandex/maps/idl/nodes/interface.h>
#include <yandex/maps/idl/utils.h>
#include <yandex/maps/idl/utils/exception.h>

#include <iterator>

namespace yandex::maps::idl {

namespace {

std::optional<nodes::Interface::Ownership> getInterfaceOwnership(const nodes::TypeRef& type, const FullScope& scope)
{
    if (type.id != nodes::TypeId::Custom)
        return std::nullopt;

    const auto& typeInfo = scope.type(*type.name);
    auto interface = std::get_if<const nodes::Interface*>(&typeInfo.type);
    return interface ? std::make_optional<nodes::Interface::Ownership>((**interface).ownership) : std::nullopt;
}

bool isValidCollectionParameters(const nodes::TypeRef& typeRef, const FullScope& scope) {
    for (const auto& parameter: typeRef.parameters) {
        auto ownership = getInterfaceOwnership(parameter, scope);
        if (ownership && ownership.value() == nodes::Interface::Ownership::Strong) 
            return false;
    }
    return true;
}

bool containsInterfaces(const nodes::TypeRef& type, const FullScope& scope)
{
    if (type.id == nodes::TypeId::Vector || type.id == nodes::TypeId::Dictionary) {
        for (const auto& parameter : type.parameters)
            return containsInterfaces(parameter, scope);
    } else if (type.id == nodes::TypeId::Custom) {
        const auto& typeInfo = scope.type(*type.name);
        return std::holds_alternative<const nodes::Interface*>(typeInfo.type);
    }

    return false;
}

} // namespace

Validator::Validator(ValidatorsState* state)
    : state_(state)
{
}

void Validator::onVisited(const nodes::Enum& e)
{
    ScopeGuard guard(&state_->scope.scope, e.name.original());

    checkDoc("", e.doc);

    if (e.fields.size() == 1) {
        addError("", "Enum has only one constant");
    } else {
        checkDuplicateItems(e);

        for (const auto& field : e.fields) {
            if (hasInternalDoc(field))
                addInternalTagError(field.name, "Enum cannot have @internal tag");
            checkDoc(field.name, field.doc);
        }

        if (e.isBitField) { // All enum constants must have values
            for (const auto& field : e.fields) {
                if (!field.value) {
                    addError("", "Bitfield enum constant '" +
                        field.name + "' has no value");
                }
            }
        } else { // All enum constants must not have values
            for (const auto& field : e.fields) {
                if (field.value) {
                    addError("", "Non-bitfield enum constant '" + field.name +
                        "' has value");
                }
            }
        }
    }
}

void Validator::onVisited(const nodes::Interface& i)
{
    ScopeGuard guard(&state_->scope.scope, i.name.original());

    checkDoc("", i.doc);
    checkBaseType(i);

    i.nodes.traverse(InterfaceValidator(state_, &i));
}

void Validator::onVisited(const nodes::Listener& l)
{
    ScopeGuard guard(&state_->scope.scope, l.name.original());

    checkDoc("", l.doc);
    checkBaseType(l);

    if (l.isLambda) {
        if (l.doc && !isExcludeDoc(l.doc)) {
            addError("",
                "Lambda listener cannot have docs except EXCLUDE flags");
        }
        std::unordered_set<std::string> visitedMethodNames;

        for (const auto& method : l.functions) {
            const auto& originalMethodName = method.name.original().first();

            if (method.result.typeRef.id != nodes::TypeId::Void) {
                addError(originalMethodName,
                    "Lambda-listener methods must return void");
            }

            if (!visitedMethodNames.insert(originalMethodName).second) {
                addError(originalMethodName, "Duplicate lambda method");
            }
        }
    }

    for (const auto& method : l.functions) {
        checkFunction(method, false);
    }
}

void Validator::onVisited(const nodes::Struct& s)
{
    ScopeGuard guard(&state_->scope.scope, s.name.original());

    checkDoc("", s.doc);

    StructValidator structValidator(state_, &s);
    s.nodes.traverse(&structValidator);
    if (structValidator.numberOfFieldsVisited() == 0) {
        addError("", "Struct has no fields");
    }
}

void Validator::onVisited(const nodes::Variant& v)
{
    ScopeGuard guard(&state_->scope.scope, v.name.original());

    checkDoc("", v.doc);

    if (v.fields.size() == 1) {
        addError("", "Variant has only one item");
    } else {
        checkDuplicateItems(v);
        checkDuplicateFieldTypes(v);
    }

    for (const auto& field : v.fields) {
        if (field.typeRef.isConst) {
            addConstNotAllowedError(field.name);
        }
        if (field.typeRef.isOptional) {
            addOptionalNotAllowedError(field.name);
        }
        checkTypeRef(field.name, VariableKind::Field, field.typeRef);
    }
}

void Validator::addError(
    const std::string& context,
    const std::string& message)
{
    std::string scopeString = state_->scope.scope;
    if (!context.empty()) {
        if (!scopeString.empty()) {
            scopeString += '.';
        }
        scopeString += context;
    }

    state_->errors.push_back(scopeString + ": " + message);
}

void Validator::addInternalTagError(const std::string& context, const std::string& message)
{
    if (disableInternalChecks(state_->scope))
        return;
    addError(context, message);
}

void Validator::addConstNotAllowedError(const std::string& context)
{
    addError(context, "Constant qualifier is not allowed");
}

void Validator::addOptionalNotAllowedError(const std::string& context)
{
    addError(context, "Optional qualifier is not allowed");
}

void Validator::addTypeNotFoundError(
    const std::string& context,
    const Scope& typeName)
{
    addError(context, "Type not found '" + std::string(typeName) + "'");
}

void Validator::addTypeNotAllowedError(
    const std::string& context,
    const nodes::TypeRef& typeRef,
    const std::string& reason)
{
    addError(context, "Type not allowed '" + typeRefToString(typeRef) +
        "'. " + reason);
}

template <typename Node>
void Validator::checkDuplicateItems(const Node& n)
{
    std::unordered_set<std::string> visitedFieldNames;
    for (const auto& field : n.fields) {
        if (!visitedFieldNames.insert(field.name).second) {
            addError("", "Duplicate item found '" + field.name + "'");
        }
    }
}

void Validator::checkFunction(const nodes::Function& f, bool isNative)
{
    checkDoc(f.name.original().first(), f.doc, f);

    checkTypeRef(f.name.original().first() + ", return value",
        methodVariableKind(isNative, false), f.result.typeRef,
        f.result.typeRef.isConst, f.result.typeRef.isOptional);

    for (const auto& parameter : f.parameters) {
        checkTypeRef(f.name.original().first() +
            ", parameter '" + parameter.name + "'",
            methodVariableKind(isNative, true), parameter.typeRef,
            parameter.typeRef.isConst, parameter.typeRef.isOptional);

        FullTypeRef ref(state_->scope, parameter.typeRef);

        if (ref.isByReferenceInCpp() && !ref.isCppNullable() &&
                !ref.isBridged() && !ref.is<nodes::Variant>()) {
            if (!ref.isConst()) {
                addError(f.name.original().first(),
                    "Parameter \"" + parameter.name + "\" must be "
                    "const because in C++ it is passed by reference");
            }
        }
    }

    if (f.name.isDefined(JAVA) && f.name[JAVA].size() != 1) {
        addError(f.name.original().first(),
            "Invalid Java-specific function name");
    }

    if (f.name.isDefined(OBJC)) {
        if (f.name[OBJC].size() != f.parameters.size()) {
            if (f.name[OBJC].size() != 1 || f.parameters.size() != 0) {
                addError(f.name.original().first(),
                    "Invalid Objective-C-specific function name");
            }
        }
    }
}

void Validator::checkOwningType(
    const std::string& functionName,
    const std::optional<Scope>& owningTypeName)
{
    if (owningTypeName) {
        try {
            const auto& typeInfo = state_->scope.type(*owningTypeName);
            if (!std::holds_alternative<const nodes::Struct*>(typeInfo.type) &&
                    !std::holds_alternative<const nodes::Interface*>(typeInfo.type)) {
                addError(functionName,
                    "Only structs and interfaces can own external functions");
            }
        } catch (const utils::Exception& /* e */) {
            addTypeNotFoundError(functionName, *owningTypeName);
        }
    }
}

void Validator::checkProperty(const nodes::Property& p)
{
    std::optional<nodes::Interface::Ownership> interfaceType;
    if (p.typeRef.id == nodes::TypeId::Custom) {
        try {
            const auto& typeInfo = state_->scope.type(*p.typeRef.name);
            if (auto i = std::get_if<const nodes::Interface*>(&typeInfo.type))
                interfaceType = (**i).ownership;
        } catch (const utils::Exception& /* e */) {
            addTypeNotFoundError(p.name, *p.typeRef.name);
        }
    } else if (p.typeRef.id == nodes::TypeId::Vector || p.typeRef.id == nodes::TypeId::Dictionary) {
        if (!isValidCollectionParameters(p.typeRef, state_->scope))
            addError(p.name, "Collecton not allowed \"strong\" interface types");
    }

    if (interfaceType) {
        if (p.isGenerated) {
            addError(p.name, "Gen qualifier is not allowed on properties "
                "with interface types");
        }
        if (*interfaceType == nodes::Interface::Ownership::Strong) {
            addError(p.name, "Properties with \"strong\" interface types "
                "are not allowed - use weak_ref or shared_ref");
        }
        if (*interfaceType == nodes::Interface::Ownership::Weak && p.typeRef.isConst) {
            addError(p.name, "Constant qualifier is not allowed on \"weak\" interface types");
        }
    } else {
        if (p.typeRef.isConst) {
            addError(p.name, "Constant qualifier is not allowed on"
                " non-interface types - use readonly on the right side");
        }
    }

    if (!p.isReadonly && containsInterfaces(p.typeRef, state_->scope)) {
        addError(p.name, "Readonly qualifier required for properties with interface types");
    }
}

template <typename Node>
void Validator::checkBaseType(const Node& n)
{
    if (n.base) {
        try {
            const auto& typeInfo = state_->scope.type(*n.base);
            auto base = std::get_if<const Node*>(&typeInfo.type);
            if (base) {
                if (*base == &n) {
                    addError("", "Derives from itself");
                }
            } else {
                addError("", "Base type is of wrong kind");
            }
        } catch (const utils::Exception& /* e */) {
            addTypeNotFoundError("", *n.base);
        }
    }
}

namespace {

/**
 * Returns true if given variable can have const qualifier.
 */
bool canVariableBeConstant(
    const FullScope& scope,
    VariableKind kind,
    bool isOptional,
    const nodes::TypeRef& typeRef)
{
    if (kind == VariableKind::Field || kind == VariableKind::OptionsField) {
        return false;
    }

    bool canBeConstant = false;
    if (kind == VariableKind::NativeMethodParameter ||
            kind == VariableKind::PlatformMethodParameter) {
        if (isOptional) {
            canBeConstant = true;
        } else if (typeRef.id == nodes::TypeId::String ||
                typeRef.id == nodes::TypeId::Bytes ||
                typeRef.id == nodes::TypeId::Point ||
                typeRef.id == nodes::TypeId::Bitmap ||
                typeRef.id == nodes::TypeId::ImageProvider ||
                typeRef.id == nodes::TypeId::AnimatedImageProvider ||
                typeRef.id == nodes::TypeId::ModelProvider ||
                typeRef.id == nodes::TypeId::AnimatedModelProvider ||
                typeRef.id == nodes::TypeId::Vector ||
                typeRef.id == nodes::TypeId::Dictionary ||
                typeRef.id == nodes::TypeId::Any ||
                typeRef.id == nodes::TypeId::AnyCollection ||
                typeRef.id == nodes::TypeId::ViewProvider) {
            canBeConstant = true;
        }
    }
    if (typeRef.id == nodes::TypeId::Custom) {
        try {
            const auto& typeInfo = scope.type(*typeRef.name);
            if (kind == VariableKind::NativeMethodParameter ||
                    kind == VariableKind::PlatformMethodParameter) {
                if (!std::holds_alternative<const nodes::Enum*>(typeInfo.type)) {
                    canBeConstant = true;
                }
            } else if (auto interface = std::get_if<const nodes::Interface*>(&typeInfo.type)) {
                canBeConstant = (*interface)->ownership != nodes::Interface::Ownership::Weak;
            }
        } catch (const utils::Exception& /* e */) {
            // Ignored - handled in Validator::checkTypeRef
        }
    }
    return canBeConstant;
}

} // namespace

bool Validator::checkTypeRef(
    const std::string& errorContext,
    VariableKind kind,
    const nodes::TypeRef& typeRef,
    bool isConst,
    bool isOptional)
{
    bool isValid = true;

    if (typeRef.id == nodes::TypeId::Void) {
        if (kind != VariableKind::NativeMethodReturnValue &&
                kind != VariableKind::PlatformMethodReturnValue) {
            addTypeNotAllowedError(errorContext, typeRef,
                "Void can only be used as a type of function return value");
            isValid = false;
        }

        if (isConst || isOptional) {
            addTypeNotAllowedError(errorContext, typeRef,
                "Void can be neither const nor optional");
            isValid = false;
        }
    }

    if (isConst && !canVariableBeConstant(
            state_->scope, kind, isOptional, typeRef)) {
        addConstNotAllowedError(errorContext);
        isValid = false;
    }

    if (typeRef.id == nodes::TypeId::Bitmap) {
        if (kind == VariableKind::Field || kind == VariableKind::OptionsField) {
            addTypeNotAllowedError(errorContext, typeRef,
                "Bitmap cannot be a struct / variant field");
            isValid = false;
        } else if (kind == VariableKind::NativeMethodParameter ||
                kind == VariableKind::PlatformMethodReturnValue) {
            addTypeNotAllowedError(errorContext, typeRef,
                "Cannot pass bitmap to native code - use image-provider instead");
            isValid = false;
        }
    } else if (typeRef.id == nodes::TypeId::ImageProvider
            || typeRef.id == nodes::TypeId::AnimatedImageProvider) {
        if (kind == VariableKind::Field || kind == VariableKind::OptionsField) {
            addTypeNotAllowedError(errorContext, typeRef,
                "Image-Provider cannot be a struct / variant field");
            isValid = false;
        } else if (kind == VariableKind::NativeMethodReturnValue ||
                kind == VariableKind::PlatformMethodParameter) {
            addTypeNotAllowedError(errorContext, typeRef,
                "Cannot pass image-provider to platform code - use bitmap instead");
            isValid = false;
        }
    } else if (typeRef.id == nodes::TypeId::ModelProvider
            || typeRef.id == nodes::TypeId::AnimatedModelProvider) {
        if (kind == VariableKind::Field || kind == VariableKind::OptionsField) {
            addTypeNotAllowedError(errorContext, typeRef,
                "Model provider cannot be a struct / variant field");
            isValid = false;
        } else if (kind == VariableKind::NativeMethodReturnValue ||
                kind == VariableKind::PlatformMethodParameter) {
            addTypeNotAllowedError(errorContext, typeRef,
                "Cannot pass model provider to platform code");
            isValid = false;
        }
    } else if (typeRef.id == nodes::TypeId::Vector) {
        if (!isValidCollectionParameters(typeRef, state_->scope)) {
            addError(errorContext,
                "Vector cannot contains interface type");
            isValid = false;
        }

        if (typeRef.parameters[0].id == nodes::TypeId::Bytes) {
            addTypeNotAllowedError(errorContext, typeRef,
                "Cannot have a vector of bytes vectors");
            isValid = false;
        } else {
            isValid &= checkTypeRef(errorContext, kind, typeRef.parameters[0]);
        }
    } else if (typeRef.id == nodes::TypeId::Dictionary) {
        if (!isValidCollectionParameters(typeRef, state_->scope)) {
            addError(errorContext,
                "Dictionary cannot contains interface type");
            isValid = false;
        }

        if (typeRef.parameters[0].id != nodes::TypeId::String) {
            addError(errorContext,
                "Only dictionaries with string keys are supported");
            isValid = false;
        } else  if (typeRef.parameters[1].id == nodes::TypeId::Bytes) {
            addTypeNotAllowedError(errorContext, typeRef,
                "Cannot have a dictionary of bytes vectors");
            isValid = false;
        } else {
            isValid &= checkTypeRef(errorContext, kind, typeRef.parameters[1]);
        }
    } else if (typeRef.id == nodes::TypeId::Any) {
        if (kind == VariableKind::Field || kind == VariableKind::OptionsField) {
            addTypeNotAllowedError(errorContext, typeRef,
                "Any-Collection cannot be a struct / variant field");
            isValid = false;
        }
    } else if (typeRef.id == nodes::TypeId::PlatformView) {
        if (kind == VariableKind::Field || kind == VariableKind::OptionsField) {
            addTypeNotAllowedError(errorContext, typeRef,
                "PlatformView cannot be a struct / variant field");
            isValid = false;
        }
    } else if (typeRef.id == nodes::TypeId::Custom) {
        try {
            const auto& typeInfo = state_->scope.type(*typeRef.name);
            auto i = std::get_if<const nodes::Interface*>(&typeInfo.type);
            if (i) {
                if (kind == VariableKind::Field) {
                    addTypeNotAllowedError(errorContext, typeRef,
                        "Interface cannot be a non options struct / variant field");
                    isValid = false;
                } else if (kind == VariableKind::PlatformMethodReturnValue) {
                    addTypeNotAllowedError(errorContext, typeRef,
                        "Cannot return interface from listener method");
                    isValid = false;
                } else if (kind == VariableKind::NativeMethodParameter) {
                    if ((**i).ownership == nodes::Interface::Ownership::Strong) {
                        addTypeNotAllowedError(errorContext, typeRef,
                            "Cannot pass 'unique' ownership to native code");
                        isValid = false;
                    }
                }
            } else {
                auto l = std::get_if<const nodes::Listener*>(&typeInfo.type);
                if (l) {
                    if (kind == VariableKind::PlatformMethodReturnValue) {
                        if ((**l).isLambda) {
                            addTypeNotAllowedError(errorContext, typeRef,
                                "Cannot return lambda-listener from listener method");
                            isValid = false;
                        }
                    } else if (kind == VariableKind::PlatformMethodParameter ||
                            kind == VariableKind::NativeMethodReturnValue) {
                        if (!(**l).isStrongRef) {
                            addTypeNotAllowedError(errorContext, typeRef,
                                "Cannot pass weakly held listener to platform code"
                                " - use strong_ref platform interface instead");
                            isValid = false;
                        }
                    } else if (kind == VariableKind::Field || kind == VariableKind::OptionsField) {
                        addTypeNotAllowedError(errorContext, typeRef,
                            "Listener cannot be a struct / variant field");
                        isValid = false;
                    }
                }
            }
        } catch (const utils::Exception& /* e */) {
            addTypeNotFoundError(errorContext, *typeRef.name);
            isValid = false;
        }
    }

    return isValid;
}

void Validator::checkDuplicateFieldTypes(const nodes::Variant& v)
{
    std::unordered_set<std::string> visitedFieldTypes;
    for (const auto& field : v.fields) {
        std::string typeString = typeRefToString(field.typeRef);
        if (!visitedFieldTypes.insert(typeString).second) {
            addError("", "Duplicate field type found '" + typeString + "'");
        }
    }
}

void Validator::checkDoc(
    const std::string& errorContext,
    const std::optional<nodes::Doc>& doc,
    const std::optional<nodes::Function>& function)
{
    if (doc) {
        checkDocBlock(errorContext, doc->description);

        if (function) {
            for (const auto& pair : doc->parameters) {
                bool parameterFound = false;
                for (const auto& parameter : function->parameters) {
                    if (parameter.name == pair.first) {
                        parameterFound = true;
                    }
                }
                if (!parameterFound) {
                    addError(errorContext + " documentation",
                        "Parameter '" + pair.first + "' not found");
                }

                checkDocBlock(errorContext, pair.second);
            }

            checkDocBlock(errorContext, doc->result);
        } else {
            if (!doc->parameters.empty()) {
                addError(errorContext, "Non-function doc has @param section");
            }
            if (!doc->result.format.empty()) {
                addError(
                    errorContext, "Non-function doc has @return section");
            }
        }
    }
}

void Validator::checkDocBlock(
    const std::string& errorContext,
    const nodes::DocBlock& block)
{
    for (const auto& link : block.links) {
        if (link.scope.isEmpty()) { // refers to current object
            checkDocLink(errorContext, link,
                state_->scope.idl->type(Scope(), state_->scope.scope));
        } else { // can only represent a custom type
            try {
                checkDocLink(
                    errorContext, link, state_->scope.type(link.scope));
            } catch (const utils::Exception& /* e */) {
                addTypeNotFoundError(
                    errorContext + " documentation", link.scope);
            }
        }
    }
}

namespace {

std::string docLinkMemberToString(const nodes::DocLink& link)
{
    return std::string(link.scope) + '#' + link.memberName;
}

} // namespace

/**
 * Base implementation - works for enums and variants only!
 */
template <typename Node>
void Validator::checkDocLinkMember(
    const std::string& errorContext,
    const nodes::DocLink& link,
    const TypeInfo& info)
{
    auto n = std::get_if<const Node*>(&info.type);
    if (n) {
        if (link.parameterTypeRefs) {
            addError(errorContext,
                "Doc refers to a method inside enum or variant '" +
                docLinkMemberToString(link) + "(...)'");
            return;
        }

        for (const auto& f : (**n).fields) {
            if (f.name == link.memberName) {
                return;
            }
        }
        addError(errorContext,
            "Doc refers to enum's or variant's inexistent field '" +
            docLinkMemberToString(link) + "'");
    }
}

template <>
void Validator::checkDocLinkMember<nodes::Interface>(
    const std::string& errorContext,
    const nodes::DocLink& link,
    const TypeInfo& typeInfo)
{
    auto i = std::get_if<const nodes::Interface*>(&typeInfo.type);
    if (i) {
        if (!link.parameterTypeRefs) {
            addError(errorContext,
                "Doc refers to a field inside interface '" +
                docLinkMemberToString(link) + "'");
            return;
        }

        for (const auto& typeRef : *link.parameterTypeRefs) {
            if (!checkTypeRef(errorContext + " documentation",
                    VariableKind::NativeMethodParameter, typeRef,
                    typeRef.isConst)) {
                return;
            }
        }

        if (!findMethod(state_->scope, link.memberName, *link.parameterTypeRefs,
                { typeInfo.idl, typeInfo.scope.original() }, **i)) {
            addError(errorContext,
                "Doc refers to interface's nonexistent method '" +
                docLinkMemberToString(link) + "(...)'");
        }
    }
}

template <>
void Validator::checkDocLinkMember<nodes::Listener>(
    const std::string& errorContext,
    const nodes::DocLink& link,
    const TypeInfo& typeInfo)
{
    auto l = std::get_if<const nodes::Listener*>(&typeInfo.type);
    if (l) {
        if (link.parameterTypeRefs) {
            if ((**l).isLambda) {
                addError(errorContext,
                    "Doc refers to a method of lambda listener '" +
                    docLinkMemberToString(link) + "(...)'");
                return;
            }
        } else {
            addError(errorContext,
                "Doc refers to a field inside listener '" +
                docLinkMemberToString(link) + "'");
            return;
        }

        for (const auto& typeRef : *link.parameterTypeRefs) {
            if (!checkTypeRef(errorContext + " documentation",
                    VariableKind::PlatformMethodParameter, typeRef,
                    typeRef.isConst)) {
                return;
            }
        }

        if (!findMethod(state_->scope, link.memberName, *link.parameterTypeRefs,
                { typeInfo.idl, typeInfo.scope.original() }, **l)) {
            addError(errorContext,
                "Doc refers to listener's nonexistent method '" +
                docLinkMemberToString(link) + "(...)'");
        }
    }
}

template <>
void Validator::checkDocLinkMember<nodes::Struct>(
    const std::string& errorContext,
    const nodes::DocLink& link,
    const TypeInfo& typeInfo)
{
    auto s = std::get_if<const nodes::Struct*>(&typeInfo.type);
    if (s) {
        if (link.parameterTypeRefs) {
            addError(errorContext,
                "Doc refers to a method inside struct '" +
                docLinkMemberToString(link) + "(...)'");
            return;
        }

        bool fieldFound = false;
        (**s).nodes.lambdaTraverse(
            [&](const nodes::StructField& f)
            {
                if (f.name == link.memberName) {
                    fieldFound = true;
                }
            });
        if (!fieldFound) {
            addError(errorContext,
                "Doc refers to struct's inexistent field '" +
                docLinkMemberToString(link) + "'");
        }
    }
}

// This method must go below checkDocLinkMember(...) template specializations!
void Validator::checkDocLink(
    const std::string& errorContext,
    const nodes::DocLink& link,
    const TypeInfo& info)
{
    if (!link.memberName.empty()) {
        checkDocLinkMember<nodes::Enum>(errorContext, link, info);
        checkDocLinkMember<nodes::Interface>(errorContext, link, info);
        checkDocLinkMember<nodes::Listener>(errorContext, link, info);
        checkDocLinkMember<nodes::Struct>(errorContext, link, info);
        checkDocLinkMember<nodes::Variant>(errorContext, link, info);
    }
}

StructValidator::StructValidator(
    ValidatorsState* state,
    const nodes::Struct* s)
    : Validator(state)
    , s_(s)
{
}

void StructValidator::onVisited(const nodes::StructField& f)
{
    FullTypeRef ref(state_->scope, f.typeRef);
    bool isInternalStruct = FullTypeRef(state_->scope, s_->name).isInternal();
    bool isInternalFieldType = ref.isInternal();
    bool markedAsInternal = hasInternalDoc(f);
    bool isCollectionFieldType = !ref.subRefs().empty();
    bool canBeInternal = ref.isOptional() || f.defaultValue || isCollectionFieldType;

    if (isInternalStruct) {
        if (markedAsInternal)
            addInternalTagError(f.name, "Internal struct cannot have fields with @internal tag");
    } else {
        if (isInternalFieldType && !markedAsInternal)
            addInternalTagError(f.name, "Field with internal type has not @internal tag");

        if (markedAsInternal && !canBeInternal)
            addInternalTagError(f.name, "Field with @internal tag must: be optional or be of type vector/dict or have default value");
    }

    checkDoc(f.name, f.doc);

    if (f.typeRef.isConst) {
        addConstNotAllowedError(f.name);
    }

    if (!visitedFieldNames_.insert(f.name).second) {
        addError(f.name, "Duplicate field");
    }

    VariableKind kind = (s_->kind == nodes::StructKind::Options ? VariableKind::OptionsField : VariableKind::Field);
    checkTypeRef(f.name, kind, f.typeRef, false, f.typeRef.isOptional);

    if (f.typeRef.isOptional) {
        if (f.typeRef.id == nodes::TypeId::Vector ||
                f.typeRef.id == nodes::TypeId::Dictionary ||
                f.typeRef.id == nodes::TypeId::Any ||
                f.typeRef.id == nodes::TypeId::AnyCollection) {
            addError(f.name, "Field of '" + typeRefToString(f.typeRef) +
                "' type is optional");
        }
    }

    if (s_->kind == nodes::StructKind::Lite && ref.isBridged()) {
        addError(f.name, "Lite struct cannot have bridged fields");
    }
}

unsigned int StructValidator::numberOfFieldsVisited() const
{
    return visitedFieldNames_.size();
}

InterfaceValidator::InterfaceValidator(
    ValidatorsState* state,
    const nodes::Interface* i)
    : Validator(state)
    , interface_(i)
    , isInternal_(FullTypeRef(state_->scope, interface_->name).isInternal())
{
    if (i->isStatic) {
        if (i->doc && !isExcludeDoc(i->doc)) {
            addError("",
                "Static interface cannot have docs - add them to functions");
        }

        auto nodesCount = i->nodes.count();
        if (nodesCount == 0) {
            addError(
                "", "Static interface has no functions and no properties");
        } else {
            auto validNodesCount = i->nodes.count<nodes::Function>() +
                i->nodes.count<nodes::Property>();
            if (nodesCount > validNodesCount) {
                addError("",
                    "Static interface can only have functions and properties");
            }
        }

        i->nodes.lambdaTraverse(
            [this](const nodes::Function& f)
            {
                if (f.isConst) {
                    addError(f.name.original().first(),
                        "Const qualifier is not allowed "
                        "on static interface function");
                }
            });
    }
}

void InterfaceValidator::onVisited(const nodes::Function& f)
{
    bool allowInternalTypes = isInternal_ || hasInternalDoc(f);
    if (!allowInternalTypes && hasInternalTypes(state_->scope, f))
        addInternalTagError(f.name.original().first(), "Non internal function has internal arguments or internal return type");
    checkFunction(f, true);
}

void InterfaceValidator::onVisited(const nodes::Property& p)
{
    bool allowInternalTypes = isInternal_ || hasInternalDoc(p);
    if (!allowInternalTypes && hasInternalTypes(state_->scope, p))
        addInternalTagError(p.name, "Non internal property has internal type");
    checkProperty(p);
}

std::vector<std::string> validate(const Idl* idl)
{
    ValidatorsState state{ { idl, { } }, { } };
    idl->root.nodes.traverse(Validator(&state));
    return state.errors;
}

} // namespace yandex::maps::idl
