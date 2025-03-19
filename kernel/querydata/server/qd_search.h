#pragma once

#include "qd_server.h"
#include "qd_source.h"

#include <kernel/querydata/request/qd_rawkeys.h>

namespace NQueryData {

    class TSearchVisitor : public ISourceVisitor {
    public:
        TSearchVisitor(TQueryData& res, const TRequestRec& req, TSearchOpts opts)
            : Result(res)
            , Request(req)
            , Opts(opts)
        { }

        void Visit(const TSource&) override;

        TSearchOpts& GetOpts() {
            return Opts;
        }

    private:
        TSubkeysCache SubkeysCache;
        TSearchBuffer MainBuffer;
        TSearchBuffer ListBuffer;

        TQueryData& Result;
        const TRequestRec& Request;
        TSearchOpts Opts;
    };

}
