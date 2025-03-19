#include "compound_filter.h"

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/ptr.h>


namespace NFacts {

class TCompoundFilterTestAdapter {
public:
    using TConfig = TCompoundFilter::TConfig;

    static const TConfig& GetConfig(const TCompoundFilter& compoundFilter) noexcept {
        return compoundFilter.Config;
    }
};

} // namespace NFacts


using namespace NFacts;


Y_UNIT_TEST_SUITE(TCompoundFilter) {

Y_UNIT_TEST(ConstructFromString) {
    const TString configJson = R"ELEPHANT({
            "blacklist": {
                "types": [
                    "fact_snippet",
                    "featured_snippet"
                ],
                "sources": [
                    "alisa_toloka",
                    "toloka_indirect"
                ],
                "substrings": [
                    "иванов",
                    "петров",
                    "сидоров"
                ]
            },
            "whitelist": {
                "sources": [
                    "calculator",
                    "object_facts"
                ],
                "hostnames": [
                    "ru.m.wikipedia.org",
                    "ru.wikipedia.org"
                ],
                "source_and_hostnames": [
                    {
                        "source": "fact_instruction",
                        "hostname": "povar.ru"
                    },
                    {
                        "source": "toloka",
                        "hostname": "1tv.ru"
                    }
                ]
            }
        })ELEPHANT";
    THolder<TCompoundFilter> compoundFilterPtr;
    UNIT_ASSERT_NO_EXCEPTION(compoundFilterPtr.Reset(new TCompoundFilter(configJson)));
    const TCompoundFilterTestAdapter::TConfig& config = TCompoundFilterTestAdapter::GetConfig(*compoundFilterPtr);
    UNIT_ASSERT(config.Blacklist.Types.size() == 2);
    UNIT_ASSERT(config.Blacklist.Types.contains("fact_snippet"));
    UNIT_ASSERT(config.Blacklist.Types.contains("featured_snippet"));
    UNIT_ASSERT(config.Blacklist.Sources.size() == 2);
    UNIT_ASSERT(config.Blacklist.Sources.contains("alisa_toloka"));
    UNIT_ASSERT(config.Blacklist.Sources.contains("toloka_indirect"));
    UNIT_ASSERT(config.Blacklist.Substrings.size() == 3);
    UNIT_ASSERT(config.Blacklist.Substrings[0] == "иванов");
    UNIT_ASSERT(config.Blacklist.Substrings[1] == "петров");
    UNIT_ASSERT(config.Blacklist.Substrings[2] == "сидоров");
    UNIT_ASSERT(config.Whitelist.Sources.size() == 2);
    UNIT_ASSERT(config.Whitelist.Sources.contains("calculator"));
    UNIT_ASSERT(config.Whitelist.Sources.contains("object_facts"));
    UNIT_ASSERT(config.Whitelist.Hostnames.size() == 2);
    UNIT_ASSERT(config.Whitelist.Hostnames.contains("ru.m.wikipedia.org"));
    UNIT_ASSERT(config.Whitelist.Hostnames.contains("ru.wikipedia.org"));
    UNIT_ASSERT(config.Whitelist.SourceAndHostnames.size() == 2);
    UNIT_ASSERT(config.Whitelist.SourceAndHostnames.contains(std::make_tuple("fact_instruction", "povar.ru")));
    UNIT_ASSERT(config.Whitelist.SourceAndHostnames.contains(std::make_tuple("toloka", "1tv.ru")));
}

Y_UNIT_TEST(ValidateJsonFormat) {
    const TString configJson = R"ELEPHANT({
        )ELEPHANT";
    UNIT_ASSERT_EXCEPTION_CONTAINS(TCompoundFilter(configJson),
        yexception, "JSON error at offset 10 (invalid or truncated)");
}

Y_UNIT_TEST(ValidateBlacklistRequired) {
    const TString configJson = R"ELEPHANT({
        })ELEPHANT";
    UNIT_ASSERT_EXCEPTION_CONTAINS(TCompoundFilter(configJson),
        yexception, "Error: path \"/blacklist\" is a required field and is not found");
}

Y_UNIT_TEST(ValidateBlacklistSubstringsRequired) {
    const TString configJson = R"ELEPHANT({
            "blacklist": {
            }
        })ELEPHANT";
    UNIT_ASSERT_EXCEPTION_CONTAINS(TCompoundFilter(configJson),
        yexception, "Error: path \"/blacklist/substrings\" is a required field and is not found");
}

Y_UNIT_TEST(ValidateBlacklistSubstringsMayBeEmpty) {
    const TString configJson = R"ELEPHANT({
            "blacklist": {
                "substrings": [
                ]
            }
        })ELEPHANT";
    UNIT_ASSERT_NO_EXCEPTION(TCompoundFilter(configJson));
}

