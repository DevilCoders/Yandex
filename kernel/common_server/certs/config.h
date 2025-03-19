#pragma once

#include <kernel/common_server/util/accessor.h>
#include <library/cpp/yconf/conf.h>
#include <util/folder/path.h>

namespace NCS {
    class TCertsMangerConfig {
    private:
        CSA_READONLY_DEF(TFsPath, CAFile);
        CSA_READONLY_DEF(TFsPath, CAPath);

    public:
        virtual void Init(const TYandexConfig::Section* section);
        virtual void ToString(IOutputStream& os) const;
    };
}
