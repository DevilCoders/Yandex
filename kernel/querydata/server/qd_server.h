#pragma once

#include "qd_constants.h"

#include <kernel/querydata/client/querydata.h>
#include <kernel/querydata/cgi/qd_request.h>

#include <util/memory/blob.h>

namespace NQueryData {

    class TSource;

    class ISourceVisitor {
    public:
        virtual ~ISourceVisitor() {}

        virtual void Visit(const TSource&) = 0;
        virtual void Done() {}
    };


    class TQueryDatabase: TNonCopyable {
        class TImpl;

    public:
        TQueryDatabase(TServerOpts opts = TServerOpts());
        ~TQueryDatabase();

        NSc::TValue GetStats(EStatsVerbosity) const;

        void SetSourceFilesDir(TStringBuf fname);
        void AddSourceFileList(TStringBuf fname);
        void AddSourceFile(TStringBuf fname);
        void AddSourceData(const TBlob& data, const TString& fname = TString());
        void AddSource(const TIntrusivePtr<TSource>&);

        bool HasSourceData() const;

    public:
        void GetQueryData(const TRequestRec& request, TQueryData& data, bool skipnorm = false) const;
        TQueryDataWrapper GetWrappedQueryData(const TRequestRec& request, bool skipnorm = false) const;

    public:
        void VisitSources(ISourceVisitor&) const;

        void GetQueryData(TStringBuf rawkeytouple, TQueryData&) const;
        TQueryDataWrapper GetWrappedQueryData(TStringBuf rawkeytuple) const;

    private:
        THolder<TImpl> Impl;
    };

}
