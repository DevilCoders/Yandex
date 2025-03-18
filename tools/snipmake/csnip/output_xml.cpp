#include "output_xml.h"
#include "job.h"
#include "html_hilite.h"
#include "lines_count.h"

#include <util/generic/vector.h>

namespace NSnippets {

    void TXmlOutput::Start()
    {
        Writer.Start();
        Writer.StartPool("4q");
    }

    TXmlOutput::TXmlOutput(const TString& name)
      : File(new TUnbufferedFileOutput(name))
      , Writer(*File.Get())
    {
        Start();
    }

    TXmlOutput::TXmlOutput(IOutputStream& out)
        : Writer(out)
    {
        Start();
    }

    void TXmlOutput::PrintXml(const TString& req, const TString& qtree, const TString& url, const TPassageReply& res, ui64 region) {
        TString tmpUrl = url;
        if (!(tmpUrl.StartsWith("http://") || tmpUrl.StartsWith("https://"))) {
            tmpUrl = "http://" + tmpUrl;
        }

        Writer.StartQDPair(req, tmpUrl, "1", ToString(region), qtree);
        NSnippets::TReqSnip snippet;
        snippet.TitleText = RehighlightAndHtmlEscape(res.GetTitle());
        const TVector<TUtf16String>& passages = res.GetPassages();
        const TUtf16String headline = res.GetHeadline();
        snippet.Lines = GetLinesCount(res.GetSnippetsExplanation());
        if (headline.size()) {
            NSnippets::TSnipFragment hf;
            hf.Text += RehighlightAndHtmlEscape(headline);
            snippet.SnipText.push_back(hf);
        }

        for (TVector<TUtf16String>::const_iterator it = passages.begin(); it != passages.end(); ++it) {
            NSnippets::TSnipFragment sf;
            sf.Text = RehighlightAndHtmlEscape(*it);
            snippet.SnipText.push_back(sf);
        }

        Writer.WriteSnippet(snippet);
        Writer.FinishQDPair();
    }

    void TXmlOutput::Process(const TJob& job, bool exp) {
        PrintXml(job.UserReq, job.B64QTree, job.ArcUrl, exp ? job.ReplyExp : job.Reply, job.Region);
    }

    void TXmlOutput::Process(const TJob& job) {
        Process(job, false);
    }

    void TXmlOutput::Complete() {
        Writer.FinishPool();
        Writer.Finish();
    }


    TXmlDiffOutput::TXmlDiffOutput(const TString& prefix)
      : Left(prefix + "base.xml")
      , Right(prefix + "exp.xml")
    {
    }

    void TXmlDiffOutput::Process(const TJob& job) {
        if (!Differs(job.Reply, job.ReplyExp)) {
            return;
        }
        Left.Process(job, false);
        Right.Process(job, true);
    }

    void TXmlDiffOutput::Complete() {
        Left.Complete();
        Right.Complete();
    }

} //namespace NSnippets