Y_UNIT_TEST(ValidateWhitelistOptional) {
    const TString configJson = R"ELEPHANT({
            "blacklist": {
                "substrings": [
                    "иванов",
                    "петров",
                    "сидоров"
                ]
            }
        })ELEPHANT";
    UNIT_ASSERT_NO_EXCEPTION(TCompoundFilter(configJson));
}

Y_UNIT_TEST(ValidateWhitelistFieldsOptional) {
    const TString configJson = R"ELEPHANT({
            "blacklist": {
                "substrings": [
                    "иванов",
                    "петров",
                    "сидоров"
                ]
            },
            "whitelist": {
            }
        })ELEPHANT";
    UNIT_ASSERT_NO_EXCEPTION(TCompoundFilter(configJson));
}

Y_UNIT_TEST(ValidateWhitelistSourceAndHostnamesItemSourceRequired) {
    const TString configJson = R"ELEPHANT({
            "blacklist": {
                "substrings": [
                    "иванов",
                    "петров",
                    "сидоров"
                ]
            },
            "whitelist": {
                "source_and_hostnames": [
                    {
                        "hostname": "povar.ru"
                    }
                ]
            }
        })ELEPHANT";
    UNIT_ASSERT_EXCEPTION_CONTAINS(TCompoundFilter(configJson),
        yexception, "Error: path \"/whitelist/source_and_hostnames/0/source\" is a required field and is not found");
}

Y_UNIT_TEST(ValidateWhitelistSourceAndHostnamesItemHostnameRequired) {
    const TString configJson = R"ELEPHANT({
            "blacklist": {
                "substrings": [
                    "иванов",
                    "петров",
                    "сидоров"
                ]
            },
            "whitelist": {
                "source_and_hostnames": [
                    {
                        "source": "fact_instruction"
                    }
                ]
            }
        })ELEPHANT";
    UNIT_ASSERT_EXCEPTION_CONTAINS(TCompoundFilter(configJson),
        yexception, "Error: path \"/whitelist/source_and_hostnames/0/hostname\" is a required field and is not found");
}

Y_UNIT_TEST(FindBlacklistSubstrings) {
    const TString configJson = R"ELEPHANT({
            "blacklist": {
                "substrings": [
                    "иванов",
                    "петров",
                    "сидоров"
                ]
            }
        })ELEPHANT";
    THolder<TCompoundFilter> compoundFilterPtr;
    UNIT_ASSERT_NO_EXCEPTION(compoundFilterPtr.Reset(new TCompoundFilter(configJson)));
    UNIT_ASSERT(compoundFilterPtr->Filter(/*type*/ "", /*source*/ "", /*hostname*/ {}, /*text*/ "иванов") == "иванов");
    UNIT_ASSERT(compoundFilterPtr->Filter(/*type*/ "", /*source*/ "", /*hostname*/ {}, /*text*/ "петров") == "петров");
    UNIT_ASSERT(compoundFilterPtr->Filter(/*type*/ "", /*source*/ "", /*hostname*/ {}, /*text*/ "сидоров") == "сидоров");
    UNIT_ASSERT(compoundFilterPtr->Filter(/*type*/ "", /*source*/ "", /*hostname*/ {}, /*text*/ "иванова") == "иванов");
    UNIT_ASSERT(compoundFilterPtr->Filter(/*type*/ "", /*source*/ "", /*hostname*/ {}, /*text*/ "эксиванов") == "иванов");
    UNIT_ASSERT(compoundFilterPtr->Filter(/*type*/ "", /*source*/ "", /*hostname*/ {}, /*text*/ "товарищ иванов, вас вызывают") == "иванов");
    UNIT_ASSERT(compoundFilterPtr->Filter(/*type*/ "", /*source*/ "", /*hostname*/ {}, /*text*/ "иванов петров") == "иванов");
    UNIT_ASSERT(compoundFilterPtr->Filter(/*type*/ "", /*source*/ "", /*hostname*/ {}, /*text*/ "ивонов петров") == "петров");
    UNIT_ASSERT(compoundFilterPtr->Filter(/*type*/ "", /*source*/ "", /*hostname*/ {}, /*text*/ "абвгде").empty());
    UNIT_ASSERT(compoundFilterPtr->Filter(/*type*/ "", /*source*/ "", /*hostname*/ {}, /*text*/ "").empty());
}

