#pragma once

#include <yandex/maps/idl/full_type_ref.h>
#include <yandex/maps/idl/idl.h>
#include <yandex/maps/idl/nodes/struct.h>
#include <yandex/maps/idl/scope.h>

#include <ctemplate/template.h>

#include <string>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace common {

/**
 * Some objects have built-in "null" state. If they cannot be null due to
 * some program logic (e.g. because of function preconditions), we add
 * corresponding assert section in given dictionary for all target languages.
 *
 * @return - true if assert section was indeed generated.
 */
void addNullAsserts(
    ctemplate::TemplateDictionary* dict,
    const FullTypeRef& typeRef,
    const std::string& name);

/**
 * Set name space.
 */
void setNamespace(
    const Scope& nameSpace,
    ctemplate::TemplateDictionary* dict);

/**
 * Set namespace in the way appropriate to given target language.
 */
void setNamespace(
    const std::string& targetLang,
    ctemplate::TemplateDictionary* dict,
    const Idl* idl);

} // namespace common
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
