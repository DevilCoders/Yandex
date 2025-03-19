#include "qd_search.h"
#include "qd_server.h"
#include "qd_registry.h"
#include "qd_source.h"

#include <util/system/hostname.h>

namespace NQueryData {

    class TQueryDatabase::TImpl {
    public:
        TImpl(TServerOpts opts)
            : Sources(opts)
            , Opts(opts)
        {
        }

        bool HasSourceData() const {
            return Sources.HasSourcesOrLists();
        }

        NSc::TValue GetStats(EStatsVerbosity verb) const {
            return Sources.GetStats(verb);
        }

        void VisitSources(ISourceVisitor& vis) {
            TSources sources;
            Sources.GetSources(sources);

            for (TSources::const_iterator it = sources.begin(); it != sources.end(); ++it) {
                TSourcePtr ptr = *it;

                if (!ptr)
                    continue;

                vis.Visit(*ptr);
            }

            vis.Done();
        }

        TSourcesRegistry Sources;
        TServerOpts Opts;
    };

    TQueryDatabase::TQueryDatabase(TServerOpts opts)
        : Impl(new TImpl(opts))
    {
    }

    TQueryDatabase::~TQueryDatabase() {
    }

    void TQueryDatabase::AddSourceFileList(TStringBuf fname) {
        Impl->Sources.RegisterSourceList(fname);
    }

    void TQueryDatabase::AddSourceFile(TStringBuf fname) {
        Impl->Sources.RegisterStaticSourceFile(fname);
    }

    void TQueryDatabase::AddSourceData(const TBlob& data, const TString& fname) {
        Impl->Sources.RegisterFakeSource(data, fname);
    }

    void TQueryDatabase::AddSource(const TSource::TPtr& source) {
        Impl->Sources.RegisterFakeSource(source);
    }

    void TQueryDatabase::SetSourceFilesDir(TStringBuf name) {
        Impl->Sources.SetSourceFilesDir(name);
    }

    bool TQueryDatabase::HasSourceData() const {
        return Impl->HasSourceData();
    }

    void TQueryDatabase::VisitSources(ISourceVisitor& vis) const {
        Impl->VisitSources(vis);
    }

    TQueryDataWrapper TQueryDatabase::GetWrappedQueryData(const TRequestRec& request, bool skipnorm) const {
        TQueryData data;
        GetQueryData(request, data, skipnorm);
        return TQueryDataWrapper(data);
    }

    TQueryDataWrapper TQueryDatabase::GetWrappedQueryData(TStringBuf normuserquery) const {
        TRequestRec rec;
        rec.UserQuery = normuserquery;
        return GetWrappedQueryData(rec, true);
    }

    void TQueryDatabase::GetQueryData(TStringBuf rawkeytuple, TQueryData& data) const {
        TRequestRec rec;
        rec.UserQuery = rawkeytuple;
        data.Clear();
        TSearchOpts opts;
        opts.SkipNorm = true;
        opts.EnableDebugInfo = Impl->Opts.EnableDebugInfo;
        TSearchVisitor res(data, rec, opts);
        VisitSources(res);
    }

    void TQueryDatabase::GetQueryData(const TRequestRec& req, TQueryData& data, bool skipnorm) const {
        TSearchOpts opts;
        opts.SkipNorm = skipnorm;
        opts.EnableDebugInfo = Impl->Opts.EnableDebugInfo;
        TSearchVisitor res(data, req, opts);
        VisitSources(res);
    }

    NSc::TValue TQueryDatabase::GetStats(EStatsVerbosity verb) const {
        return Impl->GetStats(verb);
    }

}
