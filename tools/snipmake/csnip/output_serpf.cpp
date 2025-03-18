#include "output_serpf.h"

#include "html_hilite.h"

#include <kernel/snippets/iface/passagereply.h>
#include <kernel/snippets/util/xml.h>

#include <util/charset/wide.h>
#include <library/cpp/string_utils/url/url.h>
#include <util/system/guard.h>
#include <util/system/spinlock.h>

namespace NSnippets {

    void TSerpFormatOutput::Process(const TJob& job) {
        const TString key = job.UserReq + "\t" + ToString(job.Region);
        const TString url = AddSchemePrefix(job.ArcUrl);
        const TString hurl = WideToUTF8(RehighlightAndHtmlEscape(UTF8ToWide(job.Reply.GetHilitedUrl())));
        TString value = url + "\t" + EncodeTextForXml10(WideToUTF8(RehighlightAndHtmlEscape(job.Reply.GetTitle())), false) + "\t";
        for (size_t i = 0; i < job.Reply.GetPassages().size(); ++i) {
            if (i) {
                value += " ... ";
            }
            value += EncodeTextForXml10(WideToUTF8(RehighlightAndHtmlEscape(job.Reply.GetPassages()[i])), false);
        }
        value += "\t" + hurl;
        {
            TGuard g(Lock);
            Data[key].push_back(value);
        }
    }

    void TSerpFormatOutput::Complete() {
        for (TData::const_iterator it = Data.begin(); it != Data.end(); ++it) {
            Cout << it->first << Endl;
            for (TItems::const_iterator jt = it->second.begin(); jt != it->second.end(); ++jt) {
                Cout << *jt << Endl;
            }
        }
    }

} //namespace NSnippets
