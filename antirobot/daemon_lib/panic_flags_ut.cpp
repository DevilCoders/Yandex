#include <library/cpp/testing/unittest/env.h>
#include <library/cpp/testing/unittest/registar.h>

#include "panic_flags.h"

#include <antirobot/daemon_lib/ut/utils.h>

#include <util/folder/tempdir.h>
#include <util/stream/file.h>

using namespace NAntiRobot;

Y_UNIT_TEST_SUITE(TestPanicFlags) {
    Y_UNIT_TEST(IsPanicMayBanForSet) {
        auto mayBanFlags = MakeAtomicShared<TServiceParamHolder<TAtomic>>();
        TPanicFlags flags(mayBanFlags,
                          MakeAtomicShared<TServiceParamHolder<TAtomic>>(),
                          MakeAtomicShared<TServiceParamHolder<TAtomic>>(),
                          MakeAtomicShared<TServiceParamHolder<TAtomic>>(),
                          MakeAtomicShared<TServiceParamHolder<TAtomic>>(),
                          MakeAtomicShared<TServiceParamHolder<TAtomic>>());

        UNIT_ASSERT(!flags.IsPanicMayBanForSet(EHostType::HOST_AUTO));

        AtomicSet(mayBanFlags->GetByService(EHostType::HOST_AUTO), 1);

        UNIT_ASSERT(flags.IsPanicMayBanForSet(EHostType::HOST_AUTO));
    }

    Y_UNIT_TEST(IsPanicCanShowCaptchaSet) {
        auto canShowCaptchaFlags = MakeAtomicShared<TServiceParamHolder<TAtomic>>();
        TPanicFlags flags(MakeAtomicShared<TServiceParamHolder<TAtomic>>(),
                         canShowCaptchaFlags,
                         MakeAtomicShared<TServiceParamHolder<TAtomic>>(),
                         MakeAtomicShared<TServiceParamHolder<TAtomic>>(),
                         MakeAtomicShared<TServiceParamHolder<TAtomic>>(),
                         MakeAtomicShared<TServiceParamHolder<TAtomic>>());

        UNIT_ASSERT(!flags.IsPanicCanShowCaptchaSet(EHostType::HOST_AUTO));

        AtomicSet(canShowCaptchaFlags->GetByService(EHostType::HOST_AVIA), 1);

        UNIT_ASSERT(flags.IsPanicCanShowCaptchaSet(EHostType::HOST_AVIA));
    }

    Y_UNIT_TEST(PanicMainOnly) {
        auto panicMainOnlyByService = MakeAtomicShared<TServiceParamHolder<TAtomic>>();
        TPanicFlags flags(MakeAtomicShared<TServiceParamHolder<TAtomic>>(),
                         MakeAtomicShared<TServiceParamHolder<TAtomic>>(),
                         panicMainOnlyByService,
                         MakeAtomicShared<TServiceParamHolder<TAtomic>>(),
                         MakeAtomicShared<TServiceParamHolder<TAtomic>>(),
                         MakeAtomicShared<TServiceParamHolder<TAtomic>>());

        UNIT_ASSERT(!flags.IsPanicMayBanForSet(EHostType::HOST_WEB, EReqType::REQ_MAIN));
        UNIT_ASSERT(!flags.IsPanicCanShowCaptchaSet(EHostType::HOST_WEB, EReqType::REQ_MAIN));

        AtomicSet(panicMainOnlyByService->GetByService(EHostType::HOST_WEB), 1);

        UNIT_ASSERT(flags.IsPanicMayBanForSet(EHostType::HOST_WEB, EReqType::REQ_MAIN));
        UNIT_ASSERT(flags.IsPanicCanShowCaptchaSet(EHostType::HOST_WEB, EReqType::REQ_MAIN));
    }

    Y_UNIT_TEST(PanicDzensearchOnly) {
        auto panicDzensearchByService = MakeAtomicShared<TServiceParamHolder<TAtomic>>();
        TPanicFlags flags(MakeAtomicShared<TServiceParamHolder<TAtomic>>(),
                         MakeAtomicShared<TServiceParamHolder<TAtomic>>(),
                         MakeAtomicShared<TServiceParamHolder<TAtomic>>(),
                         panicDzensearchByService,
                         MakeAtomicShared<TServiceParamHolder<TAtomic>>(),
                         MakeAtomicShared<TServiceParamHolder<TAtomic>>());

        UNIT_ASSERT(!flags.IsPanicMayBanForSet(EHostType::HOST_WEB, EReqType::REQ_DZENSEARCH));
        UNIT_ASSERT(!flags.IsPanicCanShowCaptchaSet(EHostType::HOST_WEB, EReqType::REQ_DZENSEARCH));

        AtomicSet(panicDzensearchByService->GetByService(EHostType::HOST_WEB), 1);

        UNIT_ASSERT(flags.IsPanicMayBanForSet(EHostType::HOST_WEB, EReqType::REQ_DZENSEARCH));
        UNIT_ASSERT(flags.IsPanicCanShowCaptchaSet(EHostType::HOST_WEB, EReqType::REQ_DZENSEARCH));
    }

    Y_UNIT_TEST(Reassignment) {
        auto mayBanFlags = MakeAtomicShared<TServiceParamHolder<TAtomic>>();
        auto canShowCaptchaFlags = MakeAtomicShared<TServiceParamHolder<TAtomic>>();
        auto panicMainOnlyByService = MakeAtomicShared<TServiceParamHolder<TAtomic>>();
        TPanicFlags flags = TPanicFlags::CreateFake();

        UNIT_ASSERT(!flags.IsPanicMayBanForSet(EHostType::HOST_AUTO));
        UNIT_ASSERT(!flags.IsPanicCanShowCaptchaSet(EHostType::HOST_REALTY));
        UNIT_ASSERT(!flags.IsPanicMayBanForSet(EHostType::HOST_AFISHA, EReqType::REQ_MAIN));
        UNIT_ASSERT(!flags.IsPanicCanShowCaptchaSet(EHostType::HOST_AFISHA, EReqType::REQ_MAIN));

        flags = TPanicFlags(mayBanFlags,
                            canShowCaptchaFlags,
                            panicMainOnlyByService,
                            MakeAtomicShared<TServiceParamHolder<TAtomic>>(),
                            MakeAtomicShared<TServiceParamHolder<TAtomic>>(),
                            MakeAtomicShared<TServiceParamHolder<TAtomic>>());
        AtomicSet(mayBanFlags->GetByService(EHostType::HOST_AUTO), 1);
        AtomicSet(canShowCaptchaFlags->GetByService(EHostType::HOST_REALTY), 1);
        AtomicSet(panicMainOnlyByService->GetByService(EHostType::HOST_AFISHA), 1);

        UNIT_ASSERT(flags.IsPanicMayBanForSet(EHostType::HOST_AUTO));
        UNIT_ASSERT(flags.IsPanicCanShowCaptchaSet(EHostType::HOST_REALTY));
        UNIT_ASSERT(flags.IsPanicMayBanForSet(EHostType::HOST_AFISHA, EReqType::REQ_MAIN));
        UNIT_ASSERT(flags.IsPanicCanShowCaptchaSet(EHostType::HOST_AFISHA, EReqType::REQ_MAIN));
    }
}
