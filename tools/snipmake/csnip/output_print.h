#pragma once
#include "job.h"

#include <util/generic/strbuf.h>
#include <util/stream/output.h>

namespace NSnippets {

    class TPassageReply;

    struct TPrintOutput : IOutputProcessor {
        IOutputStream& Out;
        bool NoDoc = false;
        bool NoSuffix = false;
        bool WantPassageAttrs = false;
        bool PrintAltSnippets = false;

        TPrintOutput(IOutputStream& out, bool noDoc, bool noSuffix, bool wantAttrs, bool printAltSnippets);
        static TString FieldName(const char* name, const TStringBuf& postfix);
        static void Print(const TPassageReply& res, IOutputStream& out, const TStringBuf& postfix, bool wantAttrs, bool wantAltSnippets);
        static void PrintNoRecurse(const TPassageReply& res, IOutputStream& out, const TStringBuf& postfix, bool wantAttrs);
        void Process(const TJob& job) override;
    };

}