Y_UNIT_TEST(FindBlacklistTypes) {
    const TString configJson = R"ELEPHANT({
            "blacklist": {
                "types": [
                    "fact_snippet",
                    "featured_snippet"
                ],
                "substrings": [
                    "иванов",
                    "петров",
                    "сидоров"
                ]
            }
        })ELEPHANT";
    THolder<TCompoundFilter> compoundFilterPtr;
    UNIT_ASSERT_NO_EXCEPTION(compoundFilterPtr.Reset(new TCompoundFilter(configJson)));
    UNIT_ASSERT(compoundFilterPtr->Filter(/*type*/ "fact_snippet",     /*source*/ "", /*hostname*/ {}, /*text*/ "иванов") == "иванов");
    UNIT_ASSERT(compoundFilterPtr->Filter(/*type*/ "featured_snippet", /*source*/ "", /*hostname*/ {}, /*text*/ "иванов") == "иванов");
    UNIT_ASSERT(compoundFilterPtr->Filter(/*type*/ "suggest_fact",     /*source*/ "", /*hostname*/ {}, /*text*/ "иванов").empty());
}

Y_UNIT_TEST(FindBlacklistSources) {
    const TString configJson = R"ELEPHANT({
            "blacklist": {
                "sources": [
                    "alisa_toloka",
                    "toloka_indirect"
                ],
                "substrings": [
                    "иванов",
                    "петров",
                    "сидоров"
                ]
            }
        })ELEPHANT";
    THolder<TCompoundFilter> compoundFilterPtr;
    UNIT_ASSERT_NO_EXCEPTION(compoundFilterPtr.Reset(new TCompoundFilter(configJson)));
    UNIT_ASSERT(compoundFilterPtr->Filter(/*type*/ "", /*source*/ "alisa_toloka",          /*hostname*/ {}, /*text*/ "иванов") == "иванов");
    UNIT_ASSERT(compoundFilterPtr->Filter(/*type*/ "", /*source*/ "toloka_indirect",       /*hostname*/ {}, /*text*/ "иванов") == "иванов");
    UNIT_ASSERT(compoundFilterPtr->Filter(/*type*/ "", /*source*/ "list_featured_snippet", /*hostname*/ {}, /*text*/ "иванов").empty());
}

Y_UNIT_TEST(FindBlacklistTypesOrSources) {
    const TString configJson = R"ELEPHANT({
            "blacklist": {
                "types": [
                    "fact_snippet",
                    "featured_snippet"
                ],
                "sources": [
                    "alisa_toloka",
                    "toloka_indirect"
                ],
                "substrings": [
                    "иванов",
                    "петров",
                    "сидоров"
                ]
            }
        })ELEPHANT";
    THolder<TCompoundFilter> compoundFilterPtr;
    UNIT_ASSERT_NO_EXCEPTION(compoundFilterPtr.Reset(new TCompoundFilter(configJson)));
    UNIT_ASSERT(compoundFilterPtr->Filter(/*type*/ "featured_snippet", /*source*/ "list_featured_snippet", /*hostname*/ {}, /*text*/ "иванов") == "иванов");
    UNIT_ASSERT(compoundFilterPtr->Filter(/*type*/ "suggest_fact",     /*source*/ "alisa_toloka",          /*hostname*/ {}, /*text*/ "иванов") == "иванов");
    UNIT_ASSERT(compoundFilterPtr->Filter(/*type*/ "alisa_toloka",     /*source*/ "fact_snippet",          /*hostname*/ {}, /*text*/ "иванов").empty());
}

Y_UNIT_TEST(FindWhitelistSources) {
    const TString configJson = R"ELEPHANT({
            "blacklist": {
                "substrings": [
                    "иванов",
                    "петров",
                    "сидоров"
                ]
            },
            "whitelist": {
                "sources": [
                    "calculator",
                    "object_facts"
                ]
            }
        })ELEPHANT";
    THolder<TCompoundFilter> compoundFilterPtr;
    UNIT_ASSERT_NO_EXCEPTION(compoundFilterPtr.Reset(new TCompoundFilter(configJson)));
    UNIT_ASSERT(compoundFilterPtr->Filter(/*type*/ "", /*source*/ "",                  /*hostname*/ {}, /*text*/ "иванов") == "иванов");
    UNIT_ASSERT(compoundFilterPtr->Filter(/*type*/ "", /*source*/ "calculator",        /*hostname*/ {}, /*text*/ "иванов").empty());
    UNIT_ASSERT(compoundFilterPtr->Filter(/*type*/ "", /*source*/ "object_facts",      /*hostname*/ {}, /*text*/ "иванов").empty());
    UNIT_ASSERT(compoundFilterPtr->Filter(/*type*/ "", /*source*/ "wizard_calculator", /*hostname*/ {}, /*text*/ "иванов") == "иванов");
    UNIT_ASSERT(compoundFilterPtr->Filter(/*type*/ "", /*source*/ "object_factz",      /*hostname*/ {}, /*text*/ "иванов") == "иванов");
}

