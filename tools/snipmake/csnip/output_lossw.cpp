#include "output_lossw.h"

#include <kernel/snippets/explain/losswords.h>

namespace NSnippets {

    TLossWordsOutput::TLossWordsOutput(IOutputStream& out)
        : Exp(new TLossWordsExplain(out))
    {
    }

    void TLossWordsOutput::Process(const TJob& job) {
        Exp->Parse(job.Reply.GetSnippetsExplanation());
    }

    void TLossWordsOutput::Complete() {
        Exp->Print();
    }

} //namespace NSnippets
