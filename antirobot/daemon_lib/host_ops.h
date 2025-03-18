#pragma once
#include <util/generic/strbuf.h>
#include <library/cpp/uilangdetect/bytld.h>

namespace NAntiRobot {

TStringBuf GetYandexDomainFromHost(const TStringBuf& host);

TStringBuf GetCookiesDomainFromHost(const TStringBuf& host);

/// @return top level domain, i.e. for "video.yandex.com.tr" it returns "tr"
TStringBuf GetTldFromHost(TStringBuf host);

ELanguage GetLangFromHost(TStringBuf host);

}