Y_UNIT_TEST(FindWhitelistHostnames) {
    const TString configJson = R"ELEPHANT({
            "blacklist": {
                "substrings": [
                    "иванов",
                    "петров",
                    "сидоров"
                ]
            },
            "whitelist": {
                "hostnames": [
                    "ru.m.wikipedia.org",
                    "ru.wikipedia.org"
                ]
            }
        })ELEPHANT";
    THolder<TCompoundFilter> compoundFilterPtr;
    UNIT_ASSERT_NO_EXCEPTION(compoundFilterPtr.Reset(new TCompoundFilter(configJson)));
    UNIT_ASSERT(compoundFilterPtr->Filter(/*type*/ "", /*source*/ "", /*hostname*/ {},                                       /*text*/ "иванов") == "иванов");
    UNIT_ASSERT(compoundFilterPtr->Filter(/*type*/ "", /*source*/ "", /*hostname*/ {"ru.m.wikipedia.org"},                     /*text*/ "иванов").empty());
    UNIT_ASSERT(compoundFilterPtr->Filter(/*type*/ "", /*source*/ "", /*hostname*/ {"ru.wikipedia.org"},                       /*text*/ "иванов").empty());
    UNIT_ASSERT(compoundFilterPtr->Filter(/*type*/ "", /*source*/ "", /*hostname*/ {"ru.wikipedia.org", "ru.m.wikipedia.org"}, /*text*/ "иванов").empty());
    UNIT_ASSERT(compoundFilterPtr->Filter(/*type*/ "", /*source*/ "", /*hostname*/ {"ru.wikipedia.org", "en.wikipedia.org"},   /*text*/ "иванов") == "иванов");
    UNIT_ASSERT(compoundFilterPtr->Filter(/*type*/ "", /*source*/ "", /*hostname*/ {"en.wikipedia.org"},                       /*text*/ "иванов") == "иванов");
    UNIT_ASSERT(compoundFilterPtr->Filter(/*type*/ "", /*source*/ "", /*hostname*/ {"wikipedia.org"},                          /*text*/ "иванов") == "иванов");
    UNIT_ASSERT(compoundFilterPtr->Filter(/*type*/ "", /*source*/ "", /*hostname*/ {"beru.wikipedia.org"},                     /*text*/ "иванов") == "иванов");
    UNIT_ASSERT(compoundFilterPtr->Filter(/*type*/ "", /*source*/ "", /*hostname*/ {"ru.wikipedia.org?"},                      /*text*/ "иванов") == "иванов");
}

Y_UNIT_TEST(FindWhitelistHostAndSources) {
    const TString configJson = R"ELEPHANT({
            "blacklist": {
                "substrings": [
                    "иванов",
                    "петров",
                    "сидоров"
                ]
            },
            "whitelist": {
                "source_and_hostnames": [
                    {
                        "source": "fact_instruction",
                        "hostname": "povar.ru"
                    },
                    {
                        "source": "toloka",
                        "hostname": "1tv.ru"
                    }
                ]
            }
        })ELEPHANT";
    THolder<TCompoundFilter> compoundFilterPtr;
    UNIT_ASSERT_NO_EXCEPTION(compoundFilterPtr.Reset(new TCompoundFilter(configJson)));
    UNIT_ASSERT(compoundFilterPtr->Filter(/*type*/ "", /*source*/ "",                 /*hostname*/ {},                    /*text*/ "иванов") == "иванов");
    UNIT_ASSERT(compoundFilterPtr->Filter(/*type*/ "", /*source*/ "fact_instruction", /*hostname*/ {"povar.ru"},            /*text*/ "иванов").empty());
    UNIT_ASSERT(compoundFilterPtr->Filter(/*type*/ "", /*source*/ "fact_instruction", /*hostname*/ {"povar.ru", "1tv.ru"},  /*text*/ "иванов") == "иванов");
    UNIT_ASSERT(compoundFilterPtr->Filter(/*type*/ "", /*source*/ "toloka",           /*hostname*/ {"1tv.ru"},              /*text*/ "иванов").empty());
    UNIT_ASSERT(compoundFilterPtr->Filter(/*type*/ "", /*source*/ "",                 /*hostname*/ {"povar.ru"},            /*text*/ "иванов") == "иванов");
    UNIT_ASSERT(compoundFilterPtr->Filter(/*type*/ "", /*source*/ "toloka",           /*hostname*/ {""},                    /*text*/ "иванов") == "иванов");
    UNIT_ASSERT(compoundFilterPtr->Filter(/*type*/ "", /*source*/ "toloka",           /*hostname*/ {"povar.ru"},            /*text*/ "иванов") == "иванов");
    UNIT_ASSERT(compoundFilterPtr->Filter(/*type*/ "", /*source*/ "fact_instruction", /*hostname*/ {"povar.ru/"},           /*text*/ "иванов") == "иванов");
}

}  // Y_UNIT_TEST_SUITE(TCompoundFilter)
