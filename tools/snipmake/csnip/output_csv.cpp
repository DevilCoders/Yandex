#include "output_csv.h"
#include "job.h"

#include <kernel/snippets/iface/passagereply.h>

#include <util/charset/wide.h>
#include <util/generic/string.h>
#include <util/stream/output.h>

namespace NSnippets {

    static TString Prettify(const TString &s) {
        TString res;
        res.push_back('"');
        for (size_t i = 0; i < s.size(); ++i) {
            if (s[i] == '"') {
                res.push_back('"');
            }
            res.push_back(s[i]);
        }
        res.push_back('"');
        return res;
    }

    void TCsvOutput::Process(const TJob& job)
    {
        Cout << Prettify(job.UserReq) << ";";
        Cout << Prettify(job.ArcUrl) << ";";
        const NSnippets::TPassageReply& res = job.Reply;
        Cout << Prettify(WideToUTF8(res.GetTitle())) << ";";

        bool needEllipsis = false;
        TString passage;
        if (!!res.GetHeadline()) {
            passage.append(WideToUTF8(res.GetHeadline()));
            needEllipsis = true;
        }
        for (size_t i = 0; i < res.GetPassages().size(); ++i) {
            if (needEllipsis) {
                passage.append(" … ");
            }
            passage.append(WideToUTF8(res.GetPassages()[i]));
            needEllipsis = true;
        }
        Cout << Prettify(passage) << Endl;
    }

} //namespace NSnippets
