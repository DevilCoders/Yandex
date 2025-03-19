#pragma once

#include <library/cpp/testing/unittest/registar.h>

#include "seinfo.h"


#define COMPARE_TINFOS(a, b)                                                  \
            UNIT_ASSERT_EQUAL(a.Name, b.Name);                                \
            UNIT_ASSERT_EQUAL(a.Type, b.Type);                                \
            UNIT_ASSERT_EQUAL(a.Flags, b.Flags);                              \
            if (a.Query != b.Query) {                                         \
                Cerr << b.Query << Endl;                                      \
            };                                                                \
            UNIT_ASSERT_EQUAL(a.Query, b.Query);                              \
            UNIT_ASSERT_EQUAL(a.Platform, b.Platform);                        \
            UNIT_ASSERT_EQUAL(a.BadQuery, b.BadQuery);                        \
            UNIT_ASSERT_EQUAL(a.GetPageSize(), b.GetPageSize());              \
            UNIT_ASSERT_EQUAL(a.GetPageNum(), b.GetPageNum());                \
            UNIT_ASSERT_EQUAL(a.GetPageStart(), b.GetPageStart());

#define KS_TEST_URL(url, res)                                                 \
    {                                                                         \
        const TInfo resTmp = NSe::GetSeInfo(url, true, false);                \
        COMPARE_TINFOS(res, resTmp);                                          \
    }

#define KS_TEST_URL_CASELESS(url, res)                                        \
    {                                                                         \
        const TInfo resTmp = NSe::GetSeInfo(url, true, true);                 \
        COMPARE_TINFOS(res, resTmp);                                          \
    }

#define KS_TEST_URL_WITH_KEYS(url, res, keys)                                 \
    {                                                                         \
        const TInfo resTmp = NSe::GetSeInfo(url, true, false, true, keys);    \
        COMPARE_TINFOS(res, resTmp);                                          \
    }

static inline TString GenerateYandex2_0SearchUrl(const TString& domain, const TString& handler, const TString& service, const TString& platform = "", const TString& version = "") {
    Y_ASSERT(domain.find(';') == TString::npos);
    Y_ASSERT(handler.find(';') == TString::npos);
    Y_ASSERT(service.find(';') == TString::npos);
    Y_ASSERT(platform.find(';') == TString::npos);
    Y_ASSERT(version.find(';') == TString::npos);

    return TString("http://clck.yandex.ru/jsredir?state=AiuY0DBWFJ4ePaEse6rgeBRtUKZphe4qTCt8_XtUCJZyp2TIE2d9hRrUldrzyU8azezVxla4iOR3KNXdlRdFxPoHYzW-1AY4MmYp2Kyld_uKGf2dzlFcJxh3jlEebyRWmiPaddbSoWxKksa8CFw5Q7L5XBV-pJQw_OSRuhTUbQbh0EhPOVjlQjC4eEenuPZdQROY0AZPcPnQhQ21a80XEEwsXz7EMmMB95-FCQPd6_GghUMgQYz7VA&text=зимнее пальто купить в москве&") + "from=" + domain + ";" + handler + ";" + service + ";" + platform + ";" + version +
           +"&data=UlNrNmk5WktYejR0eWJFYk1LdmtxdTh1QklDQlg2a0t2YTZWOW1NSnQzZHhwM3NNdERXSDNWTkJWcTcya2dXdUhRTTZLbWlxUDlmTWMwN3VEQWhHcXc&b64e=2&sign=a92d765d05b846114334c4b82c021383&keyno=0";
}
