#include "eventlog.h"
#include "unsorted.h"

#include <search/idl/events.ev.pb.h>

#include <library/cpp/eventlog/logparser.h>

#include <util/string/cast.h>
#include <util/stream/output.h>
#include <util/generic/ptr.h>
#include <util/generic/ylimits.h>
#include <util/generic/yexception.h>
#include <util/datetime/base.h>

TEventLogLoader::TEventLogLoader()
    : BeginDateTime(0)
    , EndDateTime(Max<ui64>())
{
}

TEventLogLoader::~TEventLogLoader() {
}

TString TEventLogLoader::Opts() {
    return "b:e:s:";
}

void TEventLogLoader::Usage() const {
    Cerr << "   -b lower bound events datetime (microseconds since Epoch)" << Endl
         << "   -e upper bound events datetime (microseconds since Epoch)" << Endl
         << "   -s string for substring filtering" << Endl;
}

bool TEventLogLoader::HandleOpt(const TOption* option) {
    switch (option->key) {
        case 'b': {
            if (!option->opt) {
                ythrow yexception() << "-b require argument";
            }

            BeginDateTime = FromString<ui64>(option->opt);

            return true;
        }

        case 'e': {
            if (!option->opt) {
                ythrow yexception() << "-e require argument";
            }

            EndDateTime = FromString<ui64>(option->opt);

            return true;
        }

        case 's': {
            if (!option->opt) {
                ythrow yexception() << "-s require argument";
            }

            SubstringFilter = option->opt;

            return true;
        }

        default:
            break;
    }

    return false;
}

void TEventLogLoader::Process(TParams* params) {
    IEventFactory* factory = NEvClass::Factory();
    TFrameStreamer frameStream(*params->Input(), factory);

    TIntrusivePtr<TEventFilter> filter(new TEventFilter(true));
    filter->AddEventClass(NEvClass::TSubSourceRequest::ID);

    TEventStreamer eventStream(frameStream, BeginDateTime, EndDateTime, true, filter);

    TMonotonicAdapter ma(params);

    bool substringFilter = !SubstringFilter.empty();

    while (eventStream.Avail()) {
        const TEvent& e = **eventStream;

        try {
            const NEvClass::TSubSourceRequest* event = e.Get<NEvClass::TSubSourceRequest>();

            if (!substringFilter || event->GetURL().find(SubstringFilter) != TString::npos) {
                TDevastateRequest request;
                request.Url = event->GetURL();
                request.Body = event->GetBody();
                ma.Add(TInstant::MicroSeconds(e.Timestamp), TDevastateItem(TDuration::Zero(), request, 0));
            }
        } catch (...) {
            Cerr << CurrentExceptionMessage() << " at event " << e.Timestamp << Endl;
        }

        eventStream.Next();
    }

    ma.Flush();
}
