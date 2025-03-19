#include "info.h"

#include <kernel/remorph/info/info.h>

namespace NRemorphAPI {

namespace NImpl {

TInfo::TInfo()
{
}

unsigned int TInfo::GetMajorVersion() const {
    return static_cast<unsigned int>(NReMorph::REMORPH_INFO.Version.Major);
}

unsigned int TInfo::GetMinorVersion() const {
    return static_cast<unsigned int>(NReMorph::REMORPH_INFO.Version.Minor);
}

unsigned int TInfo::GetPatchVersion() const {
    return static_cast<unsigned int>(NReMorph::REMORPH_INFO.Version.Patch);
}

} // NImpl

} // NRemorphAPI

extern "C"
NRemorphAPI::IInfo* GetRemorphInfo() {
    return new NRemorphAPI::NImpl::TInfo();
}
