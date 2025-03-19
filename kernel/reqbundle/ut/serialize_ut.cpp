#include "reqbundle_builder.h"

#include <kernel/reqbundle/reqbundle.h>
#include <kernel/reqbundle/serializer.h>
#include <kernel/reqbundle/print.h>
#include <kernel/reqbundle/proto/reqbundle.pb.h>

#include <library/cpp/testing/unittest/registar.h>

using namespace NReqBundle;

Y_UNIT_TEST_SUITE(SerializeReqBundleTest) {
    Y_UNIT_TEST(TestDeserializeEmpty) {
        NReqBundleProtocol::TReqBundle proto;

        TReqBundle result;
        TReqBundleDeserializer::TOptions options;
        options.FailMode = TReqBundleDeserializer::EFailMode::ThrowOnError;
        TReqBundleDeserializer deser(options);
        UNIT_ASSERT_NO_EXCEPTION(deser.DeserializeProto(proto, result));
        UNIT_ASSERT(NReqBundle::NDetail::IsValidReqBundle(result, false));
        UNIT_ASSERT_EQUAL(ToString(result), "");
    }

    Y_UNIT_TEST(TestDeserializeBadExpansion) {
        TReqBundle bundle = *(TReqBundleBuilder{}
            << "a" << MakeFacetId(TExpansion::OriginalRequest) << 1.0f
            << "b" << MakeFacetId(TExpansion::Experiment0) << 0.7f
            << "c" << MakeFacetId(TExpansion::Experiment0) << 0.6f).GetResult();

        NReqBundleProtocol::TReqBundle proto;
        TReqBundleSerializer().SerializeToProto(bundle, proto);
        UNIT_ASSERT_EQUAL(proto.RequestsSize(), 3);
        UNIT_ASSERT_EQUAL(proto.GetRequests(1).GetExpansionType(), TExpansion::Experiment0);
        proto.MutableRequests(1)->SetExpansionType(99); // make bad expansion type

        UNIT_ASSERT(!TExpansion::HasIndex(99));

        Cdbg << "PROTO = (" << proto.ShortDebugString() << ")" << Endl;

        TReqBundle result;
        TReqBundleDeserializer::TOptions options;
        options.FailMode = TReqBundleDeserializer::EFailMode::ThrowOnError;
        TReqBundleDeserializer deser(options);
        UNIT_ASSERT_EXCEPTION(deser.DeserializeProto(proto, result), yexception);

        result = TReqBundle{};
        options.FailMode = TReqBundleDeserializer::EFailMode::SkipOnError;
        TReqBundleDeserializer deserNoThrow(options);
        UNIT_ASSERT_NO_EXCEPTION(deserNoThrow.DeserializeProto(proto, result));
        UNIT_ASSERT(deserNoThrow.IsInErrorState());

        Cdbg << "ERROR = (" << deserNoThrow.GetFullErrorMessage() << ")" << Endl;
        UNIT_ASSERT_UNEQUAL(deserNoThrow.GetFullErrorMessage().find("bad expansion type index, 99 [in /reqbundle/request(1)/facet]"), TString::npos);

        Cdbg << "RESULT = (" << PrintableReqBundle(result, PF_FACET_NAME) << ")" << Endl;

        UNIT_ASSERT(NReqBundle::NDetail::IsValidReqBundle(result));
        UNIT_ASSERT_EQUAL(ToString(PrintableReqBundle(result, PF_FACET_NAME)), "a {OriginalRequest_World}\nb {}\nc {Experiment0_World}");
    }

    Y_UNIT_TEST(TestDeserializeBadFreq) {
        TReqBundle bundle = *(TReqBundleBuilder{}
            << "a" << MakeFacetId(TExpansion::OriginalRequest) << 1.0f).GetResult();

        NReqBundleProtocol::TReqBundle proto;
        TReqBundleSerializer().SerializeToProto(bundle, proto);
        UNIT_ASSERT_EQUAL(proto.BlocksSize(), 1);
        UNIT_ASSERT_EQUAL(proto.GetBlocks(0).WordsSize(), 1);
        UNIT_ASSERT_EQUAL(proto.GetBlocks(0).GetWords(0).RevFreqsByTypeSize(), 0);
        auto* freqProto = proto.MutableBlocks(0)->MutableWords(0)->AddRevFreqsByType();
        freqProto->SetRevFreq(1000);
        freqProto->SetFreqType(100); // bad freq type
        freqProto = proto.MutableBlocks(0)->MutableWords(0)->AddRevFreqsByType();
        freqProto->SetRevFreq(2000);
        freqProto->SetFreqType(TWordFreq::Default);

        UNIT_ASSERT(!TWordFreq::HasIndex(100));

        Cdbg << "PROTO = (" << proto.ShortDebugString() << ")" << Endl;

        TReqBundle result;
        TReqBundleDeserializer::TOptions options;
        options.FailMode = TReqBundleDeserializer::EFailMode::ThrowOnError;
        TReqBundleDeserializer deser(options);
        UNIT_ASSERT_EXCEPTION(deser.DeserializeProto(proto, result), yexception);

        result = TReqBundle{};
        options.FailMode = TReqBundleDeserializer::EFailMode::SkipOnError;
        TReqBundleDeserializer deserNoThrow(options);
        UNIT_ASSERT_NO_EXCEPTION(deserNoThrow.DeserializeProto(proto, result));
        UNIT_ASSERT(deserNoThrow.IsInErrorState());

        Cdbg << "ERROR = (" << deserNoThrow.GetFullErrorMessage() << ")" << Endl;
        UNIT_ASSERT_UNEQUAL(deserNoThrow.GetFullErrorMessage().find("bad freq type index, 100 [in /reqbundle/sequence/elem(0)/word(0)]"), TString::npos);

        Cdbg << "RESULT = (" << PrintableReqBundle(result, PF_FACET_NAME) << ")" << Endl;

        UNIT_ASSERT(NReqBundle::NDetail::IsValidReqBundle(result));
        UNIT_ASSERT_EQUAL(ToString(PrintableReqBundle(result, PF_FACET_NAME)), "a {OriginalRequest_World}");
        UNIT_ASSERT_EQUAL(result.GetSequence().GetBlock(0).GetWord(0).GetRevFreq(), 2000);
    }

    Y_UNIT_TEST(TestDeserializeBadMatch) {
        TReqBundle bundle = *(TReqBundleBuilder{}
            << "a b" << MakeFacetId(TExpansion::OriginalRequest) << 1.0f
            << "c" << MakeFacetId(TExpansion::Experiment0) << 0.6f).GetResult();

        NReqBundleProtocol::TReqBundle proto;
        TReqBundleSerializer().SerializeToProto(bundle, proto);
        UNIT_ASSERT_EQUAL(proto.RequestsSize(), 2);
        UNIT_ASSERT_EQUAL(proto.GetRequests(0).MatchesSize(), 2);
        auto* infoProto = proto.MutableRequests(0)->MutableMatches(0)->MutableInfo();
        infoProto->SetMatchType(100);

        UNIT_ASSERT(!TMatch::HasIndex(100));

        Cdbg << "PROTO = (" << proto.ShortDebugString() << ")" << Endl;

        TReqBundle result;
        TReqBundleDeserializer::TOptions options;
        options.FailMode = TReqBundleDeserializer::EFailMode::ThrowOnError;
        TReqBundleDeserializer deser(options);
        UNIT_ASSERT_EXCEPTION(deser.DeserializeProto(proto, result), yexception);

        result = TReqBundle{};
        options.FailMode = TReqBundleDeserializer::EFailMode::SkipOnError;
        TReqBundleDeserializer deserNoThrow(options);
        UNIT_ASSERT_NO_EXCEPTION(deserNoThrow.DeserializeProto(proto, result));
        UNIT_ASSERT(deserNoThrow.IsInErrorState());

        Cdbg << "ERROR = (" << deserNoThrow.GetFullErrorMessage() << ")" << Endl;
        UNIT_ASSERT_UNEQUAL(deserNoThrow.GetFullErrorMessage().find("bad match type index, 100 [in /reqbundle/request(0)/match(0)]"), TString::npos);
        UNIT_ASSERT_UNEQUAL(deserNoThrow.GetFullErrorMessage().find("bad request [in /reqbundle/request(0)]"), TString::npos);

        Cdbg << "RESULT = (" << PrintableReqBundle(result, PF_FACET_NAME) << ")" << Endl;

        UNIT_ASSERT(NReqBundle::NDetail::IsValidReqBundle(result, false));
        UNIT_ASSERT_EQUAL(ToString(PrintableReqBundle(result, PF_FACET_NAME)), "c {Experiment0_World}");
    }
};

