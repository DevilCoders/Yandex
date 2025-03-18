#include "parsejson.h"
#include "parseitem.h"

#include <library/cpp/json/json_value.h>

#include <util/generic/strbuf.h>
#include <util/string/cast.h>

namespace NSnippets {
    bool TSerpNode::HasClass(TStringBuf x) const {
        TStringBuf s = Class;
        TStringBuf c;
        while (s.NextTok(' ', c)) {
            if (c == x) {
                return true;
            }
        }
        return false;
    }
    static bool ParseBounds(const NJson::TJsonValue& v, TBounds& res) {
        if (!v.IsMap()) {
            return false;
        }
        for (auto i : v.GetMap()) {
            const auto& key = i.first;
            const auto& val = i.second;
            int x;
            if (!val.IsInteger()) {
                if (val.IsDouble()) {
                    x = int(val.GetDouble());
                } else {
                    Cerr << "bad value for key: " << key << Endl;
                    return false;
                }
            } else {
                x = static_cast<int>(val.GetInteger());
            }
            if (key == "top") {
                res.Top = x;
            } else if (key == "left") {
                res.Left = x;
            } else if (key == "width") {
                res.Width = x;
            } else if (key == "height") {
                res.Height = x;
            } else if (key == "bottom") {
                res.Bottom = x;
            } else if (key == "right") {
                res.Right = x;
            } else {
                Cerr << "bad key: " << key << Endl;
                return false;
            }
        }
        return true;
    }
    static bool ParseSerpNode(const NJson::TJsonValue& v, TSerpNode& res) {
        if (!v.IsMap()) {
            return false;
        }
        for (auto i : v.GetMap()) {
            const auto& key = i.first;
            const auto& val = i.second;
            if (key == "class" && val.IsString()) {
                res.Class = val.GetString();
            } else if (key == "href" && val.IsString()) {
                res.Href = val.GetString();
            } else if (key == "src" && val.IsString()) {
                res.Src = val.GetString();
            } else if (key == "tag" && val.IsString()) {
                res.Src = val.GetString();
            } else if (key == "name" && val.IsString()) {
                res.Name = val.GetString();
            } else if (key == "value" && val.IsString()) {
                res.Value = val.GetString();
            } else if (key == "value" && val.IsInteger()) {
                res.Value = ToString(val.GetString());
            } else if (key == "innerText" && val.IsString()) {
                res.InnerText = val.GetString();
            } else if (key == "bounds") {
                if (!ParseBounds(val, res.Bounds)) {
                    return false;
                }
            } else if (key == "domChildren" && val.IsArray()) {
                for (const auto& j : val.GetArray()) {
                    TSerpNodePtr n(new TSerpNode());
                    if (!ParseSerpNode(j, *n)) {
                        return false;
                    }
                    res.Children.push_back(n);
                }
            } else {
                Cerr << "unknown key: " << key << Endl;
                return false;
            }
        }
        return true;
    }
    bool ParseSerp(const NJson::TJsonValue& v, TSerp& res) {
        if (!v.IsMap()) {
            return false;
        }
        for (auto i : v.GetMap()) {
            const auto& key = i.first;
            const auto& val = i.second;
            if (key == "pngImage" && val.IsString()) {
                res.PngImage = val.GetString();
            } else if (key == "layout") {
                TSerpNodePtr l(new TSerpNode());
                if (!ParseSerpNode(val, *l.Get())) {
                    return false;
                }
                res.Layout = l;
            } else {
                Cerr << "unknown top-level key: " << key << Endl;
                return false;
            }
        }
        res.Query = FindQuery(res.Layout);
        res.NumFound = FindNumFound(res.Layout);
        return true;
    }
}
