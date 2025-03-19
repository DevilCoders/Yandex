#pragma once

#include <kernel/text_machine/interface/hit.h>
#include <kernel/text_machine/proto/text_machine.pb.h>
#include <kernel/text_machine/util/hits_serializer.h>

#include <kernel/reqbundle/reqbundle.h>

#include <util/stream/output.h>
#include <util/generic/map.h>
#include <util/generic/deque.h>
#include <util/generic/list.h>
#include <util/generic/vector.h>
#include <util/generic/maybe.h>

class THitsMap {
public:
    struct TWord {
        TVector<const NTextMachine::TBlockHit*> Hits;
    };

    struct TSentence {
        TList<float> Values;
        TVector<TWord> Words;
    };

    struct TDoc {
        TMap<ui32, TSentence> Sentences;

        struct TStorage {
            TDeque<NTextMachine::TBlockHit> Hits;
            THolder<NTextMachine::THitsAuxData> AuxData;
        };

        TStorage Storage;
    };

    class IPrinter {
    public:
        virtual ~IPrinter() {}

        virtual void PrintWord(IOutputStream& out, const TWord& word) const = 0;
        virtual void PrintSentence(IOutputStream& out, const TSentence& sentence) const = 0;
        virtual void PrintDoc(IOutputStream& out, const TDoc& doc) const = 0;
    };

    enum class EPrinterType {
        Token,
        Word,
        BlockId,
        Expansion,
        RequestId,
        Stream,
        BreakId
    };

    static THolder<IPrinter> CreateTokenPrinter();
    static THolder<IPrinter> CreateBlockIdPrinter();
    static THolder<IPrinter> CreateStreamPrinter();
    static THolder<IPrinter> CreateBreakIdPrinter();
    static THolder<IPrinter> CreateWordPrinter(NReqBundle::TConstSequenceAcc seq);
    static THolder<IPrinter> CreateExpansionPrinter(NReqBundle::TConstReqBundleAcc bundle);
    static THolder<IPrinter> CreateRequestIdPrinter(NReqBundle::TConstReqBundleAcc bundle);

    static THolder<TDoc> CreateForDoc(
        const NTextMachineProtocol::TPbDocHits& hits,
        const TMaybe<NLingBoost::EExpansionType>& expansionType,
        const TMaybe<NLingBoost::EStreamType>& streamType);

    static THolder<TDoc> CreateForRequest(
        const NTextMachineProtocol::TPbDocHits& hits,
        const TMaybe<NLingBoost::EExpansionType>& expansionType,
        const TMaybe<NLingBoost::EStreamType>& streamType);
};
