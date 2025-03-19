#include <library/cpp/testing/unittest/registar.h>

#include <google/protobuf/text_format.h>

#include <kernel/reqbundle_iterator/cache.h>
#include <kernel/reqbundle_iterator/factory.h>
#include <kernel/reqbundle_iterator/form_index_assigner.h>
#include <kernel/reqbundle_iterator/index_offroad_accessor.h>
#include <kernel/reqbundle_iterator/index_yndex_accessor.h>
#include <kernel/reqbundle_iterator/iterator.h>
#include <kernel/reqbundle_iterator/iterator_impl.h>
#include <kernel/reqbundle_iterator/iterator_offroad_impl.h>
#include <kernel/reqbundle_iterator/iterator_yndex_impl.h>
#include <kernel/reqbundle_iterator/iterator_and_impl.h>
#include <kernel/reqbundle_iterator/iterator_ordered_and_impl.h>
#include <kernel/reqbundle_iterator/offroad_packed_key_converter.h>
#include <kernel/reqbundle_iterator/position.h>
#include <kernel/reqbundle_iterator/saveload.h>
#include <kernel/reqbundle_iterator/reqbundle_iterator_yndex_builder.h>
#include <kernel/reqbundle_iterator/reqbundle_iterator_offroad_builder.h>
#include <kernel/doom/wad/mega_wad_writer.h>
#include <kernel/doom/offroad_sent_wad/offroad_sent_wad_io.h>
#include <kernel/doom/standard_models_storage/standard_models_storage.h>
#include <kernel/reqbundle/serializer.h>

#include <library/cpp/blob_cache/paged_blob_hasher.h>
#include <kernel/keyinv/invkeypos/keyconv.h>

#include <kernel/doom/offroad_wad/offroad_ann_wad_io.h>

using namespace NReqBundle;
using namespace NReqBundleIterator;
using namespace NReqBundleIteratorImpl;

using TOffroadSearcher = NDoom::TOffroadAnnWadIoSortedMultiKeys::TSearcher;
using TOffroadKeySearcher = NDoom::TOffroadAnnWadIoSortedMultiKeys::TKeySearcher;
using TOffroadHitSearcher = NDoom::TOffroadAnnWadIoSortedMultiKeys::THitSearcher;
using TOffroadIterator = typename TOffroadSearcher::TIterator;
using TOffroadSampler = NDoom::TOffroadAnnWadIoSortedMultiKeys::TUniSampler;
using TOffroadWriter = NDoom::TOffroadAnnWadIoSortedMultiKeys::TWriter;

namespace {

    const char protoIz[] =
"Words {\
    Lemmas {\
        Text: \"из\"\
        Language: 1\
        Forms { Text: \"из\" Exact: true }\
    }\
    Text: \"из\"\
    Languages { Bits: 1 }\
    NlpType: 1\
    StopWord: true\
}";

    const char protoOkna[] =
"Words {\
    Lemmas {\
        Text: \"окно\"\
        Language: 1\
        Forms { Text: \"окнами\" }\
        Forms { Text: \"окне\" }\
        Forms { Text: \"окон\" }\
        Forms { Text: \"окну\" }\
        Forms { Text: \"окно\" }\
        Forms { Text: \"окна\" Exact: true }\
        Forms { Text: \"окном\" }\
        Forms { Text: \"окнам\" }\
        Forms { Text: \"окнах\" }\
    }\
    Text: \"окна\"\
    Languages { Bits: 1 }\
    NlpType: 1\
    StopWord: false\
}";

    const char protoOknami[] =
"Words {\
    Lemmas {\
        Text: \"окно\"\
        Language: 1\
        Forms { Text: \"окнами\" Exact: true }\
        Forms { Text: \"окне\" }\
        Forms { Text: \"окон\" }\
        Forms { Text: \"окну\" }\
        Forms { Text: \"окно\" }\
        Forms { Text: \"окна\" }\
        Forms { Text: \"окном\" }\
        Forms { Text: \"окнам\" }\
        Forms { Text: \"окнах\" }\
    }\
    Text: \"окнами\"\
    Languages { Bits: 1 }\
    NlpType: 1\
    StopWord: false\
}";

    const char protoDulo[] =
"Words {\
    Lemmas {\
        Text: \"дуть\"\
        Best: false\
        Language: 1\
        Forms { Text: \"дуло\" Exact: true }\
    }\
    Lemmas {\
        Text: \"дуло\"\
        Best: true\
        Language: 1\
        Forms { Text: \"дулом\" }\
        Forms { Text: \"дуле\" }\
        Forms { Text: \"дула\" }\
        Forms { Text: \"дул\" }\
        Forms { Text: \"дуло\" Exact: true }\
        Forms { Text: \"дулу\" }\
        Forms { Text: \"дулах\" }\
        Forms { Text: \"дулами\" }\
        Forms { Text: \"дулам\" }\
    }\
    Text: \"дуло\"\
    Languages { Bits: 1 }\
    NlpType: 1\
    StopWord: false\
}";

    const char protoAnyWord[] =
"Words {\
    AnyWord: true\
}";

