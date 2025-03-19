#include "reqbundle_builder.h"

#include <kernel/reqbundle/reqbundle.h>
#include <kernel/reqbundle/remap.h>
#include <kernel/reqbundle/print.h>

#include <library/cpp/testing/unittest/registar.h>

using namespace NReqBundle;

TReqBundlePtr Remapped(
    TConstReqBundleAcc bundle,
    const TAllRemapOptions& opts)
{
    THolder<TReqBundle> res = MakeHolder<TReqBundle>(bundle);
    for (auto& ptr : NReqBundle::NDetail::BackdoorAccess(*res).Requests) {
        ptr = new TRequest(*ptr);
    }
    RemapValues(*res, opts);
    return res.Release();
}

Y_UNIT_TEST_SUITE(RemapReqBundleTest) {
    Y_UNIT_TEST(TestParseError) {
        UNIT_ASSERT_EXCEPTION(
            TAllRemapOptions().FromJsonString("{}"),
            yexception
        );

        UNIT_ASSERT_EXCEPTION(
            TAllRemapOptions().FromJsonString("{Facet:{}}"),
            yexception
        );

        UNIT_ASSERT_EXCEPTION(
            TAllRemapOptions().FromJsonString("{Facet:{},Quantize:{N:10},ConstVal:{Value:0.8}}"),
            yexception
        );
    }

    Y_UNIT_TEST(TestRemapEmpty) {
        TReqBundle bundle = *(TReqBundleBuilder{}).GetResult();

        Cdbg << "BUNDLE = (" << bundle << ")" << Endl;

        TReqBundle result = *Remapped(bundle, {});

        Cdbg << "RESULT = (" << result << ")" << Endl;

        UNIT_ASSERT(IsValidReqBundle(result, false));
        UNIT_ASSERT_EQUAL(result.GetNumRequests(), 0);
        UNIT_ASSERT_EQUAL(result.GetSequence().GetNumElems(), 0);
        UNIT_ASSERT_EQUAL(ToString(result), "");
    }

    Y_UNIT_TEST(TestRemapConst) {
        TReqBundle bundle = *(TReqBundleBuilder{}
            << "a b c" << MakeFacetId(TExpansion::OriginalRequest) << 1.0f
            << "b c d" << MakeFacetId(TExpansion::Experiment0) << 0.6f
            << "c d e" << MakeFacetId(TExpansion::Experiment1) << 0.7f
            << "d e f" << MakeFacetId(TExpansion::Experiment0) << 0.8f).GetResult();

        Cdbg << "BUNDLE = (" << PrintableReqBundle(bundle, PF_FACET_VALUE) << ")" << Endl;

        TReqBundle result = *Remapped(bundle, TAllRemapOptions{}.FromJsonString(
            "{Facet:{Expansion:Experiment0},ConstVal:{Value:0.42}}"
        ));

        Cdbg << "RESULT = (" << PrintableReqBundle(result, PF_FACET_VALUE) << ")" << Endl;

        UNIT_ASSERT(IsValidReqBundle(result, true));
        UNIT_ASSERT_EQUAL(ToString(PrintableReqBundle(result, PF_FACET_VALUE)), "a b c {1}"
            "\nb c d {0.42}"
            "\nc d e {0.7}"
            "\nd e f {0.42}");
    }

    Y_UNIT_TEST(TestRemapRenormFrac) {
        TReqBundle bundle = *(TReqBundleBuilder{}
            << "x" << MakeFacetId(TExpansion::OriginalRequest) << 1.0f
            << "y" << MakeFacetId(TExpansion::Experiment0) << 0.6f
            << "z" << MakeFacetId(TExpansion::Experiment1) << 0.7f
            << "u" << MakeFacetId(TExpansion::Experiment0) << 0.9f).GetResult();

        Cdbg << "BUNDLE = (" << PrintableReqBundle(bundle, PF_FACET_VALUE) << ")" << Endl;

        TReqBundle resultX = *Remapped(bundle, TAllRemapOptions{}.FromJsonString(
            "{Facet:{Expansion:Experiment0,RegionId:World},RenormFrac:{Scale:1.0,Center:0.6,Offset:0.0}}"
        ));

        Cdbg << "RESULT_X = (" << PrintableReqBundle(resultX, PF_FACET_VALUE) << ")" << Endl;

        UNIT_ASSERT(IsValidReqBundle(resultX, true));
        UNIT_ASSERT_EQUAL(ToString(PrintableReqBundle(resultX, PF_FACET_VALUE)), "x {1}"
            "\ny {0.5}"
            "\nz {0.7}"
            "\nu {0.6}");

        TReqBundle resultY = *Remapped(bundle, TAllRemapOptions{}.FromJsonString(
            "["
                "{Facet:{Expansion:Experiment0,RegionId:World},RenormFrac:{Scale:0.5,Center:0.6,Offset:0.1}},"
                "{Facet:{Expansion:Experiment0},Quantize:{N:1000}}"
            "]"
        ));

        Cdbg << "RESULT_Y = (" << PrintableReqBundle(resultY, PF_FACET_VALUE) << ")" << Endl;

        UNIT_ASSERT(IsValidReqBundle(resultY, true));
        UNIT_ASSERT_EQUAL(ToString(PrintableReqBundle(resultY, PF_FACET_VALUE)), "x {1}"
            "\ny {0.25}"
            "\nz {0.7}"
            "\nu {0.294}");
    }

    Y_UNIT_TEST(TestRemapSequence) {
        TReqBundle bundle = *(TReqBundleBuilder{}
            << "x" << MakeFacetId(TExpansion::OriginalRequest) << 1.0f
            << "y" << MakeFacetId(TExpansion::Experiment0, TRegionId::Turkey()) << 0.6f
            << "z" << MakeFacetId(TExpansion::Experiment1, TRegionId::Russia()) << 0.7f
            << "u" << MakeFacetId(TExpansion::Experiment0) << 0.9f).GetResult();

        Cdbg << "BUNDLE = (" << PrintableReqBundle(bundle, PF_FACET_VALUE) << ")" << Endl;

        TReqBundle result = *Remapped(bundle, TAllRemapOptions{}.FromJsonString(
            "["
                "{Facet:{Expansion:Experiment0},RenormFrac:{Scale:1.0,Center:0.6,Offset:0.0}},"
                "{Facet:{RegionId:Country},ConstVal:{Value:0.9}},"
                "{Facet:{Expansion:Experiment0},RenormFrac:{Scale:1.0,Center:0.6,Offset:0.0}}"
            "]"
        ));

        Cdbg << "RESULT = (" << PrintableReqBundle(result, PF_FACET_VALUE) << ")" << Endl;

        UNIT_ASSERT(IsValidReqBundle(result, true));
        UNIT_ASSERT_EQUAL(ToString(PrintableReqBundle(result, PF_FACET_VALUE)), "x {1}"
            "\ny {0.6}"
            "\nz {0.9}"
            "\nu {0.5}");
    }
};
