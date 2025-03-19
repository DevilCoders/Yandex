#pragma once

#include "qd_source.h"

#include <kernel/querydata/common/qd_util.h>

#include <library/cpp/file_checker/registry_checker.h>

namespace NQueryData {

    struct TSourceCheckerOpts : NChecker::TFileCheckerOpts, TServerOpts {
        TSourceCheckerOpts(TServerOpts srvopts = TServerOpts()
                           , NChecker::TFileOptions opts = NChecker::TFileOptions::GetZeroOnDelete()
                           , ui32 timer = 10000)
            : TFileCheckerOpts(opts, timer)
            , TServerOpts(srvopts)
        { }
    };


    class TFakeSourceUpdater: public NChecker::TFakeObjectHolder<TSource> {
    public:
        explicit TFakeSourceUpdater(const TBlob& data, const TString& file = TString(), time_t tstamp = 0)
            : NChecker::TFakeObjectHolder<TSource>(new TSource)
        {
            GetObject()->InitFake(data, file, tstamp);
        }

        explicit TFakeSourceUpdater(TSource::TPtr p)
            : NChecker::TFakeObjectHolder<TSource>(p)
        {}
    };


    class TSourceChecker : public NChecker::TFileChecker<TSource> {
        using TParent = NChecker::TFileChecker<TSource>;
        TSourceCheckerOpts Opts;

    public:
        explicit TSourceChecker(const TString& fname, const TSourceCheckerOpts& opts)
            : TParent(fname, opts.Options, opts.Timer)
            , Opts(opts)
        {
            Start();
        }

        ~TSourceChecker() override {
            Stop();
        }

        TSharedObjectPtr MakeNewObject(const TFile&, const TFileStat&, TSharedObjectPtr, bool beforestart) const override;

        NChecker::EUpdateStrategy GetUpdateStrategy(const TFile&, const TFileStat& fs, const TSharedObjectPtr& ptr) const override;
    };


    class TSourceList : public NChecker::TRegistryChecker<TSourceChecker, TSourceCheckerOpts> {
        typedef NChecker::TRegistryChecker<TSourceChecker, TSourceCheckerOpts> TParent;
        TSourceCheckerOpts SourceOpts;

        static_assert(sizeof(TSourceCheckerOpts) == sizeof(TFileSet::value_type::second_type), "expect sizeof(TSourceCheckerOpts) == sizeof(TFileSet::value_type::second_type)");

    public:
        TSourceList(const TString& f, ui32 t, const TSourceCheckerOpts& opts)
            : TParent(f, t)
            , SourceOpts(opts)
        {
            Start();
        }

        ~TSourceList() override {
            Stop();
        }

        void GetNewFileSet(const TFile& fh, TFileSet& fs) const override;
    };


    using TAbstractSourceUpdater = NChecker::IObjectHolder<TSource>;
    using TFakeSourceUpdaterPtr = TIntrusivePtr<TFakeSourceUpdater>;
    using TSourceCheckerPtr = TIntrusivePtr<TSourceChecker>;
    using TSourceListPtr = TIntrusivePtr<TSourceList>;
    using TSourcePtr = TFakeSourceUpdater::TSharedObjectPtr;
    using TFakeSourceUpdaters = TVector<TFakeSourceUpdaterPtr>;
    using TFakeSourceUpdatersG = TGuarded<TFakeSourceUpdaters>;
    using TSourceCheckers = TVector<TSourceCheckerPtr>;
    using TSourceCheckersG = TGuarded<TSourceCheckers>;
    using TSourceLists = TVector<TSourceListPtr>;
    using TSourceListsG = TGuarded<TSourceLists>;
    using TSources = TVector<TSourcePtr>;

}
