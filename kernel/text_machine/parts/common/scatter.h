#pragma once

#include "types.h"
#include "floats.h"

#include <kernel/text_machine/module/module.h>

namespace NTextMachine {
namespace NCore {
    struct TScatterMethod {
        template <size_t Index>
        struct TScatterIndex {};

        template <size_t Index>
        using TStaticScatterByIndex = ::NModule::TStaticScatterTag< TScatterIndex<Index> >;

        template <size_t Index>
        using TScatterByIndex = ::NModule::TScatterTag< TScatterIndex<Index> >;

        using CollectExpansions = TStaticScatterByIndex<0>;
        using CollectStreams  = TStaticScatterByIndex<1>;
        using CollectFeatures  = TStaticScatterByIndex<2>;
        using CollectValues  = TStaticScatterByIndex<3>;
        using CollectValueRefs  = TStaticScatterByIndex<4>;

        using ConfigureSharedState  = TScatterByIndex<0>;
        using RegisterValues  = TScatterByIndex<1>;
        using BindValueRefs  = TScatterByIndex<2>;
        using ConfigureFeatures  = TScatterByIndex<3>;
        using NotifyQueryFeatures = TScatterByIndex<4>;
        using RegisterDataExtractors = TScatterByIndex<5>;

        template <typename T>
        struct TAggregateFeaturesType {};

        template <typename T>
        using AggregateFeatures = ::NModule::TScatterTag< TAggregateFeaturesType<T> >;
    };

    // CollectExpansions
    //
    struct TStaticExpansionsInfo {
        TSet<EExpansionType>& Expansions;
    };

    // CollectStreams
    //
    struct TStaticStreamsInfo {
        TSet<EStreamType>& Streams;
    };

    // CollectFeatures
    //
    struct TSlotFeaturesEntry {
        TVector<TFFId> Ids;
    };

    using TFeaturesStream = TStructuralStream<TSlotFeaturesEntry>;

    struct TStaticFeaturesInfo {
        TFeaturesStream::TWriter& Features;
    };

    // ConfigureFeatures
    //
    struct TSlotFeaturesConfigEntry {
        TConstCoordsBuffer Coords;
    };

    using TFeaturesConfigStream = TStructuralStream<TSlotFeaturesConfigEntry>;

    struct TConfigureFeaturesInfo {
        TFeaturesConfigStream::TReader& FeaturesConfig;
    };

    // Collect values
    //
    struct TValueEntry {
        TValueIdWithHash Id;
    };

    using TValuesStream = TStructuralStream<TValueEntry>;

    struct TStaticValuesInfo {
        TValuesStream::TWriter& Values;
    };

    // Collect value refs
    //
    struct TValueRefEntry {
        TValueIdWithHash Id;
        i64 ValueIndex = -1;
    };

    using TValueRefsStream = TStructuralStream<TValueRefEntry>;

    // Register floats
    //
    struct TRegisterValuesInfo {
        TValuesStream::TReader& Values;
        TFloatsRegistry& FloatsRegistry;
        TUints64Registry& Uints64Registry;
    };

    // Bind floats
    //
    struct TBindValuesInfo {
        TValueRefsStream::TReader& ValueRefs;
        const TFloatsRegistry& FloatsRegistry;
        const TUints64Registry& Uints64Registry;
    };

    struct TStaticValueRefsInfo {
        TValueRefsStream::TWriter& ValueRefs;
    };

    // Configure shared state
    //
    struct TConfigureSharedStateInfo {
        TMemoryPool& Pool;
        bool EnableCompactEnums;
        IInfoCollector* Collector;
    };

    // Broadcast features vector computed for query
    //
    struct TNotifyQueryFeaturesInfo {
        EExpansionType Expansion;
        size_t Index;
        const TFFIds& Ids;
        const TConstFloatsBuffer& Features;
    };

    // Aggregare features
    //
    struct TAggregateFeaturesInfo {
        const TQuery& Query;
        const TConstFloatsBuffer& Features;
    };
} // NCore
} // NTextMachine

