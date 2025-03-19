#pragma once

#include <kernel/common_server/processors/common/handler.h>

namespace NCS {

    namespace NHandlers {
        class TServiceConstantsProcessor: public TCommonSystemHandler<TServiceConstantsProcessor> {
        private:
            using TBase = TCommonSystemHandler<TServiceConstantsProcessor>;
        public:
            using TBase::TBase;

            enum class EConstantTraits {
                RTBackground = 1 /* "rt_background" */,
                Notifiers = 1 << 1 /* "notifiers" */,
                ServiceCustoms = 1 << 2 /* "service_customs" */,
            };


            static TString GetTypeName() {
                return "service-constant";
            }

            virtual void ProcessRequestWithPermissions(TJsonReport::TGuard& g, TSystemUserPermissions::TPtr permissions) override;
        };

        class TServiceHandlersProcessor: public TCommonSystemHandler<TServiceHandlersProcessor> {
        private:
            using TBase = TCommonSystemHandler<TServiceHandlersProcessor>;
        public:
            using TBase::TBase;

            static TString GetTypeName() {
                return "service-handlers";
            }

            virtual void ProcessRequestWithPermissions(TJsonReport::TGuard& g, TSystemUserPermissions::TPtr permissions) override;
        };
    }
}
