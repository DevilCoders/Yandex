#include "request_iterator.h"

#include <antirobot/idl/antirobot.ev.pb.h>

#include <library/cpp/eventlog/iterator.h>

#include <util/stream/file.h>
#include <library/cpp/string_utils/quote/quote.h>


namespace {
    // Здесь содаётся пустой классификатор, который всегда отдаёт
    // REQ_OTHER на DetectRequestType
    // false на IsMobileRequest
    const NAntiRobot::TRequestClassifier classifier;
}

class TEventLogIterator : public IRequestIterator {
public:
    TEventLogIterator(const TString& fileName) {
        using namespace NEventLog;

        NEventLog::TOptions opt;
        if (fileName != "-") {
            opt.SetFileName(fileName);
        }

        EvLogIterator = NEventLog::CreateIterator(opt);
    }

    THolder<NAntiRobot::TRequest> Next() override {
        while (const auto event = EvLogIterator->Next()) {
            if (!event) {
                return nullptr;
            }

            if (event->Class != NAntirobotEvClass::TRequestData::ID) {
                continue;
            }

            const NAntirobotEvClass::TRequestData* reqData = event->Get<NAntirobotEvClass::TRequestData>();

            return NAntiRobot::CreateDummyParsedRequest(reqData->GetData(), classifier);
        }

        return nullptr;
    }

private:
    THolder<NEventLog::IIterator> EvLogIterator;
};


TAutoPtr<IRequestIterator> CreateEventLogIterator(const TString& fileName) {
    return new TEventLogIterator(fileName);
}

class TTextLogIterator : public IRequestIterator {
public:
    TTextLogIterator(const TString& fileName)
        : Input(fileName)
    {
    }

    THolder<NAntiRobot::TRequest> Next() override {
        TString str;
        size_t size = Input.ReadLine(str);
        if (size == 0) {
            return nullptr;
        }

        UrlUnescape(str);
        return NAntiRobot::CreateDummyParsedRequest(str, classifier);
    }
private:
    TFileInput Input;
};

TAutoPtr<IRequestIterator> CreateTextLogIterator(const TString& fileName) {
    return new TTextLogIterator(fileName);
}
