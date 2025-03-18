#pragma once

#include <yandex/maps/idl/idl.h>
#include <yandex/maps/idl/full_type_ref.h>
#include <yandex/maps/idl/nodes/function.h>
#include <yandex/maps/idl/nodes/interface.h>
#include <yandex/maps/idl/nodes/listener.h>
#include <yandex/maps/idl/nodes/type_ref.h>
#include <yandex/maps/idl/scope.h>
#include <yandex/maps/idl/type_info.h>

#include <string>
#include <vector>

namespace yandex {
namespace maps {
namespace idl {

/**
 * Returns true if given function reference matches given function - refers to
 * it.
 *
 * @param functionRefScope - scope where you found the signature - scope of
 *                           the function _reference_. For function
 *                           signatures inside the documentation it's the
 *                           scope of the item that is being documented
 * @param functionScope - scope of the function itself
 */
bool isMatchingFunctionRef(
    const FullScope& functionRefScope,
    const std::string& functionRefName,
    const std::vector<nodes::TypeRef>& parameterTypeRefs,
    const FullScope& functionScope,
    const nodes::Function& function);

/**
 * Searches for an interface method. For more info on parameters see docs for
 * isMatchingFunctionRef(...).
 */
const nodes::Function* findMethod(
    const FullScope& functionRefScope,
    const std::string& functionRefName,
    const std::vector<nodes::TypeRef>& functionRefParameterTypeRefs,
    const FullScope& interfaceScope,
    const nodes::Interface& i);

/**
 * Searches for a listener method. For more info on parameters see docs for
 * isMatchingFunctionRef(...).
 */
const nodes::Function* findMethod(
    const FullScope& functionRefScope,
    const std::string& functionRefName,
    const std::vector<nodes::TypeRef>& functionRefParameterTypeRefs,
    const FullScope& listenerScope,
    const nodes::Listener& l);

/**
 * Searches for a function inside type with given info. Returns nullptr if
 * type is not an interface or a listener, or when function couldn't be found.
 * For more info on parameters see docs for isMatchingFunctionRef(...).
 */
const nodes::Function* findFunction(
    const FullScope& functionRefScope,
    const std::string& functionRefName,
    const std::vector<nodes::TypeRef>& functionRefParameterTypeRefs,
    const TypeInfo& typeInfo);

} // namespace idl
} // namespace maps
} // namespace yandex
