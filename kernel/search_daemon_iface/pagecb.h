#pragma once

/******************************************************************************
 *         This file is a part of Yandex.Software ver.3.6                     *
 *         Copyright (c) 1996-2009 OOO "Yandex". All rights reserved.         *
 *         Call software@yandex-team.ru for support.                          *
 ******************************************************************************/

#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <clocale>

#include "cntintrf.h"

class TCgiParameters;

class IReportCallback {
public:
    virtual ~IReportCallback() = default;
    // if returned true, not need use standard error serialization (set error code/msg + call IPageCallback::Report())
    virtual bool OnException(int /*yxErrorCode*/, const TString& /*errText*/) const { return false; }
    // if return not zero skip standard method success response serialization (call IPageCallback::Report())
    virtual int BeforeReport(ISearchContext* ctx) const = 0;

    virtual void AfterReport(ISearchContext*) const {};
    virtual void AfterReport(IPassageContext*) const {};
};

struct TSearchContextOptions {
    /// Create new or use existing statistics frame from IRequestContext
    bool CreateNewStatFrame = false;

    /// Create new or use existing log frame from IRequestContext
    bool CreateNewLogFrame = false;

    /// Create new or use existing TRequestParams
    bool CreateNewRequestParams = false;
};

class IRequestContext : public IReportCallback {
public:
    ~IRequestContext() override = default;
    virtual ISearchContext *CreateContext(const TSearchContextOptions& options = {}) = 0;
    virtual IInfoContext* CreateInfoContext() = 0;
    virtual void DestroyContext(ISearchContext *sc) = 0;
    virtual ISearchContext* CopyContext(ISearchContext *sc) = 0;
    virtual void ResetContext(ISearchContext **sc) = 0;
    virtual int Search(ISearchContext *sc) = 0;
    virtual int LoadOrSearch(ISearchContext **sc) = 0;
    virtual int Print(const void *buffer, size_t size) = 0;
    virtual int Print(const google::protobuf::Message& msg) = 0;

    virtual void HeaderOut(TStringBuf header, TStringBuf value) = 0;
    virtual void EnableKeepAlive(bool) = 0;
    /*
     * XXX: Be careful! EndOfHeader() must be called before document output,
     * usually at the end of HTTPHead()
     */
    virtual void EndOfHeader() = 0;

    // HTTP Request Data
    virtual int         FormFieldCount(const char* key) = 0;
    virtual const char *FormField(const char* key, int num) = 0;
    virtual const char *Environment(const char* key) = 0;
    virtual const TString *HeaderIn(TStringBuf key) = 0;
    TStringBuf HeaderInOrEmpty(TStringBuf key) {
        const auto* ptr = HeaderIn(key);
        return ptr ? TStringBuf{*ptr} : TStringBuf{};
    }
    virtual void SetHttpStatus(int status) = 0;

    virtual bool GetNeedSafeLogAdd() const = 0;
    virtual void SetNeedSafeLogAdd(bool val) = 0;

    //
    virtual IRemoteRequester* CreateRequester(ISearchContext* context = nullptr) = 0;
};

inline void YxPrint(ISearchContext* sc, const char* str) {
    if (str)
        sc->ReqEnv()->PrintS(str, strlen(str));
}

inline void YxPrint(ISearchContext* sc, const char* str, size_t length) {
    sc->ReqEnv()->PrintS(str, length);
}

inline void YxPrint(ISearchContext* sc, TStringBuf str) {
    if (str)
        sc->ReqEnv()->PrintS(str.data(), str.size());
}

inline void YxPrint(ISearchContext* sc, long number) {
    char buf[21];
    char* t = buf + 20;
    bool minus = false;
    if (number < 0) {
        minus = true;
        number = -number;
    }
    do {
        *t-- = char(int(number % 10) + 48);
        number /= 10;
    } while (number);
    if (minus)
        *t-- = '-';
    sc->ReqEnv()->PrintS(t+1, size_t(buf + 20 - t));
}
