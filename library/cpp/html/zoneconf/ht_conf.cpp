#include "ht_conf.h"
#include "findin.h"

#include <library/cpp/html/entity/htmlentity.h>
#include <library/cpp/html/spec/tags.h>
#include <library/cpp/html/face/propface.h>
#include <library/cpp/yconf/conf.h>

#include <library/cpp/containers/str_hash/str_hash.h>
#include <library/cpp/charset/ci_string.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/string/strip.h>
#include <util/string/vector.h>
#include <util/system/compat.h>
#include <util/system/defaults.h>
#include <util/system/maxlen.h>

#include <algorithm>
#include <cctype>
#include <cstdlib>

namespace {
    class HTParserConf: public TYandexConfig {
        class TMain: public TYandexConfig::Directives {
        public:
            TMain() {
            }
        };

    public:
        explicit HTParserConf(THtConfigurator* conf)
            : Conf(conf)
        {
        }

    protected:
        bool AddKeyValue(Section& sec, const char* key, const char* value) override {
            if (stricmp(sec.Name, "Zones") == 0) {
                Conf->AddZoneConf(key, value);
                return true;
            } else if (stricmp(sec.Name, "Attributes") == 0) {
                Conf->AddAttrConf(key, value);
                return true;
            }
            return TYandexConfig::AddKeyValue(sec, key, value);
        }

        bool OnBeginSection(Section& sec) override {
            bool allow = (*sec.Parent->Name == 0 || stricmp(sec.Parent->Name, "HtmlParser") == 0);
            if (allow && (*sec.Name == 0 || stricmp(sec.Name, "HtmlParser") == 0)) {
                sec.Cookie = &Main;
                return true;
            } else if (allow && stricmp(sec.Name, "Zones") == 0) {
                sec.Cookie = &Main;
                return true;
            } else if (allow && stricmp(sec.Name, "Attributes") == 0) {
                sec.Cookie = &Main;
                return true;
            }
            ReportError(sec.Name, false, "section \'%s\' not allowed here", sec.Name);
            return false;
        }

    private:
        THtConfigurator* Conf;
        TMain Main;
    };

    inline bool FreeConfEntry(THtElemConf* data) {
        delete data;
        return true;
    }

    // returns number of non-empty attributes
    size_t split(const char* source, const char* delim, TVector<TCiString>& result, bool spaces = false) {
        const char *s, *d;
        TCiString token;
        size_t res = 0;

        for (s = source; (d = strpbrk(s, delim)) != nullptr; s = d + (spaces ? strspn(d, delim) : 1)) {
            token.assign(s, 0, d - s);
            StripInPlace(CollapseInPlace(token));
            if (!token.is_null()) {
                res++;
                result.push_back(token);
            } else if (!spaces) {
                result.push_back(token);
            }
        }
        // last or the only token
        token.assign(s);
        StripInPlace(CollapseInPlace(token));
        if (!token.is_null()) {
            res++;
            result.push_back(token);
        } else if (!spaces) {
            result.push_back(token);
        }
        return res;
    }

    size_t separate_slash(const char* s, TCiString& elem_list, TCiString& yxattr) {
        TVector<TCiString> result;
        size_t res = split(s, "/", result);
        if (res < 1 || result[0].is_null())
            return 0; // must have at least first part
        elem_list = result[0];
        if (res < 2)
            return 1;
        else {
            yxattr = result[1];
            return 2;
        }
    }

    size_t separate_comma(const char* s, TVector<TCiString>& vect) {
        vect.clear();
        return split(s, ",", vect);
    }

    size_t separate_dot(const char* s, TCiString& htelem, TCiString& htattr) {
        TVector<TCiString> result;
        size_t res = split(s, ".", result);
        if (res < 1)
            return 0; // must have at least one part
        htelem = result[0];
        if (res < 2)
            return 1; // must have both parts
        htattr = result[1];
        return 2;
    }

    size_t separate_spaces(char* s, TVector<TCiString>& vect) {
        vect.clear();
        return split(s, " \t", vect, true);
    }
}

