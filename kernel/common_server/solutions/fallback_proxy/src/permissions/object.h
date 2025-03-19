#pragma once
#include <kernel/common_server/roles/actions/common.h>

namespace NCS {
    namespace NFallbackProxy {
        class TUserPermissions: public TSystemUserPermissions {
        private:
            using TBase = TSystemUserPermissions;

        public:
            using TBase::TBase;
        };
    }
}
