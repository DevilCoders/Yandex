#include "yconf.h"

#include <library/cpp/config/config.h>
#include <library/cpp/yconf/conf.h>
#include <library/cpp/scheme/scheme.h>

#include <library/cpp/string_utils/relaxed_escaper/relaxed_escaper.h>

using namespace NConfig;

namespace NConfig {
    const TStringBuf JSON_ATTRIBUTES = "Attributes";
    const TStringBuf JSON_DIRECTIVES = "Directives";
    const TStringBuf JSON_SECTIONS = "Sections";
}

namespace {
    struct TDir : TYandexConfig::Directives {
        TDir()
            : Directives(false)
        {
        }
    };

    struct TAnyYConf : TYandexConfig {
        bool OnBeginSection(Section& s) override {
            if (!s.Cookie) {
                s.Cookie = new TDir;
                s.Owner = true;
            }
            return true;
        }
    };
}

template <typename TMapType>
static void PrintMap(TStringBuf name, const TMapType& map, IOutputStream& out, bool& next) {
    if (map.empty())
        return;

    if (next)
        out << ",";

    NEscJ::EscapeJ<true, true>(name, out);
    out << ":{";
    for (typename TMapType::const_iterator it = map.begin(); it != map.end(); ++it) {
        if (it != map.begin())
            out << ",";
        TString key = it->first;
        NEscJ::EscapeJ<true, true>(to_lower(key), out);
        out << ":";
        NEscJ::EscapeJ<true, true>(it->second, out);
    }
    out << "}";

    next = true;
}

static void PrintConfigToJson(const TYandexConfig::Section* s, IOutputStream& out) {
    if (!s)
        return;

    typedef TVector<const TYandexConfig::Section*> TSectsVec;
    typedef TMap<TString, TSectsVec> TSects;
    TSects sects;

    for (const TYandexConfig::Section* c = s->Child; c; c = c->Next) {
        TSectsVec& v = sects[to_lower(TString(c->Name))];
        v.push_back(c);
    }

    const TYandexConfig::Directives& dirs = s->GetDirectives();
    const TYandexConfig::SectionAttrs& attrs = s->Attrs;

    bool next = false;

    out << "{";

    PrintMap(JSON_ATTRIBUTES, attrs, out, next);
    PrintMap(JSON_DIRECTIVES, dirs, out, next);

    if (!sects.empty()) {
        if (next)
            out << ",";
        out << "\"" << JSON_SECTIONS << "\":{";

        for (TSects::const_iterator it = sects.begin(); it != sects.end(); ++it) {
            if (it != sects.begin())
                out << ",";

            NEscJ::EscapeJ<true, true>(it->first, out);
            out << ":[";

            const TSectsVec& vec = it->second;
            for (TSectsVec::const_iterator sit = vec.begin(); sit != vec.end(); ++sit) {
                if (sit != vec.begin())
                    out << ",";
                PrintConfigToJson(*sit, out);
            }
            out << "]";
        }
        out << "}";
    }
    out << "}";
}

TConfig NConfig::ParseRawYConf(IInputStream& in) {
    const TString data = in.ReadAll();

    TAnyYConf c;

    if (!c.ParseMemory(data.data())) {
        TString errors;
        c.PrintErrors(errors);
        ythrow TConfigParseError() << "could not parse yandex cfg " << data << " because errors " << errors;
    }

    TStringStream s;
    PrintConfigToJson(c.GetRootSection(), s);

    return TConfig::ReadJson(s.Str());
}

static const NSc::TValue& EnsureDict(const NSc::TValue& dict) {
    if (!dict.IsDict() && !dict.IsNull())
        ythrow TConfigError() << "dict expected, " << dict.ToJson(true) << " found";
    return dict;
}

static const NSc::TValue& EnsureArray(const NSc::TValue& array) {
    if (!array.IsArray() && !array.IsNull())
        ythrow TConfigError() << "array expected, " << array.ToJson(true) << " found";
    return array;
}

static void PrintSectionInternals(IOutputStream& out, TString offset, const NSc::TValue& section) {
    const NSc::TValue& directives = EnsureDict(section.Get(JSON_DIRECTIVES));
    const NSc::TValue& sections = EnsureDict(section.Get(JSON_SECTIONS));

    for (TStringBuf dirname : directives.DictKeys(true)) {
        out << offset << dirname << " " << directives.Get(dirname).GetString() << "\n";
    }

    for (TStringBuf sect : sections.DictKeys(true)) {
        const NSc::TArray& subsects = EnsureArray(sections.Get(sect));
        for (ui32 i = 0, sz = subsects.size(); i < sz; ++i) {
            const NSc::TValue& subsect = EnsureDict(subsects[i]);
            const NSc::TValue& attrs = EnsureDict(subsect.Get(JSON_ATTRIBUTES));

            out << offset << "<" << sect;
            for (TStringBuf attr : attrs.DictKeys(true)) {
                out << " " << attr << "=\"" << attrs.Get(attr).GetString() << "\"";
            }
            out << ">\n";

            PrintSectionInternals(out, offset + "  ", subsect);
            out << offset << "</" << sect << ">\n";
        }
    }
}

void NConfig::Json2RawYConf(IOutputStream& out, TStringBuf json) {
    PrintSectionInternals(out, TString(), NSc::TValue::FromJsonThrow(json));
}

void NConfig::DumpYandexCfg(const TConfig& cfg, IOutputStream& out) {
    TStringStream s;

    cfg.ToJson(s);
    Json2RawYConf(out, s.Str());
}

TConfig NConfig::FromYandexCfg(IInputStream& in, const TGlobals& g) {
    auto pin = CreatePreprocessor(g, in);

    return ParseRawYConf(*pin);
}

TConfig NConfig::ReadYandexCfg(TStringBuf in, const TGlobals& g) {
    TMemoryInput mi(in.data(), in.size());

    return FromYandexCfg(mi, g);
}
