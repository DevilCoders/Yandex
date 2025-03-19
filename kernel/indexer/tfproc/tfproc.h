#pragma once

#include <util/folder/dirut.h>

#include <library/cpp/html/face/parsface.h>
#include <library/cpp/numerator/numerate.h>
#include <library/cpp/stopwords/stopwords.h>
#include <kernel/indexer/face/inserter.h>
#include <kernel/indexer/face/docinfo.h>
#include <kernel/indexer/baseproc/docactionface.h>

#include <yweb/autoclassif/eshop/eshop_lib/eshop_numerator.h>
#include <yweb/autoclassif/shop_lib/numerator.h>

#include <yweb/autoclassif/soft404/numerator.h>

#include <yweb/autoclassif/ebook/ebook_lib/data_holder.h>
#include <yweb/autoclassif/ebook/ebook_lib/ebook_reader.h>
#include <yweb/autoclassif/ebook/ebook_lib/ebook_download.h>

#include "metadescr.h"
#include "titleproc.h"
#include "extbreakproc.h"
#include "poetry_numerator.h"
#include "syn_numerator.h"
#include "date_recognizer_numerator.h"
#include "tfnumerator.h"
#include "forumlib/forums.h"

// Config containing information about word stresses
struct TPoetryConfig {
    TString ForcesFileName;
};

struct TSynonymsConfig {
    TString CountersFileName;
    TString ResultFileName;
    TString WordsFileName;
    TString CoordsFileName;
};

struct TMetaDescrConfig {
    TString StopWordsFile;
};

struct TShopConfig {
    TString ReviewModelFileName;
    TString ShopDatFileName;
    TString ShopModelFileName;
};


struct TTextFeaturesConfig : public TPoetryConfig
                           , public TSynonymsConfig
                           , public TMetaDescrConfig
                           , public TShopConfig
{
    void InitDefaultTfConfig(const TString& configDir) {
        ForcesFileName = TString::Join(configDir, "forces_info");

        CountersFileName = TString::Join(configDir, "antisyn/counters.bin");
        ResultFileName = TString::Join(configDir, "antisyn/result.bin");
        WordsFileName = TString::Join(configDir, "antisyn/paradigms.bin");
        CoordsFileName = TString::Join(configDir, "antisyn/dict_coords.bin");

        StopWordsFile = TString::Join(configDir, "stopword.lst");

        ReviewModelFileName = TString::Join(configDir, "shop/review.model");
        ShopDatFileName = TString::Join(configDir, "shop/shop.dat");
        ShopModelFileName = TString::Join(configDir, "shop/shop.model");

        EbookConfig.ReadModelFileName = TString::Join(configDir, "ebook/read.model");
        EbookConfig.DownloadModelFileName = TString::Join(configDir, "ebook/download.model");
        EbookConfig.DataFileName = TString::Join(configDir, "ebook/words.txt");
    }

    NEbooksClassification::TEbookConfig EbookConfig;
};


struct TFeaturesData {
    TFeaturesData(const NEbooksClassification::TEbookConfig& ebookData)
        : EbooksData(ebookData)
    {}
    TWordFilter StopWords;
    TStreamDateRecognizer Recognizer;
    TTestSyn SynTester;
    TWordForces WordForces;
    NShop::TFeatureCollection ShopFeatureCollection;
    NEbooksClassification::TDataHolder EbooksData;
};

class TFeaturesHandler : public INumeratorHandler {
private:
    TDateRecognizerHandler DateRecognizerHandler;
    TTitleHandler TitleHandler;
    TExtBreaksHandler ExtBreaksHandler;
    eshop::TEshopHandler EshopHandler;
    TTextFeaturesHandler FeaturesHandler;
    TMetaDescrHandler MetaDescrHandler;
    TSynonymsHandler SynonymsHandler;
    TPoetryHandler PoetryHandler;
    NShop::TShopHandler ShopHandler;
    TForumsHandler ForumsHandler;
    NEbooksClassification::TEbookReader EbookReader;
    NSoft404::TSoft404Handler Soft404Handler;
public:
    void OnTextStart(const IParsedDocProperties* parser) override {
        ShopHandler.OnTextStart(parser);
        ForumsHandler.OnTextStart(parser);
        EbookReader.OnTextStart(parser);
        Soft404Handler.OnTextStart(parser);
    }

    void OnTextEnd(const IParsedDocProperties* ps, const TNumerStat& stat) override {
        TitleHandler.OnTextEnd(ps, stat);
        ExtBreaksHandler.OnTextEnd(ps, stat);
        EshopHandler.OnTextEnd(ps, stat);
        MetaDescrHandler.OnTextEnd(ps, stat);
        FeaturesHandler.OnTextEnd(ps, stat);
        SynonymsHandler.OnTextEnd(ps, stat);
        ShopHandler.OnTextEnd(ps, stat);
        ForumsHandler.OnTextEnd(ps, stat);
        EbookReader.OnTextEnd(ps, stat);
        Soft404Handler.OnTextEnd(ps, stat);
    }

    void OnTokenStart(const TWideToken& tok, const TNumerStat& stat) override {
        DateRecognizerHandler.OnTokenStart(tok, stat);
        TitleHandler.OnTokenStart(tok, stat);
        ExtBreaksHandler.OnTokenStart(tok, stat);
        MetaDescrHandler.OnTokenStart(tok, stat);
        FeaturesHandler.OnTokenStart(tok, stat);
        SynonymsHandler.OnTokenStart(tok, stat);
        PoetryHandler.OnTokenStart(tok, stat);
        ShopHandler.OnTokenStart(tok, stat);
        ForumsHandler.OnTokenStart(tok, stat);
        EbookReader.OnTokenStart(tok, stat);
        Soft404Handler.OnTokenStart(tok, stat);
    }

