#pragma once

#include <kernel/common_server/certs/config.h>

namespace NCS {
    class TCertsManger {
    private:
        const TCertsMangerConfig Config;

    public:
        TCertsManger(const TCertsMangerConfig& config)
            : Config(config)
        {
        }

        virtual ~TCertsManger() = default;

        virtual bool Start() noexcept;
        virtual bool Stop() noexcept;

        void SetNehCaFile(const TString& path) const;
        void SetNehCaDir(const TString& path) const;
    };
}
