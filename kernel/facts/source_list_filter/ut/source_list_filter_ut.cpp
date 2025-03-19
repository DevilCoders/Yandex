#include "source_list_filter.h"

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/generic/yexception.h>


using namespace NFacts;

static const TString TestConfigJson = R"ELEPHANT({
        "sources": [
            "",
            "fake_source",
            "fresh_console_fact",
            "object_facts"
        ],
        "serp_types": [
            "calculator",
            "time"
        ],
        "fact_types": [
            "entity_as_fact"
        ],
        "hostnames": [
            "ru.wikipedia.org",
            "ru.m.wikipedia.org"
        ]
    })ELEPHANT";

static const TString TestConfigJson1 = R"ELEPHANT({
        "sources": [
            "",
            "fake_source",
            "fresh_console_fact"
        ],
        "fact_types": [
            "entity_as_fact"
        ],
        "hostnames": [
            "ru.wikipedia.org",
            "ru.m.wikipedia.org"
        ]
    })ELEPHANT";

static const TString TestConfigJson2 = R"ELEPHANT({
        "sources": [
            "fresh_console_fact",
            "object_facts"
        ],
        "serp_types": [
            "calculator",
            "time"
        ]
    })ELEPHANT";


Y_UNIT_TEST_SUITE(TSourceListFilter_Construct) {

Y_UNIT_TEST(Successful) {
    THolder<TSourceListFilter> sourceListFilter;
    UNIT_ASSERT_NO_EXCEPTION(sourceListFilter.Reset(new TSourceListFilter(TestConfigJson)));
}

Y_UNIT_TEST(SuccessfulMulti) {
    THolder<TSourceListFilter> sourceListFilter;
    UNIT_ASSERT_NO_EXCEPTION(sourceListFilter.Reset(new TSourceListFilter({TestConfigJson1, TestConfigJson2})));
}

Y_UNIT_TEST(EmptyLists) {
    THolder<TSourceListFilter> sourceListFilter;
    UNIT_ASSERT_NO_EXCEPTION(sourceListFilter.Reset(new TSourceListFilter("{}")));
}

Y_UNIT_TEST(ErrorInvalidJson) {
    const TString configJson = R"ELEPHANT({
            "sources": [
                "fake_source",
                "fresh_console_fact",
            ]
        })ELEPHANT";

    UNIT_ASSERT_EXCEPTION_CONTAINS(TSourceListFilter(configJson),
        yexception, "JSON error at offset 108 (Offset: 108, Code: 3, Error: Invalid value.)");
}

Y_UNIT_TEST(ErrorInvalidSchemeNotArray) {
    const TString configJson = R"ELEPHANT({
            "sources": 1
        })ELEPHANT";

    UNIT_ASSERT_EXCEPTION_CONTAINS(TSourceListFilter(configJson),
        yexception, "Error: path '/sources' is not an array");
}

Y_UNIT_TEST(ErrorInvalidSchemeNotString) {
    const TString configJson = R"ELEPHANT({
            "sources": [1, 2 ,3]
        })ELEPHANT";

    UNIT_ASSERT_EXCEPTION_CONTAINS(TSourceListFilter(configJson),
        yexception, "Error: path '/sources/0' is not string");
}

}  // Y_UNIT_TEST_SUITE(TSourceListFilter_Construct)


