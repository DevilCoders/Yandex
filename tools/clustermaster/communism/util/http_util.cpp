#include "http_util.h"

void NHttpUtil::ServeSimpleStatus(IOutputStream& out, HttpCodes code, const TString& urlRoot, const TString& title, const TString& annotation) {
    out << "HTTP/1.0 " << static_cast<int>(code) << " " << HttpCodeStr(code) << "\r\n";
    out << "Connection: close\r\n";
    out << "Cache-control: no-cache, max-age=0\r\n";
    out << "Expires: Thu, 01 Jan 1970 00:00:01 GMT\r\n";

    if (code != HTTP_NO_CONTENT) {
        out << "Content-Type: text/html\r\n\r\n";

        out << "<html><head>";
        out << "<title>" << title << "</title>";
        out << "<link href=\"" << urlRoot << "style.css\" rel=\"stylesheet\" type=\"text/css\" />";
        out << "</head><body class=\"blank\">";
        out << "<h1>" << static_cast<int>(code) << " " << HttpCodeStr(code) << "</h1><div>" << annotation << "</div>";
        out << "</body></html>";
    } else {
        out << "\r\n";
    }
}

THttpRequestContext::THttpRequestContext()
    : Requester()
    , UrlString()
    , Url()
    , Query()
    , UrlRoot()
{
}

TMaybe<THttpRequestContext> THttpRequestContext::Cons(const TServerRequestData& rd, TString& requestString) {
    THttpRequestContext result;

    result.Requester = rd.RemoteAddr();

    char* ptr = requestString.begin();

    if (strnicmp(ptr, "GET ", 4) != 0) {
        return TMaybe<THttpRequestContext>();
    }

    ptr += 4;
    char* end = strchr(ptr, ' ');

    if (!end) {
        return TMaybe<THttpRequestContext>();
    }

    *end = '\0';

    result.UrlString = ptr;

    // Parse query
    char* query = strchr(ptr, '?');
    if (query) {
        *(query++) = '\0';
        TVector<TString> pairs;
        Split(query, "&", pairs);

        for (TVector<TString>::iterator i = pairs.begin(); i < pairs.end(); ++i) {
            TString name, value;

            size_t eq = i->find_first_of('=');
            if (eq == TString::npos) {
                name = *i;
            } else {
                name = i->substr(0, eq);
                value = i->substr(eq + 1);
            }

            CGIUnescape(name);
            CGIUnescape(value);

            result.Query[name] = value;
        }
    }

    TString u(ptr);
    Split(u.data(), "/", result.Url);

    // Get level of url and construct relative path properly
    TVector<TString> UrlDirs;
    Split(u.substr(0, u.rfind('/')).data(), "/", UrlDirs);

    for (size_t i = 0; i < UrlDirs.size(); ++i)
        result.UrlRoot += "../";

    return result;
}
