#include "options.h"
#include <library/cpp/getopt/last_getopt.h>

#include <util/charset/unidata.h>
#include <library/cpp/digest/md5/md5.h>
#include <util/draft/date.h>
#include <util/folder/dirut.h>
#include <util/generic/hash_set.h>
#include <util/generic/hash.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/random/random.h>
#include <util/stream/file.h>
#include <util/string/builder.h>
#include <util/string/cast.h>
#include <util/string/join.h>
#include <util/string/split.h>
#include <library/cpp/string_utils/url/url.h>


namespace {

static const double UNKNOWN_K = 0.03;
static const double GENERIC_K = 0.01;

static const TSnippetType2K SNIPPET_TO_K_DEFAULT = {
{"__bna", 0.4},

{"foto_recipe", 0.3},
{"people", 0.3},
{"prayers", 0.3},
{"social_snippet", 0.3},
{"video", 0.3},
{"video2", 0.3},
{"video_desc", 0.3},

{"chords", 0.2},
{"eksisozluk_snip", 0.2},
{"encyc", 0.2},
{"forum_forums", 0.2},
{"forum_topic", 0.2},
{"forums", 0.2},
{"infected", 0.2},
{"list_snip", 0.2},
{"table_snip", 0.2},
{"torrent_film", 0.2},

{"kinopoisk", 0.06},
{"litera", 0.06},
{"muz_performer", 0.06},
{"productoffer_snip", 0.06},
{"referat", 0.06},
{"schema_movie", 0.06},
{"youtube_channel", 0.06},

{"creativework_snip", 0.03},
{"dmoz", 0.03},
{"enwiki", 0.03},
{"kino", 0.03},
{"market_text", 0.03},
{"mediawiki_snip", 0.03},
{"meta_descr", 0.03},
{"news", 0.03},
{"ogtext", 0.03},
{"question", 0.03},
{"recipe", 0.03},
{"review", 0.03},
{"robots_txt_stub", 0.03},
{"ruwiki", 0.03},
{"static_annotation", 0.03},
{"text_for_nps", 0.03},
{"trash_annotation", 0.03},
{"trwiki", 0.03},
{"ukwiki", 0.03},
{"yaca", 0.03},

{"adresa/company", 0.03},
{"adresa/list", 0.03},
{"adresa/map", 0.03},
{"adresa/net", 0.03},
{"adresa/phone_number_company", 0.03},
{"adresa/phone_number_list", 0.03},
{"blogs", 0.03},
{"encyc_wizard", 0.03},
{"facebook", 0.03},
{"imdb", 0.03},
{"list_snip_ctr", 0.03},
{"microanswer", 0.03},
{"mobile_apps", 0.03},
{"mobile_apps_unified", 0.03},
{"movie", 0.03},
{"music", 0.03},
{"music_artist", 0.03},
{"nav", 0.03},
{"people_wizard", 0.03},
{"performer", 0.03},
{"photorecipe", 0.03},
{"prog", 0.03},
{"rabota", 0.03},
{"rabota/company", 0.03},
{"rabota/company_only", 0.03},
{"rabota/profession", 0.03},
{"rabota/region", 0.03},
{"rlsfacts", 0.03},
{"table_snip_ctr", 0.03},
{"vertis", 0.03},
{"wikifacts", 0.03},

{"__unknown", UNKNOWN_K},

{"", GENERIC_K},
{"generic", GENERIC_K}

};

}

TOptions::TOptions(int argc, const char* argv[]) {
    NLastGetopt::TOpts opts;
    opts.AddHelpOption();
    opts.AddCharOption('s', "mapreduce server")
        .DefaultValue("cedar00.search.yandex.net:8013")
        .StoreResult(&ServerName);
    opts.AddCharOption('u', "mapreduce user")
        .DefaultValue("tmp")
        .StoreResult(&UserName);
    opts.AddCharOption('m', "min date of period YYYYMMDD (e.g. 20141001)")
        .Required()
        .RequiredArgument("DATE")
        .StoreResult(&MinDate);
    opts.AddCharOption('M', "max date of period YYYYMMDD (e.g. 20141031)")
        .Required()
        .RequiredArgument("DATE")
        .StoreResult(&MaxDate);
    opts.AddCharOption('d', "filter by domain (e.g. ru, ua, tr)")
        .StoreResult(&DomainFilter, "");
    opts.AddCharOption('r', "file to write result")
        .Required()
        .RequiredArgument("PATH")
        .StoreResult(&OutputFileName);
    opts.AddLongOption("fast", "sample by uid (1%)")
        .NoArgument()
        .SetFlag(&SampleByUid);
    opts.AddCharOption('c', "file with Coefficients (snippetType,coefficient)")
        .StoreResult(&ConfigFileName);
    opts.AddCharOption('l', "local statistics file (YYYMMDD<TAB>domain<TAB>statistics)")
         .StoreResult(&StatisticsFileName);
    opts.SetFreeArgsMax(0);
    opts.SetAllowSingleDashForLong(true);
    NLastGetopt::TOptsParseResult(&opts, argc, argv);
    LoadCoefficients(ConfigFileName);
}

void TOptions::LoadCoefficients(const TString& configFileName) {
    if (!configFileName) {
        Cerr << "Using default coefficients" << Endl;
        SnippetType2K = SNIPPET_TO_K_DEFAULT;
        return;
    }
    Cout << "Loading coefficients from " << configFileName << Endl;
    TFileInput file(configFileName);
    TString line;
    while (file.ReadLine(line)) {
       TString snippetType;
       double k;
       Split(TStringBuf(line.data()), ',', snippetType, k);
       SnippetType2K[snippetType] = k;
    }
}

bool TOptions::IsUnknownSnippetType(const TString& snippetType) const {
    return !SnippetType2K.contains(snippetType);
}

double TOptions::GetK(const TString& snippetType) const {
    {
       const auto& it = SnippetType2K.find(snippetType);
        if (it != SnippetType2K.end()) {
            return it->second;
        }
    }
    {
        const auto& it = SnippetType2K.find("__unknown");
        if (it != SnippetType2K.end())
            return it->second;
    }
    return UNKNOWN_K;
}
