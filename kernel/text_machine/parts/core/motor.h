#pragma once

#include "types.h"

#include "configure.h"
#include "expansion_trackers.h"
#include "parts_positionless.h"
#include "parts_collector.h"
#include "parts_base.h"

#include <kernel/text_machine/interface/text_machine.h>
#include <kernel/text_machine/parts/common/features_filter.h>

#include <kernel/factor_storage/float_utils.h>

#include <kernel/text_machine/module/module_def.inc>

namespace NTextMachine {
namespace NCore {
    MACHINE_PARTS(Core) {
        template <typename M>
        class TMotor
            : public TTextMachineBase
            , public M
        {
        public:
            TMotor()
                : Config(*GetCoreStatic().FeaturesHelper)
            {
                MACHINE_LOG("Core::TMotor", "CreateMachine", TStringBuilder{}
                    << "{Machine: " << static_cast<const void*>(this) << "}");

                ConfigureMachine(TMachineOptions{});
            }

            bool IsAllowedExpansion(EExpansionType expansion) const {
                return ExpansionRemap.HasKey(expansion);
            }
            bool IsAllowedStream(EStreamType stream) const {
                return StreamRemap.HasKey(stream);
            }

            const TVector<EExpansionType>& GetAllowedExpansions() const override {
                return GetCoreProc().GetAllowedExpansions(Options.EnableCompactEnums);
            }
            const TVector<EStreamType>& GetAllowedStreams() const override {
                return GetCoreProc().GetAllowedStreams(Options.EnableCompactEnums);
            }

            void ConfigureMachine(const TMachineOptions& options) override {
                MACHINE_LOG("Core::TMotor", "ConfigureMachine", TStringBuilder{}
                    << "{Machine: " << static_cast<const void*>(this)
                    << ", EnableCompactEnums: " << options.EnableCompactEnums << "}");

                Options = options;

                ExpansionRemap = GetCoreProc().GetExpansionRemap(Options.EnableCompactEnums);
                StreamRemap = GetCoreProc().GetStreamRemap(Options.EnableCompactEnums);
            }
            void ConfigureFeatures(const TFeatureOptions& options) override {
                MACHINE_LOG("Core::TMotor", "ConfigureFeatures", TStringBuilder{}
                    << "{Machine: " << static_cast<const void*>(this)
                    << ", Count: " << options.Size() << "}");

                Config.ConfigureFromList(options.begin(), options.end());
                ClearSavedIds();
            }
            void ConfigureExpansions(const TExpansionOptions& options) override {
                TBitMap<TExpansion::Size> enabledExpansions;
                enabledExpansions.Clear();
                for (EExpansionType expansionType : GetCoreStatic().AllowedExpansions) {
                    if (options.IsEnabled(expansionType)) {
                        enabledExpansions.Set(static_cast<int>(expansionType));

                        MACHINE_LOG("Core::TMotor", "EnableExpansion", TStringBuilder{}
                            << "{Machine: " << static_cast<const void*>(this)
                            << ", Type: " << expansionType << "}");
                    }
                }

                Config.ConfigureFromPredicate([&enabledExpansions](const TFFIdWithHash& id) -> bool
                {
                    return enabledExpansions.Test(static_cast<int>(id.Get<TFeaturePart::Expansion>()));
                },
                false);

                ClearSavedIds();
            }
            void NarrowByLevel(const TStringBuf& levelName) override {
                MACHINE_LOG("Core::TMotor", "NarrowByLevel", TStringBuilder{}
                    << "{Machine: " << static_cast<const void*>(this)
                    << ", LevelName: " << levelName << "}");

                LevelName = levelName;
                ClearSavedIds();
            }
            void SetInfoCollector(IInfoCollector* collector) override {
                Collector = collector;
            }

            void NewQueryInPool(const TMultiQuery& multiQuery, TMemoryPool& pool) override {
                MACHINE_LOG("Core::TMotor", "NewQueryInPool", TStringBuilder{}
                    << "{Machine: " << static_cast<const void*>(this) << "}");

                if (LevelName) {
                    auto filter = M::GetLevelFilter(LevelName);
                    Y_ASSERT(!filter.Empty());

                    Config.ConfigureFromPredicate([&filter](const TFFIdWithHash& id) -> bool
                    {
                        return filter.IsAccepted(id.ToId());
                    },
                    true);
                    LevelName = TString();
                }

                QueriesHelper = pool.New<TQueriesHelper>(pool,
                    multiQuery,
                    *GetCoreStatic().ExpansionRemap);

                const auto totalCounters = GetCoreStatic().Values.NumItems();
                const auto intCounters = GetCoreStatic().NumIntCounters;
                TFloatsRegistry floatsRegistry(pool, totalCounters - intCounters);
                TUints64Registry uintsRegistry(pool, intCounters);

                typename M::TUnitsMask allMask;
                allMask.Set(0, M::NumUnits);
                M::SetUnitsEnabled(allMask);

                {
                    TConfigureSharedStateInfo configInfo{pool, Options.EnableCompactEnums, Collector};
                    M::Scatter(TScatterMethod::ConfigureSharedState(), configInfo);
                }

                {
                    auto featuresConfigReader = Config.GetStream().CreateReader();
                    TConfigureFeaturesInfo configInfo{featuresConfigReader};
                    M::Scatter(TScatterMethod::ConfigureFeatures(), configInfo);
                }

                {
                    TMultiQueryInfo queryInfo{*QueriesHelper};
                    M::NewMultiQuery(pool, queryInfo);
                }

                {
                    auto valuesReader = GetCoreStatic().Values.CreateReader();
                    TRegisterValuesInfo regInfo{valuesReader, floatsRegistry, uintsRegistry};
                    M::Scatter(TScatterMethod::RegisterValues(), regInfo);
                }

                {
                    auto valueRefsReader = GetCoreStatic().ValueRefs.CreateReader();
                    TBindValuesInfo bindInfo{valueRefsReader, floatsRegistry, uintsRegistry};
                    M::Scatter(TScatterMethod::BindValueRefs(), bindInfo);
                }

                {
                    for(auto p : DataExtractorsRegistry) {
                        p.Value().clear();
                    }
                    M::Scatter(TScatterMethod::RegisterDataExtractors(), &DataExtractorsRegistry);
                }
            }
            void NewDoc() override {
                MACHINE_LOG("Core::TMotor", "NewDoc", TStringBuilder{}
                    << "{Machine: " << static_cast<const void*>(this) << "}");

                TDocInfo info{};
                M::NewDoc(info);
            }
            void AddMultiHits(const TMultiHit* buf, size_t count) override {
                MACHINE_LOG("Core::TMotor", "AddMultiHits", TStringBuilder{}
                    << "{Machine: " << static_cast<const void*>(this)
                    << ", Count: " << count << "}");

                for (size_t i = 0; i != count; ++i) {
                    SwitchHit(buf[i]);
                }
            }
            void SaveOptFeatures(TOptFeatures& features) override {
                MACHINE_LOG("Core::TMotor", "SaveOptFeatures", TStringBuilder{}
                    << "{Machine: " << static_cast<const void*>(this) << "}");

                TDocInfo finishInfo{};
                M::FinishDoc(finishInfo);

                const size_t offset = features.size();

                features.resize(offset + Config.GetNumFeatures());
                TFeaturesBuffer featuresBuf = TFeaturesBuffer(features.data() + offset, Config.GetNumFeatures(), EStorageMode::Empty);
                TSaveFeaturesInfo saveInfo{featuresBuf};
                M::SaveFeatures(saveInfo);
                Y_ASSERT(0 == featuresBuf.Avail());

                Config.JoinFeatureIds(featuresBuf);
                Config.Reorder(featuresBuf);

                NormalizeFeaturesSoft(featuresBuf);
            }
            void SaveFeatureIds(TFFIds& ids) const override {
                MACHINE_LOG("Core::TMotor", "SaveFeatureIds", TStringBuilder{}
                    << "{Machine: " << static_cast<const void*>(this) << "}");

                TMotor<M> machine;
                machine.Config = Config;
                machine.LevelName = LevelName;
                SaveFeatureIdsHelper(machine, ids);
            }
            void SaveToJson(NJson::TJsonValue& value) const override {
                M::SaveToJson(value);
            }

        public:
            Y_FORCE_INLINE const typename M::template TGetProc<TCoreUnit>& GetCoreProc() const {
                return M::template Proc<TCoreUnit>();
            }
            Y_FORCE_INLINE const typename M::template TGetState<TCoreUnit>& GetCoreState() const {
                return M::template Vars<TCoreUnit>();
            }
            Y_FORCE_INLINE static const typename M::template TGetStatic<TCoreUnit>& GetCoreStatic() {
                return M::template Static<TCoreUnit>();
            }

            virtual TDataExtractorsList FetchExtractors(EDataExtractorType Type) final {
                return DataExtractorsRegistry[Type];
            }

        private:
            Y_FORCE_INLINE void SwitchHit(const TMultiHit& hit) {
                if (Y_UNLIKELY(!hit.BlockHit.Position.Annotation)) {
                    Y_ASSERT(false);
                    return;
                }
                if (Y_UNLIKELY(!hit.BlockHit.Position.Annotation->Stream)) {
                    Y_ASSERT(false);
                    return;
                }
                if (hit.BlockHit.BlockId >= QueriesHelper->GetNumBlocks()) {
                    return;
                }

                const EStreamType streamType = hit.BlockHit.Position.Annotation->Stream->Type;
                if (!TMotor::IsAllowedStream(streamType)) {
                    return;
                }

                MACHINE_LOG("Core::TMotor", "AddBlockHit", TStringBuilder{}
                    << "{BlockId: " << hit.BlockHit.BlockId
                    << ", Stream: " << streamType
                    << ", BreakNumber: " << hit.BlockHit.Position.Annotation->BreakNumber
                    << ", LeftWordPos: " << hit.BlockHit.Position.LeftWordPos
                    << ", RightWordPos: " << hit.BlockHit.Position.RightWordPos
                    << ", SubHitsCount: " << hit.Count << "}");

                M::SwitchHitCodegen(hit, streamType, *QueriesHelper);
            }

            void NormalizeFeaturesSoft(TFeaturesBuffer& features) { // SEARCH-2227
                for (auto& f : features) {
                    f.SetValue(SoftClipFloat(f.GetValue()).AbsTolerance(4 * FloatEpsilon));
                }
            }
        private:
            TMachineOptions Options;
            TQueriesHelper* QueriesHelper = nullptr;
            IInfoCollector* Collector = nullptr;

            TExpansionRemapView ExpansionRemap;
            TStreamRemapView StreamRemap;

            TFeaturesConfig Config;
            TString LevelName;
            TSimpleExtractorsRegistry DataExtractorsRegistry;
        };
    } // MACHINE_PARTS(Core)
} // NCore
} // NTextMachine

#include <kernel/text_machine/module/module_undef.inc>
