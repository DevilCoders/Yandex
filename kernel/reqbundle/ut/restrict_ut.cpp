#include "reqbundle_builder.h"

#include <kernel/reqbundle/restrict.h>
#include <kernel/reqbundle/print.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/algorithm.h>

using namespace NReqBundle;

using TRestrictVector = TDeque<TRestrictOptions>;

Y_UNIT_TEST_SUITE(RestrictReqBundleTest) {
    void CheckSerializer(const TAllRestrictOptions& opts) {
        NJson::TJsonValue value;
        value = opts.ToJson();
        TAllRestrictOptions opts2;
        Cdbg << "PARSE " << value << Endl;
        UNIT_ASSERT_NO_EXCEPTION(opts2.FromJson(value));
        UNIT_ASSERT_EQUAL(opts, opts2);
    }

    void InitAllOptions(TAllRestrictOptions& opts) {
        opts.Add(TFacetId{TExpansion::XfDtShow}).SetMaxBlocks(60);
        opts.Add(TFacetId{TExpansion::QueryToDoc}).SetMaxRequests(50);
    }

    Y_UNIT_TEST(TestSerialize) {
        {
            TAllRestrictOptions allOpts;
            CheckSerializer(allOpts);
            NJson::TJsonValue value;
            value = allOpts.ToJson();
            UNIT_ASSERT_EQUAL(ToString(value), "[]");
        }
        {
            TAllRestrictOptions allOpts;
            InitAllOptions(allOpts);
            CheckSerializer(allOpts);
        }
    }

    Y_UNIT_TEST(TestFromJson) {
        TAllRestrictOptions opts;
        opts.FromJsonString(
            "{\"Facet\":{\"Expansion\":\"Experiment0\"},\"MaxRequests\":11}");
        UNIT_ASSERT_EQUAL(opts.size(), 1);
        UNIT_ASSERT_EQUAL(
            opts.FindAll(MakeFacetIdPredicate(TExpansion::Experiment0)),
            TRestrictVector{TRestrictOptions().SetMaxRequests(11)}
        );

        opts.clear();

        opts.FromJsonString(
            "[{\"Facet\":{\"Expansion\":\"Experiment0\",\"RegionId\":\"Country\"},\"MaxRequests\":12,\"MaxBlocks\":13},"
            "{\"Facet\":{\"Expansion\":\"Experiment1\"},\"Enabled\":false}]");

        UNIT_ASSERT_EQUAL(opts.size(), 2);
        UNIT_ASSERT_EQUAL(
            opts.FindAll(MakeFacetIdPredicate(TExpansion::Experiment0, TRegionId::ClassCountry())),
            TRestrictVector{TRestrictOptions().SetMaxRequests(12).SetMaxBlocks(13)}
        );
        UNIT_ASSERT_EQUAL(
            opts.FindAll(MakeFacetIdPredicate(TExpansion::Experiment1)),
            TRestrictVector{TRestrictOptions().SetEnabled(false)}
        );
    }

    Y_UNIT_TEST(TestFromScheme) {
        TAllRestrictOptions opts;
        opts.FromJsonString(
            "{Facet:{Expansion:Experiment0},MaxRequests:11}");
        UNIT_ASSERT_EQUAL(opts.size(), 1);
        UNIT_ASSERT_EQUAL(
            opts.FindAll(MakeFacetIdPredicate(TExpansion::Experiment0)),
            TRestrictVector{TRestrictOptions().SetMaxRequests(11)}
        );

        opts.clear();

        opts.FromJsonString(
            "[{Facet:{Expansion:Experiment0,RegionId:Country},MaxRequests:12,MaxBlocks:13},"
            "{Facet:{Expansion:Experiment1},Enabled:false}]");

        UNIT_ASSERT_EQUAL(opts.size(), 2);
        UNIT_ASSERT_EQUAL(
            opts.FindAll(MakeFacetIdPredicate(TExpansion::Experiment0, TRegionId::ClassCountry())),
            TRestrictVector{TRestrictOptions().SetMaxRequests(12).SetMaxBlocks(13)}
        );
        UNIT_ASSERT_EQUAL(
            opts.FindAll(MakeFacetIdPredicate(TExpansion::Experiment1)),
            TRestrictVector{TRestrictOptions().SetEnabled(false)}
        );
    }

    Y_UNIT_TEST(TestScale) {
        {
            TAllRestrictOptions opts;
            TAllRestrictOptions savedOpts = opts;
            ScaleRestrictOptions(opts, 1.0);
            UNIT_ASSERT_EQUAL(opts, savedOpts);
            ScaleRestrictOptions(opts, 2.0);
            UNIT_ASSERT_EQUAL(opts, savedOpts);
        }
        {
            TAllRestrictOptions opts;
            TAllRestrictOptions savedOpts = opts;
            ScaleRestrictOptions(opts, 1.0);
            UNIT_ASSERT_EQUAL(opts, savedOpts);
            ScaleRestrictOptions(opts, 2.0);

            UNIT_ASSERT_EQUAL(opts.size(), savedOpts.size());

            auto savedPtr = savedOpts.begin();
            for (auto& entry : opts) {
                UNIT_ASSERT(savedPtr != savedOpts.end());
                auto& savedEntry = *savedPtr++;
                if (!savedEntry.Restrict.MaxBlocks.Defined()) {
                    UNIT_ASSERT_EQUAL(entry.Restrict.MaxBlocks, savedEntry.Restrict.MaxBlocks);
                } else {
                    UNIT_ASSERT(entry.Restrict.MaxBlocks.Defined());
                    UNIT_ASSERT_EQUAL(entry.Restrict.MaxBlocks.GetRef(), 2 * savedEntry.Restrict.MaxBlocks.GetRef());
                }
                if (!savedEntry.Restrict.MaxRequests.Defined()) {
                    UNIT_ASSERT_EQUAL(entry.Restrict.MaxRequests, savedEntry.Restrict.MaxRequests);
                } else {
                    UNIT_ASSERT(entry.Restrict.MaxRequests.Defined());
                    UNIT_ASSERT_EQUAL(entry.Restrict.MaxRequests.GetRef(), 2 * savedEntry.Restrict.MaxRequests.GetRef());
                }
            }
        }
    }

    Y_UNIT_TEST(TestRestrictEmpty) {
        TReqBundle bundle = *(TReqBundleBuilder{}).GetResult();

        Cdbg << "BUNDLE = (" << bundle << ")" << Endl;

        TReqBundle result = *(RestrictReqBundle(bundle, {})->GetResult());

        Cdbg << "RESULT = (" << result << ")" << Endl;

        UNIT_ASSERT(IsValidReqBundle(result, false));
        UNIT_ASSERT_EQUAL(result.GetNumRequests(), 0);
        UNIT_ASSERT_EQUAL(result.GetSequence().GetNumElems(), 0);
        UNIT_ASSERT_EQUAL(ToString(result), "");
    }

    Y_UNIT_TEST(TestRestrictNoConstraints) {
        TReqBundle bundle = *(TReqBundleBuilder{}
            << "a b c" << MakeFacetId(TExpansion::OriginalRequest) << 1.0f
            << "b c d" << MakeFacetId(TExpansion::Experiment0) << 0.6f
            << "c d e" << MakeFacetId(TExpansion::Experiment1) << 0.7f
            << "d e f" << MakeFacetId(TExpansion::Experiment0) << 0.8f).GetResult();

        Cdbg << "BUNDLE = (" << PrintableReqBundle(bundle, PF_FACET_NAME | PF_FACET_VALUE) << ")" << Endl;

        TReqBundle result = *(RestrictReqBundle(bundle, {})->GetResult());

        Cdbg << "RESULT = (" << PrintableReqBundle(result, PF_FACET_NAME | PF_FACET_VALUE) << ")" << Endl;

        UNIT_ASSERT(IsValidReqBundle(result, true));
        UNIT_ASSERT_EQUAL(result.GetNumRequests(), 4);
        UNIT_ASSERT_EQUAL(result.GetSequence().GetNumElems(), 6);
        UNIT_ASSERT_EQUAL(ToString(PrintableReqBundle(result, PF_FACET_NAME | PF_FACET_VALUE)), "a b c {OriginalRequest_World=1}"
            "\nd e f {Experiment0_World=0.8}"
            "\nb c d {Experiment0_World=0.6}"
            "\nc d e {Experiment1_World=0.7}");
    }

    Y_UNIT_TEST(TestRestrictRemoveOriginalRequest) {
        TReqBundle bundle = *(TReqBundleBuilder{}
            << "a b c" << MakeFacetId(TExpansion::OriginalRequest) << 1.0f
            << "b c d" << MakeFacetId(TExpansion::Experiment0) << 0.6f
            << "c d e" << MakeFacetId(TExpansion::Experiment1) << 0.7f
            << "d e f" << MakeFacetId(TExpansion::Experiment0) << 0.8f).GetResult();

        Cdbg << "BUNDLE = (" << PrintableReqBundle(bundle, PF_FACET_NAME | PF_FACET_VALUE) << ")" << Endl;

        TReqBundle result = *(RestrictReqBundle(bundle, {{MakeFacetIdPredicate(TExpansion::OriginalRequest), TRestrictOptions().SetEnabled(false)}})->GetResult());

        Cdbg << "RESULT = (" << PrintableReqBundle(result, PF_FACET_NAME | PF_FACET_VALUE) << ")" << Endl;

        UNIT_ASSERT(IsValidReqBundle(result, false));
        UNIT_ASSERT_EQUAL(result.GetNumRequests(), 3);
        UNIT_ASSERT_EQUAL(result.GetSequence().GetNumElems(), 5);
        UNIT_ASSERT_EQUAL(ToString(PrintableReqBundle(result, PF_FACET_NAME | PF_FACET_VALUE)), "d e f {Experiment0_World=0.8}"
            "\nb c d {Experiment0_World=0.6}"
            "\nc d e {Experiment1_World=0.7}");
    }

    Y_UNIT_TEST(TestRestrictByExpansions) {
        TReqBundle bundle = *(TReqBundleBuilder{}
            << "a b c" << MakeFacetId(TExpansion::OriginalRequest) << 1.0f
            << "b c d" << MakeFacetId(TExpansion::Experiment0) << 0.6f
            << "c d e" << MakeFacetId(TExpansion::Experiment1) << 0.7f
            << "d e f" << MakeFacetId(TExpansion::Experiment0) << 0.8f).GetResult();

        Cdbg << "BUNDLE = (" << PrintableReqBundle(bundle, PF_FACET_NAME | PF_FACET_VALUE) << ")" << Endl;

        TReqBundle result = *(RestrictReqBundleByExpansions(bundle, {TExpansion::Experiment0})->GetResult());

        Cdbg << "RESULT = (" << PrintableReqBundle(result, PF_FACET_NAME | PF_FACET_VALUE) << ")" << Endl;

        UNIT_ASSERT(IsValidReqBundle(result, false));
        UNIT_ASSERT_EQUAL(result.GetNumRequests(), 2);
        UNIT_ASSERT_EQUAL(result.GetSequence().GetNumElems(), 5);
        UNIT_ASSERT_EQUAL(ToString(PrintableReqBundle(result, PF_FACET_NAME | PF_FACET_VALUE)), "d e f {Experiment0_World=0.8}"
            "\nb c d {Experiment0_World=0.6}");
    }

    Y_UNIT_TEST(TestRestrictRemoveOneExpansionType) {
        TReqBundle bundle = *(TReqBundleBuilder{}
            << "a b c" << MakeFacetId(TExpansion::OriginalRequest) << 1.0f
            << "b c d" << MakeFacetId(TExpansion::Experiment0) << 0.6f
            << "c d e" << MakeFacetId(TExpansion::Experiment1) << 0.7f
            << "d e f" << MakeFacetId(TExpansion::Experiment0) << 0.8f).GetResult();

        Cdbg << "BUNDLE = (" << PrintableReqBundle(bundle, PF_FACET_NAME | PF_FACET_VALUE) << ")" << Endl;

        TReqBundle result = *(RestrictReqBundle(bundle,
            {{MakeFacetIdPredicate(TExpansion::Experiment0), TRestrictOptions().SetEnabled(false)}})->GetResult());

        Cdbg << "RESULT = (" << PrintableReqBundle(result, PF_FACET_NAME | PF_FACET_VALUE) << ")" << Endl;

        UNIT_ASSERT(IsValidReqBundle(result, true));
        UNIT_ASSERT_EQUAL(result.GetNumRequests(), 2);
        UNIT_ASSERT_EQUAL(result.GetSequence().GetNumElems(), 5);
        UNIT_ASSERT_EQUAL(ToString(PrintableReqBundle(result, PF_FACET_NAME | PF_FACET_VALUE)), "a b c {OriginalRequest_World=1}"
            "\nc d e {Experiment1_World=0.7}");
    }

    Y_UNIT_TEST(TestSingleExpansions) {
        TReqBundle bundle = *(TReqBundleBuilder{}
        << "a b c" << MakeFacetId(TExpansion::OriginalRequest) << 1.0f
        << "b c d" << MakeFacetId(TExpansion::Experiment0) << 0.6f
        << "c d e" << MakeFacetId(TExpansion::RequestWithRegionName) << 0.7f
        << "w b c" << MakeFacetId(TExpansion::RequestWithRegionName) << 1.0f
        << "d e f" << MakeFacetId(TExpansion::Experiment0) << 0.8f).GetResult();

        Cdbg << "BUNDLE = (" << PrintableReqBundle(bundle, PF_FACET_NAME | PF_FACET_VALUE) << ")" << Endl;

        UNIT_ASSERT(!IsValidReqBundle(bundle));
    }

    Y_UNIT_TEST(TestRestrictSimpleByMaxBlocks) {
        TReqBundle bundle = *(TReqBundleBuilder{}
            << "a b c" << MakeFacetId(TExpansion::Experiment0) << 0.5f).GetResult();

        Cdbg << "BUNDLE = (" << PrintableReqBundle(bundle, PF_FACET_NAME | PF_FACET_VALUE) << ")" << Endl;

        TReqBundle resultX = *(RestrictReqBundle(bundle, {{MakeFacetIdPredicate(TExpansion::Experiment0), {3, 100}}})->GetResult());

        Cdbg << "RESULT_X = (" << PrintableReqBundle(resultX, PF_FACET_NAME | PF_FACET_VALUE) << ")" << Endl;

        UNIT_ASSERT(IsValidReqBundle(resultX, false));
        UNIT_ASSERT_EQUAL(resultX.GetNumRequests(), 1);
        UNIT_ASSERT_EQUAL(resultX.GetSequence().GetNumElems(), 3);
        UNIT_ASSERT_EQUAL(ToString(PrintableReqBundle(resultX, PF_FACET_NAME | PF_FACET_VALUE)), "a b c {Experiment0_World=0.5}");

        TReqBundle resultY = *(RestrictReqBundle(bundle, {{MakeFacetIdPredicate(TExpansion::Experiment0), {2, 100}}})->GetResult());

        Cdbg << "RESULT_Y = (" << PrintableReqBundle(resultY, PF_FACET_NAME | PF_FACET_VALUE) << ")" << Endl;

        UNIT_ASSERT(IsValidReqBundle(resultY, false));
        UNIT_ASSERT_EQUAL(resultY.GetNumRequests(), 0);
        UNIT_ASSERT_EQUAL(resultY.GetSequence().GetNumElems(), 0);
        UNIT_ASSERT_EQUAL(ToString(PrintableReqBundle(resultY, PF_FACET_NAME | PF_FACET_VALUE)), "");
    }

    Y_UNIT_TEST(TestRestrictSimpleByMaxRequests) {
        TReqBundle bundle = *(TReqBundleBuilder{}
            << "a" << MakeFacetId(TExpansion::Experiment0) << 0.5f
            << "b" << MakeFacetId(TExpansion::Experiment0) << 0.6f
            << "c" << MakeFacetId(TExpansion::Experiment0) << 0.7f).GetResult();

        Cdbg << "BUNDLE = (" << PrintableReqBundle(bundle, PF_FACET_NAME | PF_FACET_VALUE) << ")" << Endl;

        TReqBundle resultX = *(RestrictReqBundle(bundle, {{MakeFacetIdPredicate(TExpansion::Experiment0), {100, 3}}})->GetResult());

        Cdbg << "RESULT_X = (" << PrintableReqBundle(resultX, PF_FACET_NAME | PF_FACET_VALUE) << ")" << Endl;

        UNIT_ASSERT(IsValidReqBundle(resultX, false));
        UNIT_ASSERT_EQUAL(resultX.GetNumRequests(), 3);
        UNIT_ASSERT_EQUAL(resultX.GetSequence().GetNumElems(), 3);
        UNIT_ASSERT_EQUAL(ToString(PrintableReqBundle(resultX, PF_FACET_NAME | PF_FACET_VALUE)), "c {Experiment0_World=0.7}\nb {Experiment0_World=0.6}\na {Experiment0_World=0.5}");

        TReqBundle resultY = *(RestrictReqBundle(bundle, {{MakeFacetIdPredicate(TExpansion::Experiment0), {100, 2}}})->GetResult());

        Cdbg << "RESULT_Y = (" << PrintableReqBundle(resultY, PF_FACET_NAME | PF_FACET_VALUE) << ")" << Endl;

        UNIT_ASSERT(IsValidReqBundle(resultY, false));
        UNIT_ASSERT_EQUAL(resultY.GetNumRequests(), 2);
        UNIT_ASSERT_EQUAL(resultY.GetSequence().GetNumElems(), 2);
        UNIT_ASSERT_EQUAL(ToString(PrintableReqBundle(resultY, PF_FACET_NAME | PF_FACET_VALUE)), "c {Experiment0_World=0.7}\nb {Experiment0_World=0.6}");
    }

    Y_UNIT_TEST(TestRestrictCountryByMaxRequests) {
        TReqBundle bundle = *(TReqBundleBuilder{}
            << "a" << MakeFacetId(TExpansion::Experiment0, TRegionId::Russia()) << 0.7f
            << "b" << MakeFacetId(TExpansion::Experiment0, TRegionId::Russia()) << 0.6f
            << "c" << MakeFacetId(TExpansion::Experiment0) << 0.5f).GetResult();

        Cdbg << "BUNDLE = (" << PrintableReqBundle(bundle, PF_FACET_NAME | PF_FACET_VALUE) << ")" << Endl;

        TReqBundle resultX = *(RestrictReqBundle(bundle, {
            {MakeFacetIdPredicate(TExpansion::Experiment0), {100, 2}},
            {MakeFacetIdPredicate(TExpansion::Experiment0, EncodeRegionClass(TRegionClass::Country)), {100, 1}}
        })->GetResult());

        Cdbg << "RESULT_X = (" << PrintableReqBundle(resultX, PF_FACET_NAME | PF_FACET_VALUE) << ")" << Endl;

        UNIT_ASSERT(IsValidReqBundle(resultX, false));
        UNIT_ASSERT_EQUAL(resultX.GetNumRequests(), 2);
        UNIT_ASSERT_EQUAL(resultX.GetSequence().GetNumElems(), 2);
        UNIT_ASSERT_EQUAL(ToString(PrintableReqBundle(resultX, PF_FACET_NAME | PF_FACET_VALUE)), "c {Experiment0_World=0.5}\na {Experiment0_Country(ru)=0.7}");
    }

     Y_UNIT_TEST(TestRestrictExpansionsOrdering) {
        TReqBundle bundle = *(TReqBundleBuilder{}
            << "a b c" << MakeFacetId(TExpansion::OriginalRequest) << 1.0f
            << "b c d" << MakeFacetId(TExpansion::Experiment0) << 0.6f
            << "c d e" << MakeFacetId(TExpansion::Experiment1) << 0.7f
            << "d e f" << MakeFacetId(TExpansion::Experiment0) << 0.8f).GetResult();

        Cdbg << "BUNDLE = (" << PrintableReqBundle(bundle, PF_FACET_NAME | PF_FACET_VALUE) << ")" << Endl;

        // NOTE. Count 2 doesn't include OriginalRequest (always accepted as special case)
        TReqBundle result = *(RestrictReqBundle(bundle, {{MakeFacetIdPredicate(TExpansion::Experiment0), {100, 1}},
            {MakeFacetIdPredicate(TExpansion::Experiment1), {1, 100}}})->GetResult());

        Cdbg << "RESULT = (" << PrintableReqBundle(result, PF_FACET_NAME | PF_FACET_VALUE) << ")" << Endl;

        UNIT_ASSERT(IsValidReqBundle(result, true));
        UNIT_ASSERT_EQUAL(result.GetNumRequests(), 3);
        UNIT_ASSERT_EQUAL(result.GetSequence().GetNumElems(), 6);
        UNIT_ASSERT_EQUAL(ToString(PrintableReqBundle(result, PF_FACET_NAME | PF_FACET_VALUE)), "a b c {OriginalRequest_World=1}"
            "\nd e f {Experiment0_World=0.8}"
            "\nc d e {Experiment1_World=0.7}");
    }

    Y_UNIT_TEST(TestRestrictBlocksReuse) {
        TReqBundle bundle = *(TReqBundleBuilder{}
            << "a b c" << MakeFacetId(TExpansion::OriginalRequest) << 1.0f
            << "b c d" << MakeFacetId(TExpansion::Experiment0) << 0.6f
            << "c d e" << MakeFacetId(TExpansion::Experiment1) << 0.7f
            << "d e f" << MakeFacetId(TExpansion::Experiment0) << 0.8f).GetResult();

        Cdbg << "BUNDLE = (" << PrintableReqBundle(bundle, PF_FACET_NAME | PF_FACET_VALUE) << ")" << Endl;

        // NOTE. Blocks from OriginalRequest are always accepted and can be reused "for free"
        TReqBundle result = *(RestrictReqBundle(bundle, {{MakeFacetIdPredicate(), {2, 100}}})->GetResult());

        Cdbg << "RESULT = (" << PrintableReqBundle(result, PF_FACET_NAME | PF_FACET_VALUE) << ")" << Endl;

        UNIT_ASSERT(IsValidReqBundle(result, true));
        UNIT_ASSERT_EQUAL(result.GetNumRequests(), 3);
        UNIT_ASSERT_EQUAL(result.GetSequence().GetNumElems(), 5);
        UNIT_ASSERT_EQUAL(ToString(PrintableReqBundle(result, PF_FACET_NAME | PF_FACET_VALUE)), "a b c {OriginalRequest_World=1}"
            "\nb c d {Experiment0_World=0.6}"
            "\nc d e {Experiment1_World=0.7}");
    }

    Y_UNIT_TEST(TestRestrictFacetsInclusion) {
        TReqBundle bundle = *(TReqBundleBuilder{}
            << "a" << MakeFacetId(TExpansion::Experiment0, TRegionId::Russia()) << 0.7f
            << "b" << MakeFacetId(TExpansion::Experiment0, TRegionId::Russia()) << 0.6f
            << "x" << MakeFacetId(TExpansion::Experiment0, TRegionId::Turkey()) << 0.6f
            << "c" << MakeFacetId(TExpansion::Experiment0) << 0.8f
            << "d" << MakeFacetId(TExpansion::Experiment0) << 0.5f).GetResult();

        Cdbg << "BUNDLE = (" << PrintableReqBundle(bundle, PF_FACET_NAME | PF_FACET_VALUE) << ")" << Endl;

        TReqBundle resultX = *(RestrictReqBundle(bundle, {
            {MakeFacetIdPredicate(TRegionId::ClassCountry()), TRestrictOptions().SetEnabled(false)},
            {MakeFacetIdPredicate(TRegionId::Russia()), TRestrictOptions(1, 1).SetEnabled(true)}
        })->GetResult());

        Cdbg << "RESULT_X = (" << PrintableReqBundle(resultX, PF_FACET_NAME | PF_FACET_VALUE) << ")" << Endl;

        UNIT_ASSERT(IsValidReqBundle(resultX, false));
        UNIT_ASSERT_EQUAL(resultX.GetNumRequests(), 3);
        UNIT_ASSERT_EQUAL(resultX.GetSequence().GetNumElems(), 3);
        UNIT_ASSERT_EQUAL(ToString(PrintableReqBundle(resultX, PF_FACET_NAME | PF_FACET_VALUE)),
            "c {Experiment0_World=0.8}"
            "\nd {Experiment0_World=0.5}"
            "\na {Experiment0_Country(ru)=0.7}");

        TReqBundle resultY = *(RestrictReqBundle(bundle, {
            {MakeFacetIdPredicate(), TRestrictOptions(1, 1)},
            {MakeFacetIdPredicate(), TRestrictOptions().SetEnabled(false)},
            {MakeFacetIdPredicate(TRegionId::Russia()), TRestrictOptions().SetEnabled(true)},
            {MakeFacetIdPredicate(TRegionId::World()), TRestrictOptions().SetEnabled(true)}
        })->GetResult());

        Cdbg << "RESULT_Y = (" << PrintableReqBundle(resultY, PF_FACET_NAME | PF_FACET_VALUE) << ")" << Endl;

        UNIT_ASSERT(IsValidReqBundle(resultY, false));
        UNIT_ASSERT_EQUAL(resultY.GetNumRequests(), 2);
        UNIT_ASSERT_EQUAL(resultY.GetSequence().GetNumElems(), 2);
        UNIT_ASSERT_EQUAL(ToString(PrintableReqBundle(resultY, PF_FACET_NAME | PF_FACET_VALUE)),
            "c {Experiment0_World=0.8}"
            "\na {Experiment0_Country(ru)=0.7}");

        TReqBundle resultZ = *(RestrictReqBundle(bundle, {
            {MakeFacetIdPredicate(TExpansion::Experiment0), TRestrictOptions(1, 1)},
            {MakeFacetIdPredicate(TRegionId::ClassCountry()), TRestrictOptions().SetEnabled(false)},
            {MakeFacetIdPredicate(TRegionId::Turkey()), TRestrictOptions().SetEnabled(true)},
        })->GetResult());

        Cdbg << "RESULT_Z = (" << PrintableReqBundle(resultZ, PF_FACET_NAME | PF_FACET_VALUE) << ")" << Endl;

        UNIT_ASSERT_EQUAL(ToString(PrintableReqBundle(resultZ, PF_FACET_NAME | PF_FACET_VALUE)),
            "c {Experiment0_World=0.8}"
            "\nx {Experiment0_Country(tr)=0.6}"
        );

        TReqBundle resultU = *(RestrictReqBundle(bundle, {
            {MakeFacetIdPredicate(TRegionId::ClassCountry()), TRestrictOptions().SetEnabled(false)},
            {MakeFacetIdPredicate(TRegionId::Turkey()), TRestrictOptions().SetEnabled(true)},
            {MakeFacetIdPredicate(TExpansion::Experiment0), TRestrictOptions(1, 1)},
            {MakeFacetIdPredicate(TExpansion::Experiment0, TRegionId::ClassCountry()), TRestrictOptions().SetEnabled(false)},
        })->GetResult());

        Cdbg << "RESULT_U = (" << PrintableReqBundle(resultU, PF_FACET_NAME | PF_FACET_VALUE) << ")" << Endl;

        UNIT_ASSERT_EQUAL(ToString(PrintableReqBundle(resultU, PF_FACET_NAME | PF_FACET_VALUE)),
            "c {Experiment0_World=0.8}"
        );
    }
};

