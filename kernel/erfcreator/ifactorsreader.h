#pragma once

#include <ysite/yandex/erf_format/erf_format.h>
#include <ysite/yandex/erf_format/host_erf_format.h>

class IFactorsReader {
public:
    virtual void GetHostErfData(const TString& host, THostErfInfo& herf) = 0;
    virtual void GetErfData(const TString& url, SDocErfInfo3& erf) = 0;
};
