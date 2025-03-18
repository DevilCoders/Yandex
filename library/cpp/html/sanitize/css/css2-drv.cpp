#include <stdio.h>

//#include <fstream>
//#include <sstream>
#include <util/stream/file.h>
#include <util/generic/yexception.h>

#include "css2-drv.h"
#include "css2-lexer.h"
#include "str_tools.h"
#include <library/cpp/html/sanitize/common/url_processor/url_processor.h>
#include <library/cpp/html/sanitize/common/filter_entities/filter_entities.h>

#ifdef WITH_DEBUG_OUT
#define DEBUG_OUT(a) OutDebug a
#else
#define DEBUG_OUT(a)
#endif

namespace NCssSanit {
    class TCSS2Driver;
}
extern int yydebug;
extern int yyparse_css2(NCssSanit::TCSS2Driver& driver); // see #define yyparse in css2_y.y

namespace NCssSanit {
    using namespace NCssConfig;

    static bool NeedQuote(const TString& str);

    TCSS2Driver::TCSS2Driver()
        : OutStream(nullptr)
        , Policy(nullptr)
        , ProcessAllUrls(false)
        , ErrorCount(0)
#ifdef WITH_DEBUG_OUT
        , TraceScanning(false)
        , TraceParsing(false)
        , CanDebugOut(false)
#endif
    {
        TagName = nullptr;
        UrlProcessor = nullptr;
        FilterEntities = nullptr;
    }

    bool TCSS2Driver::ParseStream(IInputStream& in, const TString& sname, bool inline_css) {
        StreamName = sname;

        TCSS2Lexer lex(in, inline_css, Gc);

        this->Lexer = &lex;

        ParseErrorStream.clear();

#ifdef WITH_DEBUG_OUT
        lex.SetDebug(TraceScanning);
        if (TraceParsing)
            SetEnv("YYDEBUG", "1");
#endif

        Gc.DoGc();
        ErrorCount = 0;
        return yyparse_css2(*this) == 0;
    }

    bool TCSS2Driver::ParseFile(const TString& filename) {
        try {
            TIFStream in(filename.c_str());
            return ParseStream(in, filename, false);
        } catch (...) {
            ParseErrorStream << "Could not open file: " << filename;
            return false;
        }
    }

    bool TCSS2Driver::ParseString(const TString& input, const TString& sname) {
        TStringInput iss(input);
        return ParseStream(iss, sname, false);
    }

    bool TCSS2Driver::ParseInlineStyle(const TString& input, const TString& sname) {
        TStringInput iss(input);
        SetTagName("style");
        return ParseStream(iss, sname, true);
    }

    void TCSS2Driver::Error(const TString& m) {
        ErrorCount++;
        Cerr << Lexer->lineno() << ": " << m << Endl;
    }

    void TCSS2Driver::Error(const char* msg) {
        ErrorCount++;
        ParseErrorStream << StreamName << ':' << Lexer->lineno() << ' ' << msg << '\n';
    }

