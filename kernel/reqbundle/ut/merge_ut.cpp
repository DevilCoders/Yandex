#include "reqbundle_builder.h"

#include <kernel/reqbundle/merge.h>
#include <kernel/reqbundle/print.h>

#include <library/cpp/testing/unittest/registar.h>

using namespace NReqBundle;

Y_UNIT_TEST_SUITE(MergeReqBundlesTest) {
    Y_UNIT_TEST(TestMergeEmpty) {
        TReqBundle bundleX;
        TReqBundle bundleY;

        Cdbg << "BUNDLE_X = (" << bundleX << ")" << Endl;
        Cdbg << "BUNDLE_Y = (" << bundleY << ")" << Endl;

        TReqBundle result = *MergeBundles({bundleX, bundleY});

        Cdbg << "RESULT = (" << result << ")" << Endl;

        UNIT_ASSERT(IsValidReqBundle(result, false));
        UNIT_ASSERT_EQUAL(result.GetNumRequests(), 0);
        UNIT_ASSERT_EQUAL(result.GetSequence().GetNumElems(), 0);

        TReqBundleMerger::TOptions opts;
        opts.HashRequests = true;

        TReqBundle resultHashed = *MergeBundles({bundleX, bundleY}, opts);

        Cdbg << "RESULT_HASHED = (" << resultHashed << ")" << Endl;

        UNIT_ASSERT_EQUAL(ToString(resultHashed), ToString(result));
    }

    Y_UNIT_TEST(TestMergeWithEmpty) {
        TReqBundle bundleX = *(TReqBundleBuilder{}
            << "a" << MakeFacetId(TExpansion::OriginalRequest) << 1.0f).GetResult();
        TReqBundle bundleY;

        Cdbg << "BUNDLE_X = (" << bundleX << ")" << Endl;
        Cdbg << "BUNDLE_Y = (" << bundleY << ")" << Endl;

        TReqBundle result = *MergeBundles({bundleX, bundleY});

        Cdbg << "RESULT = (" << result << ")" << Endl;

        UNIT_ASSERT(IsValidReqBundle(result, false));
        UNIT_ASSERT_EQUAL(result.GetNumRequests(), 1);
        UNIT_ASSERT_EQUAL(result.GetSequence().GetNumElems(), 1);
        UNIT_ASSERT_EQUAL(ToString(result), "a");
    }

    Y_UNIT_TEST(TestMergeDuplicate) {
        TReqBundle bundleX = *(TReqBundleBuilder{}
            << "a" << MakeFacetId(TExpansion::Experiment0) << 1.0f).GetResult();
        TReqBundle bundleY = bundleX;

        Cdbg << "BUNDLE_X = (" << PrintableReqBundle(bundleX, PF_FACET_NAME) << ")" << Endl;
        Cdbg << "BUNDLE_Y = (" << PrintableReqBundle(bundleY, PF_FACET_NAME) << ")" << Endl;

        TReqBundle result = *MergeBundles({bundleX, bundleY});

        Cdbg << "RESULT = (" << PrintableReqBundle(result, PF_FACET_NAME) << ")" << Endl;

        UNIT_ASSERT(IsValidReqBundle(result, false));
        UNIT_ASSERT_EQUAL(result.GetNumRequests(), 2);
        UNIT_ASSERT_EQUAL(result.GetSequence().GetNumElems(), 1);
        UNIT_ASSERT_EQUAL(ToString(PrintableReqBundle(result, PF_FACET_NAME)), "a {Experiment0_World}\na {Experiment0_World}");
    }

    Y_UNIT_TEST(TestMergeDuplicateHashed) {
        TReqBundle bundleX = *(TReqBundleBuilder{}
            << "a" << MakeFacetId(TExpansion::Experiment0) << 0.7f).GetResult();

        TReqBundle bundleY = *(TReqBundleBuilder{}
            << "a" << MakeFacetId(TExpansion::Experiment0) << 0.5f).GetResult();

        Cdbg << "BUNDLE_X = (" << PrintableReqBundle(bundleX, PF_FACET_NAME) << ")" << Endl;
        Cdbg << "BUNDLE_Y = (" << PrintableReqBundle(bundleY, PF_FACET_NAME) << ")" << Endl;

        TReqBundleMerger::TOptions opts;
        opts.HashRequests = true;

        TReqBundle result = *MergeBundles({bundleX, bundleY}, opts);

        Cdbg << "RESULT = (" << PrintableReqBundle(result, PF_FACET_NAME) << ")" << Endl;

        UNIT_ASSERT(IsValidReqBundle(result, false));
        UNIT_ASSERT_EQUAL(result.GetNumRequests(), 2);
        UNIT_ASSERT_EQUAL(result.GetSequence().GetNumElems(), 1);
        UNIT_ASSERT_EQUAL(ToString(PrintableReqBundle(result, PF_FACET_NAME)), "a {Experiment0_World}\na {Experiment0_World}");

        NReqBundle::NDetail::TTakeMaxResolver resolver;
        opts.DuplicatesResolver = &resolver;

        TReqBundle resultResolved = *MergeBundles({bundleX, bundleY}, opts);

        Cdbg << "RESULT_RESOLVED = (" << PrintableReqBundle(resultResolved, PF_FACET_NAME | PF_FACET_VALUE) << ")" << Endl;

        UNIT_ASSERT_EQUAL(ToString(PrintableReqBundle(resultResolved, PF_FACET_NAME | PF_FACET_VALUE)), "a {Experiment0_World=0.7}");
    }

    Y_UNIT_TEST(TestMergeFacetsHashed) {
        TReqBundle bundleX = *(TReqBundleBuilder{}
            << "a" << MakeFacetId(TExpansion::Experiment0, TRegionId::World()) << 1.0f).GetResult();
        TReqBundle bundleY = *(TReqBundleBuilder{}
            << "a" << MakeFacetId(TExpansion::Experiment0, TRegionId::Russia()) << 0.5f).GetResult();

        Cdbg << "BUNDLE_X = (" << PrintableReqBundle(bundleX, PF_FACET_NAME) << ")" << Endl;
        Cdbg << "BUNDLE_Y = (" << PrintableReqBundle(bundleY, PF_FACET_NAME) << ")" << Endl;

        TReqBundleMerger::TOptions opts;
        opts.HashRequests = true;

        TReqBundle result = *MergeBundles({bundleX, bundleY}, opts);

        Cdbg << "RESULT = (" << PrintableReqBundle(result, PF_FACET_NAME) << ")" << Endl;

        UNIT_ASSERT(IsValidReqBundle(result, false));
        UNIT_ASSERT_EQUAL(result.GetNumRequests(), 1);
        UNIT_ASSERT_EQUAL(result.GetSequence().GetNumElems(), 1);
        UNIT_ASSERT_EQUAL(ToString(PrintableReqBundle(result, PF_FACET_NAME)), "a {Experiment0_World Experiment0_Country(ru)}");
    }

    Y_UNIT_TEST(TestMergeDoubleOriginalRequest) {
        TReqBundle bundleX = *(TReqBundleBuilder{}
            << "a" << MakeFacetId(TExpansion::OriginalRequest) << 1.0f).GetResult();
        TReqBundle bundleY = *(TReqBundleBuilder{}
            << "b" << MakeFacetId(TExpansion::OriginalRequest) << 0.5f).GetResult();

        Cdbg << "BUNDLE_X = (" << PrintableReqBundle(bundleX, PF_FACET_NAME | PF_FACET_VALUE) << ")" << Endl;
        Cdbg << "BUNDLE_Y = (" << PrintableReqBundle(bundleY, PF_FACET_NAME | PF_FACET_VALUE) << ")" << Endl;

        TReqBundle result = *MergeBundles({bundleX, bundleY});

        Cdbg << "RESULT = (" << PrintableReqBundle(result, PF_FACET_NAME | PF_FACET_VALUE) << ")" << Endl;

        UNIT_ASSERT(IsValidReqBundle(result, false));
        UNIT_ASSERT_EQUAL(result.GetNumRequests(), 1);
        UNIT_ASSERT_EQUAL(result.GetSequence().GetNumElems(), 1);
        UNIT_ASSERT_EQUAL(ToString(PrintableReqBundle(result, PF_FACET_NAME | PF_FACET_VALUE)), "a {OriginalRequest_World=1}");

        TReqBundleMerger::TOptions opts;
        opts.HashRequests = true;

        TReqBundle resultHashed = *MergeBundles({bundleX, bundleY}, opts);

        Cdbg << "RESULT_HASHED = (" << PrintableReqBundle(resultHashed, PF_FACET_NAME | PF_FACET_VALUE) << ")" << Endl;

        UNIT_ASSERT_EQUAL(ToString(PrintableReqBundle(resultHashed, PF_FACET_NAME | PF_FACET_VALUE)),
            ToString(PrintableReqBundle(result, PF_FACET_NAME | PF_FACET_VALUE)));
    }

    Y_UNIT_TEST(TestMergeSharedBlocks) {
        TReqBundle bundleX = *(TReqBundleBuilder{}
            << "a b c" << MakeFacetId(TExpansion::Experiment0) << 1.0f
            << "d e f" << MakeFacetId(TExpansion::Experiment0) << 1.0f).GetResult();
        TReqBundle bundleY = *(TReqBundleBuilder{}
            << "x a b" << MakeFacetId(TExpansion::Experiment1) << 1.0f
            << "e f y" << MakeFacetId(TExpansion::Experiment1) << 1.0f).GetResult();

        Cdbg << "BUNDLE_X = (" << PrintableReqBundle(bundleX, PF_FACET_NAME) << ")" << Endl;
        Cdbg << "BUNDLE_Y = (" << PrintableReqBundle(bundleY, PF_FACET_NAME) << ")" << Endl;

        TReqBundle result = *MergeBundles({bundleX, bundleY});

        Cdbg << "RESULT = (" << PrintableReqBundle(result, PF_FACET_NAME) << ")" << Endl;

        UNIT_ASSERT(IsValidReqBundle(result, false));
        UNIT_ASSERT_EQUAL(result.GetNumRequests(), 4);
        UNIT_ASSERT_EQUAL(result.GetSequence().GetNumElems(), 8);
        UNIT_ASSERT_EQUAL(ToString(PrintableReqBundle(result)), "a b c\nd e f\nx a b\ne f y");

        TReqBundleMerger::TOptions opts;
        opts.HashRequests = true;

        TReqBundle resultHashed = *MergeBundles({bundleX, bundleY}, opts);

        Cdbg << "RESULT_HASHED = (" << PrintableReqBundle(resultHashed, PF_FACET_NAME) << ")" << Endl;

        UNIT_ASSERT_EQUAL(ToString(PrintableReqBundle(resultHashed)),
            ToString(PrintableReqBundle(result)));
    }

    Y_UNIT_TEST(TestSoftSignValue) {
        TVector<float> values{-0.1f, -0.3f, 0.2f, 1.2f, 3.14f, -5.0f};
        NReqBundle::NDetail::TSoftSignResolver resolver;
        float curSum = 0.0;
        float curGroupped = 0.5;
        TFacetId tmpId;
        for (float x : values) {
            curSum += x;
            curGroupped = resolver.GetUpdatedValue(curGroupped, x, tmpId);
            UNIT_ASSERT_DOUBLES_EQUAL(curSum / (1.0 + resolver.Scale * fabs(curSum)) + resolver.Bias, curGroupped, 1e-3);
        }
    }
};

