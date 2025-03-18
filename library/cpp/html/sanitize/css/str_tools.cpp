#include <util/generic/string.h>
#include <util/stream/output.h>
#include <util/string/printf.h>

#include "str_tools.h"

namespace NCssSanit {
    inline bool IsDangerousControlCode(ui64 c) {
        return c < 32 || c == 127;
    }

    inline bool IsForbiddenUrlSymbol(ui64 c) {
        switch (c) {
            case ';':
            case '\'':
            case '"':
            case ')':
            case '(':
                return true;
            default:
                return false;
        }
    }

    static TString SkipDangerousEntity(TString::const_iterator& it, const TString::const_iterator& end, EResolveMode mode);
    static TString TranslateEscape(TString::const_iterator& it, const TString::const_iterator& end);

    TString ResolveEscapes(const TString& str, EResolveMode mode) {
        TString res;

        res.reserve(str.size() * 2);

        TString::const_iterator it = str.begin();
        TString::const_iterator it_end = str.end();

        while (it != it_end) {
            switch (*it) {
                case '\\':
                    it++;
                    res += TranslateEscape(it, it_end);
                    break;
                case '&':
                    res += SkipDangerousEntity(it, it_end, mode);
                    break;
                default:
                    if (!IsDangerousControlCode(*it))
                        res += *it;
                    it++;
            }
        }
        return res;
    }

    TString TranslateEscape(TString::const_iterator& it, const TString::const_iterator& end) {
        TString res;
        res.reserve(4);

        while (it != end) {
            switch (*it) {
                default:
                    res += *it++;
                    return res;
            }
        }
        return res;
    }

    TString SkipDangerousEntity(TString::const_iterator& it, const TString::const_iterator& end, EResolveMode mode) {
        it++;
        if (it == end || *it != '#')
            return TString(1, '&');
        it++;

        int base = 10;
        TString result = "&#";

        if (*it == 'x' || *it == 'X') {
            base = 16;
            it++;
            result += 'x';
        }

        ui64 value = 0;
        while (it != end) {
            int c = tolower(*it);
            if (c >= '0' && c <= '9')
                value = value * base + (c - '0');
            else if (c >= 'a' && *it <= 'f')
                value = (value * base) + (c - 'a') + 10;
            else
                break;

            result += *it;
            it++;
        }

        if (it != end && *it == ';') {
            it++;
            result += ';';
        }

        const int MAX_UI64_DECIMAL_REPRESENTATION = 19;

        if (mode == ERM_URL && IsForbiddenUrlSymbol(value))
            return TString();

        if (result.size() > MAX_UI64_DECIMAL_REPRESENTATION + 3 || IsDangerousControlCode(value))
            return TString();

        return result;
    }

    TString UrlEncode(const TString& str) {
        const char hexchars[] = "0123456789ABCDEF";
        TString res;
        for (int x = 0, y = 0, len = str.length(); len--; x++, y++) {
            res += (unsigned char)str[x];
            if (res[y] == ' ')
                //res[y] = '+';
                res.replace(y, 1, 1, '+');
            else if (!((res[y] >= '0' && res[y] <= '9') || (res[y] >= 'A' && res[y] <= 'Z') || (res[y] >= 'a' && res[y] <= 'z') || res[y] == '-' || res[y] == '.' || res[y] == '_')) {
                // Allow only alphanumeric chars and '_', '-', '.'; escape the rest
                //res[y] = '%';
                res.replace(y, 1, 1, '%');
                res += hexchars[(unsigned char)str[x] >> 4];
                ++y;
                res += hexchars[(unsigned char)str[x] & 0x0F];
                ++y;
            }
        }
        return res;
    }

    TString UrlGetScheme(const TString& str) {
        size_t pos = str.find("://");
        if (pos != TString::npos) {
            return str.substr(0, pos);
        } else
            return TString();
    }

    inline bool IsSpace(TString::const_iterator& it) {
        switch (*it) {
            case ' ':
            case '\t':
            case '\n':
            case '\r':
                return true;
            default:
                return false;
        }
    }

    inline void SkipSpace(TString::const_iterator& it, const TString::const_iterator& it_end) {
        while (it != it_end && IsSpace(it))
            it++;
    }

    void GetStringList(const TString& class_list, TStrokaList& str_list) {
        TString::const_iterator it = class_list.begin();
        TString::const_iterator it_end = class_list.end();

        TString str;

        SkipSpace(it, it_end);

        for (; it != it_end;) {
            if (IsSpace(it)) {
                str_list.push_back(str);
                str = "";
                SkipSpace(it, it_end);
            } else {
                str += *it;
                it++;
            }
        }

        if (!str.empty()) {
            str_list.push_back(str);
        }
    }

    TString EscapeForbiddenUrlSymbols(const TString& str) {
        TString res;
        for (auto it : str) {
            if (IsForbiddenUrlSymbol(it)) {
                res += '%';
                res += Sprintf("%02d", it);
            } else
                res += it;
        }
        return res;
    }

} //namespace NCssSanit
