#pragma once

#include <cloud/storage/core/libs/kikimr/public.h>

#include <util/generic/ptr.h>

namespace NKikimr {
    namespace NNodeTabletMonitor {
        struct ITabletStateClassifier;
        using ITabletStateClassifierPtr = TIntrusivePtr<ITabletStateClassifier>;

        struct ITabletListRenderer;
        using ITabletListRendererPtr = TIntrusivePtr<ITabletListRenderer>;
    }

    class TControlBoard;
    using TControlBoardPtr = TIntrusivePtr<TControlBoard>;

    class TKikimrScopeId;
}
