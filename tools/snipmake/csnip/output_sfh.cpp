#include "output_sfh.h"
#include "output_print.h"

#include <library/cpp/digest/sfh/sfh.h>

#include <util/stream/buffer.h>

namespace NSnippets {

    TSfhOutput::TSfhOutput(IOutputStream& out)
      : Out(out)
    {
    }

    void TSfhOutput::Process(const TJob& job) {
        Out << job.ContextData.GetId() << ": ";
        TBufferOutput buf;
        TPrintOutput pr(buf, true, true, false, false);
        pr.Process(job);
        Out << SuperFastHash(buf.Buffer().Data(), buf.Buffer().Size()) << Endl;
    }

} //namespace NSnippets
