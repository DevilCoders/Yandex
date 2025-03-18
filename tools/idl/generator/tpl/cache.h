#pragma once

#include <ctemplate/template.h>
#include <string>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace tpl {

class TplCache : public ctemplate::TemplateCache {
public:
    TplCache();
    virtual ~TplCache() { }

    std::string tplPath(std::string tplPath) const
    {
        return tplPath;
    }

private:
    void add(const std::string& path, const std::string& contents)
    {
        StringToTemplateCache(path, contents, ctemplate::DO_NOT_STRIP);
    }
};

} // namespace tpl
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
