#include "output_diff.h"
#include "output_print.h"

#include <util/stream/input.h>

namespace NSnippets {

    TDiffOutput::TDiffOutput(bool inverse, bool wantAttrs, bool nodata)
      : Inverse(inverse)
      , WantAttrs(wantAttrs)
      , NoData(nodata)
    {
    }

    static void PrintStat(IOutputStream& out, const TString& title, const size_t total, const size_t same, const size_t diff) {
        out << "== Total " << title << ": " << total << " same: " << same << " diff: " << diff << " (" << diff * 100. / total << "%)"<< Endl;
    }

    void TDiffOutput::PrintStats() {
        PrintStat(Cout, "snippets", Total, Same, Diff);
        PrintStat(Cout, "serps", AllRequestIds.size(), AllRequestIds.size() - DiffRequestIds.size(), DiffRequestIds.size());
    }

    void TDiffOutput::Process(const TJob& job) {
        TString requestId;
        if (job.ContextData.GetProtobufData().HasLog() &&  job.ContextData.GetProtobufData().GetLog().HasRequestId()) {
            requestId = job.ContextData.GetProtobufData().GetLog().GetRequestId();
        }
        ++Total;
        AllRequestIds.insert(requestId);
        if (!Differs(job.Reply, job.ReplyExp)) {
            ++Same;
            if (!Inverse) {
                return;
            }
        } else {
            ++Diff;
            DiffRequestIds.insert(requestId);
            if (Inverse) {
                return;
            }
        }
        if (NoData) {
            return;
        }
        Cout << Endl;
        PrintStats();
        Cout << "document: " << job.ContextData.GetId() << Endl;
        Cout << "req: " << job.Req << Endl;
        Cout << "userreq: " << job.UserReq << Endl;
        if (job.MoreHlReq.size()) {
            Cout << "morehlreq: " << job.MoreHlReq << Endl;
        }
        if (job.RegionReq.size()) {
            Cout << "regionreq: " << job.RegionReq << Endl;
        }
        Cout << "url: " << job.ArcUrl << Endl;
        TPrintOutput::Print(job.Reply, Cout, job.BaseExp, WantAttrs, false);
        if (!Inverse) {
            TPrintOutput::Print(job.ReplyExp, Cout, job.Exp, WantAttrs, false);
        }
    }

    void TDiffOutput::Complete() {
        Cout << "== Done!" << Endl;
        PrintStats();
    }


    TDiffQurlsOutput::TDiffQurlsOutput(IOutputStream& out)
      : Out(out)
    {
    }

    void TDiffQurlsOutput::Process(const TJob& job) {
        if (!Differs(job.Reply, job.ReplyExp)) {
            return;
        }
        Out << job.UserReq << "\t" << job.Region << "\t" << job.ArcUrl << Endl;
        Out.Flush();
        Cerr.Flush();
    }


    TDiffScoreOutput::TDiffScoreOutput()
    {
        Marks[0] = Marks[1] = Marks[2] = 0;
    }

    void TDiffScoreOutput::Process(const TJob& job) {
        ++Total;
        if (!Differs(job.Reply, job.ReplyExp)) {
            ++Same;
            return;
        }
        ++Diff;
        Cout << Endl;
        Cout << "== Total: " << Total << " same: " << Same << " diff: " << Diff << " (" << Diff * 100. / Total << "%)"<< Endl;
        Cout << "document: " << job.ContextData.GetId() << Endl;
        Cout << "req: " << job.Req << Endl;
        Cout << "userreq: " << job.UserReq << Endl;
        Cout << "url: " << job.ArcUrl << Endl;
        TPrintOutput::Print(job.Reply, Cout, job.BaseExp, false, false);
        TPrintOutput::Print(job.ReplyExp, Cout, job.Exp, false, false);
        Cout << "Mark (1-base, 2-same, 3-exp): ";
        size_t x;
        Cin >> x;
        if (x < 1 || x > 3) {
            x = 2;
        }
        ++Marks[x - 1];
        Cout << "=== base: " << Marks[0] << " same: " << Marks[1] << " exp: " << Marks[2] << Endl;
    }

} //namespace NSnippets
