#pragma once
#include <util/generic/ptr.h>

namespace NCS {
    namespace NStorage {
        class TAbstractLock {
        public:
            using TPtr = TAtomicSharedPtr<TAbstractLock>;

        public:
            virtual ~TAbstractLock() {}

            virtual bool IsLockTaken() const;

            virtual bool IsLocked() const = 0;
        };

        class TFakeLock: public TAbstractLock {
        private:
            bool IsLockedFlag = true;
        public:

            TFakeLock(const TString& /*key*/, const bool isLocked = true)
                : IsLockedFlag(isLocked) {

            }

            virtual bool IsLockTaken() const override {
                return IsLockedFlag;
            }

            virtual bool IsLocked() const override {
                return IsLockedFlag;
            }
        };

    }
}
