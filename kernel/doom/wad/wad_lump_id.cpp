#include "wad_lump_id.h"

#include <util/generic/hash.h>
#include <util/string/cast.h>
#include <util/stream/output.h>

namespace NDoom {

/**
 * Serialization compatibility layer for old wad files.
 *
 * DO NOT ADD NEW RECORDS HERE.
 */
static const struct TWadLumpNameCompatibility {
    const char* OldName;
    const char* NewName;
} NameMapping[] = {
    { "OffroadPantherKeysModelLump",                              "panther.keys_model" },
    { "OffroadPantherKeysLump",                                   "panther.keys" },
    { "OffroadPantherKeysFatLump",                                "panther.keys_fat" },
    { "OffroadPantherKeysFatIdxLump",                             "panther.keys_idx" },
    { "OffroadPantherHitsModelLump",                              "panther.hits_model" },
    { "OffroadPantherHitsLump",                                   "panther.hits" },
    { "OffroadAttributesKeysModelLump",                           "attributes.keys_model" },
    { "OffroadAttributesKeysLump",                                "attributes.keys" },
    { "OffroadAttributesKeysFatLump",                             "attributes.keys_fat" },
    { "OffroadAttributesKeysFatIdxLump",                          "attributes.keys_idx" },
    { "OffroadAttributesHitsModelLump",                           "attributes.hits_model" },
    { "OffroadAttributesHitsLump",                                "attributes.hits" },
    { "OffroadAttributesHitsSubLump",                             "attributes.hits_sub" },
    { "OffroadFastAnnDataModelLump",                              "fast_ann_data.hits_model" },
    { "OffroadFastAnnDataHitsLump",                               "fast_ann_data.hits" },
    { "OffroadFastAnnDataSubLump",                                "fast_ann_data.hits_sub" },
    { "OffroadAnnDataModelLump",                                  "factor_ann_data.hits_model" },
    { "OffroadAnnDataHitsLump",                                   "factor_ann_data.hits" },
    { "OffroadAnnDataSubLump",                                    "factor_ann_data.hits_sub" },
    { "OffroadFastAnnKeyModelLump",                               "fast_ann.keys_model" },
    { "OffroadFastAnnHitModelLump",                               "fast_ann.hits_model" },
    { "OffroadFastAnnSortedMultiKeysHitsLump",                    "fast_ann.hits" },
    { "OffroadFastAnnSubLump",                                    "fast_ann.hits_sub" },
    { "OffroadFastAnnSortedMultiKeysLump",                        "fast_ann.keys" },
    { "OffroadFastAnnKeyFatLump",                                 "fast_ann.keys_fat" },
    { "OffroadFastAnnKeyIdxLump",                                 "fast_ann.keys_idx" },
    { "OffroadAnnKeyModelLump",                                   "factor_ann_freq_sorted.keys_model" },
    { "OffroadAnnHitModelLump",                                   "factor_ann_freq_sorted.hits_model" },
    { "OffroadAnnSortedMultiKeysHitsLump",                        "factor_ann_freq_sorted.hits" },
    { "OffroadAnnSubLump",                                        "factor_ann_freq_sorted.hits_sub" },
    { "OffroadAnnSortedMultiKeysLump",                            "factor_ann_freq_sorted.keys" },
    { "OffroadAnnKeyFatLump",                                     "factor_ann_freq_sorted.keys_fat" },
    { "OffroadAnnKeyIdxLump",                                     "factor_ann_freq_sorted.keys_idx" },
    { "OffroadSentModelLump",                                     "factor_sent.hits_model" },
    { "OffroadSentHitsLump",                                      "factor_sent.hits" },
    { "OffroadFastSentModelLump",                                 "fast_sent.hits_model" },
    { "OffroadFastSentHitsLump",                                  "fast_sent.hits" },
    { "OffroadOmniUrlLump",                                       "omni_url.struct" },
    { "OffroadOmniUrlModelLump",                                  "omni_url.struct_model" },
    { "OffroadOmniTitleLump",                                     "omni_title.struct" },
    { "OffroadOmniTitleModelLump",                                "omni_title.struct_model" },
    { "OffroadOmniDssmEmbeddingLump",                             "omni_dssm_embedding.struct" },
    { "OffroadOmniDssmEmbeddingModelLump",                        "omni_dssm_embedding.struct_model" },
    { "OffroadOmniDssmEmbeddingSizeLump",                         "omni_dssm_embedding.struct_size" },
    { "OffroadOmniDssmWeightedAnnotationLump",                    "omni_dssm_weighted_annotation.struct" },
    { "OffroadOmniDssmWeightedAnnotationModelLump",               "omni_dssm_weighted_annotation.struct_model" },
    { "OffroadOmniDssmWeightedAnnotationSizeLump",                "omni_dssm_weighted_annotation.struct_size" },
    { "OffroadOmniAnnRegStatsLump",                               "omni_ann_reg_stats.struct" },
    { "OffroadOmniAnnRegStatsModelLump",                          "omni_ann_reg_stats.struct_model" },
    { "OffroadOmniAnnRegStatsSizeLump",                           "omni_ann_reg_stats.struct_size" },
    { "OffroadOmniDssmConcatenatedAnnRegEmbeddingLump",           "omni_dssm_concatenated_ann_reg_embedding.struct" },
    { "OffroadOmniDssmConcatenatedAnnRegEmbeddingModelLump",      "omni_dssm_concatenated_ann_reg_embedding.struct_model" },
    { "OffroadOmniDssmConcatenatedAnnRegEmbeddingSizeLump",       "omni_dssm_concatenated_ann_reg_embedding.struct_size" },
    { "OffroadOmniDssmAggregatedAnnRegEmbeddingLump",             "omni_dssm_aggregated_ann_reg_embedding.struct" },
    { "OffroadOmniDssmAggregatedAnnRegEmbeddingModelLump",        "omni_dssm_aggregated_ann_reg_embedding.struct_model" },
    { "OffroadOmniDssmAggregatedAnnRegEmbeddingSizeLump",         "omni_dssm_aggregated_ann_reg_embedding.struct_size" },
    { "OffroadOmniDssmAnnXfDtShowOneEmbeddingsLump",              "omni_dssm_ann_xf_dt_show_one_embedding.struct" },
    { "OffroadOmniDssmAnnXfDtShowOneEmbeddingsModelLump",         "omni_dssm_ann_xf_dt_show_one_embedding.struct_model" },
    { "OffroadOmniDssmAnnXfDtShowOneEmbeddingsSizeLump",          "omni_dssm_ann_xf_dt_show_one_embedding.struct_size" },
    { "OffroadOmniDssmAnnXfDtShowWeightEmbeddingsLump",           "omni_dssm_ann_xf_dt_show_weight_embedding.struct" },
    { "OffroadOmniDssmAnnXfDtShowWeightEmbeddingsModelLump",      "omni_dssm_ann_xf_dt_show_weight_embedding.struct_model" },
    { "OffroadOmniDssmAnnXfDtShowWeightEmbeddingsSizeLump",       "omni_dssm_ann_xf_dt_show_weight_embedding.struct_size" },
    { "OffroadOmniDssmAnnCtrCompressedEmbeddingLump",             "omni_dssm_ann_ctr_compressed_embedding.struct" },
    { "OffroadOmniDssmAnnCtrCompressedEmbeddingModelLump",        "omni_dssm_ann_ctr_compressed_embedding.struct_model" },
    { "OffroadOmniDssmAnnCtrCompressedEmbeddingSizeLump",         "omni_dssm_ann_ctr_compressed_embedding.struct_size" },
    { "OffroadOmniDssmAnnXfDtShowWeightCompressedEmbedding1Lump",       "omni_dssm_ann_xf_dt_show_weight_compressed_embedding1.struct" },
    { "OffroadOmniDssmAnnXfDtShowWeightCompressedEmbedding1ModelLump",  "omni_dssm_ann_xf_dt_show_weight_compressed_embedding1.struct_model" },
    { "OffroadOmniDssmAnnXfDtShowWeightCompressedEmbedding1SizeLump",   "omni_dssm_ann_xf_dt_show_weight_compressed_embedding1.struct_size" },
    { "OffroadOmniDssmAnnXfDtShowWeightCompressedEmbedding2Lump",       "omni_dssm_ann_xf_dt_show_weight_compressed_embedding2.struct" },
    { "OffroadOmniDssmAnnXfDtShowWeightCompressedEmbedding2ModelLump",  "omni_dssm_ann_xf_dt_show_weight_compressed_embedding2.struct_model" },
    { "OffroadOmniDssmAnnXfDtShowWeightCompressedEmbedding2SizeLump",   "omni_dssm_ann_xf_dt_show_weight_compressed_embedding2.struct_size" },
    { "OffroadOmniDssmAnnXfDtShowWeightCompressedEmbedding3Lump",       "omni_dssm_ann_xf_dt_show_weight_compressed_embedding3.struct" },
    { "OffroadOmniDssmAnnXfDtShowWeightCompressedEmbedding3ModelLump",  "omni_dssm_ann_xf_dt_show_weight_compressed_embedding3.struct_model" },
    { "OffroadOmniDssmAnnXfDtShowWeightCompressedEmbedding3SizeLump",   "omni_dssm_ann_xf_dt_show_weight_compressed_embedding3.struct_size" },
    { "OffroadOmniDssmAnnXfDtShowWeightCompressedEmbedding4Lump",       "omni_dssm_ann_xf_dt_show_weight_compressed_embedding4.struct" },
    { "OffroadOmniDssmAnnXfDtShowWeightCompressedEmbedding4ModelLump",  "omni_dssm_ann_xf_dt_show_weight_compressed_embedding4.struct_model" },
    { "OffroadOmniDssmAnnXfDtShowWeightCompressedEmbedding4SizeLump",   "omni_dssm_ann_xf_dt_show_weight_compressed_embedding4.struct_size" },
    { "OffroadOmniDssmAnnXfDtShowWeightCompressedEmbedding5Lump",       "omni_dssm_ann_xf_dt_show_weight_compressed_embedding5.struct" },
    { "OffroadOmniDssmAnnXfDtShowWeightCompressedEmbedding5ModelLump",  "omni_dssm_ann_xf_dt_show_weight_compressed_embedding5.struct_model" },
    { "OffroadOmniDssmAnnXfDtShowWeightCompressedEmbedding5SizeLump",   "omni_dssm_ann_xf_dt_show_weight_compressed_embedding5.struct_size" },
    { "OffroadOmniDssmAnnXfDtShowOneCompressedEmbeddingLump",           "omni_dssm_ann_xf_dt_show_one_compressed_embedding.struct" },
    { "OffroadOmniDssmAnnXfDtShowOneCompressedEmbeddingModelLump",      "omni_dssm_ann_xf_dt_show_one_compressed_embedding.struct_model" },
    { "OffroadOmniDssmAnnXfDtShowOneCompressedEmbeddingSizeLump",       "omni_dssm_ann_xf_dt_show_one_compressed_embedding.struct_size" },
    { "OffroadOmniDssmAnnXfDtShowOneSeCompressedEmbeddingLump",         "omni_dssm_ann_xf_dt_show_one_se_compressed_embedding.struct" },
    { "OffroadOmniDssmAnnXfDtShowOneSeCompressedEmbeddingModelLump",    "omni_dssm_ann_xf_dt_show_one_se_compressed_embedding.struct_model" },
    { "OffroadOmniDssmAnnXfDtShowOneSeCompressedEmbeddingSizeLump",     "omni_dssm_ann_xf_dt_show_one_se_compressed_embedding.struct_size" },
    { "OffroadOmniDssmMainContentKeywordsEmbeddingLump",          "omni_dssm_main_content_keywords_embedding.struct" },
    { "OffroadOmniDssmMainContentKeywordsEmbeddingModelLump",     "omni_dssm_main_content_keywords_embedding.struct_model" },
    { "OffroadOmniDssmMainContentKeywordsEmbeddingSizeLump",      "omni_dssm_main_content_keywords_embedding.struct_size" },
    { "OffroadOmniDupUrlsLump",                                   "omni_dup_urls.struct" },
    { "OffroadOmniDupUrlsModelLump",                              "omni_dup_urls.struct_model" },
    { "OffroadOmniEmbedUrlsLump",                                 "omni_embed_urls.struct" },
    { "OffroadOmniEmbedUrlsModelLump",                            "omni_embed_urls.struct_model" },
    { "OffroadImgLinkDataSentModelLump",                          "img_link_data_sent.hits_model" },
    { "OffroadImgLinkDataSentHitsLump",                           "img_link_data_sent.hits" },
    { "OffroadImgLinkDataAnnDataModelLump",                       "img_link_data_ann_data.hits_model" },
    { "OffroadImgLinkDataAnnDataHitsLump",                        "img_link_data_ann_data.hits" },
    { "OffroadImgLinkDataAnnDataSubLump",                         "img_link_data_ann_data.hits_sub" },
    { "OffroadErfWebLump",                                        "erf.struct" },
    { "OffroadErfWebModelLump",                                   "erf.struct_model" },
    { "OffroadErfWebSizeLump",                                    "erf.struct_size" },
    { "OffroadDocChunksMappingLump",                              "chunk_mapping.hits" }
};

struct TWadLumpNameMapping {
public:
    TWadLumpNameMapping() {
        for (auto&& e : NameMapping) {
            OldByNew[e.NewName] = e.OldName;
            NewByOld[e.OldName] = e.NewName;
        }
    }

