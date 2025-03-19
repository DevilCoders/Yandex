#pragma once

#include <kernel/prs_log/serializer/common/serializer.h>

namespace NPrsLog {

    class TUniformBoundSerializer : public IWebPrsSerializer {
    public:
        virtual TString Serialize(const TWebData& data) const override;
        virtual TWebData Deserialize(const TString& raw) const override;
    };

} // NPrsLog
