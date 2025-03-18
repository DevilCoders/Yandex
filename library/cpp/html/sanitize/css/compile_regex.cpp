#include <util/generic/yexception.h>
#include <util/generic/string.h>
#include <library/cpp/regex/pcre/regexp.h>

namespace NCssConfig {
    /** Compile regular expression. Input string is in form of /reg-expr-string/x.., where x is x,s,m or i
 *
 *  Resulting expression has UTF8 option by default. Other options (i,m,x,s) should be given after '/'
 *
 * @param reg_str    a string from config ( '/bla-bla.+/ix'
 * @return           allocated via new instance of regexp. It should be deleted by caller
 */
    TRegExMatch* CompileRegexp(const TString& reg_str) {
        if (reg_str.empty())
            return nullptr;

        TString::const_iterator it = reg_str.begin();
        TString::const_iterator it_end = reg_str.end();

        int re_flags = REG_NOSUB | PCRE_UTF8;

        TString re_str;

        if (*it == '/') {
            while (it != it_end && *++it != '/')
                re_str += *it;

            if (it == it_end)
                ythrow yexception() << "Bad regexp format: must end with '/'";

            it++;

            while (it != it_end) {
                switch (*it) {
                    case 's':
                        re_flags |= PCRE_DOTALL;
                        break;
                    case 'x':
                        re_flags |= REG_EXTENDED;
                        break;
                    case 'm':
                        re_flags |= PCRE_MULTILINE;
                        break;
                    case 'i':
                        re_flags |= PCRE_CASELESS;
                        break;
                }
                it++;
            }
        } else
            re_str = reg_str;

        return new TRegExMatch(re_str.c_str(), re_flags);
    }

} //namespace NCssConfig