THtAttrExtConf::THtAttrExtConf()
    : HasValueMatch(false)
{
}

THtAttrExtConf::~THtAttrExtConf() {
    for (TExt::const_iterator it = Ext.begin(); it != Ext.end(); ++it)
        delete it->second;
}

THtElemConf::THtElemConf()
    : wildAttrConf(nullptr)
    , superWildAttrConf(nullptr)
{
}

THtElemConf::~THtElemConf() {
    for (TAttrs::const_iterator it = Attrs.begin(); it != Attrs.end(); ++it)
        delete it->second;
}

THtConfigurator::THtConfigurator()
    : Conf(HT_TagCount, nullptr)
    , Pool(16 * 1024)
{
    ClearConf();
}

THtConfigurator::~THtConfigurator() {
    ClearConf();
}

void THtConfigurator::ClearConf() {
    UriFilterIsOff = false;
    UnknownUtf = false;
    std::for_each(Conf.begin(), Conf.end(), FreeConfEntry);
    //Conf.clear();
    Conf.assign((size_t)HT_TagCount, nullptr);
}

void THtConfigurator::Configure(const char* filename) {
    if (filename == nullptr || *filename == 0) {
        // zero filename means empty config
        ClearConf();
        return;
    }
    static const TString optionPrefix = "option:";
    static const TString memoryPrefix = "memory:";
    TStringBuf str(filename);
    if (str.StartsWith(optionPrefix)) {
        TStringBuf option = str.substr(optionPrefix.size());
        if (option == "unknown-utf")
            UnknownUtf = true;
        else if (option == "URI_FILTER_TURN_OFF")
            UriFilterIsOff = true;
        else if (option == "default")
            LoadDefaultConf();
        else
            ythrow yexception() << "THtConfigurator::Configure -- unknown option: " << option;
    } else {
        bool isFilename = true;
        if (str.StartsWith(memoryPrefix)) {
            str = str.substr(memoryPrefix.size());
            isFilename = false;
        }
        ClearConf();
        HTParserConf conf(this);
        bool readOk = isFilename ? conf.Read(str.data()) : conf.ReadMemory(str.data());
        if (!readOk || !conf.ParseSection("HtmlParser")) {
            TString Err;
            conf.PrintErrors(Err);
            ythrow yexception() << "Configuration file \"" << conf.GetConfigPath() << "\" for HTML parser is bad:\n"
                                << Err.data();
        }
    }
}

void THtConfigurator::LoadDefaultConf() {
    ClearConf();
    // zones
    // [HTZones]
#ifndef HT_MINIMAL_CONFIG
    AddZoneConf("title", "title");
    AddZoneConf("address", "address");
    AddZoneConf("anchor", "a/link");
    AddZoneConf("anchorint", "a/linkint");

#ifdef MAIL
    AddAttrConf("_", "LITERAL/meta._");
    AddZoneConf("div", "div/class");
    AddAttrConf("class", "LITERAL,div/div.class");
#endif
#endif // HT_MINIMAL_CONFIG

    AddAttrConf("base", "URL/base.href");

#ifndef HT_MINIMAL_CONFIG
    AddAttrConf("_", "LITERAL/meta._");
    AddAttrConf("abstract", "LITERAL,doc,,ignore/meta.description");
    AddAttrConf("link", "URL,anchor/a.href");
    AddAttrConf("link", "URL,any/frame.src,iframe.src,area.href");
    AddAttrConf("link", "URL/link._");
    AddAttrConf("link", "URL,any,,,swf/param.movie,embed.src");

    AddAttrConf("linkint", "URL,anchorint,,,,local/a.href");
    AddAttrConf("linkint", "URL,any,,,,local/frame.src,iframe.src,area.href");

    AddAttrConf("robots", "LITERAL,doc,parse_meta_robots,ignore/meta.robots");
    AddAttrConf("yandex", "LITERAL,doc,parse_meta_yandex,ignore/meta.yandex,meta.yandexbot");
    AddAttrConf("refresh", "URL,doc,parse_http_refresh,ignore/meta.refresh");

    AddAttrConf("style", "URL/link.stylesheet");

    AddAttrConf("profile", "URL/head.profile");                  // .mcf
    AddAttrConf("script", "URL,any/script.src");                 // .js
    AddAttrConf("image", "URL,any/img.src");                     // .gif
    AddAttrConf("applet", "URL,any/applet.code,applet.object");  // .class
    AddAttrConf("object", "URL,any/object.data,object.classid"); // .cab or .jar

    //AddAttrConf("xxx", "LITERAL,any/__.__");
#endif // HT_MINIMAL_CONFIG
}

