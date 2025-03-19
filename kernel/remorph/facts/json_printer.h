#pragma once

#include <kernel/remorph/tokenizer/callback.h>
#include "fact.h"

#include <library/cpp/json/json_value.h>
#include <library/cpp/json/json_writer.h>
#include <library/cpp/json/writer/json.h>
#include <library/cpp/protobuf/json/proto2json.h>

#include <util/generic/typetraits.h>
#include <util/stream/output.h>
#include <util/system/guard.h>
#include <util/system/mutex.h>

namespace NFact {

template <bool MultiThreaded>
class TJsonPrinter {
private:
    typedef std::conditional_t<MultiThreaded, TMutex, TFakeMutex> TPseudoMutex;
    typedef TVector<NFact::TFactPtr> TFacts;

private:
    TPseudoMutex Mutex;
    NJson::TJsonWriter JsonWriter;

public:
    explicit TJsonPrinter(IOutputStream& output)
        : Mutex()
        , JsonWriter(&output, true)
    {
        JsonWriter.OpenArray();
    }

    ~TJsonPrinter() {
        JsonWriter.CloseArray();
    }

    template <class TSymbolPtr>
    inline void operator()(const NToken::TSentenceInfo& sentenceInfo, const TVector<TSymbolPtr>& symbols,
                           const TFacts& facts) {
        Y_UNUSED(sentenceInfo);
        Y_UNUSED(symbols);

        TGuard<TPseudoMutex> guard(Mutex);
        Print(facts);
    }

private:
    inline void Print(const TFacts& facts) {
        for (TFacts::const_iterator fact = facts.begin(); fact != facts.end(); ++fact) {
            NJson::TJsonValue jsonFact;
            TAutoPtr<google::protobuf::Message> pbFact = (*fact)->ToMessage();
            NProtobufJson::Proto2Json(*pbFact, jsonFact);
            JsonWriter.Write(&jsonFact);
        }
        JsonWriter.Flush();
    }
};

} // NFact
