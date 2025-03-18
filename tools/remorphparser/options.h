#pragma once

#include <kernel/remorph/text/result_print.h>
#include <kernel/remorph/tokenizer/tokenizer.h>

#include <library/cpp/solve_ambig/rank.h>
#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/enumbitset/enumbitset.h>
#include <library/cpp/langmask/langmask.h>
#include <library/cpp/mime/types/mime.h>

#include <library/cpp/charset/doccodes.h>
#include <util/datetime/base.h>
#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/stream/output.h>

namespace NRemorphParser {

class TRunOpts {
private:
    TSimpleSharedPtr<IOutputStream> OutputFile;
    NLastGetopt::TOpts Options;

private:
    void InitOptParser();
    void Parse(int argc, char* argv[]);

public:
    enum EFormat {
        FMT_Text,
        FMT_Query,
        FMT_Index
    };

    enum EInMode {
        IM_Plain,
        IM_Corpus,
        IM_Arc,
        IM_Tarcview
    };

    enum EOutMode {
        OM_Plain,
        OM_CorpusTags,
        OM_FactsJson
    };

    enum EMatcherMode {
        MM_Match,
        MM_MatchAll,
        MM_MatchBest,
        MM_Search,
        MM_SearchAll,
        MM_SearchBest,
        MM_Solutions
    };

    enum EFactMode {
        FM_Best,
        FM_All,
        FM_Solutions
    };

    enum EMatcherType {
        MT_Remorph,
        MT_Tokenlogic,
        MT_Tagger,
        MT_Char,
        MT_Fact
    };

public:
    // Processing
    TString MatcherPath;
    EMatcherType MatcherType;
    NSolveAmbig::TRankMethod RankMethod;
    EFactMode FactMode;
    EMatcherMode MatcherMode;
    TString GztPath;
    TString BaseDir;
    bool AllGazetteerResults;
    NSolveAmbig::TRankMethod GazetteerRankMethod;
    bool InitGeoGazetteer;

    // Input
    EFormat Format;
    TLangMask Lang;
    ECharset Encoding;
    EInMode InMode;

    // Output
    IOutputStream* Output;
    EOutMode OutMode;
    NText::EPrintVerbosity PrintVerbosity;
    bool PrintUnmatched;
    bool PrintInvertGrep;
    bool Colorized;

    // Time monitoring
    TDuration TimeLimit;

    // Misc
    size_t Threads;
    NToken::TTokenizeOptions TokenizerOpts;
    bool QueryPunctuation;

    // Filtering for arc/tarcview
    TLangMask FilterLang;
    TSfEnumBitSet<ECharset, CODES_UNKNOWN, CODES_MAX> FilterEncoding;
    TSfEnumBitSet<MimeTypes, MIME_UNKNOWN, MIME_MAX> FilterMimeType;

    // Special
    bool Info;

    // Files to process
    TVector<TString> FreeArgs;

public:
    TRunOpts(int argc, char* argv[]);
};

} // NRemorphParser