    void OnSpaces(TBreakType spType, const wchar16* tok, unsigned length, const TNumerStat& stat) override {
        DateRecognizerHandler.OnSpaces(spType, tok, length, stat);
        TitleHandler.OnSpaces(spType, tok, length, stat);
        MetaDescrHandler.OnSpaces(spType, tok, length, stat);
        SynonymsHandler.OnSpaces(spType, tok, length, stat);
        PoetryHandler.OnSpaces(spType, tok, length, stat);
        ForumsHandler.OnSpaces(spType, tok, length, stat);
    }

    void OnMoveInput(const THtmlChunk& chunk, const TZoneEntry* ze, const TNumerStat& stat) override {
        TitleHandler.OnMoveInput(chunk, ze, stat);
        EshopHandler.OnMoveInput(chunk, ze, stat);
        ExtBreaksHandler.OnMoveInput(chunk, ze, stat);
        MetaDescrHandler.OnMoveInput(chunk, ze, stat);
        FeaturesHandler.OnMoveInput(chunk, ze, stat);
        PoetryHandler.OnMoveInput(chunk, ze, stat);
        SynonymsHandler.OnMoveInput(chunk, ze, stat);
        ShopHandler.OnMoveInput(chunk, ze, stat);
        ForumsHandler.OnMoveInput(chunk, ze, stat);
        EbookReader.OnMoveInput(chunk, ze, stat);
        Soft404Handler.OnMoveInput(chunk, ze, stat);
    }

    void SetData(TFeaturesData* data, const TFullArchiveDocHeader* docHeader) {
        data->Recognizer.Push(docHeader->Url);
        data->Recognizer.Clear();
        DateRecognizerHandler.SetRecognizer(&data->Recognizer);
        FeaturesHandler.SetDoc(docHeader->Url);
        SynonymsHandler.SetTester(&data->SynTester);
        PoetryHandler.SetWordForces(&data->WordForces);
        ShopHandler.SetFeatures(&data->ShopFeatureCollection);
        ForumsHandler.OnAddDoc(docHeader->Url, docHeader->IndexDate, docHeader->Language);
        EbookReader.Init(data->EbooksData);
    }

    void OnCommitDoc(IDocumentDataInserter* inserter, const TWordFilter& stopWords) {
        TitleHandler.InsertFactors(*inserter);
        ExtBreaksHandler.InsertFactors(*inserter);
        EshopHandler.InsertFactors(*inserter);
        FeaturesHandler.InsertFactors(*inserter);
        MetaDescrHandler.InsertFactors(*inserter, stopWords);
        SynonymsHandler.InsertFactors(*inserter);
        PoetryHandler.InsertFactors(*inserter);
        ShopHandler.InsertFactors(*inserter);
        ForumsHandler.OnCommitDoc(inserter);
        EbookReader.InsertFactors(*inserter);
        Soft404Handler.InsertFactors(*inserter);
    }
};

class TTextFeaturesProcessor : public NIndexerCore::IDocumentAction {
private:
    TFeaturesData Data;
    THolder<TFeaturesHandler> Handler;
public:
    TTextFeaturesProcessor(const TTextFeaturesConfig *cfg, const NShop::TModel* shopModel = nullptr, const NShop::TWordsHolder* shopWordHolder = nullptr)
        : Data(cfg->EbookConfig)
    {
        TExistenceChecker ec(true);
        Data.StopWords.InitStopWordsList(ec.Check(cfg->StopWordsFile.data()));
        Data.SynTester.Load(ec.Check(cfg->CountersFileName.data()),
                            ec.Check(cfg->ResultFileName.data()),
                            ec.Check(cfg->WordsFileName.data()),
                            ec.Check(cfg->CoordsFileName.data()));
        Data.WordForces.Load(ec.Check(cfg->ForcesFileName.data()));
        // init Shop
        Y_VERIFY((shopModel == nullptr && shopWordHolder == nullptr) || (shopModel != nullptr && shopWordHolder != nullptr),
            "Wrong initialization of shop model components");

        if (shopModel != nullptr && shopWordHolder != nullptr)
            Data.ShopFeatureCollection.Init(*shopModel, *shopWordHolder);
        else
            Data.ShopFeatureCollection.Init(
                ec.Check(cfg->ShopDatFileName.data()),
                ec.Check(cfg->ReviewModelFileName.data()),
                ec.Check(cfg->ShopModelFileName.data())
            );
    }

    INumeratorHandler* OnDocProcessingStart(const TDocInfoEx* docInfo, bool isFullProcessing) override {
        if (isFullProcessing) {
            Handler.Reset(new TFeaturesHandler());
            Handler->SetData(&Data, docInfo->DocHeader);
            return Handler.Get();
        } else {
            Data.Recognizer.Clear();
            return nullptr;
        }
    }

    void OnDocProcessingStop(const IParsedDocProperties* /*pars*/, const TDocInfoEx* /*docInfo*/, IDocumentDataInserter* inserter, bool isFullProcessing) override {
        if (isFullProcessing) {
            Handler->OnCommitDoc(inserter, Data.StopWords);
            SetRecD(inserter);
        }
    }
private:
    void SetRecD(IDocumentDataInserter* inserter) {
        TRecognizedDate dt;
        if (Data.Recognizer.GetDate(&dt))
            inserter->StoreErfDocAttr("RecD", dt.ToString());
    }
};
