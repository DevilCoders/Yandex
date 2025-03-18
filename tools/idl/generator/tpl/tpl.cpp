#include "tpl/tpl.h"

#include "tpl/cache.h"

#include <yandex/maps/idl/utils/common.h>
#include <yandex/maps/idl/utils/exception.h>

#include <ctemplate/template_modifiers.h>

#include <functional>
#include <vector>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace tpl {

namespace {

class Modifier : public ctemplate::TemplateModifier {
public:
    explicit Modifier(
        std::string name, // don't prepend 'x-' - it's done in here!
        std::function<std::string(std::string value)> op)
        : op_(op)
    {
        ctemplate::AddModifier(("x-" + name).c_str(), this);
    }

private:
    std::function<std::string(std::string value)> op_;

    void Modify(
        const char* in,
        std::size_t inlen,
        const ctemplate::PerExpandData* /* per_expand_data */,
        ctemplate::ExpandEmitter* outbuf,
        const std::string& /* arg */) const override
    {
        outbuf->Emit(op_(std::string(in, inlen)));
    }
};

const TplCache* tplCache()
{
    // Modifiers must be installed before TplCache is created!
    static const Modifier CAP("cap", &utils::capitalizeWord);
    static const Modifier UNCAP("uncap", &utils::unCapitalizeWord);
    static const Modifier STRIP_SHARED_PTR(
        "strip-shared-ptr", &utils::stripSharedPtr);
    static const Modifier STRIP_SCOPE("strip-scope", &utils::stripScope);
    static const Modifier TO_UPPER("to-upper", &utils::toUpperCase);

    static const TplCache TPL_CACHE;
    return &TPL_CACHE;
}

std::vector<std::string> g_Dirs;

} // namespace

DirGuard::DirGuard(const std::string& dir)
{
    g_Dirs.push_back(dir);
}

DirGuard::~DirGuard()
{
    g_Dirs.pop_back();
}

ctemplate::TemplateDictionary* addInclude(
    ctemplate::TemplateDictionary* parentDict,
    const std::string& tag,
    const std::string& baseTplPath)
{
    auto tplPath = baseTplPath;
    if (tplPath.front() != '/') {
        tplPath = '/' + g_Dirs.back() + '/' + tplPath;
    }

    auto dict = parentDict->AddIncludeDictionary(tag);
    dict->SetFilename(tplCache()->tplPath(tplPath));
    return dict;
}

ctemplate::TemplateDictionary* addSectionedInclude(
    ctemplate::TemplateDictionary* parentDict,
    const std::string& tag,
    const std::string& baseTplPath)
{
    return addInclude(parentDict->AddSectionDictionary(tag), tag, baseTplPath);
}

std::string expand(
    const std::string& baseTplPath,
    const ctemplate::TemplateDictionary* dict)
{
    auto tplPath = '/' + g_Dirs.back() + '/' + baseTplPath;

    std::string outputText;
    REQUIRE(
        tplCache()->ExpandNoLoad(
            tplCache()->tplPath(tplPath),
            ctemplate::DO_NOT_STRIP, dict, nullptr, &outputText),
        "Error expanding template " + tplPath);

    return outputText;
}

} // namespace tpl
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
