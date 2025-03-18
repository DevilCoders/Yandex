#include "http.h"

#include "batch.h"
#include "http_static.h"

#include <library/cpp/svnversion/svnversion.h>

TRequestsHandle TSolverHttpRequest::GetRequests() {
    return reinterpret_cast<TSolverHttpServer*>(HttpServ())->GetRequests();
}

const TString& TSolverHttpRequest::GetHostName() {
    return reinterpret_cast<TSolverHttpServer*>(HttpServ())->GetHostName();
}

void TSolverHttpRequest::ServeSimpleStatus(IOutputStream& out, HttpCodes code, TMaybe<THttpRequestContext> contextMaybe) {
    NHttpUtil::ServeSimpleStatus(out, code, contextMaybe.Defined() ? contextMaybe->UrlRoot : "",
            "Solver: error " + ToString<int>(static_cast<int>(code)));
}

void TSolverHttpRequest::ReplyUnsafe(IOutputStream& out) {
    TMaybe<THttpRequestContext> contextMaybe = THttpRequestContext::Cons(RD, RequestString);

    if (contextMaybe.Empty()) {
        ServeSimpleStatus(out, HTTP_BAD_REQUEST, contextMaybe);
    }

    const THttpRequestContext::TUrlList& url = contextMaybe->Url;
    if (url.size() == 0 || url[0] == "summary") {
        ServeSummary(out, *contextMaybe);
    } else if (url[0] == "consumed_text") {
        ServeConsumedText(out);
    } else if (url[0] == "requested_text") {
        ServeRequestedText(out);
    } else if (url[0] == "granted_text") {
        ServeGrantedText(out);
    } else if (url[0] == "style.css") {
        ServeStaticData(out, "style.css", "text/css");
    } else {
        ServeSimpleStatus(out, HTTP_NOT_FOUND, contextMaybe);
    }
}

void TSolverHttpRequest::ServeSummary(IOutputStream& out, const THttpRequestContext& context) {
    out << "HTTP/1.0 200 OK\r\n";
    out << "Connection: close\r\n";
    out << "Content-Type: text/html\r\n";
    out << "Cache-control: no-cache, max-age=0\r\n";
    out << "Expires: Thu, 01 Jan 1970 00:00:01 GMT\r\n\r\n";

    out << "<html><head><title>Solver at host <" << GetHostName() << ">:</title>";
    out << "<link href=\"" << context.UrlRoot << "style.css\" rel=\"stylesheet\" type=\"text/css\"/>";
    out << "</head><body>";
    out << "<h1>Links</h1>";
    out << "<a href='consumed_text'>Consumed</a>, <a href='requested_text'>Requested</a>, <a href='granted_text'>Granted</a>";
    out << "<h1>Version</h1>";
    out << "<pre>" << GetProgramSvnVersion() << "</pre>";
    out << "</body></html>";
}

void TSolverHttpRequest::ServeConsumedText(IOutputStream& out) {
    out << "HTTP/1.0 200 OK\r\n";
    out << "Connection: close\r\n";
    out << "Content-Type: text/plain\r\n";
    out << "Cache-control: no-cache, max-age=0\r\n";
    out << "Expires: Thu, 01 Jan 1970 00:00:01 GMT\r\n\r\n";

    TGuard<TSpinLock> guard(*GetRequests()->Lock);
    TClaims consumed;
    bool empty = true;
    for (TRequests::const_iterator i = GetRequests()->Granted->Begin(); i != GetRequests()->Granted->End(); ++i) {
        consumed.Combine(*i);
        empty = false;
    }
    if (!empty) {
        consumed.OutResMultiline(out);
    } else {
        out << "nothing";
    }
}

void TSolverHttpRequest::ServeStaticData(IOutputStream& out, const char* name, const char* mime) {
    out << "HTTP/1.0 200 OK\r\n";
    out << "Connection: close\r\n";
    out << "Content-Type: " << mime << "\r\n";
    out << "Cache-control: no-cache, max-age=0\r\n";
    out << "Expires: Thu, 01 Jan 1970 00:00:01 GMT\r\n\r\n";

    GetSolverStaticReader().GetStaticFile(TString("/") + name, out);
}

void TSolverHttpRequest::ServeRequestsText(IOutputStream& out, ERequestsType type) {
    out << "HTTP/1.0 200 OK\r\n";
    out << "Connection: close\r\n";
    out << "Content-Type: text/plain\r\n";
    out << "Cache-control: no-cache, max-age=0\r\n";
    out << "Expires: Thu, 01 Jan 1970 00:00:01 GMT\r\n\r\n";

    TGuard<TSpinLock> guard(*GetRequests()->Lock);
    const TRequests& requests = (type == RT_REQUESTED ? *GetRequests()->Requested : *GetRequests()->Granted);
    bool empty = true;
    for (TRequests::const_iterator i = requests.Begin(); i != requests.End(); ++i) {
        const TRequest& request = *i;
        out << (request.Batch->Id.empty() ? "-" : request.Batch->Id) << "\t";
        out << request.Label << "\t";
        request.OutRes(out, true);
        out << "\t";
        request.OutShared(out, true);
        out << "\n";
        empty = false;
    }
    if (empty) {
        out << "empty";
    }
}

void TSolverHttpRequest::ServeRequestedText(IOutputStream& out) {
    ServeRequestsText(out, RT_REQUESTED);
}

void TSolverHttpRequest::ServeGrantedText(IOutputStream& out) {
    ServeRequestsText(out, RT_GRANTED);
}