Y_UNIT_TEST_SUITE(TSourceListFilter_IsSourceListed) {

Y_UNIT_TEST(Whitelist) {
    THolder<TSourceListFilter> sourceListFilter;
    UNIT_ASSERT_NO_EXCEPTION(sourceListFilter.Reset(new TSourceListFilter(TestConfigJson, TSourceListFilter::EFilterMode::Whitelist)));

    UNIT_ASSERT(!sourceListFilter->IsSourceListed(/*source*/ "x",                   /*serp_type*/ "x",              /*fact_type*/ "x",                  /*hostnames*/ {"x", "y", "z"}));
    UNIT_ASSERT(!sourceListFilter->IsSourceListed(/*source*/ "x",                   /*serp_type*/ "x",              /*fact_type*/ "x",                  /*hostnames*/ {}));
    UNIT_ASSERT(!sourceListFilter->IsSourceListed(/*source*/ "x",                   /*serp_type*/ "x",              /*fact_type*/ "x"));
    UNIT_ASSERT(!sourceListFilter->IsSourceListed(/*source*/ "x",                   /*serp_type*/ "x"));
    UNIT_ASSERT(!sourceListFilter->IsSourceListed(/*source*/ "x"));

    UNIT_ASSERT( sourceListFilter->IsSourceListed(/*source*/ "",                    /*serp_type*/ "x",              /*fact_type*/ "x",                  /*hostnames*/ {"x", "y", "z"}));
    UNIT_ASSERT( sourceListFilter->IsSourceListed(/*source*/ "fake_source",         /*serp_type*/ "x",              /*fact_type*/ "x",                  /*hostnames*/ {"x", "y", "z"}));
    UNIT_ASSERT( sourceListFilter->IsSourceListed(/*source*/ "fake_source"));

    UNIT_ASSERT( sourceListFilter->IsSourceListed(/*source*/ "x",                   /*serp_type*/ "calculator",     /*fact_type*/ "x",                  /*hostnames*/ {"x", "y", "z"}));
    UNIT_ASSERT( sourceListFilter->IsSourceListed(/*source*/ "x",                   /*serp_type*/ "calculator"));

    UNIT_ASSERT( sourceListFilter->IsSourceListed(/*source*/ "x",                   /*serp_type*/ "x",              /*fact_type*/ "entity_as_fact",     /*hostnames*/ {"x", "y", "z"}));
    UNIT_ASSERT( sourceListFilter->IsSourceListed(/*source*/ "x",                   /*serp_type*/ "x",              /*fact_type*/ "entity_as_fact"));

    UNIT_ASSERT( sourceListFilter->IsSourceListed(/*source*/ "x",                   /*serp_type*/ "x",              /*fact_type*/ "x",                  /*hostnames*/ {"ru.wikipedia.org"}));
    UNIT_ASSERT( sourceListFilter->IsSourceListed(/*source*/ "x",                   /*serp_type*/ "x",              /*fact_type*/ "x",                  /*hostnames*/ {"ru.wikipedia.org", "ru.m.wikipedia.org"}));
    UNIT_ASSERT(!sourceListFilter->IsSourceListed(/*source*/ "x",                   /*serp_type*/ "x",              /*fact_type*/ "x",                  /*hostnames*/ {"ru.wikipedia.org", "x"}));
}

Y_UNIT_TEST(WhitelistMulti) {
    THolder<TSourceListFilter> sourceListFilter;
    UNIT_ASSERT_NO_EXCEPTION(sourceListFilter.Reset(new TSourceListFilter({TestConfigJson1, TestConfigJson2}, TSourceListFilter::EFilterMode::Whitelist)));

    UNIT_ASSERT(!sourceListFilter->IsSourceListed(/*source*/ "x",                   /*serp_type*/ "x",              /*fact_type*/ "x",                  /*hostnames*/ {"x", "y", "z"}));

    UNIT_ASSERT( sourceListFilter->IsSourceListed(/*source*/ "",                    /*serp_type*/ "x",              /*fact_type*/ "x",                  /*hostnames*/ {"x", "y", "z"}));
    UNIT_ASSERT( sourceListFilter->IsSourceListed(/*source*/ "fake_source",         /*serp_type*/ "x",              /*fact_type*/ "x",                  /*hostnames*/ {"x", "y", "z"}));
    UNIT_ASSERT( sourceListFilter->IsSourceListed(/*source*/ "fresh_console_fact",  /*serp_type*/ "x",              /*fact_type*/ "x",                  /*hostnames*/ {"x", "y", "z"}));
    UNIT_ASSERT( sourceListFilter->IsSourceListed(/*source*/ "object_facts",        /*serp_type*/ "x",              /*fact_type*/ "x",                  /*hostnames*/ {"x", "y", "z"}));

    UNIT_ASSERT( sourceListFilter->IsSourceListed(/*source*/ "x",                   /*serp_type*/ "calculator",     /*fact_type*/ "x",                  /*hostnames*/ {"x", "y", "z"}));
    UNIT_ASSERT( sourceListFilter->IsSourceListed(/*source*/ "x",                   /*serp_type*/ "time",           /*fact_type*/ "x",                  /*hostnames*/ {"x", "y", "z"}));

    UNIT_ASSERT( sourceListFilter->IsSourceListed(/*source*/ "x",                   /*serp_type*/ "x",              /*fact_type*/ "entity_as_fact",     /*hostnames*/ {"x", "y", "z"}));
    UNIT_ASSERT( sourceListFilter->IsSourceListed(/*source*/ "x",                   /*serp_type*/ "x",              /*fact_type*/ "entity_as_fact"));

    UNIT_ASSERT( sourceListFilter->IsSourceListed(/*source*/ "x",                   /*serp_type*/ "x",              /*fact_type*/ "x",                  /*hostnames*/ {"ru.wikipedia.org"}));
    UNIT_ASSERT( sourceListFilter->IsSourceListed(/*source*/ "x",                   /*serp_type*/ "x",              /*fact_type*/ "x",                  /*hostnames*/ {"ru.wikipedia.org", "ru.m.wikipedia.org"}));
    UNIT_ASSERT(!sourceListFilter->IsSourceListed(/*source*/ "x",                   /*serp_type*/ "x",              /*fact_type*/ "x",                  /*hostnames*/ {"ru.wikipedia.org", "x"}));
}

Y_UNIT_TEST(Blacklist) {
    THolder<TSourceListFilter> sourceListFilter;
    UNIT_ASSERT_NO_EXCEPTION(sourceListFilter.Reset(new TSourceListFilter(TestConfigJson, TSourceListFilter::EFilterMode::Blacklist)));

    UNIT_ASSERT(!sourceListFilter->IsSourceListed(/*source*/ "x",                   /*serp_type*/ "x",              /*fact_type*/ "x",                  /*hostnames*/ {"x", "y", "z"}));
    UNIT_ASSERT(!sourceListFilter->IsSourceListed(/*source*/ "x",                   /*serp_type*/ "x",              /*fact_type*/ "x",                  /*hostnames*/ {}));
    UNIT_ASSERT(!sourceListFilter->IsSourceListed(/*source*/ "x",                   /*serp_type*/ "x",              /*fact_type*/ "x"));
    UNIT_ASSERT(!sourceListFilter->IsSourceListed(/*source*/ "x",                   /*serp_type*/ "x"));
    UNIT_ASSERT(!sourceListFilter->IsSourceListed(/*source*/ "x"));

    UNIT_ASSERT( sourceListFilter->IsSourceListed(/*source*/ "",                    /*serp_type*/ "x",              /*fact_type*/ "x",                  /*hostnames*/ {"x", "y", "z"}));
    UNIT_ASSERT( sourceListFilter->IsSourceListed(/*source*/ "fake_source",         /*serp_type*/ "x",              /*fact_type*/ "x",                  /*hostnames*/ {"x", "y", "z"}));
    UNIT_ASSERT( sourceListFilter->IsSourceListed(/*source*/ "fake_source"));

    UNIT_ASSERT( sourceListFilter->IsSourceListed(/*source*/ "x",                   /*serp_type*/ "calculator",     /*fact_type*/ "x",                  /*hostnames*/ {"x", "y", "z"}));
    UNIT_ASSERT( sourceListFilter->IsSourceListed(/*source*/ "x",                   /*serp_type*/ "calculator"));

    UNIT_ASSERT( sourceListFilter->IsSourceListed(/*source*/ "x",                   /*serp_type*/ "x",              /*fact_type*/ "entity_as_fact",     /*hostnames*/ {"x", "y", "z"}));
    UNIT_ASSERT( sourceListFilter->IsSourceListed(/*source*/ "x",                   /*serp_type*/ "x",              /*fact_type*/ "entity_as_fact"));

    UNIT_ASSERT( sourceListFilter->IsSourceListed(/*source*/ "x",                   /*serp_type*/ "x",              /*fact_type*/ "x",                  /*hostnames*/ {"ru.wikipedia.org"}));
    UNIT_ASSERT( sourceListFilter->IsSourceListed(/*source*/ "x",                   /*serp_type*/ "x",              /*fact_type*/ "x",                  /*hostnames*/ {"ru.wikipedia.org", "ru.m.wikipedia.org"}));
    UNIT_ASSERT( sourceListFilter->IsSourceListed(/*source*/ "x",                   /*serp_type*/ "x",              /*fact_type*/ "x",                  /*hostnames*/ {"ru.wikipedia.org", "x"}));
}

Y_UNIT_TEST(WhitelistDefault) {
    THolder<TSourceListFilter> sourceListFilter;
    UNIT_ASSERT_NO_EXCEPTION(sourceListFilter.Reset(new TSourceListFilter(TestConfigJson /*, filterMode = Whitelist*/)));

    UNIT_ASSERT(!sourceListFilter->IsSourceListed(/*source*/ "x",                   /*serp_type*/ "x",              /*fact_type*/ "x",                  /*hostnames*/ {"ru.wikipedia.org", "x"}));
}

Y_UNIT_TEST(Normalize) {
    THolder<TSourceListFilter> sourceListFilter;
    UNIT_ASSERT_NO_EXCEPTION(sourceListFilter.Reset(new TSourceListFilter(TestConfigJson)));

    UNIT_ASSERT( sourceListFilter->IsSourceListed(/*source*/ "fake:source",         /*serp_type*/ "x",              /*fact_type*/ "x",                  /*hostnames*/ {"x", "y", "z"}));
    UNIT_ASSERT( sourceListFilter->IsSourceListed(/*source*/ "x",                   /*serp_type*/ "x",              /*fact_type*/ "entity:as_fact",     /*hostnames*/ {"x", "y", "z"}));
}

}  // Y_UNIT_TEST_SUITE(TSourceListFilter_IsSourceListed)
