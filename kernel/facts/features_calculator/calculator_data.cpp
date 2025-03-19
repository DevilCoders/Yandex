#include "calculator_data.h"
#include "calculator_config.h"

#include <library/cpp/resource/resource.h>
#include <quality/trailer/trailer_common/mmap_preloaded/mmap_loader.h>
namespace NUnstructuredFeatures
{
    static void LoadNeocortex(const TConfig& config, const TString& key, THolder<NNeocortex::TTextClassifierPack>& res) {
        TString path = config.GetFilePath(key);
        if (!path.empty()) {
            res.Reset(new NNeocortex::TTextClassifierPack());
            TFileInput input(path);
            res->Load(&input);
        }
    }

    static void LoadDssm(const TConfig& config, const TString& key, NNeuralNetApplier::TModelPtr& model) {
        TString path = config.GetFilePath(key);
        if (!path.empty()) {
            model = new NNeuralNetApplier::TModel();
            model->Load(SuggestBlobFromFile(path));
            model->Init();
        } else {
            Cerr << key << " loading has been skipped" << Endl;
        }
    }

    TCalculatorData::TCalculatorData(const TConfig& config) {
        LoadDssm(config, "ru_fact_snippet_dssm", RuFactSnippetDssmModel);
        LoadDssm(config, "tomato_dssm", TomatoDssmModel);
        LoadDssm(config, "aliases_bdssm", AliasesBdssmModel);

        LoadNeocortex(config, "neocortex_ttt_192", TextToTextNeocortexModel);
        LoadNeocortex(config, "neocortex_tth_192", TextToHostsNeocortexModel);
        LoadNeocortex(config, "lemma_aliases", LemmaAliasNeocortexModel);
        LoadNeocortex(config, "neocortex_ttt_fact_snip", TextToTextFactSnipNeocortexModel);
        LoadNeocortex(config, "neocortex_serp_items", SerpItemsNeocortexModel);

        TString wordsDistTrie = config.GetFilePath("words_dist_trie");
        if (!wordsDistTrie.empty()) {
            WordsDistTrie = MakeAtomicShared<NDistBetweenWords::TWordDistTrie>(
                    SuggestBlobFromFile(wordsDistTrie));
        }

        TString bigramsDistTrie = config.GetFilePath("bigrams_dist_trie");
        if (!bigramsDistTrie.empty()) {
            BigramsDistTrie = MakeAtomicShared<NDistBetweenWords::TWordDistTrie>(
                    SuggestBlobFromFile(bigramsDistTrie));
        }

        if (config.GetGlobalParameter("skip_load_embeddings", 0.0) != 1.0) {
            TString wordEmbeddingConfigJson = NResource::Find("word_embedding_config");
            if (!wordEmbeddingConfigJson.empty()) {
                WordEmbeddingFeaturesBuilder = MakeHolder<NWordEmbedding::TWordEmbeddingFeaturesBuilder>(
                        wordEmbeddingConfigJson, config.GetUsedFilesDict());
            }
        }

        TString normalizerGztPath = config.GetFilePath("special_words.gzt.bin");
        TString normalizerRegexPath = config.GetFilePath("all_patterns.txt");
        if (!normalizerGztPath.empty() && !normalizerRegexPath.empty()) {
            Normalizer.Reset(new TNormalizeByLemmasInfo(normalizerGztPath, normalizerRegexPath));
        }

        TString pureFileName = config.GetFilePath("pure.lg.groupedtrie.rus");
        if (!pureFileName.empty()) {
            Pure = MakeHolder<TPure>(SuggestBlobFromFile(pureFileName));
        }

        TString queryUnigramModelFileName = config.GetFilePath("query_unigram.model");
        if (queryUnigramModelFileName) {
            QueryUnigramModel.Reset(new NEthos::TBinaryTextClassifierModel);
            QueryUnigramModel->LoadFromFile(queryUnigramModelFileName);
        }

        TString crossModelFileName = config.GetFilePath("cross.model");
        if (crossModelFileName) {
            CrossModel.Reset(new NEthos::TBinaryTextClassifierModel);
            CrossModel->LoadFromFile(crossModelFileName);
        }
    }
}