    TString TCSS2Driver::ExprToString(const TString& prop_name, const TContext& context) {
        if (FilterEntities && !FilterEntities->IsAcceptedAttrCSS(TagName, prop_name.c_str()))
            return "";

        TString res;

        char buf[64];
        switch (context.ValueType) {
            case VT_NUMBER:
                if (prop_name.empty() || Policy->PassProperty(prop_name, context)) {
                    sprintf(buf, "%g", context.PropValueDouble);
                    res = buf;
                    res += context.Unit;
                    DEBUG_OUT(("<<Property passed:", prop_name, ':', res, ">>\n"));
                } else {
#ifdef WITH_DEBUG_OUT
                    sprintf(buf, "%g", context.PropValueDouble);
                    res = buf;
                    res += context.Unit;
                    DEBUG_OUT(("<<Property denied:", prop_name, ':', res, ">>\n"));
                    res = "";
#endif
                }
                break;

            case VT_HASHVALUE:
                if (prop_name.empty() || Policy->PassProperty(prop_name, context)) {
                    res = context.PropValue;
                    DEBUG_OUT(("<<Property passed:", prop_name, ':', context.PropValue, ">>\n"));
                } else {
                    DEBUG_OUT(("<<Property denied:", prop_name, ':', context.PropValue, ">>\n"));
                }
                break;
            case VT_STRING:
                if (prop_name.empty() || Policy->PassProperty(prop_name, context)) {
                    if (NeedQuote(context.PropValue)) {
                        res = '"';
                        res += ResolveEscapes(context.PropValue) + '"';
                    } else {
                        res = ResolveEscapes(context.PropValue);
                    }
                    DEBUG_OUT(("<<Property passed: ", prop_name, ':', context.PropValue, ">>\n"));
                } else {
                    DEBUG_OUT(("<<Property denied: ", prop_name, ':', context.PropValue, ">>\n"));
                }

                break;

            case VT_URI:
                if (prop_name.empty() || Policy->PassProperty(prop_name, context)) {
                    //THttpURL url;
                    //TParserState state = url.Parse(context.PropValue);
                    //TString url_scheme = url.Get(FieldScheme);

                    TString url_scheme = UrlGetScheme(context.PropValue);
                    bool wantUrl = false;
                    bool nonLocalUrl = true;

                    if (url_scheme.empty()) {
                        wantUrl = true;
                        nonLocalUrl = false;
                    } else {
                        if (Policy->PassScheme(url_scheme)) {
                            wantUrl = true;
                        } else {
                            DEBUG_OUT(("<<Url scheme denied:", context.PropValue, ">>\n"));
                        }
                    }

                    if (wantUrl) {
                        TString url;
                        if ((nonLocalUrl || ProcessAllUrls) && TagName && UrlProcessor && UrlProcessor->IsAcceptedAttr(TagName, prop_name.c_str())) {
                            url = UrlProcessor->Process(context.PropValue.c_str());
                        } else {
                            url = ResolveEscapes(context.PropValue, ERM_URL);
                        }
                        if (nonLocalUrl) {
                            res = "url(" + url + ")";
                        } else {
                            res = "url('" + url + "')";
                        }
                        DEBUG_OUT(("<<Property passed:", prop_name, ':', res, ">>\n"));
                    }
                }
#ifdef WITH_DEBUG_OUT
                else {
                    res = TString("url(") + context.PropValue + ')';
                    DEBUG_OUT(("<<Property denied:", prop_name, ':', res, ">>\n"));
                    res.clear();
                }
#endif
                break;
            case VT_FUNCTION:
                if (Policy->PassProperty(prop_name, context)) {
                    res = context.PropValue;
                    res += '(';
                    res += ExprListToString(TString(), context.FuncArgs);
                    res += ')';
                    DEBUG_OUT(("<<Property passed:", prop_name, ':', res, ">>\n"));
                } else {
                    DEBUG_OUT(("<<Property denied:", prop_name, ':', res, ">>\n"));
                }
                break;

            case VT_EXPRESSION:
                if (Policy->PassExpression()) {
                    res = context.PropValue;

                    DEBUG_OUT(("<<Property passed:", prop_name, ':', context.PropValue, ">>\n"));
                } else {
                    DEBUG_OUT(("<<Property denied:", prop_name, ':', context.PropValue, ">>\n"));
                }

                break;
            case VT_SEPARATOR:
                res = context.PropValue;
                break;

            default:
                break;
        }
        return res;
    }

    TString TCSS2Driver::ExprListToString(const TString& prop_name, const TContextList& list) {
        if (!Policy)
            return TString();

        TString res;
        for (unsigned i = 0; i < list.size(); i++) {
            const TContext& context = list[i];

            if (context.ValueType == VT_SEPARATOR) {
                if (!res.empty())
                    res += context.PropValue;
            } else
                res += ExprToString(prop_name, list[i]);
        }
        return res;
    }

    TString TCSS2Driver::PropertyToString(const TString& prop_name, const TContextList& list) {
        return ExprListToString(prop_name, list);
    }

    TString TCSS2Driver::HandleImportUrl(const TString& url) {
        TString res = url;
        if (ProcessAllUrls && UrlProcessor && UrlProcessor->IsAcceptedAttr("style", "@import")) {
            res = UrlProcessor->Process(res.data());
        }
        if (Policy) {
            res = Policy->HandleImport(res);
        }
        return res;
    }

    void TCSS2Driver::SetOutStream(IOutputStream& ostream) {
        OutStream = &ostream;
    }

    const TSanitPolicy& TCSS2Driver::GetPolicy() const {
        if (Policy)
            return *Policy;
        else
            ythrow yexception() << "Policy is not defined.";
    }

    TString TCSS2Driver::MakeSelectorsAll(const TStrokaList& list) {
        TStrokaList::const_iterator it = list.begin(),
                                    it_end = list.end();

        TString res;
        if (it != it_end) {
            res = Policy ? Policy->CorrectSelector(*it) : *it;

            for (++it; it != it_end; it++)
                res += "," + (Policy ? Policy->CorrectSelector(*it) : *it);
        }

        return res;
    }

    TString TCSS2Driver::MakeSelectorsDefault(const TStrokaList& list) {
        TString res;

        if (!Policy)
            return res;

        TStrokaList::const_iterator it = list.begin(),
                                    it_end = list.end();

        for (; it != it_end; it++) {
            if (Policy->PassSelector(*it)) {
                res = Policy->CorrectSelector(*it);
                it++;
                break;
            }
        }

        for (; it != it_end; it++) {
            if (Policy->PassSelector(*it))
                res += "," + Policy->CorrectSelector(*it);
        }

        return res;
    }

    /** Checks for a space inside
 */
    bool NeedQuote(const TString& str) {
        for (unsigned i = 0; i < str.size(); i++) {
            switch (str[i]) {
                case ' ':
                case ';':
                case ',':
                    return true;
                default:
                    break;
            }
        }

        return false;
    }

    TString LTrim(const TString& str) {
        TString res;

        res.reserve(str.size());

        TString::const_iterator it = str.begin();
        TString::const_iterator end = str.end();

        while (it != end && isspace(*it))
            it++;

        return TString(it, end);
    }

}
