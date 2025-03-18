#pragma once
#include <util/stream/output.h>

#include "service_param_holder.h"

#include <library/cpp/deprecated/atomic/atomic.h>

namespace NAntiRobot {
    class TPanicFlags {
    public:
        using TPanicFlagsPtr = TAtomicSharedPtr<TServiceParamHolder<TAtomic>>;

        TPanicFlagsPtr PanicMayBanByService;
        TPanicFlagsPtr PanicCanShowCaptchaByService;
        TPanicFlagsPtr PanicMainOnlyByService;
        TPanicFlagsPtr PanicDzensearchByService;
        TPanicFlagsPtr PanicPreviewIdentTypeEnabledByService;
        TPanicFlagsPtr PanicCbbByService;

        TPanicFlags(TPanicFlagsPtr panicMayBanByService,
                    TPanicFlagsPtr panicCanShowCaptchaByService,
                    TPanicFlagsPtr panicMainOnlyByService,
                    TPanicFlagsPtr panicDzensearchByService,
                    TPanicFlagsPtr panicPreviewIdentTypeEnabledByService,
                    TPanicFlagsPtr panicCbbByService)
            : PanicMayBanByService(std::move(panicMayBanByService))
            , PanicCanShowCaptchaByService(std::move(panicCanShowCaptchaByService))
            , PanicMainOnlyByService(std::move(panicMainOnlyByService))
            , PanicDzensearchByService(std::move(panicDzensearchByService))
            , PanicPreviewIdentTypeEnabledByService(std::move(panicPreviewIdentTypeEnabledByService))
            , PanicCbbByService(std::move(panicCbbByService))
        {
        }

        TPanicFlags()
            : PanicMayBanByService(MakeAtomicShared<TServiceParamHolder<TAtomic>>())
            , PanicCanShowCaptchaByService(MakeAtomicShared<TServiceParamHolder<TAtomic>>())
            , PanicMainOnlyByService(MakeAtomicShared<TServiceParamHolder<TAtomic>>())
            , PanicDzensearchByService(MakeAtomicShared<TServiceParamHolder<TAtomic>>())
            , PanicPreviewIdentTypeEnabledByService(MakeAtomicShared<TServiceParamHolder<TAtomic>>())
            , PanicCbbByService(MakeAtomicShared<TServiceParamHolder<TAtomic>>())
        {
        }

        TPanicFlags(const TPanicFlags&) = default;

        TPanicFlags& operator=(const TPanicFlags& copy) = default;

        static TPanicFlags CreateFake() {
            return TPanicFlags{};
        }

        bool IsPanicMayBanForSet(EHostType service, EReqType reqType = EReqType::REQ_OTHER) const {
            return AtomicGet(PanicMayBanByService->GetByService(service)) ||
                   (reqType == EReqType::REQ_MAIN && AtomicGet(PanicMainOnlyByService->GetByService(service))) ||
                   (reqType == EReqType::REQ_DZENSEARCH && AtomicGet(PanicDzensearchByService->GetByService(service)));
        }

        bool IsPanicCanShowCaptchaSet(EHostType service, EReqType reqType = EReqType::REQ_OTHER) const {
            return AtomicGet(PanicCanShowCaptchaByService->GetByService(service)) ||
                   (reqType == EReqType::REQ_MAIN && AtomicGet(PanicMainOnlyByService->GetByService(service))) ||
                   (reqType == EReqType::REQ_DZENSEARCH && AtomicGet(PanicDzensearchByService->GetByService(service)));
        }

        bool IsPanicPreviewIdentTypeEnabledSet(EHostType service) const {
            return AtomicGet(PanicPreviewIdentTypeEnabledByService->GetByService(service));
        }

        bool IsPanicCbbSet(EHostType service) const {
            return AtomicGet(PanicCbbByService->GetByService(service));
        }
    };
}
