#include "output.h"
#include "job.h"
#include "html_hilite.h"

#include <kernel/snippets/iface/passagereply.h>

#include <util/charset/wide.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/string/cast.h>
#include <util/string/subst.h>
#include <util/stream/output.h>

namespace NSnippets {

    TReqUrlOutput::TReqUrlOutput(bool idPrefix)
      : IdPrefix(idPrefix)
    {
    }

    void TReqUrlOutput::Process(const TJob& job) {
        if (IdPrefix) {
            Cout << job.ContextData.GetId() << " ";
        }
        Cout << job.UserReq << " url:\"" << job.ArcUrl << "\"" << Endl;
    }

    static TUtf16String GetSnippetText(const NSnippets::TPassageReply& reply) {
        TUtf16String snippetText;
        bool needEllipsis = false;
        if (!!reply.GetHeadline()) {
            snippetText.append(reply.GetHeadline());
            needEllipsis = true;
        }
        for (size_t i = 0; i < reply.GetPassages().size(); ++i) {
            if (needEllipsis) {
                snippetText.append(u" … ");
            }
            snippetText.append(reply.GetPassages()[i]);
            needEllipsis = true;
        }
        return snippetText;
    }

    void TBriefOutput::Process(const TJob& job) {
        const NSnippets::TPassageReply& res = job.Reply;
        Cout << "     id: " << job.ContextData.GetId() << Endl
             << "userreq: " << job.UserReq << Endl
             << "    url: " << job.ArcUrl << Endl
             << "  title: " << res.GetTitle() << Endl
             << "snippet: " << GetSnippetText(res) << Endl << Endl;
    }

    void TFactorsOutput::Process(const TJob& job) {
        Cout << "query: " << job.UserReq
             << "\tregion: " << job.Region
             << "\turl: " << job.ArcUrl
             << "\ttitle: " << RehighlightAndHtmlEscape(job.Reply.GetTitle())
             << "\tsnippet: " << RehighlightAndHtmlEscape(GetSnippetText(job.Reply))
             << "\tranking factors:";
        for (size_t i = 0; i < job.RankingFactors.Size(); ++i) {
            Cout << " " << job.RankingFactors[i];
        }
        TString snippetFactors(job.Reply.GetSnippetsExplanation()); // works with -E finaldump
        if (snippetFactors) {
            SubstGlobal(snippetFactors, '\t', ' ');
            Cout << "\tsnippet factors: " << snippetFactors;
        }
        Cout << Endl;
    }

    void TNullOutput::Process(const TJob&) {
    }

} //namespace NSnippets
