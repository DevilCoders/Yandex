#include "reqbundle_builder.h"

#include <kernel/reqbundle/print.h>

#include <library/cpp/string_utils/base64/base64.h>
#include <library/cpp/testing/unittest/registar.h>

using namespace NReqBundle;

Y_UNIT_TEST_SUITE(SplitRequestsTest) {
    Y_UNIT_TEST(TestSplitNoRequests) {
        TReqBundle result = *(TReqBundleBuilder{}).GetResult();

        Cdbg << "RESULT = (" << result << ")" << Endl;

        UNIT_ASSERT(IsValidReqBundle(result, false));
        UNIT_ASSERT_EQUAL(result.GetNumRequests(), 0);
        UNIT_ASSERT_EQUAL(result.GetSequence().GetNumElems(), 0);
        UNIT_ASSERT_EQUAL(ToString(result), "");
    }

    Y_UNIT_TEST(TestSplitEmptyRequest) {
        TReqBundle result = *(TReqBundleBuilder{} << "" << MakeFacetId(TExpansion::OriginalRequest) << 0.5f).GetResult();

        Cdbg << "RESULT = (" << result << ")" << Endl;

        UNIT_ASSERT(IsValidReqBundle(result, false));
        UNIT_ASSERT_EQUAL(result.GetNumRequests(), 0);
        UNIT_ASSERT_EQUAL(result.GetSequence().GetNumElems(), 0);
        UNIT_ASSERT_EQUAL(ToString(result), "");
    }

    Y_UNIT_TEST(TestSplitSingleWord) {
        TReqBundle result = *(TReqBundleBuilder{}
            << "a" << MakeFacetId(TExpansion::OriginalRequest) << 0.5f).GetResult();

        Cdbg << "RESULT = (" << PrintableReqBundle(result, PF_FACET_NAME | PF_FACET_VALUE) << ")" << Endl;

        UNIT_ASSERT(IsValidReqBundle(result, true));
        UNIT_ASSERT_EQUAL(result.GetNumRequests(), 1);
        UNIT_ASSERT_EQUAL(result.GetSequence().GetNumElems(), 1);
        UNIT_ASSERT_EQUAL(ToString(PrintableReqBundle(result, PF_FACET_NAME | PF_FACET_VALUE)), "a {OriginalRequest_World=0.5}");
    }

    Y_UNIT_TEST(TestSplitRepeatedWord) {
        TReqBundle result = *(TReqBundleBuilder{}
            << "a a a" << MakeFacetId(TExpansion::OriginalRequest) << 0.5f).GetResult();

        Cdbg << "RESULT = (" << PrintableReqBundle(result, PF_FACET_NAME | PF_FACET_VALUE) << ")" << Endl;

        UNIT_ASSERT(IsValidReqBundle(result, true));
        UNIT_ASSERT_EQUAL(result.GetNumRequests(), 1);
        UNIT_ASSERT_EQUAL(result.GetSequence().GetNumElems(), 1);
        UNIT_ASSERT_EQUAL(ToString(PrintableReqBundle(result, PF_FACET_NAME | PF_FACET_VALUE)), "a a a {OriginalRequest_World=0.5}");
    }

    Y_UNIT_TEST(TestSplitRepeatedRequest) {
        TReqBundle result = *(TReqBundleBuilder{}
            << "a" << MakeFacetId(TExpansion::Experiment0) << 0.5f
            << "a" << MakeFacetId(TExpansion::Experiment0) << 0.6f
            << "a" << MakeFacetId(TExpansion::Experiment0) << 0.7f).GetResult();

        Cdbg << "RESULT = (" << PrintableReqBundle(result, PF_FACET_NAME | PF_FACET_VALUE) << ")" << Endl;

        UNIT_ASSERT(IsValidReqBundle(result, false));
        UNIT_ASSERT_EQUAL(result.GetNumRequests(), 3);
        UNIT_ASSERT_EQUAL(result.GetSequence().GetNumElems(), 1);
        UNIT_ASSERT_EQUAL(ToString(PrintableReqBundle(result, PF_FACET_NAME | PF_FACET_VALUE)), "a {Experiment0_World=0.5}\na {Experiment0_World=0.6}\na {Experiment0_World=0.7}");
    }

    Y_UNIT_TEST(TestSplitMix) {
        TReqBundle result = *(TReqBundleBuilder{}
            << "a b c" << MakeFacetId(TExpansion::OriginalRequest) << 1.0f
            << "b c d" << MakeFacetId(TExpansion::Experiment0) << 0.6f
            << "c d e" << MakeFacetId(TExpansion::Experiment1) << 0.7f
            << "d e f" << MakeFacetId(TExpansion::Experiment0) << 0.8f).GetResult();

        Cdbg << "RESULT = (" << PrintableReqBundle(result, PF_FACET_NAME | PF_FACET_VALUE) << ")" << Endl;

        UNIT_ASSERT(IsValidReqBundle(result, true));
        UNIT_ASSERT_EQUAL(result.GetNumRequests(), 4);
        UNIT_ASSERT_EQUAL(result.GetSequence().GetNumElems(), 6);
        UNIT_ASSERT_EQUAL(ToString(PrintableReqBundle(result, PF_FACET_NAME | PF_FACET_VALUE)), "a b c {OriginalRequest_World=1}"
            "\nb c d {Experiment0_World=0.6}"
            "\nc d e {Experiment1_World=0.7}"
            "\nd e f {Experiment0_World=0.8}");
    }

    Y_UNIT_TEST(TestMultiFacet) {
        TReqBundle result = *(TReqBundleBuilder{}
            << "a" << MakeFacetId(TExpansion::OriginalRequest) << 0.5f
                   << MakeFacetId(TExpansion::Experiment0) << 0.8f).GetResult();

        Cdbg << "RESULT = (" << PrintableReqBundle(result, PF_FACET_NAME | PF_FACET_VALUE) << ")" << Endl;

        UNIT_ASSERT(IsValidReqBundle(result, true));
        UNIT_ASSERT_EQUAL(result.GetNumRequests(), 1);
        UNIT_ASSERT_EQUAL(result.GetSequence().GetNumElems(), 1);
        UNIT_ASSERT_EQUAL(ToString(PrintableReqBundle(result, PF_FACET_NAME | PF_FACET_VALUE)), "a {OriginalRequest_World=0.5 Experiment0_World=0.8}");
    }

    Y_UNIT_TEST(TestRequestWithSynonyms) {
        TReqBundle result = *(TReqBundleBuilder{}
            << "a b c d" << MakeFacetId(TExpansion::OriginalRequest) << 0.5f
            << MakeSynonym("d", 0, 0) << MakeSynonym("yz", 1, 2)).GetResult();

        Cdbg << "RESULT = (" << PrintableReqBundle(result, PF_WORD_IDX | PF_SYNONYMS | PF_FACET_NAME | PF_FACET_VALUE) << ")" << Endl;

        UNIT_ASSERT(IsValidReqBundle(result, true));
        UNIT_ASSERT_EQUAL(result.GetNumRequests(), 1);
        UNIT_ASSERT_EQUAL(result.GetRequest(0).GetNumWords(), 4);
        UNIT_ASSERT_EQUAL(result.GetSequence().GetNumElems(), 5);
        UNIT_ASSERT_EQUAL(ToString(PrintableReqBundle(result, PF_FACET_NAME | PF_FACET_VALUE)), "a b c d {OriginalRequest_World=0.5}");
        UNIT_ASSERT_EQUAL(ToString(PrintableReqBundle(result, PF_SYNONYMS | PF_FACET_NAME | PF_FACET_VALUE)), "a b c d ^ d yz {OriginalRequest_World=0.5}");
    }

    Y_UNIT_TEST(TestCreateTelFullRequestNullptr) {
        TReqBundle result = *(TReqBundleBuilder{}
            << "a" << MakeFacetId(TExpansion::OriginalRequest) << 0.5f
                   << MakeFacetId(TExpansion::Experiment0) << 0.8f).GetResult();
        UNIT_ASSERT_EQUAL(TRequestSplitter::CreateTelFullRequest(result), nullptr);
    }

    Y_UNIT_TEST(TestCreateTelFullRequest) {
        const TStringBuf wizqbundle = "ehRMWi40AQAAAACASQkB-M8K1QEqyAF6FExaLjQBAAAAAIC2AAH_EhKLBRKsAhIS0YHQsNC90YLQtdGF0L3QuNC6GAEgASoWChoAAG4QASoYChQyAI_QsBAAKhoKFhoAAY_QvBAAKhwKGBwAAy_QuDoAB0_RhRAAcAAEH7UaAAYPUAAHP77QsmwABhG-pgAP3AAC79GDEAAyBBCuhAQSrgIS_QACLxgAMQHEH7kxAQcv0Y6DAQUDMQF_1oUEIKMCKkYCAPAHMgIIATgBQABKBBCs20FSBBCuhAQgAAAAADDKwtmVxO-d2w4KmwIqjQLYAPAC-wAB9QgSqgUSvgESCNC40LLaAPEEGAAgASoMChAAZBAAKg4KCh4Ahs0AYRAKDBAAiMwA8RsSCg4SACzQuCYAK9GFSAAbtSIAWb7QshABWgARvloAB3wAKtGDRACg0YvHAPlwhvkCwQAGkwBsvtCyGAEgTQAAXwAIpwAQvu8AC7sAABQAAJkACygAAXkADBQAdYsQACoUChD3AFG-0LLRi7MAPBYKEhYAAREBDS4AABUBnzIEEIDdAhKAAsEANh6-mQBP0L7QuRYAAgC8AQuXAE7QvtGOFwEPAwFPSSCjAioDAR0BQdbjC1JAAAIdAei-m--ln6vAue0BClIqRvQB8iI0AAF1EjQSHRILMAEA8Bk3GAAgACoFCgE3EAAyAxCaCiAAKgE3MgA4AkAASgMQ-gxSFQAAVQDYwOn_07iTpuQKClwqT1QAgj0AAXMSOxIgVABBCjk5OVYAYQcKAzk5OSsBo6jGCSAAKg8A8AVbAGMEEKzaCVIZAABdAK-QpITGlovCkNQBXgAMIjc3tABQBwoDNze2AFwEEJr_C14AQ6acDFIZAABeAOjv7OK7mpPXhogBClUqSbwAgjcAAXQSNhIevAAxHDU1uwBRBgoCNTUSAYOiZyAAKgI1NbgAUgMQzGlSFgAAWAC_3MrLj_CF5OEgClZXAAokMjJXACEyMlcAIKosVwAlMjJXADL2LFIWAABXAPgBn93DhIyJmrLmAQrEASq2AbEA8RGkAAHyBxK9AxKFAxIa0LLQvtC00L7Qv9GA0L4OAN_Rh7oDAOwC8AEeCiIACGIQASogChw0AA9CsgMA8QJxIgoeIgAJj_ICwSQKICQACy_QuEoAD74D8Q6QAAwftSIADg9oAA8_vtCyjAAOEb7WAA8cAQr_AL8DYAUQurioBqEDVWYBCPAJoQNzBRCmuY4OUh8AAYYC-QCA1dS0mo-90KYBCvkBKuvHAMTZAAH_ExLMBBI8EhSjAxDQGgNxgdC60LDRjzAB8QsYChwAAt8QATIGEIT2rv4PEt4DPwAAvrjQuc4D8UkaChZbAF_Qs9C-EFsACE0qFgoSNgAfuDIABD-40LUaAAYfuRoABj-8EACcAAI_uNC8agAHL9GFUAAFT9GPEAC2AAIA6AAPBAEBH74EAQgfvtIABh--0gAGCgDxDF--0LzRg7gABD--0Y4aAAMg0YMaAK8yBBDWtf4AOQQCAoIDQpbXCFIdAAHDAfkA0MuQ8vnb1OfaAQqqASqc_ADwCIoAAfsOEpkCEu4BEg7RgdC10LzQtdGA-QAB2ABwEgoWAGoQAS8EMSoAjKkBASwE8R9O0LgQAC4ATdGFEABYAB-1FAAADz4AAT--0LlUAAA-vtGOQAAt0YMUAPsAvtC6eQMwhqRdqwFH2wDwCKoBUobKxAJSHQABrQDo4_Cjle3k-eu3AQpmKlpuAvIRSAAB_x8SWhInEhd0ZWxfZnVsbD0iOCg5OTkpNzc3NTXZAvMNMgYQ6JzB_Q84ASAAKikABXMyADgAQABKKwCgUhsAAtkCn4uMyo22o7fbdmgAEh83aAAm2I6drIL0-f6jYgplKlnQALZHAAH_HhJYEiYSFtAAD2cACjQoAATPABwqzwC_4pDFipeBq-gsClNmBRUfAWYFDc_OwrGhn--7z58BCltnBRkjATJOBQ9nBQqfz_CzwLPg7_5mZgUbAF0AD2YFD5_hpbfpqY3Wrv1mBRoAEgEPZgULn4-jldOq7vurHWYFGQBXAA9mBQvoyvH6tPPE74LSAQpyKmYmAp9UAAH-FxJtEh4XCQUDxQhgIBIUIABBtgMkIgC3CC9DALcICtitmPSXhpCEizoKbCpgdACaTgAB9A0SbxIUswjQMgQQhvkCEhgSDBYAYSEFoRgBGgAvgN0aAAgICBo1bgALCAjwEIrVn_SNzsXsSBL9CQgHEAAaEAoGKgQQwIICEgIIABoEAAISAICetQESAggBGgQAAhIAgIq4BRICCAIaBAAIEgAgAxoEAAgSACAEGgQACBIAIAUaBAAIEgAgBhoEAPIFIQoXCAEQgYCAASEAAABgugOEQCoxBkESAggHjwA3IAoWIwBhIFdYg0AqVwVBEgIICJ8ACiIAQAAAQI8iAHGGpF0SAggJrwBBHwoTCGIAAQIAFCo9Bp8CCAoaBAgCEAYhAAcfCyEADRIMIQBxIgAiACIJCWAAPwDwPwsADo8tAACAPzIUGmYLAEgyDhoMAQeAMgMaATcyBRo8CQAHAGA3NzcyBBqCCPYJBBoCMjJCtAYIAQgCCAQICAgQCCAIQBACAgCYGAcilQYIARAAAgCiGj4IDBAGGIDg_wEAMgEgwAkAgP__ASiAIDCACgAAAgCgATiAAkAFSANQ_g4AAAIAXwFYBGABQAANFMAvAEEBOMABQAAQ_TwAAAIAfwFYA2ABGiuAAAu_QDhAQAFIA1ABWAEtAHYoIAACAIAqUQ0xhn04Fe8BPxgAIsUBAB8qFAAAETAEB_UOSAFQAFgAYABoAHAAeAKAARSNAWk9Sj8qQQ33zbRTAAkCAhUqtAwHQwATAUMA9AIDgAEdjQHxKBw_KjkN-0y8N5YAVQE3KgswAQCXNzABOAhAAUgAfgBRAYABAY36DSkqOzsAAGsCBj0APzk5OT0AGic3N3oALzc3egAMGTp6ADYCNTV5AD8wNTV5AAsLPAAnMjI8AC8yMjwACsESOwgFEDoaCBICCA1oBAAKABEOUAQACgARDzgEAAoAERDfBAAKABAR1wQBuQNhEk8IBxA0PQAVEj0AFRM9AABRAAI9AABRAAI9AABRABEEMgAAUQARBQoAAFEAEgZRAFEnCAEQQhUAFQpRABULCgCgDBoCCAAtAACAPwAAAA,,";

        TReqBundleDeserializer::TOptions deserOpts;
        deserOpts.FailMode = TReqBundleDeserializer::EFailMode::SkipOnError;
        TReqBundleDeserializer deser(deserOpts);

        TString binary;
        Base64Decode(wizqbundle, binary);
        TReqBundle result;
        deser.Deserialize(binary, result);

        UNIT_ASSERT_EQUAL(result.Sequence().GetNumElems(), 20);
        const auto req = TRequestSplitter::CreateTelFullRequest(result);
        UNIT_ASSERT_UNEQUAL(req, nullptr);
        UNIT_ASSERT_EQUAL(req->GetNumWords(), 1);
        UNIT_ASSERT_EQUAL(req->GetNumMatches(), 3);
    }
};

