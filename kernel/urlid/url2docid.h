#pragma once

#include "doc_handle.h"
#include "urlhash.h"
#include "urlid.h"

#include <kernel/multilanguage_hosts/multilanguage_hosts.h>

#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <library/cpp/string_utils/url/url.h>

namespace NUrl2DocId {

    TStringBuf GetUrlWithLowerCaseHost(TStringBuf mainUrl, TString& str);

    TString GetUrlWithLowerCaseHost(TStringBuf mainUrl);

}

TString& Url2DocIdRaw(TStringBuf url, TString& hashbuf);

TDocHandle Url2DocIdRaw(TStringBuf url);
TDocHandle Url2DocId(TStringBuf url);

// языко-зависимые докиды, используются только в базовом поиске веба
TString& Url2DocId(TStringBuf url, TStringBuf lang, TString& langbuf, TString& hashbuf);
TString Url2DocIdSimple(TStringBuf url, TStringBuf lang);

// языко-независимые докиды, используются в кликовых серверах для обратной совместимости
TString& Url2DocId(TStringBuf url, TString& urlHashBuf, bool host2lower = false);
TString& Url2DocId(TStringBuf url, TString& urlHashBuf, TString& hostCaseBuf, bool host2lower = false);
TString Url2DocIdSimple(TStringBuf url);
TString Url2DocIdSimpleHost2Lower(TStringBuf url);