    /*
    чего мы хотим от словопозиций:
    - четыре документа (docId 3;5;8;13),
    - первый - для тестов вычитывания словопозиций из одного документа,
    - второй и третий - для тестов Lookup,
    - последний - для тестов AND
    - первый документ:
    - каждое слово - в отдельном предложении
    - две леммы с пересекающимися формами, тест склейки
    - два ключа для одной леммы, тест объединения
    - разные словоформы, тест фильтрации словоформ
    - второй и третий документы:
    - только одно из слов, зато в двух разных формах с разными ключами
    - LookupNextDoc с затуханием должен возвращать второй документ, но не третий
    - последний документ: запросы AND(из окна дуло) и AND(из окна из дуло)
    - два предложения, первое без точного порядка слов, второе с точным порядком
    - первое предложение: два пересекающихся вхождения, перед первым вхождением неполное вхождение
    - первое предложение: из окна [всякая фигня] из окна окно из дула из окна
    - второе предложение (break = 8): из окна из окна дуло [всякая фигня] из из окна из дуло
    - третье предложение (break = 9): дуло окнами окнами окнами
    - четвёртое предложение (break = 10): окнами дуло
    - не стоит искать глубокий смысл в наборах слов из двух строчек выше
    */
    struct TOneKeyInfo
    {
        const char* Utf8Lemma;
        ELanguage Lang;
        TVector<const char*> Utf8Forms;
        TVector<TWordPosition> Hits;
    };
    const TOneKeyInfo testKeys[] = {
        // TWordPosition(doc, break, word, relevLevel, nform)
        {"из", LANG_RUS, { "из" }, {
            TWordPosition(3, 2, 1, MID_RELEV, 0),
            TWordPosition(13, 4, 1, MID_RELEV, 0),
            TWordPosition(13, 4, 10, MID_RELEV, 0),
            TWordPosition(13, 4, 13, MID_RELEV, 0),
            TWordPosition(13, 4, 15, MID_RELEV, 0),
            TWordPosition(13, 8, 1, MID_RELEV, 0),
            TWordPosition(13, 8, 3, MID_RELEV, 0),
            TWordPosition(13, 8, 10, MID_RELEV, 0),
            TWordPosition(13, 8, 11, MID_RELEV, 0),
            TWordPosition(13, 8, 13, MID_RELEV, 0),
        }},
        {"окно", LANG_RUS, { "окна", "окно", "окну" }, {
            TWordPosition(3, 1, 1, MID_RELEV, 0),
            TWordPosition(3, 3, 9, HIGH_RELEV, 1),
            TWordPosition(3, 7, 2, MID_RELEV, 2),
            TWordPosition(5, 2, 42, MID_RELEV, 1),
            TWordPosition(8, 42, 2, MID_RELEV, 1),
            TWordPosition(13, 4, 2, MID_RELEV, 0),
            TWordPosition(13, 4, 11, MID_RELEV, 0),
            TWordPosition(13, 4, 12, MID_RELEV, 1),
            TWordPosition(13, 4, 15, MID_RELEV, 0),
            TWordPosition(13, 8, 2, MID_RELEV, 0),
            TWordPosition(13, 8, 4, MID_RELEV, 0),
            TWordPosition(13, 8, 12, MID_RELEV, 0),
            TWordPosition(13, 10, 2, MID_RELEV, 1),
        }},
        {"окно", LANG_RUS, { "окнами", "окнах", "окном" }, {
            TWordPosition(3, 4, 12, HIGH_RELEV, 0),
            TWordPosition(3, 6, 44, MID_RELEV, 1),
            TWordPosition(3, 9, 11, LOW_RELEV, 2),
            TWordPosition(5, 2, 24, MID_RELEV, 1),
            TWordPosition(8, 24, 2, MID_RELEV, 1),
            TWordPosition(13, 9, 2, MID_RELEV, 0),
            TWordPosition(13, 9, 3, MID_RELEV, 0),
            TWordPosition(13, 9, 4, MID_RELEV, 0),
            TWordPosition(13, 10, 1, MID_RELEV, 0),
        }},
        {"дуло", LANG_RUS, { "дула", "дуло", "дулу" }, {
            TWordPosition(3, 5, 1, MID_RELEV, 0),
            TWordPosition(3, 8, 2, MID_RELEV, 1),
            TWordPosition(3, 10, 3, MID_RELEV, 2),
            TWordPosition(13, 4, 14, HIGH_RELEV, 0),
            TWordPosition(13, 8, 5, HIGH_RELEV, 1),
            TWordPosition(13, 8, 14, HIGH_RELEV, 1),
            TWordPosition(13, 9, 1, HIGH_RELEV, 1),
            TWordPosition(13, 10, 2, HIGH_RELEV, 1),
        }},
        {"дуть", LANG_RUS, { "дула", "дуло", "дуть" }, {
            TWordPosition(3, 5, 1, MID_RELEV, 0),
            TWordPosition(3, 8, 2, MID_RELEV, 1),
            TWordPosition(3, 11, 4, MID_RELEV, 2),
            TWordPosition(13, 4, 14, HIGH_RELEV, 0),
            TWordPosition(13, 8, 5, HIGH_RELEV, 1),
            TWordPosition(13, 8, 14, HIGH_RELEV, 1),
            TWordPosition(13, 9, 1, HIGH_RELEV, 1),
            TWordPosition(13, 10, 2, HIGH_RELEV, 1),
        }},
    };

    ui8 sentLens[] = { 0, 0, 0, 0, 15, 0, 0, 0, 14, 4, 2 };

    const char* Utf8ToKey(TFormToKeyConvertor& conv, TStringBuf str) {
        TUtf16String w = UTF8ToWide(str);
        return conv.Convert(w.data(), w.size());
    }

    TString ConstructKeyForKeyInvIndex(const TOneKeyInfo& info) {
        char keyBuf[MAXKEY_BUF];
        TFormToKeyConvertor conv;
        TKeyLemmaInfo lemmaInfo;
        strcpy(lemmaInfo.szLemma, Utf8ToKey(conv, info.Utf8Lemma));
        lemmaInfo.Lang = (ui8)info.Lang;
        TMemoryPool pool(1 << 8);
        TVector<const char*> formKeys;
        for (const char* form : info.Utf8Forms)
            formKeys.push_back(pool.Append(Utf8ToKey(conv, form)));
        ConstructKeyWithForms(keyBuf, sizeof(keyBuf), lemmaInfo, formKeys.size(), formKeys.data());
        return TString(keyBuf);
    }

    TBlockPtr BlockFromProto(const TString& protoString)
    {
        TBlockPtr block = new TBlock;
        NReqBundleProtocol::TBlock protoData;
        google::protobuf::TextFormat::ParseFromString(protoString, &protoData);
        NSer::TDeserializer().DeserializeProto(protoData, *block);
        return block;
    }

    void CheckIteratorsEqual(TIterator* a, TIterator* b) {
        UNIT_ASSERT_EQUAL(a->BlockType, b->BlockType);
        UNIT_ASSERT_EQUAL(a->BlockId, b->BlockId);
        UNIT_ASSERT_EQUAL(a->GetSortOrder().TermId, b->GetSortOrder().TermId);
        UNIT_ASSERT_EQUAL(a->GetSortOrder().TermRange, b->GetSortOrder().TermRange);
        UNIT_ASSERT_EQUAL(typeid(*a), typeid(*b));

        if (dynamic_cast<TIteratorOffroadBase*>(a) != nullptr) {
            TPositionTemplates& aTemplates = dynamic_cast<TIteratorOffroadBase*>(a)->GetTemplatesToFill();
            TPositionTemplates& bTemplates = dynamic_cast<TIteratorOffroadBase*>(b)->GetTemplatesToFill();

            UNIT_ASSERT_EQUAL(aTemplates.size(), bTemplates.size());
            for (size_t i = 0; i < aTemplates.size(); ++i) {
                UNIT_ASSERT_EQUAL(aTemplates[i], bTemplates[i]);
            }
        }

        // TODO: Check subiterators in And and OrderedAnd iterators (need a way to access them)


        while (a->Current().Valid() && b->Current().Valid()) {
            UNIT_ASSERT_EQUAL(a->Current(), b->Current());
            a->Next();
            b->Next();
        }
        UNIT_ASSERT(!a->Current().Valid());
        UNIT_ASSERT(!b->Current().Valid());
    }
}

