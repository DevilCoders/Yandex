#pragma once

// Predefined index properties

constexpr const char* const IP_DOCCOUNT       = "_DocCount";
constexpr const char* const IP_MODTIME        = "_ModTime";
constexpr const char* const IP_MODTIMESTR     = "_ModTimeStr";
constexpr const char* const IP_INDGENERATION  = "_IndGen";

// Predefined document properties

// public
constexpr const char* const DP_URL                   = "_Url";
constexpr const char* const DP_HILITED_URL           = "_HilitedUrl";
constexpr const char* const DP_URLMENU               = "_UrlMenu";
constexpr const char* const DP_MODTIME               = "_ModTime";
constexpr const char* const DP_MODTIMESTR            = "_ModTimeStr";
constexpr const char* const DP_SIZE                  = "_Size";
constexpr const char* const DP_CHARSET               = "_Charset";
constexpr const char* const DP_MIMETYPE              = "_MimeType";
constexpr const char* const DP_TITLE                 = "_Title";
constexpr const char* const DP_HEADLINE              = "_HeadLine";
constexpr const char* const DP_PASSAGE               = "_Passage";
constexpr const char* const DP_PASSAGEATTRS          = "_PassageAttrs";
constexpr const char* const DP_SPECSNIPATTRS         = "_SpecSnipAttrs";
constexpr const char* const DP_LINKSNIPATTRS         = "_LinkSnipAttrs";
constexpr const char* const DP_MINEDDATE             = "_MinedDate";
constexpr const char* const DP_REALDATE              = "_RealDate";
constexpr const char* const DP_DATESRC               = "_DateSource";
constexpr const char* const DP_IMGDUPSATTRS          = "_ImageDupsTable";
constexpr const char* const DP_DATEACCURACY          = "_DateAccuracy";
constexpr const char* const DP_ORANGE_ATTRS          = "_OrangeAttrs";
constexpr const char* const DP_SEARCH_SCRIPT         = "_SearchScript";
constexpr const char* const DP_HANDLE                = "_Handle";
constexpr const char* const DP_DOC_OMNI_TITLE        = "_DocOmniTitle";
constexpr const char* const DP_DOC_OMNI_URL          = "_DocOmniUrl";

// predefined error values
constexpr const char* const EV_NOTITLE    = ">>>>no title";
constexpr const char* const EV_NOHEADLINE = ">>>>no headline";
constexpr const char* const EV_EMPTY      = "";

// ex-Undocumented predefined document properties - now public
constexpr const char* const DP_PASSAGESTYPE    = "_PassagesType";
constexpr const char* const DP_IS_FAKE         = "_IsFake";