/*
2. синтаксис описания зон

yxzone = htelem{,htelem}
yxzone = htelem{,htelem}/yxattr

где
yxattr - атрибут, при котором зона считается зоной
*/
void THtConfigurator::AddZoneConf(const TCiString& key, const TString& value) {
    TCiString elem_list, yxattr;
    if (!separate_slash(value.data(), elem_list, yxattr))
        ythrow THtConfigError() << "zone config [" << value << "] has no element name";
    TVector<TCiString> htelem;
    if (!separate_comma(elem_list.data(), htelem))
        ythrow THtConfigError() << "zone config [" << value << "] has no element name";
    for (unsigned i = 0; i < htelem.size(); i++) {
        if (htelem[i].is_null())
            ythrow THtConfigError() << "zone config [" << value << "]: element " << i << " is empty";

        const NHtml::TTag& h = NHtml::FindTag(htelem[i].data());
        if (h == HT_any && htelem[i] != ZONE_WILD) {
            ythrow THtConfigError() << "zone config [" << value << "] has unknown element: " << htelem[i];
        }
        THtElemConf* elemConf = Conf[(size_t)h.id()];
        if (!elemConf)
            elemConf = Conf[h.id()] = new THtElemConf;

        elemConf->CondAttrZones[yxattr] = key; // zone depends on cond_attr
    }
}