Y_UNIT_TEST_SUITE(YndexIteratorsTest) {

    class TTestKeysAndPositions : public IKeysAndPositions
    {
    private:
        struct TKeyData {
            TString Key;
            size_t NumHits = 0;
            TVector<char> Inv;
            bool operator<(const TKeyData& r) const {
                return Key < r.Key;
            }
            bool operator<(const char* word) const {
                return Key < TStringBuf(word);
            }
        };
        TVector<TKeyData> Keys;
        TIndexInfo IndexInfo;

    public:
        void GetBlob(TBlob& data, i64 offset, ui32, READ_HITS_TYPE) const override {
            const TKeyData& info = Keys[offset];
            data = TBlob::NoCopy(info.Inv.data(), info.Inv.size());
        }
        const YxRecord* EntryByNumber(TRequestContext& rc, i32 num, i32&) const override {
            if (num < 0 || static_cast<ui32>(num) >= Keys.size())
                return nullptr;
            const TKeyData& info = Keys[num];
            YxRecord& rec = rc.GetRecord();
            memcpy(rec.TextPointer, info.Key.Data(), info.Key.Size());
            rec.TextPointer[info.Key.Size()] = 0;
            rec.Length = info.Inv.size();
            rec.Offset = num;
            rec.Counter = info.NumHits;
            return &rec;
        }
        i32 LowerBound(const char* word, TRequestContext&) const override {
            return ::LowerBound(Keys.begin(), Keys.end(), word) - Keys.begin();
        }
        const TIndexInfo& GetIndexInfo() const override {
            return IndexInfo;
        }

        TTestKeysAndPositions() {
            IndexInfo.Version = YNDEX_VERSION_BLK8;
            IndexInfo.SubIndexInfo = TSubIndexInfo::NullSubIndex;
            IndexInfo.StrictHitsOrder = false;
            for (const TOneKeyInfo& info : testKeys) {
                Keys.emplace_back();
                Keys.back().Key = ConstructKeyForKeyInvIndex(info);
                Keys.back().NumHits = info.Hits.size();
                char hitsBuf[1024];
                size_t hitsBufWritten = 0;
                CHitCoder hitCoder(HIT_FMT_BLK8);
                for (TWordPosition hit : info.Hits)
                    hitsBufWritten += hitCoder.Output(hit.SuperLong(), hitsBuf + hitsBufWritten);
                hitsBufWritten += hitCoder.Finish(hitsBuf + hitsBufWritten);
                Keys.back().Inv.assign(hitsBuf, hitsBuf + hitsBufWritten);
            }
            Sort(Keys.begin(), Keys.end());
        }
    };

    class TTestYndexRequester : public TYndexRequester
    {
    public:
        TTestYndexRequester() {
            Yndex4Searching = new TTestKeysAndPositions;
        }
    };


    struct TTestSentIndexData {
        TBufferStream SentIndexData;
        THolder<NDoom::IWad> SentWad;
        THolder<TSentenceLengthsReader> SentenceLengthsReader;

        TTestSentIndexData() {
            using TWriter = typename NDoom::TOffroadSentWadIo::TWriter;
            using THitModel = typename NDoom::TOffroadSentWadIo::TWriter::TModel;
            using THit = typename NDoom::TOffroadSentWadIo::TWriter::THit;
            THitModel model = NDoom::TStandardIoModelsStorage::Model<THitModel>(NDoom::EStandardIoModel::DefaultSentIoModel);
            NDoom::TMegaWadWriter megaWadWriter(&SentIndexData);
            TWriter writer(model, &megaWadWriter);
            for (size_t i = 0; i < sizeof(sentLens); i++) {
                THit hit(13, sentLens[i]);
                writer.WriteHit(hit);
            }
            writer.WriteDoc(13);
            writer.Finish();
            megaWadWriter.Finish();
            SentWad = NDoom::IWad::Open(TArrayRef<const char>(SentIndexData.Buffer().data(), SentIndexData.Buffer().size()));
            SentenceLengthsReader = MakeHolder<TSentenceLengthsReader>(SentWad.Get(), SentWad.Get(), NDoom::SentIndexType);
        }
    };

    THolder<NDoom::IWad> GenerateOffroadTestData() {
        TBufferStream indexData;
        TOffroadSampler sampler;
        auto models = sampler.Finish();
        TOffroadWriter indexWriter(models.first, models.second, &indexData);
        TVector<std::pair<TString, const TVector<TWordPosition>*>> sortedKeys;
        for (const TOneKeyInfo& info : testKeys) {
            sortedKeys.emplace_back(ConstructKeyForKeyInvIndex(info), &info.Hits);
        }
        Sort(sortedKeys);
        for (const auto& info : sortedKeys) {
            for (TWordPosition pos : *info.second)
                indexWriter.WriteHit(NDoom::TReqBundleHit::FromSuperLong(pos.SuperLong()));
            indexWriter.WriteKey(info.first);
        }
        indexWriter.Finish();
        return NDoom::IWad::Open(std::move(indexData.Buffer()));
    }

    class TOffroadTestData
    {
    private:
        THolder<NDoom::IWad> Wad;
        THolder<TOffroadSearcher> Searcher;
        THolder<TOffroadSharedIteratorData<TOffroadSearcher>> SharedData;
        TTestSentIndexData TestIndexSent;

    public:
        TOffroadTestData()
        {
            Wad = GenerateOffroadTestData();
            Searcher = MakeHolder<TOffroadSearcher>(Wad.Get());
            SharedData = MakeHolder<TOffroadSharedIteratorData<TOffroadSearcher>>(*Searcher, MakeHolder<TOffroadIterator>(), TestIndexSent.SentenceLengthsReader.Get());
        }

        TOffroadSharedIteratorData<TOffroadSearcher>& GetSharedData() {
            return *SharedData;
        }
    };

    class TOffroadTestDataForTwoPhaseOpening {
    public:
        TOffroadTestDataForTwoPhaseOpening() {
            THolder<NDoom::IWad> singleWad = GenerateOffroadTestData();
            InitFromWad(*singleWad);
        }

        TOffroadKeySearcher& FirstPhaseSearcher() {
            return *GlobalPartSearcher;
        }

        TOffroadHitSearcher& SecondPhaseSearcher() {
            return *DocumentPartSearcher;
        }
    private:
        // Splits a single wad containing an inverted index into the global and document parts
        void InitFromWad(const NDoom::IWad& wad) {
            // Global lumps go into one wad
            TBufferStream globalIndexData;
            NDoom::TMegaWadWriter globalWriter(&globalIndexData);
            TVector<TString> globalLumpNames = wad.GlobalLumpsNames();
            for (const auto& lumpName : globalLumpNames) {
                IOutputStream* stream = globalWriter.StartGlobalLump(lumpName);
                TBlob lump = wad.LoadGlobalLump(lumpName);
                stream->Write(lump.Data(), lump.Size());
            }
            globalWriter.Finish();

            // Doc lumps go into another wad (+ the hits model)
            TBufferStream docIndexData;
            NDoom::TMegaWadWriter docWriter(&docIndexData);
            {
                static constexpr auto IndexType = NDoom::TOffroadAnnWadIoSortedMultiKeys::IndexType;
                NDoom::TWadLumpId lumpId(IndexType, NDoom::EWadLumpRole::HitsModel);
                IOutputStream* stream = docWriter.StartGlobalLump(lumpId);
                TBlob lump = wad.LoadGlobalLump(lumpId);
                stream->Write(lump.Data(), lump.Size());
            }
            TVector<NDoom::TWadLumpId> docLumpNames = wad.DocLumps();
            for (const auto& docLump : docLumpNames) {
                docWriter.RegisterDocLumpType(docLump);
            }
            TVector<size_t> mapping(docLumpNames.size());
            wad.MapDocLumps(docLumpNames, mapping);
            for (ui32 docId : xrange(wad.Size())) {
                TVector<TConstArrayRef<char>> regions(docLumpNames.size());
                TBlob holder = wad.LoadDocLumps(docId, mapping, regions);
                for (size_t i : xrange(docLumpNames.size())) {
                    const auto& name = docLumpNames[i];
                    const auto& region = regions[i];
                    IOutputStream* stream = docWriter.StartDocLump(docId, name);
                    stream->Write(region.data(), region.size());
                }
            }
            docWriter.Finish();

            DocumentPartWad = MakeHolder<NDoom::TSingleChunkedWad>(NDoom::IWad::Open(std::move(docIndexData.Buffer())));
            DocumentPartSearcher = MakeHolder<TOffroadHitSearcher>(DocumentPartWad.Get());

            NDoom::TKeyInvPortionsKeyRanges ranges({ "" });
            TVector<THolder<NDoom::IWad>> wadRanges;
            wadRanges.push_back(NDoom::IWad::Open(std::move(globalIndexData.Buffer())));
            GlobalPartSearcher = MakeHolder<TOffroadKeySearcher>(std::move(ranges), std::move(wadRanges));

        }
    private:
        THolder<TOffroadKeySearcher> GlobalPartSearcher;
        THolder<NDoom::IChunkedWad> DocumentPartWad;
        THolder<TOffroadHitSearcher> DocumentPartSearcher;

    };

    struct TExpectedLowLevelPosition
    {
        ui32 Brk, WordPosBeg, WordPosEnd, BlockId, LemmId, LowLevelFormId;
        EFormClass FormClass;
        RelevLevel Relev;
    };

    struct TYndexIteratorBuilder
    {
    private:
        TTestYndexRequester YR;
        //TTestKeysAndPositions Data;
        THolder<TYndexIndexAccessor> IndexAccessor;
        THolder<IReqBundleIteratorBuilder> Builder;
        TTestSentIndexData TestIndexSent;
        TSharedIteratorData SharedData;
    public:
        TYndexIteratorBuilder()
            : SharedData(TestIndexSent.SentenceLengthsReader.Get())
        {
        }

        IIndexAccessor* CreateIndexAccessor() {
            IndexAccessor = MakeHolder<TYndexIndexAccessor>(YR, IHitsAccess::IDX_TEXT, TLangMask(LANG_RUS, LANG_ENG), TLangMask());
            return IndexAccessor.Get();
        }
        TSharedIteratorData& GetSharedData() {
            return SharedData;
        }
        IReqBundleIteratorBuilder& CreateBuilder() {
            Builder = MakeHolder<TReqBundleYndexIteratorBuilder>(YR, IHitsAccess::IDX_TEXT, TLangMask(LANG_RUS, LANG_ENG), TLangMask());
            return *Builder;
        }
        TRBIteratorPtr CreateRBIterator(TConstReqBundleAcc bundle, IRBIteratorsHasher* hasher, const TRBIteratorOptions& options = {}) {
            TReqBundleYndexIteratorBuilder builder(YR, IHitsAccess::IDX_TEXT, TLangMask(LANG_RUS, LANG_ENG), TLangMask());
            return TRBIteratorsFactory(bundle).OpenIterator(builder, hasher, options);
        }
        using SupportsNextDoc = std::true_type;
    };

    struct TOffroadIteratorBuilder
    {
    private:
        THolder<TMemoryPool> Pool;
        TOffroadTestData Index;
        THolder<TOffroadIndexAccessor<TOffroadSearcher, TKeyToOffroadPackedKeyConverter>> IndexAccessor;
        THolder<IReqBundleIteratorBuilder> Builder;
    public:
        IIndexAccessor* CreateIndexAccessor() {
            Pool = MakeHolder<TMemoryPool>(1 << 16);
            IndexAccessor = MakeHolder<TOffroadIndexAccessor<TOffroadSearcher, TKeyToOffroadPackedKeyConverter>>(Index.GetSharedData().OffroadSearcher, *Index.GetSharedData().OffroadIterator, *Pool);
            return IndexAccessor.Get();
        }
        TOffroadSharedIteratorData<TOffroadSearcher>& GetSharedData() {
            return Index.GetSharedData();
        }
        IReqBundleIteratorBuilder& CreateBuilder() {
            Builder = MakeHolder<TReqBundleOffroadIteratorBuilder<TOffroadSearcher, TKeyToOffroadPackedKeyConverter, true>>(Index.GetSharedData().OffroadSearcher);
            return *Builder;
        }
        TRBIteratorPtr CreateRBIterator(TConstReqBundleAcc bundle, IRBIteratorsHasher* hasher, const TRBIteratorOptions& options = {}) {
            TReqBundleOffroadIteratorBuilder<TOffroadSearcher, TKeyToOffroadPackedKeyConverter, true> builder(Index.GetSharedData().OffroadSearcher);
            return TRBIteratorsFactory(bundle).OpenIterator(builder, hasher, options);
        }
        using SupportsNextDoc = std::false_type;
    };

    struct TOffroadTwoPhaseIteratorBuilder {
    public:
        TRBIteratorPtr CreateRBIterator(TConstReqBundleAcc bundle, IRBIteratorsHasher* hasher, const TRBIteratorOptions& options = {}) {
            Y_UNUSED(hasher, options);
            using TGlobalBuilder = TReqBundleOffroadIteratorBuilderGlobal<TOffroadKeySearcher, TKeyToOffroadPackedKeyConverter, true>;
            auto globalBuilder = MakeHolder<TGlobalBuilder>(Index.FirstPhaseSearcher());

            TRBIteratorsFactory factory(bundle, {});
            const TString serialized = factory.PreOpenIterator(*globalBuilder);

            using TSharedDataFactory = TReqBundleOffroadSharedDataFactory<TOffroadHitSearcher>;
            using TIteratorDeserializer = TReqBundleOffroadIteratorDeserializer<TOffroadHitSearcher>;
            auto sharedDataFactory = MakeHolder<TSharedDataFactory>(Index.SecondPhaseSearcher());
            auto deserializer = MakeHolder<TIteratorDeserializer>();
            return factory.OpenIterator(serialized, *sharedDataFactory, *deserializer);
        }
        using SupportsNextDoc = std::false_type;
    private:
        TOffroadTestDataForTwoPhaseOpening Index;
    };

    static void InitSingleIterator(TIterator& iterator, ui32 docId, const TIteratorOptions& options, TSharedIteratorData& data)
    {
        data.HitsStorage.Reset();
        //data.BrkBuffer.Clear();
        data.SharedSentenceLengths.Reset();
        iterator.InitForDoc(docId, true, options, data);
        iterator.InitForDoc(docId, false, options, data);
    }

    static void AssertIteratorPositions(TIterator& iterator, const TVector<TExpectedLowLevelPosition>& expected)
    {
        for (auto& pos : expected) {
            UNIT_ASSERT(iterator.Current().Valid());
            UNIT_ASSERT_EQUAL(iterator.Current().Break(), pos.Brk);
            UNIT_ASSERT_EQUAL(iterator.Current().WordPosBeg(), pos.WordPosBeg);
            UNIT_ASSERT_EQUAL(iterator.Current().WordPosEnd(), pos.WordPosEnd);
            UNIT_ASSERT_EQUAL(iterator.Current().BlockId(), pos.BlockId);
            UNIT_ASSERT_EQUAL(iterator.Current().LemmId(), pos.LemmId);
            UNIT_ASSERT_EQUAL(iterator.Current().LowLevelFormId(), pos.LowLevelFormId);
            UNIT_ASSERT_EQUAL(iterator.Current().Match(), pos.FormClass);
            UNIT_ASSERT_EQUAL(iterator.Current().Relev(), (ui32)pos.Relev);
            iterator.Next();
        }
        UNIT_ASSERT(!iterator.Current().Valid());
    }

    void FillTemplatesToTestBasicIterator(TPositionTemplates& templates)
    {
        templates.resize(N_MAX_FORMS_PER_KISHKA, TPosition::Invalid);
        templates[2].Clear();
        templates[2].TLemmId::SetClean(6);
        templates[2].TLowLevelFormId::SetClean(33);
        templates[2].TMatch::SetClean(EQUAL_BY_LEMMA);
    }

    void TestBasicIterator(
        TIterator* iterator,
        IReqBundleIteratorBuilder& builder,
        const TIteratorOptions& options,
        TSharedIteratorData& data,
        bool supportLookup,
        bool protoSerializationSupported)
    {
        TBufferStream saveStream;
        TIteratorSaveLoad::SaveIterator(&saveStream, *iterator);

        NReqBundleIteratorProto::TBlockIterator proto;
        if (protoSerializationSupported) {
            iterator->SerializeToProto(&proto);
        }

        InitSingleIterator(*iterator, 1, options, data);
        AssertIteratorPositions(*iterator, {});
        ui32 nextDocId = Max<ui32>();
        if (supportLookup) {
            iterator->LookupNextDoc(nextDocId);
            UNIT_ASSERT_EQUAL(nextDocId, 3);
        }
        InitSingleIterator(*iterator, 3, options, data);
        AssertIteratorPositions(*iterator, { { 11, 4, 4, 42, 6, 33, EQUAL_BY_LEMMA, MID_RELEV } });
        if (supportLookup) {
            nextDocId = Max<ui32>();
            iterator->LookupNextDoc(nextDocId);
            UNIT_ASSERT_EQUAL(nextDocId, Max<ui32>());
        }
        TMemoryPool reloadPool(1 << 16);
        TIteratorPtr reloadIterator = TIteratorSaveLoad::LoadIterator(&saveStream, 43, reloadPool, builder); // it must be possible to change blockId
        InitSingleIterator(*reloadIterator, 1, options, data);
        AssertIteratorPositions(*reloadIterator, {});
        InitSingleIterator(*reloadIterator, 3, options, data);
        AssertIteratorPositions(*reloadIterator, { { 11, 4, 4, 43, 6, 33, EQUAL_BY_LEMMA, MID_RELEV } });
        InitSingleIterator(*reloadIterator, 13, options, data);
        AssertIteratorPositions(*reloadIterator, {});

        if (protoSerializationSupported) {
            TReqBundleOffroadIteratorDeserializer<TOffroadSearcher> deserializer;
            TIteratorPtr iter2 = deserializer.DeserializeFromProto(proto, reloadPool);

            InitSingleIterator(*iterator, 3, options, data);
            InitSingleIterator(*iter2, 3, options, data);
            CheckIteratorsEqual(iterator, iter2.Get());

            InitSingleIterator(*iterator, 13, options, data);
            InitSingleIterator(*iter2, 13, options, data);
            CheckIteratorsEqual(iterator, iter2.Get());
        }
    }

    Y_UNIT_TEST(TestBasicIteratorYndex)
    {
        TMemoryPool pool(1 << 16);
        TRequestContext rc;
        TYndexIteratorBuilder builder;
        i32 ignored;
        TBlob hitsBlob;
        TFormToKeyConvertor conv;
        TTestKeysAndPositions yr;
        const YxRecord* dulo = yr.EntryByNumber(rc, yr.LowerBound(Utf8ToKey(conv, "дуть"), rc), ignored);
        THitsForRead hitsForRead;
        hitsForRead.ReadHits(yr, dulo->Offset, dulo->Length, dulo->Counter, RH_DEFAULT);
        THolder<TIteratorYndexBase, TDestructor> iterator = MakeIteratorYndex(DefaultIteratorType, 42, pool, hitsForRead);
        FillTemplatesToTestBasicIterator(iterator->GetTemplatesToFill());
        TSharedIteratorData data;
        TIteratorOptions options;
        TestBasicIterator(iterator.Get(), builder.CreateBuilder(), options, data, true, false);
    }

    Y_UNIT_TEST(TestBasicIteratorOffroad)
    {
        TMemoryPool pool(1 << 16);
        TOffroadIteratorBuilder builder;
        TFormToKeyConvertor conv;
        std::vector<NDoom::TOffroadWadKey> keys;
        builder.GetSharedData().OffroadSearcher.FindTerms("дуть", builder.GetSharedData().OffroadIterator.Get(), &keys);
        THolder<TIteratorOffroadBase, TDestructor> iterator = MakeIteratorOffroad<TOffroadSearcher>(DefaultIteratorType, 42, pool, keys.at(0).Id);
        FillTemplatesToTestBasicIterator(iterator->GetTemplatesToFill());
        TIteratorOptions options;
        TestBasicIterator(iterator.Get(), builder.CreateBuilder(), options, builder.GetSharedData(), false, true);
    }

    static void CheckAndIterator(TIterator* iterator, TVector<TExpectedLowLevelPosition> expected, bool lookupSupported, IReqBundleIteratorBuilder& builder, const TIteratorOptions& options, TSharedIteratorData& sharedData, bool protoSerializationSupported)
    {
        TBufferStream saveStream;
        TIteratorSaveLoad::SaveIterator(&saveStream, *iterator);

        NReqBundleIteratorProto::TBlockIterator proto;
        if (protoSerializationSupported) {
            iterator->SerializeToProto(&proto);
        }

        InitSingleIterator(*iterator, 1, options, sharedData);
        AssertIteratorPositions(*iterator, {});
        ui32 nextDocId = Max<ui32>();
        if (lookupSupported) {
            iterator->LookupNextDoc(nextDocId);
            // LookupNextDoc for AND iterator is permitted to return documents where only subset of words is present
            UNIT_ASSERT(nextDocId >= 3 && nextDocId <= 13);
            // however, it must be possible to call Get*DocumentPositions* for documents before return value of LookupNextDoc
        }
        InitSingleIterator(*iterator, 3, options, sharedData);
        AssertIteratorPositions(*iterator, {});
        InitSingleIterator(*iterator, 13, options, sharedData);
        AssertIteratorPositions(*iterator, expected);
        if (lookupSupported) {
            nextDocId = Max<ui32>();
            iterator->LookupNextDoc(nextDocId);
            UNIT_ASSERT_EQUAL(nextDocId, Max<ui32>());
        }
        TMemoryPool reloadPool(1 << 8);
        TIteratorPtr reloadIterator = TIteratorSaveLoad::LoadIterator(&saveStream, 43, reloadPool, builder); // it must be possible to change blockId
        InitSingleIterator(*reloadIterator, 3, options, sharedData);
        AssertIteratorPositions(*reloadIterator, {});
        InitSingleIterator(*reloadIterator, 13, options, sharedData);
        for (auto& pos : expected)
            pos.BlockId = 43;
        AssertIteratorPositions(*reloadIterator, expected);

        if (protoSerializationSupported) {
            TReqBundleOffroadIteratorDeserializer<TOffroadSearcher> deserializer;
            TIteratorPtr iter2 = deserializer.DeserializeFromProto(proto, reloadPool);

            InitSingleIterator(*iterator, 3, options, sharedData);
            InitSingleIterator(*iter2, 3, options, sharedData);
            CheckIteratorsEqual(iterator, iter2.Get());


            InitSingleIterator(*iterator, 13, options, sharedData);
            InitSingleIterator(*iter2, 13, options, sharedData);
            CheckIteratorsEqual(iterator, iter2.Get());
        }
    }

    struct TNormalAnd
    {
        using SupportsNextDoc = std::true_type;
        template<class... Args>
        static TIteratorPtr Create(size_t numWords, ui32 blockId, TMemoryPool& pool, Args&&... args) {
            return MakeIteratorAnd(numWords, blockId, pool, std::forward<Args>(args)...);
        }
    };

    struct TOrderedAnd
    {
        using SupportsNextDoc = std::false_type;
        template<class... Args>
        static TIteratorPtr Create(size_t numWords, ui32 blockId, TMemoryPool& pool, Args&&... args) {
            return MakeIteratorOrderedAnd(numWords, blockId, pool, std::forward<Args>(args)...);
        }
    };

    template<typename TIteratorBuilder, typename TAnd>
    void TestAndIteratorForBlock(NReqBundle::TBlockPtr block, std::initializer_list<TExpectedLowLevelPosition> expected, bool utf8IndexKeys, bool serialize)
    {
        TMemoryPool pool(1 << 16);
        TIteratorBuilder builder;
        TBlockBundleData bundleData(*block, pool, {});
        TBlockIndexData indexData(pool);
        TIndexLookupMapping keys;
        IIndexAccessor& indexAccessor = *builder.CreateIndexAccessor();
        indexData.CollectIndexKeys(bundleData, pool/*, indexAccessor*/, utf8IndexKeys, {0}, keys);
        for (auto& [k, v] : keys) {
            indexAccessor.PrepareHits(k.Key, v, 0, {});
        }
        TIteratorPtr iterator = TAnd::Create(block->GetNumWords(), 42, pool, &bundleData, indexData);
        TIteratorOptions options;
        CheckAndIterator(
            iterator.Get(),
            expected,
            TIteratorBuilder::SupportsNextDoc::value && TAnd::SupportsNextDoc::value,
            builder.CreateBuilder(),
            options,
            builder.GetSharedData(),
            serialize);
    }

    template<typename TIteratorBuilder>
    void TestAndIterator(bool utf8IndexKeys, bool testSerialization)
    {
        TestAndIteratorForBlock<TIteratorBuilder, TNormalAnd>(
            BlockFromProto(TString::Join(protoIz, protoOkna, protoDulo, "Distance: 1")), {
            { 4, 12, 14, 42, 0, 0, EQUAL_BY_LEMMA, MID_RELEV },
            { 8, 3, 5, 42, 0, 0, EQUAL_BY_STRING, MID_RELEV },
            { 8, 12, 14, 42, 0, 0, EQUAL_BY_STRING, MID_RELEV },
            }, utf8IndexKeys, testSerialization);
    }

    Y_UNIT_TEST(TestAndIteratorYndex)
    {
        TestAndIterator<TYndexIteratorBuilder>(false, false);
    }

    Y_UNIT_TEST(TestAndIteratorOffroad)
    {
        TestAndIterator<TOffroadIteratorBuilder>(true, true);
    }

    template<typename TIteratorBuilder>
    void TestAndIteratorRepeatedWords(bool utf8IndexKeys, bool testSerialization)
    {
        TestAndIteratorForBlock<TIteratorBuilder, TNormalAnd>(
            BlockFromProto(TString::Join(protoIz, protoOkna, protoIz, protoDulo, "Distance: 1")), {
            { 4, 12, 14, 42, 0, 0, EQUAL_BY_LEMMA, MID_RELEV },
            { 8, 3, 5, 42, 0, 0, EQUAL_BY_STRING, MID_RELEV },
            { 8, 12, 14, 42, 0, 0, EQUAL_BY_STRING, MID_RELEV },
            }, utf8IndexKeys, testSerialization);
    }

    Y_UNIT_TEST(TestAndIteratorRepeatedWordsYndex)
    {
        TestAndIteratorRepeatedWords<TYndexIteratorBuilder>(false, false);
    }

    Y_UNIT_TEST(TestAndIteratorRepeatedWordsOffroad)
    {
        TestAndIteratorRepeatedWords<TOffroadIteratorBuilder>(true, false);
    }

    template<typename TIteratorBuilder>
    void TestOrderedAndIterator(bool utf8IndexKeys, bool testSerialization)
    {
        TestAndIteratorForBlock<TIteratorBuilder, TOrderedAnd>(
            BlockFromProto(TString::Join(protoIz, protoOkna, protoDulo, "Type: 1")), {
            { 8, 3, 5, 42, 0, 0, EQUAL_BY_STRING, MID_RELEV },
            }, utf8IndexKeys, testSerialization);

        TestAndIteratorForBlock<TIteratorBuilder, TOrderedAnd>(
            BlockFromProto(TString::Join(protoIz, protoAnyWord, protoDulo, "Type: 1")), {
            { 8, 3, 5, 42, 0, 0, EQUAL_BY_STRING, MID_RELEV },
            { 8, 13, 14, 42, 0, 0, EQUAL_BY_STRING, MID_RELEV },
            }, utf8IndexKeys, testSerialization);
    }

    Y_UNIT_TEST(TestOrderedAndIteratorYndex)
    {
        TestOrderedAndIterator<TYndexIteratorBuilder>(false, false);
    }

    Y_UNIT_TEST(TestOrderedAndIteratorOffroad)
    {
        TestOrderedAndIterator<TOffroadIteratorBuilder>(true, false);
    }

    template<typename TIteratorBuilder>
    void TestOrderedAndIteratorRepeatedWords(bool utf8IndexKeys, bool testSerialization)
    {
        TestAndIteratorForBlock<TIteratorBuilder, TOrderedAnd>(
            BlockFromProto(TString::Join(protoIz, protoOkna, protoIz, protoDulo, "Type: 1")), {
            { 8, 11, 14, 42, 0, 0, EQUAL_BY_STRING, MID_RELEV },
            }, utf8IndexKeys, testSerialization);
    }

    Y_UNIT_TEST(TestOrderedAndIteratorRepeatedWordsYndex)
    {
        TestOrderedAndIteratorRepeatedWords<TYndexIteratorBuilder>(false, false);
    }

    Y_UNIT_TEST(TestOrderedAndIteratorRepeatedWordsOffroad)
    {
        TestOrderedAndIteratorRepeatedWords<TOffroadIteratorBuilder>(true, true);
    }

    template<typename TIteratorBuilder>
    void TestOrderedAndIteratorWordsInDifferentBreaksRepeatedWords(bool utf8IndexKeys, bool testSerialization)
    {
        TestAndIteratorForBlock<TIteratorBuilder, TOrderedAnd>(
            BlockFromProto(TString::Join(protoOknami, protoOknami, protoOknami, protoDulo, "Type: 1")), {
            { 9, 3, 4, 42, 0, 0, EQUAL_BY_STRING, MID_RELEV },
            }, utf8IndexKeys, testSerialization);
    }

    Y_UNIT_TEST(TestOrderedAndIteratorWordsInDifferentBreaksRepeatedWordsYndex)
    {
        TestOrderedAndIteratorWordsInDifferentBreaksRepeatedWords<TYndexIteratorBuilder>(false, false);
    }

    Y_UNIT_TEST(TestOrderedAndIteratorWordsInDifferentBreaksRepeatedWordsOffroad)
    {
        TestOrderedAndIteratorWordsInDifferentBreaksRepeatedWords<TOffroadIteratorBuilder>(true, true);
    }

    struct TExpectedPosition
    {
        ui32 Brk, WordPosBeg, WordPosEnd, BlockId, LemmId;
        const char* FormText;
        EFormClass FormClass;
        RelevLevel Relev;
    };

    static void AssertArrayPositions(
        const TPosition* posArray,
        const ui16* richTreeFormsArray,
        size_t numPos,
        const TVector<TFormIndexAssigner>& forms,
        const TExpectedPosition* expected,
        size_t numExpected)
    {
        UNIT_ASSERT_EQUAL(numPos, numExpected);
        for (size_t i = 0; i < numPos; i++) {
            UNIT_ASSERT(posArray[i].Valid());
            UNIT_ASSERT_EQUAL(posArray[i].Break(), expected[i].Brk);
            UNIT_ASSERT_EQUAL(posArray[i].WordPosBeg(), expected[i].WordPosBeg);
            UNIT_ASSERT_EQUAL(posArray[i].WordPosEnd(), expected[i].WordPosEnd);
            UNIT_ASSERT_EQUAL(posArray[i].BlockId(), expected[i].BlockId);
            UNIT_ASSERT_EQUAL(posArray[i].LemmId(), expected[i].LemmId);
            UNIT_ASSERT_EQUAL(WideToUTF8(forms[expected[i].BlockId].Id2Form.at(richTreeFormsArray[i])), expected[i].FormText);
            UNIT_ASSERT_EQUAL(posArray[i].Match(), expected[i].FormClass);
            UNIT_ASSERT_EQUAL(posArray[i].Relev(), (ui32)expected[i].Relev);
        }
    }

    static const TExpectedPosition PositionsInFirstDoc[] = {
        // Brk, WordPosBeg, WordPosEnd, BlockId, LemmId, FormText, FormClass, Relev
        { 1, 1, 1, 1, 0, "окна", EQUAL_BY_STRING, MID_RELEV },
        { 2, 1, 1, 0, 0, "из", EQUAL_BY_STRING, MID_RELEV },
        { 3, 9, 9, 1, 0, "окно", EQUAL_BY_LEMMA, HIGH_RELEV },
        { 4, 12, 12, 1, 0, "окнами", EQUAL_BY_LEMMA, HIGH_RELEV },
        { 5, 1, 1, 2, 1, "дула", EQUAL_BY_LEMMA, MID_RELEV },
        { 6, 44, 44, 1, 0, "окнах", EQUAL_BY_LEMMA, MID_RELEV },
        { 7, 2, 2, 1, 0, "окну", EQUAL_BY_LEMMA, MID_RELEV },
        { 8, 2, 2, 2, 0, "дуло", EQUAL_BY_STRING, MID_RELEV },
        { 9, 11, 11, 1, 0, "окном", EQUAL_BY_LEMMA, LOW_RELEV },
        { 10, 3, 3, 2, 1, "дулу", EQUAL_BY_LEMMA, MID_RELEV },
    };

    template<typename TIteratorBuilder, bool DisableBlocks = false>
    void TestReqBundleIterator()
    {
        TMemoryPool pool(1 << 16);
        TReqBundle bundle;
        bundle.Sequence().AddElem(BlockFromProto(protoIz));
        bundle.Sequence().AddElem(BlockFromProto(protoOkna));
        bundle.Sequence().AddElem(BlockFromProto(protoDulo));
        TVector<TFormIndexAssigner> forms;
        for (int i = 0; i < 3; i++)
            forms.emplace_back(bundle.Sequence().GetElem(i).GetBlock().GetWord(), pool);
        TIteratorBuilder builder;
        TRBIteratorOptions options;
        if (DisableBlocks) {
            options.TypesMask = 0;
        }
        TRBIteratorPtr iter = builder.CreateRBIterator(bundle, nullptr, options);
        TPosition posBuffer[1024];
        ui16 richTreeForms[Y_ARRAY_SIZE(posBuffer)];
        size_t numPos = iter->GetAllDocumentPositions(1, posBuffer, Y_ARRAY_SIZE(posBuffer));

        if (DisableBlocks) {
            return; // just check that it doesn't fail
        }

        AssertArrayPositions(posBuffer, richTreeForms, numPos, forms, nullptr, 0);
        ui32 nextDocId = 0xBAADF00D;
        if (TIteratorBuilder::SupportsNextDoc::value) {
            UNIT_ASSERT(iter->LookupNextDoc(nextDocId));
            UNIT_ASSERT_EQUAL(nextDocId, 3);
        }
        numPos = iter->GetAllDocumentPositions(3, posBuffer, Y_ARRAY_SIZE(posBuffer));
        for (size_t i = 0; i < numPos; i++)
            richTreeForms[i] = iter->GetRichTreeFormId(posBuffer[i].BlockId(), posBuffer[i].LowLevelFormId());
        AssertArrayPositions(posBuffer, richTreeForms, numPos, forms, PositionsInFirstDoc, Y_ARRAY_SIZE(PositionsInFirstDoc));
        if (TIteratorBuilder::SupportsNextDoc::value) {
            for (ui32 expectedDocId : {5, 8, 13}) {
                nextDocId = 0xBAADF00D;
                UNIT_ASSERT(iter->LookupNextDoc(nextDocId));
                UNIT_ASSERT_EQUAL(nextDocId, expectedDocId);
                iter->GetAllDocumentPositions(nextDocId, nullptr, 0);
            }
            nextDocId = 0xF00DBAAD;
            UNIT_ASSERT(!iter->LookupNextDoc(nextDocId));
            UNIT_ASSERT_EQUAL(nextDocId, Max<ui32>());
        }
    }

    Y_UNIT_TEST(TestReqBundleIteratorYndex)
    {
        TestReqBundleIterator<TYndexIteratorBuilder>();
        TestReqBundleIterator<TYndexIteratorBuilder, true>();
    }

    Y_UNIT_TEST(TestReqBundleIteratorOffroad)
    {
        TestReqBundleIterator<TOffroadIteratorBuilder>();
        TestReqBundleIterator<TOffroadIteratorBuilder, true>();
    }

    Y_UNIT_TEST(TestReqBundleIteratorOffroadTwoPhase)
    {
        TestReqBundleIterator<TOffroadTwoPhaseIteratorBuilder>();
        TestReqBundleIterator<TOffroadTwoPhaseIteratorBuilder, true>();
    }

    template<typename TIteratorBuilder>
    void TestReqBundleIteratorCache()
    {
        // делаем одно и то же два раза с кэшом, во второй раз всё должно подтянуться из кэша
        TIteratorBuilder builder;
        TRBIteratorsHashers cache;
        cache.Init(HsDefault);
        for (int attempt = 0; attempt < 2; attempt++) {
            TMemoryPool pool(1 << 16);
            TReqBundle bundle;
            bundle.Sequence().AddElem(BlockFromProto(protoIz));
            bundle.Sequence().AddElem(BlockFromProto(protoOkna));
            bundle.Sequence().AddElem(BlockFromProto(protoDulo));
            TVector<TFormIndexAssigner> forms;
            for (int i = 0; i < 3; i++)
                forms.emplace_back(bundle.Sequence().GetElem(i).GetBlock().GetWord(), pool);
            {
                TReqBundleSerializer serializer;
                bundle.Sequence().PrepareAllBinaries(serializer);
                //bundle.Sequence().DiscardAllBlocks(); // don't do this, otherwise LemmId will change
            }
            TRBIteratorPtr iter = builder.CreateRBIterator(bundle, cache.TrHasher.Get());
            TPosition posBuffer[1024];
            ui16 richTreeForms[Y_ARRAY_SIZE(posBuffer)];
            size_t numPos = iter->GetAllDocumentPositions(3, posBuffer, Y_ARRAY_SIZE(posBuffer));
            for (size_t i = 0; i < numPos; i++)
                richTreeForms[i] = iter->GetRichTreeFormId(posBuffer[i].BlockId(), posBuffer[i].LowLevelFormId());
            AssertArrayPositions(posBuffer, richTreeForms, numPos, forms, PositionsInFirstDoc, Y_ARRAY_SIZE(PositionsInFirstDoc));
        }
    }

    Y_UNIT_TEST(TestReqBundleIteratorYndexCache)
    {
        TestReqBundleIteratorCache<TYndexIteratorBuilder>();
    }

    Y_UNIT_TEST(TestReqBundleIteratorOffroadCache)
    {
        TestReqBundleIteratorCache<TOffroadIteratorBuilder>();
    }

    Y_UNIT_TEST(TestYndexLookupDecay)
    {
        TMemoryPool pool(1 << 16);
        TReqBundle bundle;
        bundle.Sequence().AddElem(BlockFromProto(protoIz));
        bundle.Sequence().AddElem(BlockFromProto(protoOkna));
        bundle.Sequence().AddElem(BlockFromProto(protoDulo));
        TTestYndexRequester yr;
        TReqBundleYndexIteratorBuilder builder(yr, IHitsAccess::IDX_TEXT, TLangMask(LANG_RUS, LANG_ENG), TLangMask());
        TRBIteratorPtr iter = TRBIteratorsFactory(bundle).OpenIterator(builder);
        // decay=1 означает, что каждый блок побеждает в выборах следующего документа только один раз
        // коллизии разрешаются в пользу блока, идущего первым
        // в документе 3 есть все слова, за него голосует блок "из"
        // слово "окна" отдаёт свой единственный голос за документ 5
        // документ 8 забанен из-за decay=1
        // остаётся последнее слово и документ 13
        ui32 nextDocId;
        for (ui32 expectedDocId : {3, 5, 13}) {
            nextDocId = 0xBAADF00D;
            UNIT_ASSERT(iter->LookupNextDoc(nextDocId, 1));
            UNIT_ASSERT_EQUAL(nextDocId, expectedDocId);
            iter->GetAllDocumentPositions(nextDocId, nullptr, 0);
        }
        nextDocId = 0xF00DBAAD;
        UNIT_ASSERT(!iter->LookupNextDoc(nextDocId, 1));
        UNIT_ASSERT_EQUAL(nextDocId, 0xF00DBAAD);
    }
}