// private components
constexpr const char* const DP_PAGERANK        = "_PageRank";
constexpr const char* const DP_SEARCHER_HOSTNAME     = "_SearcherHostname";
constexpr const char* const DP_METASEARCHER_HOSTNAME = "_MetaSearcherHostname";
constexpr const char* const DP_RELEV_FORMULA = "_RelevFormula";
constexpr const char* const DP_RANKING_MODEL = "_RankingModel";
constexpr const char* const DP_RELEV_FACTORS = "_RelevFactors";
constexpr const char* const DP_JSON_FACTORS = "_JsonFactors";
constexpr const char* const DP_SNIPPETS_EXPLANATION = "_SnippetsExplanation";
constexpr const char* const DP_SNIPPETS_CTX = "_SnippetsCtx";
constexpr const char* const DP_ALT_SNIPPETS = "_AltSnippets";
constexpr const char* const DP_IS_CHEAT = "_IsCheat";
constexpr const char* const DP_ALLDOCINFOS = "_AllDocInfos";
constexpr const char* const DP_CRC = "_Crc";
constexpr const char* const DP_DOC_ID = "_DocId";
constexpr const char* const DP_HEADLINE_SRC = "_HeadlineSrc";
constexpr const char* const DP_SHARD = "_Shard";
constexpr const char* const DP_SEARCH_ZONE = "_SearchZone";
constexpr const char* const DP_PANTHER_POSITION = "_PantherPosition";
constexpr const char* const DP_SEQ2SEQ = "_Seq2SeqResult";
constexpr const char* const DP_NLGSEARCH_FACTORS = "_NlgSearchFactorsResult";
constexpr const char* const DP_QUORUM_ACCEPTED = "_QuorumAccepted";
constexpr const char* const DP_REGION = "_Region";
constexpr const char* const DP_GEO_LL = "ll";
constexpr const char* const DP_GEO_SPN = "spn";
constexpr const char* const DP_PROGWIZARDREQUEST = "PROGWIZARDREQUEST";
constexpr const char* const DP_RELEV_PREDICTION = "_RelevPrediction";
constexpr const char* const DP_RELEV_PREDICT = "_RelevPredict";
constexpr const char* const DP_ROBOT_DATE_TIMESTAMP = "_RobotDateTimestamp";
constexpr const char* const DP_ROBOT_ADD_TIME = "_RobotAddTime";
constexpr const char* const DP_ALL_FACTORS = "_AllFactors";
constexpr const char* const DP_DOCQUERYSIG = "_DocQuerySig";
constexpr const char* const DP_DOCSTATICSIG = "_DocStaticSig";
constexpr const char* const DP_ENTITYCLASSIFY = "EntityClassify";
constexpr const char* const DP_PERMALINKS = "_Permalinks";
constexpr const char* const DP_PERMALINKS_W_LL = "_PermalinksWithLL";
constexpr const char* const DP_SEARCH_HTTP_URL = "_SearchHttpUrl";
constexpr const char* const DP_BCLM_DATA = "_BclmData";
constexpr const char* const DP_COMPRESSED_FACTORS = "_CompressedFactors";
constexpr const char* const DP_COMPRESSED_FRESH_FACTORS = "_CompressedFreshFactors";
constexpr const char* const DP_COMPRESSED_ALL_FACTORS = "_CompressedAllFactors";
constexpr const char* const DP_TEXT_MACHINE_HITS = "_TextMachineHits";
constexpr const char* const DP_ALL_FACTOR_BORDERS = "_AllFactorBorders";
constexpr const char* const DP_MAX_DOC_ID = "_MaxDocId";
constexpr const char* const DP_MARKERS = "_Markers";
constexpr const char* const DP_SCHEMA_VTHUMB = "SchemaVthumb";
constexpr const char* const DP_CLICK_LIKE_SNIP = "Snippet"; // request doc attrs in format of click server
constexpr const char* const DP_DOC_TEXT_ARC = "_DocTextArc";
constexpr const char* const DP_DOCTEXT_BLOB = "_DocTextBlob";
constexpr const char* const DP_DOCINFO_BLOB = "_DocInfoBlob";
constexpr const char* const DP_LONG_SIMHASH = "LongSimhash";
constexpr const char* const DP_LAST_SEEN = "_LastSeen";
constexpr const char* const DP_IS_NON_TRASH_FORCE_L2_PASS = "_IsNonTrashForceL2Pass";
// For experiments with the ranking on the middle search
constexpr const char* const DP_HANDJOB_RESULT = "_HandJobResult";
constexpr const char* const DP_HANDJOB_BOOSTS = "_HandJobBoosts";
constexpr const char* const DP_PESS_FACTOR = "_PessFactor";
constexpr const char* const DP_CHEATS = "_Cheats";
constexpr const char* const DP_QUERY_PRIOR = "_QueryPrior";
constexpr const char* const DP_SOURCE_RELEVANCE = "_SourceRelevance";
constexpr const char* const DP_IS_FAKE_RELEVANCE = "_IsFakeRelevance";
constexpr const char* const DP_BASE_TIMESTAMP = "_BaseTimestamp";
constexpr const char* const DP_SEA = "sea";
// Images search
constexpr const char* const DP_IMAGESJSON = "_ImagesJson";
constexpr const char* const DP_IMAGES_DUPS = "_ImagesDups";
constexpr const char* const DP_IMAGE_ABS_REL = "_ImagesAbsRel"; // used in blender for image wizard
constexpr const char* const DP_IMAGE_ABS_REL_NEXT = "_ImagesAbsRelNext"; // used in blender for image wizard
constexpr const char* const DP_IMAGE_CLICK_REL = "_ImagesClickRelev";


constexpr const char* const DP_F_MAX_DOC_ID = "_F_MaxDocId";
constexpr const char* const DP_F_DOC_ID = "_F_DocId";

constexpr const char* const DP_ATTR_NAME_RICH_TREE_STAT = "rty_setsum_rich_tree_stat";
constexpr const char* const DP_ATTR_NAME_DOCS_COUNT = "rty_sum_docs_count";
constexpr const char* const DP_ATTR_NAME_EXPECTED_DOCS_COUNT = "rty_sum_expected_docs_count";

enum {
    DP_URL_HREF = 0,
    DP_URL_PROP = 1,
    DP_URL_DSRC = 2,
    DP_URL_END,
};
