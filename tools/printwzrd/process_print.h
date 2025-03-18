#pragma once

#include <library/cpp/eventlog/eventlog.h>

#include <util/generic/fwd.h>
#include <util/generic/map.h>
#include <util/generic/ptr.h>
#include <util/generic/strbuf.h>
#include <util/generic/vector.h>

class IRequestWizard;
class IRulesResults;
class ISourceRequests;
class IWizardResultsPrinter;
class TCgiParameters;
struct TPrintwzrdOptions;
struct TWizardResults;

namespace NPrintWzrd {

    void PrintFailInfo(IOutputStream& out, const TString& request, bool StripLineNumbers = false);

    TAutoPtr<TWizardResults> ProcessWizardRequest(const IRequestWizard* const wizard, const TString& req,
                                                  const TPrintwzrdOptions& options, TSelfFlushLogFramePtr frame, IWizardResultsPrinter* printer = nullptr);

    void ProcessSingleTest(const IRequestWizard* const wizard, const TPrintwzrdOptions& options, TEventLogPtr eventLog);

    void ProcessCompareTest(const IRequestWizard* const wizard1, const IRequestWizard* const wizard2,
                            const TPrintwzrdOptions& options, TEventLogPtr eventLog);
}