    TStringBuf OldToNew(const TStringBuf old) const {
        return NewByOld.Value(old, old);
    }

    TStringBuf NewToOld(const TStringBuf& mew) const {
        return OldByNew.Value(mew, mew);
    }

private:
    THashMap<TStringBuf, TStringBuf> OldByNew, NewByOld;
};

const TWadLumpNameMapping& WadLumpNameMapping() {
    static TWadLumpNameMapping result;
    return result;
}

} // namespace NDoom

template <>
void Out<NDoom::TWadLumpId>(IOutputStream& out, const NDoom::TWadLumpId& id) {
    out << id.Index << "." << id.Role;
}

template<>
NDoom::TWadLumpId FromStringImpl<NDoom::TWadLumpId>(const char* data, size_t len) {
    TStringBuf string = NDoom::WadLumpNameMapping().OldToNew(TStringBuf(data, len));
    TStringBuf part;

    if (!string.NextTok('.', part))
        ythrow yexception() << "Cannot parse NDoom::TWadLumpId from string \"" << string << "\"";

    NDoom::TWadLumpId result;
    result.Index = FromString<NDoom::EWadIndexType>(part);
    result.Role = FromString<NDoom::EWadLumpRole>(string);
    return result;
}

template<>
bool TryFromStringImpl<NDoom::TWadLumpId, char>(const char* data, size_t len, NDoom::TWadLumpId& result) {
    TStringBuf string = NDoom::WadLumpNameMapping().OldToNew(TStringBuf(data, len));
    TStringBuf part;

    if (!string.NextTok('.', part))
        return false;

    if (!TryFromString(part, result.Index)) {
        return false;
    }
    if (!TryFromString(string, result.Role)) {
        return false;
    }
    return true;
}
