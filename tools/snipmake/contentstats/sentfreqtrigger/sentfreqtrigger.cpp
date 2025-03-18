#include <yweb/structhtml/htmlstatslib/htmlstatslib.h>
#include <yweb/structhtml/htmlstatslib/runtime/zonestub.h>
#include <kernel/indexer/baseproc/doc_attr_filler.h>
#include <kernel/indexer/face/blob/directzoneinserter.h>
#include <kernel/indexer/face/blob/datacontainer.h>
#include <kernel/tarc/iface/tarcface.h>
#include <yweb/robot/kiwi/kwcalc/udflib/udflib.h>
#include <library/cpp/numerator/blob/numeratorevents.h>
#include <yweb/robot/kiwi_queries/robot/lib/indexingparams/returnvalueinserter.h>
#include <library/cpp/string_utils/url/url.h>
#include <util/stream/file.h>
#include <util/folder/path.h>
#include <util/generic/ptr.h>

using namespace NUdfLib;

class THashCollector: public NHtmlStats::ISentenceEater
{
public:
    using THashRec = std::pair<ui16, size_t>;
    using THashList = TVector<THashRec>;

    THashList Hashes;

    void OnSent(ui16 sentNum, TUtf16String& sent, size_t hash) override
    {
        Y_UNUSED(sent);

        Hashes.push_back(std::make_pair(sentNum, hash));
    }
};


class TSentFreqTrigger {
    NHtmlStats::THashGroups HashStats;
    NHtmlStats::TUrlGrouper Grouper;

public:
    explicit TSentFreqTrigger(const char* dataPath)
    {
        TFileInput inf(TFsPath(dataPath) / "learned_freqs.txt");
        NHtmlStats::LoadHashes(inf, HashStats);
        Grouper.LoadGroups(TFsPath(dataPath) / "mined_groups.txt");
    }

    int Run(const TParamList& input, TReturnValues& retVals) const {
        if (input.Size() != 3) {
            ythrow yexception() << "SentFreqTrigger: invalid number of parameters: " << input.Size() << ", expected 3";
        }

        const ISerializedValue* value = input[0]->As<ISerializedValue>();
        if (!value)
            return NKwTupleMeta::PARAMETER_NOT_FOUND;
        const TString url(value->Data(), value->Size());

        const TNumeratorEvents events(GetRawData(input[1].Get()));
        const ISerializedValue* zones = input[2]->As<ISerializedValue>();
        if (!zones)
            ythrow yexception() << "SentFreqTrigger: zone data not found";


        TString group = Grouper.GetGroupName(url);
        TSimpleSharedPtr<const NHtmlStats::THashGroup> groupStats;
        decltype(HashStats)::const_iterator ii = HashStats.find(group);
        if (ii == HashStats.end()) {
            groupStats.Reset(new NHtmlStats::THashGroup(group));
        }
        else {
            groupStats = ii->second;
        }
        NHtmlStats::TSentenceEvaluator evaluator(*groupStats);

        NHtmlStats::TParserOptions options;
        NHtmlStats::TParserCallback callback(&evaluator, options);
        callback.Encoding = events.GetDocProperties()->GetCharset();
        NStructuredHtml::THtml5HandlerWithNLP<NHtmlStats::TParserCallback> handler;
        handler.StartDoc(&callback);
        events.Numerate(handler, zones->Data(), zones->Size());
        handler.OnEndDoc();

        NIndexerCore::TDataContainer inserter;
        size_t nMarked = 0;
        for (const auto& hsh : evaluator.Sentences) {
            if (hsh.Freq < 0.8) {
                continue;
            }
            ++nMarked;
            TPosting posBegin, posEnd;
            SetPosting(posBegin, hsh.SentNum, TWordPosition::FIRST_CHILD);
            SetPosting(posEnd, hsh.SentNum + 1, TWordPosition::FIRST_CHILD);
            inserter.StoreZone(ToString(AZ_TEMPLATE_SENT), posBegin, posEnd, true);
            TUtf16String attrVal = UTF8ToWide(ToString(hsh.Freq));
            inserter.StoreArchiveZoneAttr(NArchiveZoneAttr::NHtmlStats::SENTHASH, attrVal.data(), attrVal.size(), posBegin);
        }
        TPosting urlPos;
        SetPosting(urlPos, 0, 0);
        inserter.StoreKey(Sprintf("#url=\"%s", CutHttpPrefix(TStringBuf(url)).data()).data(), urlPos);
        inserter.StoreTextArchiveDocAttr("sent_hash_db_size", ToString(groupStats->Hashes.size()));
        inserter.StoreTextArchiveDocAttr("template_sent_count", ToString(nMarked));

        retVals.Set(0, MakeValueNoCopy(inserter.SerializeToBuffer()));
        return 0;
    }
};

DECLARE_UDF(TSentFreqTrigger)
