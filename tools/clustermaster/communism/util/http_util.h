#pragma once

#include <library/cpp/http/misc/httpcodes.h>
#include <library/cpp/http/server/http_ex.h>
#include <library/cpp/string_utils/quote/quote.h>

#include <util/generic/bt_exception.h>
#include <util/generic/maybe.h>
#include <util/string/split.h>

template<typename T>
class THttpClientRequestForHuman: public THttpClientRequestEx {
private:
    bool Reply(void*) override;
    virtual void ReplyUnsafe(IOutputStream& httpOutput) = 0;
};

namespace {

    class TCountingOutputStream: public IOutputStream {
    private:
        IOutputStream& Os;
        bool Written;
    public:
        TCountingOutputStream(IOutputStream& os)
            : Os(os)
            , Written(false)
        { }

        void DoWrite(const void* buf, size_t len) override {
            Written = true;
            Os.Write(buf, len);
        }

        bool IsWritten() {
            return Written;
        }
    };

}

template<typename T>
inline
bool THttpClientRequestForHuman<T>::Reply(void*) {
    if (!ProcessHeaders())
        return true;

    TStringStream title;
    title << RequestString << " " << RD.RemoteAddr();

    TCountingOutputStream os(Output());
    try {
        T::Log(title.Str());
        ReplyUnsafe(os);
        T::Log(title.Str() + " OK");
    } catch (...) {
        const auto currentExceptionMessage = CurrentExceptionMessage();
        T::ErrorLog(title.Str() + " FAILED: " + currentExceptionMessage);
        Cerr << currentExceptionMessage;

        if (!os.IsWritten()) {
            os << "HTTP/1.0 500 Internal Server Error\r\n"
               << "Connection: Close\r\n"
               << "\r\n"
               << "500\n"
               << "\n"
               << "Exception: " << currentExceptionMessage;
        }
    }

    return true;
}

class THttpRequestContext {
public:
    typedef THashMap<TString, TString> TQueryMap;
    typedef TVector<TString> TUrlList;

private:
    THttpRequestContext();

public:
    TString Requester;

    TString UrlString;
    TUrlList Url;

    TQueryMap Query;

    TString UrlRoot;

    static TMaybe<THttpRequestContext> Cons(const TServerRequestData& rd, TString& requestString);
};

namespace NHttpUtil {
    void ServeSimpleStatus(IOutputStream& out, HttpCodes code, const TString& urlRoot, const TString& title, const TString& annotation = "");
}

