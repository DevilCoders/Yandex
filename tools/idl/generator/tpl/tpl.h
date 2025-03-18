#pragma once

#include <yandex/maps/idl/env.h>

#include <ctemplate/template.h>

#include <string>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace tpl {

/**
 * Pushes given .tpl directory onto .tpl directories stack when constructed,
 * and pops it back when destroyed.
 *
 * .tpl file directories are stored in a stack-like fashion - with top-most
 * item being the current .tpl directory. This is useful when you expand .tpl
 * to generate value that will be used when expanding another .tpl: e.g. when
 * generating documentation.
 */
class DirGuard {
public:
    DirGuard(const std::string& dir);
    ~DirGuard();
};

/**
 * Adds include dictionary.
 *
 * @param tplName - .tpl file name. If it does not start with '/', it is
 *                  considered relative and current .tpl dir is prepended.
 */
ctemplate::TemplateDictionary* addInclude(
    ctemplate::TemplateDictionary* parentDict,
    const std::string& tag,
    const std::string& tplName);

/**
 * Adds section, and then inside it, include dictionary.
 *
 * @param tplName - .tpl file name. If it does not start with '/', it is
 *                  considered relative and current .tpl dir is prepended.
 */
ctemplate::TemplateDictionary* addSectionedInclude(
    ctemplate::TemplateDictionary* parentDict,
    const std::string& tag,
    const std::string& tplName);

/**
 * Expands the .tpl with given dictionary and returns the output.
 *
 * @param tplName - name of the .tpl file to expand
 * @param dict - dictionary to use when expanding
 */
std::string expand(
    const std::string& tplName,
    const ctemplate::TemplateDictionary* dict);

} // namespace tpl
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
