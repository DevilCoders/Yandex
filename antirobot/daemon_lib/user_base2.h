#pragma once

#include "threat_level.h"

#include <util/generic/ptr.h>

namespace NAntiRobot {

    class TUserBase2 {
        class TImpl;
    public:
        class TValue {
            friend class TImpl;
        private:
            TImpl& UserBase;
            TUid Uid;
            mutable bool Ownership;

            TValue(TImpl& userBase, const TUid& uid);
            TValue& operator=(const TValue& copy);
        public:
            TValue(const TValue& copy);

            ~TValue();

            TThreatLevel* operator->() const;
        };

    public:
        TUserBase2(const TString& dbFileName);
        ~TUserBase2();

        TValue Get(const TUid& uid);
        void DeleteOldUsers(size_t& compressedSize, size_t robotsSize);
        void PrintStats(IOutputStream& out) const;
        void PrintMemStats(IOutputStream& out) const;
        void DumpCache(IOutputStream& out) const;
    private:
        THolder<TImpl> Impl;
    };
}

