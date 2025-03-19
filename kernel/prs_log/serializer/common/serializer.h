#pragma once

#include <kernel/prs_log/data_types/web.h>
#include <util/generic/string.h>

namespace NPrsLog {

    class IWebPrsSerializer {
    public:
        virtual TString Serialize(const TWebData& data) const = 0;
        virtual TWebData Deserialize(const TString& raw) const = 0;

        virtual ~IWebPrsSerializer() = default;
    };

} // NPrsLog
