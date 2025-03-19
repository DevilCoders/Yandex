#pragma once

#include <library/cpp/codecs/codecs.h>
#include <library/cpp/codecs/codecs_registry.h>

#include <kernel/tarc/iface/tarcface.h>

#include <library/cpp/offroad/tuple/tuple_writer.h>
#include <library/cpp/offroad/tuple/tuple_reader.h>
#include <library/cpp/offroad/tuple/tuple_sampler.h>
#include <library/cpp/offroad/codec/multi_table.h>

namespace NRepack {

template <class T, class Vectorizer, class Subtractor>
class TOffroadCodecBase : public NCodecs::ICodec {
protected:
    using TSampler = NOffroad::TTupleSampler<T, Vectorizer, Subtractor>;
    using TWriter = NOffroad::TTupleWriter<T, Vectorizer, Subtractor>;
    using TReader = NOffroad::TTupleReader<T, Vectorizer, Subtractor>;

    using TModel = typename TReader::TModel;
    using TTable = THolder<NOffroad::TMultiTable<typename TModel::TEncoderTable, typename TModel::TDecoderTable>>;

public:
    explicit TOffroadCodecBase() {
        MyTraits.NeedsTraining = true;
        MyTraits.RecommendedSampleSize = std::numeric_limits<ui32>::max(); // Take all sampling data

        // These don't matter in our case
        //MyTraits.SizeOnEncodeMultiplier = 1;
        //MyTraits.SizeOnEncodeAddition = 0;
        //MyTraits.SizeOnDecodeMultiplier = 1;

        // Not sure about these two, but they're currently not used anywhere  
        MyTraits.SizeOfInputElement = 1;
        MyTraits.AssumesStructuredInput = false; 
    }

private:
    bool DoTryToLearn(NCodecs::ISequenceReader& in) override {
        try {
            DoLearn(in);
        } catch (...) {
            return false;
        }
        return true;
    }
};

class TArchiveTextBlockSentInfoVectorizer {
public:
    enum {
        TupleSize = 3
    };

    template <class Slice>
    static void Scatter(const TArchiveTextBlockSentInfo& hit, Slice&& slice) {
        slice[0] = hit.OrigOffset;
        slice[1] = hit.EndOffset;
        slice[2] = hit.Flag;
    }

    template <class Slice>
    static void Gather(Slice&& slice, TArchiveTextBlockSentInfo* hit) {
        hit->OrigOffset = slice[0];
        hit->EndOffset = slice[1];
        hit->Flag = slice[2];
    }
};

struct TArchiveTextBlockSentInfoSubtractor {
    enum {
        TupleSize = 3,
        PrefixSize = 0,
    };

    template<class Storage>
    static void Integrate(Storage&&) {
    }

    template<class Value, class Delta, class Next>
    static void Integrate(Value&& value, Delta&& delta, Next&& next) {
        next[0] = value[0] + delta[0];
        next[1] = value[1] + delta[1];
        next[2] = delta[2];
    }

    template<class Value, class Delta, class Next>
    static void Differentiate(Value&& value, Next&& next, Delta&& delta) {
        delta[0] = next[0] - value[0];
        delta[1] = next[1] - value[1];
        delta[2] = next[2];
    }
};

class TOffroadSentInfoCodec : public TOffroadCodecBase<TArchiveTextBlockSentInfo, TArchiveTextBlockSentInfoVectorizer, TArchiveTextBlockSentInfoSubtractor> {
public:
    static TStringBuf MyName() { return Name_; }
    TString GetName() const override { return MyName().Data(); };

    ui8 Encode(TStringBuf in, TBuffer& out) const override;
    void Decode(TStringBuf in, TBuffer& out) const override;

private:
    void DoLearn(NCodecs::ISequenceReader& in) override;
    void Save(IOutputStream* out) const override;
    void Load(IInputStream* in) override;

    static TConstArrayRef<TArchiveTextBlockSentInfo> GetInfos(TStringBuf in);

    static inline TStringBuf Name_ = "offroad-sentinfo";

    TModel Model_;
    TTable Table_;
};

class TArchiveZoneSpanVectorizer {
public:
    enum {
        TupleSize = 4
    };

    template <class Slice>
    static void Scatter(const TArchiveZoneSpan& hit, Slice&& slice) {
        slice[0] = hit.SentBeg;
        slice[1] = hit.OffsetBeg;
        slice[2] = hit.SentEnd;
        slice[3] = hit.OffsetEnd;
    }

    template <class Slice>
    static void Gather(Slice&& slice, TArchiveZoneSpan* hit) {
        hit->SentBeg = slice[0];
        hit->OffsetBeg = slice[1];
        hit->SentEnd = slice[2];
        hit->OffsetEnd = slice[3];
    }
};

struct TArchiveZoneSpanSubtractor {
    enum {
        TupleSize = 4,
        PrefixSize = 0,
    };

    template<class Storage>
    static void Integrate(Storage&&) {
    }

    template<class Value, class Delta, class Next>
    static void Integrate(Value&& value, Delta&& delta, Next&& next) {
        next[0] = value[0] + delta[0];
        next[1] = delta[1];
        next[2] = value[2] + delta[2];
        next[3] = delta[3];
    }

    template<class Value, class Delta, class Next>
    static void Differentiate(Value&& value, Next&& next, Delta&& delta) {
        delta[0] = next[0] - value[0];
        delta[1] = next[1];
        delta[2] = next[2] - value[2];
        delta[3] = next[3];
    }
};

class TOffroadWeightZonesCodec : public TOffroadCodecBase<TArchiveZoneSpan, TArchiveZoneSpanVectorizer, TArchiveZoneSpanSubtractor> {
public:
    static TStringBuf MyName() { return Name_; }
    TString GetName() const override { return MyName().Data(); };

    ui8 Encode(TStringBuf in, TBuffer& out) const override;
    void Decode(TStringBuf in, TBuffer& out) const override;

private:
    void DoLearn(NCodecs::ISequenceReader& in) override;
    void Save(IOutputStream* out) const override;
    void Load(IInputStream* in) override;

    static void EncodeOneZone(const TArchiveZone& zone, IOutputStream* output, const TTable& tables);
    static void DecodeOneZone(IInputStream& in, TArchiveZone& zone, const TTable& tables);
    static void SampleOneZone(const TArchiveZone& zone, TSampler& sampler);

    static inline TStringBuf Name_ = "offroad-weight-zones";

    TModel LowZoneModel_;
    TModel HighZoneModel_;
    TModel BestZoneModel_;

    TTable LowZoneTable_;
    TTable HighZoneTable_;
    TTable BestZoneTable_;
};

inline void RegisterOffroadCodecs() {
    static bool isDone = false;
    if (!isDone){
        NCodecs::RegisterCodec<NRepack::TOffroadWeightZonesCodec>();
        NCodecs::RegisterCodec<NRepack::TOffroadSentInfoCodec>();
        isDone = true;
    }
}

}
