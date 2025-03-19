#pragma once

#include "qd_saas_key.h"

#include <kernel/querydata/client/qd_client_utils.h>
#include <kernel/querydata/idl/querydata_common.pb.h>

#include <library/cpp/infected_masks/infected_masks.h>

#include <util/generic/string.h>
#include <util/generic/strbuf.h>

namespace NQueryDataSaaS {

    // Добавляет http://, приводит хост к нижнему регистру
    TStringBuf NormalizeDocUrl(TStringBuf url, TString& buffer);

    TString NormalizeDocUrl(const TString& url);

    TStringBuf GetUrlMaskFromCategUrl(TStringBuf categUrl);

    TStringBuf GetOwnerFromNormalizedDocUrl(TStringBuf url);

    void GetMasksFromNormalizedDocUrl(TVector<NInfectedMasks::TLiteMask>&, TStringBuf url);

    void GetMasksFromNormalizedDocUrl(TVector<TString>&, TStringBuf url);

    TStringBuf GetZDocIdFromNormalizedUrl(TStringBuf url, TString& buffer);

    TString GetZDocIdFromNormalizedUrl(TStringBuf url);

    TStringBuf GetNoCgiFromNormalizedUrl(const TStringBuf url);

    TString GetNoCgiFromNormalizedUrlStr(const TString& url);

    TStringBuf GetStructKeyFromPair(TStringBuf nSpace, TStringBuf key, TString& buffer);

    TString GetStructKeyFromPair(TStringBuf nSpace, TStringBuf key);

    bool ParseStructKey(TStringBuf& nSpace, TStringBuf& key, TStringBuf sk);

    ESaaSSubkeyType GetSaaSKeyType(NQueryData::EKeyType kt);

    NQueryData::EKeyType GetQueryDataKeyType(ESaaSSubkeyType kt);

    TSaaSKeyType GetSaaSKeyType(const NQueryData::TKeyTypeVec&);

}
