#pragma once

#include "threat_level.h"

#include <antirobot/lib/stats_writer.h>

#include <util/generic/ptr.h>

namespace NAntiRobot {

    class TUserBase {
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
        TUserBase(const TString& dbFileName);
        ~TUserBase();

        TValue Get(const TUid& uid);
        void DeleteOldUsers();
        void PrintStats(TStatsWriter& out) const;
        void PrintMemStats(IOutputStream& out) const;
        void DumpCache(IOutputStream& out) const;
    private:
        THolder<TImpl> Impl;
    };
}

