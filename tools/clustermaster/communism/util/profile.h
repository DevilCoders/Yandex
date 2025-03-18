#pragma once

#include <library/cpp/deprecated/atomic/atomic.h>
#include <util/system/tls.h>

// #define PROFILING

#ifdef PROFILING

#define PROFILE_ACQUIRE(ID) TGuard<NProfile::TProfileData<ID>, NProfile::TProfileOps> __ ## ID ## _ProfileGuard(NProfile::TProfileData<ID>::Instance());
#define PROFILE_RELEASE(ID) __ ## ID ## _ProfileGuard.Release();
#define PROFILE_OUTPUT(ID) PROFILE_STREAM << '\t' << #ID << '\t' << NProfile::TProfileData<ID>::ProfiledTime() << '\n';
#define PROFILE_STRING(STR) PROFILE_STREAM << '\t' << (STR) << '\n';

#else

#define PROFILE_ACQUIRE(ID) (void)(ID);
#define PROFILE_RELEASE(ID) (void)(ID);
#define PROFILE_OUTPUT(ID) (void)(ID);
#define PROFILE_STRING(STR) (void)(STR);

#endif

namespace NProfile {

template <int ID>
class TProfileData {
private:
    NTls::TValue<TInstant::TValue> Acquired;
    TAtomic Counter;

public:
    friend struct TProfileOps;
    friend TDuration ProfiledTime();

    static inline TProfileData& Instance() noexcept;

    static inline TDuration ProfiledTime() noexcept {
        return TDuration(AtomicGet(Instance().Counter));
    }
};

template <int ID>
inline TProfileData<ID>& TProfileData<ID>::Instance() noexcept {
    static TProfileData Data;
    return Data;
}

struct TProfileOps {
    template <int ID>
    static inline void Acquire(TProfileData<ID>* data) noexcept {
        Y_ASSERT(!data->Acquired);
        data->Acquired = TInstant::Now().GetValue();
    }

    template <int ID>
    static inline void Release(TProfileData<ID>* data) noexcept {
        Y_ASSERT(data->Acquired);
        AtomicAdd(data->Counter, TInstant::Now().GetValue() - data->Acquired);
        data->Acquired = 0;
    }
};

}
