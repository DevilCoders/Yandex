#pragma once

#include <util/generic/string.h>
#include <util/generic/vector.h>

const TVector<TString> RTYMetaCgiParamsToBase = {
    "au",
    "bu",
    "gafacets",
    "facets",
    "nofacet",
    "forcefacet",
    "facetprefix",
    "qs_req",
    "sums",
    "borders",
    "sgkps",
    "component",
    "rty_hits_count",
    "rty_hits_detail",
    "delay",
    "key_name",
    "normal_kv_report",
    "sleep",
    "comp_search"
};

const TVector<TString> RTYMetaCgiParamsToBackend = {
    "ag0",
    "raId",
    "uuid",
    "strictsyntax",
    "keepjoins",
    "queryId",
    "service",
    "meta_search",
    "metaservice",
    "instant"
};

const TVector<TString> RTYMetaCgiParamsToService = {
    "wizextra",
    "restrict",
    "template",
    "msp",
    "rwr",
    "sp_meta_search"
};

class TRTYMetaCgiParams {
private:
    TVector<TString> ToBase;
    TVector<TString> ToBackend;
    TVector<TString> ToService;
public:
    TRTYMetaCgiParams()
        : ToBase(RTYMetaCgiParamsToBase)
        , ToBackend(RTYMetaCgiParamsToBackend)
        , ToService(RTYMetaCgiParamsToService)
    {
        ToBackend.insert(ToBackend.end(), ToBase.begin(), ToBase.end());
        ToService.insert(ToService.end(), ToBackend.begin(), ToBackend.end());
    }

    static const TVector<TString>& GetParamsToBase() {
        return Singleton<TRTYMetaCgiParams>()->ToBase;
    }
    static const TVector<TString>& GetParamsToBackend() {
        return Singleton<TRTYMetaCgiParams>()->ToBackend;
    }
    static const TVector<TString>& GetParamsToService() {
        return Singleton<TRTYMetaCgiParams>()->ToService;
    }
};
