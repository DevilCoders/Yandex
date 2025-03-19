#include "reqbundle_builder.h"

#include <kernel/reqbundle/print.h>
#include <kernel/reqbundle/validate.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/algorithm.h>

using namespace NReqBundle;
using namespace NReqBundle::NDetail;


Y_UNIT_TEST_SUITE(ReqBundleValidateTest) {
    Y_UNIT_TEST(TestValidateCorrect) {
        TReqBundlePtr ReqBundlePtr = new TReqBundle(*(TReqBundleBuilder{}
            << "a b c" << MakeFacetId(TExpansion::OriginalRequest) << 1.0f
            << "b c d" << MakeFacetId(TExpansion::Experiment0) << 0.6f
            << "c d e" << MakeFacetId(TExpansion::Experiment1) << 0.7f
            << "d e f" << MakeFacetId(TExpansion::Experiment0) << 0.8f).GetResult());


        Cdbg << "BUNDLE = (" << PrintableReqBundle(*ReqBundlePtr, PF_FACET_NAME | PF_FACET_VALUE) << ")" << Endl;
        UNIT_ASSERT(IsValidReqBundle(*ReqBundlePtr));

        TReqBundleValidater validater;
        validater.Validate(ReqBundlePtr);

        Cdbg << "RESULT = (" << PrintableReqBundle(*ReqBundlePtr, PF_FACET_NAME | PF_FACET_VALUE) << ")" << Endl;
        UNIT_ASSERT(IsValidReqBundle(*ReqBundlePtr));
        UNIT_ASSERT_EQUAL(ReqBundlePtr->GetNumRequests(), 4);
        UNIT_ASSERT_EQUAL(ReqBundlePtr->GetSequence().GetNumElems(), 6);
        UNIT_ASSERT_EQUAL(ToString(PrintableReqBundle(*ReqBundlePtr, PF_FACET_NAME | PF_FACET_VALUE)), "a b c {OriginalRequest_World=1}"
            "\nb c d {Experiment0_World=0.6}"
            "\nc d e {Experiment1_World=0.7}"
            "\nd e f {Experiment0_World=0.8}");
    }

    Y_UNIT_TEST(TestValidateSingleExpansions) {
        TReqBundlePtr ReqBundlePtr = new TReqBundle(*(TReqBundleBuilder{}
            << "a b c" << MakeFacetId(TExpansion::OriginalRequest) << 1.0f
            << "b c d" << MakeFacetId(TExpansion::RequestWithRegionName) << 0.6f
            << "c d e" << MakeFacetId(TExpansion::Experiment1) << 0.7f
            << "d e f" << MakeFacetId(TExpansion::RequestWithRegionName) << 0.8f).GetResult());

        UNIT_ASSERT(!IsValidReqBundle(*ReqBundlePtr));

        TReqBundleValidater validater;
        validater.Validate(ReqBundlePtr);

        Cdbg << "RESULT = (" << PrintableReqBundle(*ReqBundlePtr, PF_FACET_NAME | PF_FACET_VALUE) << ")" << Endl;
        UNIT_ASSERT(IsValidReqBundle(*ReqBundlePtr));
        UNIT_ASSERT_EQUAL(ReqBundlePtr->GetNumRequests(), 3);
        UNIT_ASSERT_EQUAL(ReqBundlePtr->GetSequence().GetNumElems(), 5);
        UNIT_ASSERT_EQUAL(ToString(PrintableReqBundle(*ReqBundlePtr, PF_FACET_NAME | PF_FACET_VALUE)), "a b c {OriginalRequest_World=1}"
            "\nb c d {RequestWithRegionName_World=0.6}"
            "\nc d e {Experiment1_World=0.7}");
    }

    Y_UNIT_TEST(TestValidateMultiOriginalsExpansions) {
        TReqBundlePtr ReqBundlePtr = new TReqBundle(*(TReqBundleBuilder{}
            << "a b c" << MakeFacetId(TExpansion::OriginalRequest) << 1.0f
            << "b c d" << MakeFacetId(TExpansion::Experiment0) << 0.6f
            << "e b c" << MakeFacetId(TExpansion::OriginalRequest) << 0.9f
            << "t b c" << MakeFacetId(TExpansion::Experiment0) << 1.0f
            << "o b c" << MakeFacetId(TExpansion::RequestWithCountryName) << 0.3f
            << "d e f" << MakeFacetId(TExpansion::RequestWithCountryName) << 0.8f).GetResult());

        UNIT_ASSERT(!IsValidReqBundle(*ReqBundlePtr));

        TReqBundleValidater validater;
        validater.Validate(ReqBundlePtr);

        Cdbg << "RESULT = (" << PrintableReqBundle(*ReqBundlePtr, PF_FACET_NAME | PF_FACET_VALUE) << ")" << Endl;
        UNIT_ASSERT_EQUAL(ReqBundlePtr->GetNumRequests(), 4);
        UNIT_ASSERT_EQUAL(ReqBundlePtr->GetSequence().GetNumElems(), 6);
        UNIT_ASSERT_EQUAL(ToString(PrintableReqBundle(*ReqBundlePtr, PF_FACET_NAME | PF_FACET_VALUE)), "a b c {OriginalRequest_World=1}"
        "\no b c {RequestWithCountryName_World=0.3}"
        "\nt b c {Experiment0_World=1}"
        "\nb c d {Experiment0_World=0.6}");
    }

    Y_UNIT_TEST(TestValidateSingleAndOriginalExpansions) {
        TReqBundlePtr ReqBundlePtr = new TReqBundle(*(TReqBundleBuilder{}
            << "v o t" << MakeFacetId(TExpansion::OriginalRequest) << 1.0f
            << "p i n" << MakeFacetId(TExpansion::RequestWithCountryName) << 0.3f
            << "k y v" << MakeFacetId(TExpansion::RequestWithRegionName) << 0.6f
            << "o t i" << MakeFacetId(TExpansion::OriginalRequest) << 0.7f
            << "b r a" << MakeFacetId(TExpansion::RequestWithRegionName) << 0.3f
            << "i n v" << MakeFacetId(TExpansion::RequestWithCountryName) << 0.8f).GetResult());

        UNIT_ASSERT(!IsValidReqBundle(*ReqBundlePtr));

        TReqBundleValidater validater;
        validater.Validate(ReqBundlePtr);

        Cdbg << "RESULT = (" << PrintableReqBundle(*ReqBundlePtr, PF_FACET_NAME | PF_FACET_VALUE) << ")" << ReqBundlePtr->GetSequence().GetNumElems()<< Endl;
        UNIT_ASSERT_EQUAL(ReqBundlePtr->GetNumRequests(), 3);
        UNIT_ASSERT_EQUAL(ReqBundlePtr->GetSequence().GetNumElems(), 8);
        UNIT_ASSERT_EQUAL(ToString(PrintableReqBundle(*ReqBundlePtr, PF_FACET_NAME | PF_FACET_VALUE)), "v o t {OriginalRequest_World=1}"
        "\nk y v {RequestWithRegionName_World=0.6}"
        "\np i n {RequestWithCountryName_World=0.3}");
    }
};