/*
3. синтаксис описания атрибутов

yxattr = TYPE/htelem.htattr{,htelem.htattr}
yxattr = TYPE,yxzone/htelem.htattr{,htelem.htattr}
yxattr = TYPE,yxzone,function/htelem.htattr{,htelem.htattr}
yxattr = TYPE,yxzone,function,ignore/htelem.htattr{,htelem.htattr}
yxattr = TYPE,yxzone,function,ignore,ext{ ext}/htelem.htattr{,htelem.htattr}
yxattr = TYPE,yxzone,function,ignore,ext{ ext},local/htelem.htattr{,htelem.htattr}
*/
void THtConfigurator::AddAttrConf(const TCiString& key, const TString& value) {
    TCiString elem_attr_pairs, attr_parameters;
    if (separate_slash(value.data(), attr_parameters, elem_attr_pairs) < 2)
        ythrow THtConfigError() << "attr config [" << value << "] must have slash";

    TVector<TCiString> token;
    // what about default attr_type?
    if (!separate_comma(attr_parameters.data(), token))
        throw THtConfigError() << "attr config [" << value << "] must have at least ATTR_TYPE";

    // 1. ATTR_TYPE
    ATTR_TYPE atype;
    if (!GetAttrType(token[0].data(), atype))
        throw THtConfigError() << "attr config [" << value << "] has unknown type: " << token[0];

    // 2. zone name ("doc" and "any"/"empty" are predefined names)
    // 2. ATTR_POS
    TCiString yxzone;
    ATTR_POS apos;
    if (token.size() < 2 || token[1].is_null() || token[1] == ZONE_DOC) {
        apos = APOS_DOCUMENT;
    } else if (token[1] == ZONE_EMPTY || token[1] == ZONE_ANY) {
        apos = APOS_ZONE;
    } else {
        apos = APOS_ZONE;
        yxzone = token[1];
    }

    // 3. Parser function
    HTATTR_PARSE_FUNC func = PARSE_FUNC_UNKNOWN;
    if (token.size() >= 3 && !token[2].is_null()) {
        func = GetParseFuncByName(token[2].data());
        if (!func) // not recognized,not supported function
            throw THtConfigError() << "attr config [" << value << "] has unknown function: " << token[2];
    }

    // 4. Ignore flag
    bool ignore = false;
    if (token.size() >= 4 && token[3] == "ignore")
        ignore = true;

    // 5. Extensions modificators
    TVector<TCiString> ext;
    if (token.size() < 5 || !separate_spaces(token[4].begin(), ext))
        ext.push_back(TCiString());

    // THtAttrExtConf::Ext contains lowercase keys
    for (auto& it : ext)
        it.to_lower();

    // 6. Local-global-link flag
    bool local = false;
    if (token.size() >= 6 && token[5] == "local")
        local = true;

    // after slash
    // get all html element/attribute pairs
    separate_comma(elem_attr_pairs.data(), token);
    for (unsigned i = 0; i < token.size(); ++i) {
        TCiString htelem, htattr;
        size_t zone_plus_attr = separate_dot(token[i].begin(), htelem, htattr);
        if (zone_plus_attr < 1)
            throw THtConfigError() << "attr config [" << value << "] contains incorrect pair: " << token[i];

        htattr.to_lower(); // THtElemConf::Attrs contains lowercase keys

        if (htelem == ZONE_SUPER_WILD) {
            htelem = ZONE_WILD;
        }

        const NHtml::TTag& h = NHtml::FindTag(htelem.data(), htelem.size());
        if (h == HT_any && htelem != ZONE_WILD) {
            // conf error
            ythrow THtConfigError() << "attr config [" << value << "] contains unknown tag: " << htelem;
        }
        THtElemConf* elemConf = Conf[(size_t)h.id()];
        if (!elemConf)
            elemConf = Conf[(size_t)h.id()] = new THtElemConf;

        if (zone_plus_attr == 1) {
            elemConf->AnyAttr = key;
            // maybe also throw
            continue;
        }

        THtAttrExtConf* attrExtConf = nullptr;
        // something already in there?
        if (!FindInHash(elemConf->Attrs, htattr.c_str(), &attrExtConf)) {
            char* s = Pool.append(htattr.c_str(), htattr.size());
            elemConf->Attrs.insert(std::make_pair(s, attrExtConf = new THtAttrExtConf));
            if (!strcmp(htattr.data(), ZONE_WILD))
                elemConf->wildAttrConf = attrExtConf;
            else if (!strcmp(htattr.data(), ZONE_SUPER_WILD))
                elemConf->superWildAttrConf = attrExtConf;
        }

        for (unsigned j = 0; j < ext.size(); ++j) {
            THtAttrConf* attrConf = nullptr;
            if (!FindInHash(attrExtConf->Ext, ext[j].c_str(), &attrConf)) {
                char* s = Pool.append(ext[j].c_str(), ext[j].size());
                attrExtConf->Ext.insert(std::make_pair(s, attrConf = new THtAttrConf));
                if (ext[j].size() > 1 && ext[j][0] == '=')
                    attrExtConf->HasValueMatch = true;
            } else if (!ext[j].is_null()) {
                // when "local" modifier used ext must be same = null
                ythrow THtConfigError() << "attr config [" << value << "] has local definition with different extensions";
            }

            // fill it again without any warning
            attrConf->Type = atype;
            attrConf->Pos = apos;
            attrConf->Func = func;
            attrConf->Ignore = ignore;
            if (local) {
                attrConf->LocalName = key;
                attrConf->YxLocalCondZone = yxzone;
            } else {
                attrConf->Name = key;
                attrConf->YxCondZone = yxzone;
            }
        }
    }
}

bool THtConfigurator::CheckConfig(const char* key, const char* value) const {
    THtElemConf* elemConf = Conf[(size_t)HT_META];
    if (!elemConf)
        return false;
    THtAttrExtConf* attrExtConf = nullptr;

    if (*key == 0)
        return false;
    if (!FindInHash(elemConf->Attrs, key, &attrExtConf))
        return false;
    THtAttrConf* attrConf = nullptr;
    if (!FindInHash(attrExtConf->Ext, "", &attrConf))
        return false;
    TString pname = attrConf->Name;
    if (!(pname.size()) || pname != value)
        return false;
    return true;
}
