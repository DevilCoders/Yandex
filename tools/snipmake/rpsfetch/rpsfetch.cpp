#include "rpsfetch.h"

namespace NSnippets {
    struct TPoliteFetcher::TImpl {
        NNeh::IMultiRequesterRef Requester; //keeps handles alive until explicit Wait or it's own d-tor
        const int Rps;
        const int MaxInFlight;
        int InFlight = 0;
        struct TAcc {
            ui32 Timestamp = 0;
            int Count = 0;

            bool Check(int rps) {
                if (Count < rps) { //here we don't call Now() and save time but may waste latency in sparse case as old counts aren't dropped
                    return true;
                }
                const ui32 timestamp = Now().TimeT();
                if (timestamp == Timestamp) {
                    return false;
                }
                Timestamp = timestamp;
                Count = 0;
                return true;
            }
        };
        TAcc SecondAcc;
        TImpl(int rps, int maxInFlight)
          : Requester(NNeh::CreateRequester())
          , Rps(rps)
          , MaxInFlight(maxInFlight)
        {
        }

        bool Add(const NNeh::THandleRef& req, const TInstant& deadline) {
            RelaxMany(Now());
            while (!SecondAcc.Check(Rps)) { //no need to check for deadline due to Check impl
                RelaxOne(Now() + TDuration::MilliSeconds(100)); //instead of just sleeping, try to do some work
            }
            while (InFlight >= MaxInFlight && Now() < deadline) {
                RelaxOne(deadline);
            }
            if (InFlight >= MaxInFlight) {
                return false;
            }
            Requester->Add(req);
            ++InFlight;
            ++SecondAcc.Count;
            return true;
        }

        void RelaxMany(const TInstant& deadline) {
            while (RelaxOne(deadline)) {
                continue;
            }
        }

        bool RelaxOne(const TInstant& deadline) {
            NNeh::THandleRef resp;
            while (Requester->Wait(resp, deadline)) {
                --InFlight; //ref d-tor will call OnRecv
                return true;
            }
            return false;
        }
    };

    TPoliteFetcher::TPoliteFetcher(int rps, int maxInFlight)
      : Impl(new TImpl(rps, maxInFlight))
    {
    }

    TPoliteFetcher::~TPoliteFetcher() {
    }

    bool TPoliteFetcher::Add(const NNeh::THandleRef& req, const TInstant& addBefore) {
        return Impl->Add(req, addBefore);
    }

    bool TPoliteFetcher::WaitAll(const TInstant& deadline) {
        Impl->RelaxMany(deadline);
        return Impl->InFlight == 0;
    }
}
