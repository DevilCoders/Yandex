#pragma once

#include "config.h"

#include "tstab.h"

#include <yweb/robot/gefest/libkiwiexport/kiwi_export_actualizer.h>

#include <mapreduce/interface/value.h>

namespace NHebe {

    int Run(TConfig cfg) {
        if (!cfg.MRPrefix.EndsWith('/')) {
            cfg.MRPrefix += '/';
        }
        if (cfg.DropInput && cfg.DropResult) {
            Cerr << "--drop-result without --keep-input would kill all the data, not good" << Endl;
            return -1;
        }

        NMR::TServer server(cfg.MRServer);
        server.SetDefaultUser(cfg.MRUser);

        NComputeGraph::TJobRunner runner;

        NKiwiExportActualizer::TKiwiExportDescription config;
        config.DeltasPathPrefix = cfg.MRPrefix + "final/";
        config.AggregatedTableName = cfg.MRPrefix + cfg.OutputName;
        config.ShouldDropAggregated = cfg.DropResult;
        config.ShouldDropDeltas = cfg.DropInput;
        if (cfg.Format == "tstab") {
            config.Format = NKiwiExportActualizer::Custom;
            config.CustomFormat = new TTsTabFormat;
        } else if (cfg.Format == "protobin") {
            config.Format = NKiwiExportActualizer::ProtoBin;
        } else {
            Cerr << "unknown format: '" << cfg.Format << "'" << Endl;
            return -1;
        }
        NKiwiExportActualizer::TKiwiExportActualizer exportActualizer(config);

        TVector<NKiwiExportActualizer::TKiwiExportActualizer> actualizers;
        actualizers.push_back(exportActualizer);

        NKiwiExportActualizer::TKiwiActualizer actualizer;
        actualizer.Actualize(server, actualizers, {}, {}, {}, cfg.MaxAge);
        return 0;
    }

}
