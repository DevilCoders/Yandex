#pragma once

#include "qd_source_updater.h"

#include <library/cpp/file_checker/registry_checker.h>

#include <util/folder/path.h>
#include <util/string/strip.h>

namespace NQueryData {

    class TSourcesRegistry;

    struct TTotalSize {
        ui64 AllSize = 0;
        ui64 RAMSize = 0;

        ui64 RAMCount = 0;
        ui64 FastMMapCount = 0;
        ui64 MMapCount = 0;
        ui64 InvalidCount = 0;

        ui64 AllCount() const {
            return RAMCount + FastMMapCount + MMapCount + InvalidCount;
        }
    };


    class TSourcesRegistry {
        TSourceCheckerOpts SourceOpts;

    public:
        TSourcesRegistry(TServerOpts opts)
            : SourceOpts(opts)
        {
            StaticSources.Unsafe().reserve(10);
            SourceLists.Unsafe().reserve(10);
        }

        bool HasSourcesOrLists() const {
            return !(StaticSources.Unsafe().empty() && SourceLists.Unsafe().empty() && FakeSources.Unsafe().empty());
        }

        void SetSourceFilesDir(TStringBuf name) {
            SourceFilesDir = name;
        }

        void RegisterStaticSourceFile(TStringBuf file) {
            TSourceCheckerPtr sch = new TSourceChecker(GetPath(SourceFilesDir, TString{file}), SourceOpts);
            TSourceCheckersG::TWrite(StaticSources).Object.push_back(sch);
        }

        void RegisterFakeSource(const TBlob& b, const TString& fname = TString()) {
            TFakeSourceUpdaterPtr ptr = new TFakeSourceUpdater(b, fname);
            TFakeSourceUpdatersG::TWrite(FakeSources).Object.push_back(ptr);
        }

        void RegisterFakeSource(TSource::TPtr b) {
            TFakeSourceUpdaterPtr ptr = new TFakeSourceUpdater(b);
            TFakeSourceUpdatersG::TWrite(FakeSources).Object.push_back(ptr);
        }

        void RegisterSourceList(TStringBuf file) {
            TSourceListPtr ptr = new TSourceList(GetPath(SourceFilesDir, TString{file}), 10000, SourceOpts);
            TSourceListsG::TWrite(SourceLists).Object.push_back(ptr);
        }

        NSc::TValue GetStats(EStatsVerbosity sv) const;

        TTotalSize TotalSize() const;

        void GetSources(TSources& src) const;

    private:
        TString SourceFilesDir;

        TFakeSourceUpdatersG FakeSources;
        TSourceCheckersG StaticSources;
        TSourceListsG SourceLists;
    };

}
