#pragma once

#include <util/generic/ptr.h>

#include <memory>

namespace NActors {
    struct TActorContext;

    class IActor;
    using IActorPtr = std::unique_ptr<IActor>;

    class IEventBase;
    using IEventBasePtr = std::unique_ptr<IEventBase>;

    class IEventHandle;
    using IEventHandlePtr = std::unique_ptr<IEventHandle>;
}

namespace NKikimr {
    class TTabletStorageInfo;
    using TTabletStorageInfoPtr = TIntrusivePtr<TTabletStorageInfo>;

    class TTabletSetupInfo;
    using TTabletSetupInfoPtr = TIntrusivePtr<TTabletSetupInfo>;
}

namespace NKikimrConfig {
    class TAppConfig;
    using TAppConfigPtr = std::shared_ptr<TAppConfig>;
}

namespace NCloud {

////////////////////////////////////////////////////////////////////////////////

struct IActorSystem;
using IActorSystemPtr = TIntrusivePtr<IActorSystem>;

}   // namespace NCloud
