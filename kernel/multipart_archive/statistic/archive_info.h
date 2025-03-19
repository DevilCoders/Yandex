#pragma once

#include <kernel/multipart_archive/abstract/optimization_options.h>

#include <library/cpp/json/json_value.h>

#include <util/string/cast.h>
#include <util/generic/map.h>


namespace NRTYArchive {
    struct TArchiveInfo {
        struct TOptimizationInfo {
            TOptimizationOptions Opts;
            ui64 PartsToOptimize = 0;
            ui64 DocsToMove = 0;

            TString GetLabel() const {
                return ToString(Opts.GetPartSizeDeviation()) + "-" + ToString(Opts.GetPopulationRate());
            }
        };

        ui64 PartsCount = 0;
        ui64 RemovedDocsCount = 0;
        ui64 AliveDocsCount = 0;
        ui64 FullSizeInBytes = 0;
        TMap<TString, TOptimizationInfo> Optimizations;

        void ToJson(NJson::TJsonValue& json) const {
            json["parts_count"] = PartsCount;
            json["removed_count"] = RemovedDocsCount;
            json["alive_count"] = AliveDocsCount;
            json["full_size_bytes"] = FullSizeInBytes;
            json["optimizations"] = NJson::TJsonValue(NJson::JSON_MAP);
            for (const auto& info : Optimizations) {
                json["optimizations"][info.first]["parts"] = info.second.PartsToOptimize;
                json["optimizations"][info.first]["docs"] = info.second.DocsToMove;
            }
        }

        void Add(const TArchiveInfo& info) {
            PartsCount += info.PartsCount;
            RemovedDocsCount += info.RemovedDocsCount;
            AliveDocsCount += info.AliveDocsCount;
            FullSizeInBytes += info.FullSizeInBytes;
            for (const auto& opt : info.Optimizations) {
                auto& old = Optimizations[opt.first];
                old.PartsToOptimize += opt.second.PartsToOptimize;
                old.DocsToMove += opt.second.DocsToMove;
            }
        }
    };
}
