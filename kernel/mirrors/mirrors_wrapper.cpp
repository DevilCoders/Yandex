#include "mirrors_wrapper.h"

TAutoPtr<IMirrors> IMirrors::Load(const TString& fileName, int precharge) {
    if (fileName.EndsWith(".hash")) {
        return TAutoPtr<IMirrors>(new TMirrorsHashed(fileName, precharge));
    } else {
        return TAutoPtr<IMirrors>(new TMirrors(fileName));
    }
}

