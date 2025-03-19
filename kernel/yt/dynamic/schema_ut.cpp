#include "schema.h"
#include "ut/lib.h"

#include <yt/yt/client/api/client.h>
#include <yt/yt/client/table_client/row_buffer.h>
#include <yt/yt/core/ytree/convert.h>
#include <yt/yt/core/ytree/node.h>
#include <kernel/proto_fuzz/fuzzer.h>
#include <library/cpp/protobuf/util/pb_io.h>

#include <util/generic/map.h>
#include <util/generic/xrange.h>

namespace NYT {
namespace NProtoApiTest {

static ETableSchemaKind Primary{ETableSchemaKind::Primary};

class TTestProtoSchema : public TTestBase {
public:
    void SetUp() override {
        Buffer = New<TRowBuffer>();
    }

    auto NameToFieldTag(TProtoSchema const& schema) {
        TMap<TString, int> retval;
        auto& from = schema.NameToFieldTag;
        retval.insert(from.cbegin(), from.cend());
        return retval;
    }

    void TestDeduce();
    void TestDeduceFailures();
    void TestFilter();
    void TestConvertReconstruct();
    void TestConvertCompatibility();
    void TestConvertExceptionSafety();
    void TestConvertStrict();
    void TestConvertPartial();
    void TestConvertNull();
    void TestCache();
    void TestMutation();

    UNIT_TEST_SUITE(TTestProtoSchema);
        UNIT_TEST(TestDeduce);
        UNIT_TEST(TestDeduceFailures);
        UNIT_TEST(TestFilter);
        UNIT_TEST(TestConvertReconstruct);
        UNIT_TEST(TestConvertCompatibility);
        UNIT_TEST(TestConvertExceptionSafety);
        UNIT_TEST(TestConvertStrict);
        UNIT_TEST(TestConvertPartial);
        UNIT_TEST(TestConvertNull);
        UNIT_TEST(TestCache);
        UNIT_TEST(TestMutation);
    UNIT_TEST_SUITE_END();

private:
    TProtoSchema Schema{TZoo::descriptor(), Primary};
    TRowBufferPtr Buffer;
};

UNIT_TEST_SUITE_REGISTRATION(TTestProtoSchema);

template<class T>
TString ToStringPretty(T const& obj) {
    NYTree::INodePtr node = NYTree::ConvertToNode(obj);
    // XXX: please fix if this conversion will not be stable
    return NYTree::ConvertToYsonString(node, NYson::EYsonFormat::Pretty)
        .GetData();
}

void TTestProtoSchema::TestDeduce() {
    TDescriptorPtr testset[] = {
        TIntegral::descriptor(),
        TOneOf::descriptor(),
        TAggregated::descriptor(),
        TZoo::descriptor(),
        TOrdered::descriptor()
    };

    for (auto desc : testset) {
        for (auto kind : TEnumTraits<ETableSchemaKind>::GetDomainValues()) {
            if (kind == ETableSchemaKind::VersionedWrite) {
                continue; // JUPITER-169
            }
            if (kind == ETableSchemaKind::PrimaryWithTabletIndex) {
                continue;
            }
            TProtoSchema schema{desc, kind};
            Cout << "=== " << schema.ReprStr() << " ===" << Endl;
            Cout << ToStringPretty(schema.Schema()) << Endl;
            Cout << "name_to_tag: " << DbgDump(NameToFieldTag(schema)) << Endl;
            Cout << Endl;
        }
    }
}

void TTestProtoSchema::TestDeduceFailures() {
    TVector<TDescriptorPtr> testset;
    auto addFrom = [&](TDescriptorPtr desc) {
        int n = desc->nested_type_count();
        for (int i = 0; i < n; ++i) {
            testset.push_back(desc->nested_type(i));
        }
    };

    addFrom(NMalformedAggregate::descriptor());
    addFrom(TRepeated::descriptor());

    for (auto desc : testset) try {
        TProtoSchema schema{desc, Primary};
        Cout << desc->full_name() << ": not failed" << Endl;
        ASSERT_TRUE(false);
    } catch (TErrorException& ex) {
        Cout << ex.Error().Attributes().Get<TString>("protobuf") << ": "
             << ex.Error().GetMessage()
             << " ("
             << TStringBuf(ex.Error().InnerErrors()[0].GetMessage()).RAfter(':')
             << ")" << Endl;
    }
}

void TTestProtoSchema::TestFilter() {
    Cout << "All: " << Endl;
    Cout << ToStringPretty(TProtoSchema{TZoo::descriptor(), Primary}
        .Filter({}).Schema()) << Endl;

    Cout << "TZoo: " << Endl;
    Cout << ToStringPretty(TProtoSchema{TZoo::descriptor(), Primary}
        .Filter({
            TZoo::kSignedFieldNumber,
            TZoo::kString,
            TZoo::kFloatFieldNumber,
            TZoo::kBoxFieldNumber
        }).Schema()) << Endl;

    Cout << "TOrdered: " << Endl;
    Cout << ToStringPretty(TProtoSchema{TOrdered::descriptor(), ETableSchemaKind::Query}
        .Filter({
            TOrdered::kIndexFieldNumber
        }).Schema()) << Endl;
}

void FuzzProb(NProtoBuf::Message& msg, ui64 seed, double prob = 1.0) {
    using namespace NProtoFuzz;
    TFuzzer::TOptions opts(seed);
    opts.FuzzRatio = prob;
    opts.FuzzStructureRatio = prob;
    TFuzzer{opts}(msg);
}

template<class TFull, class TDomain>
void ReconstructConvertCheck(TProtoSchema const& schema) {
    for (size_t seed : xrange(100)) {
        TFull src;
        {
            TDomain msg;
            TFastRng64 rng(seed);
            FuzzProb(msg, rng.GenRand64(), 1.0);
            src.ParsePartialFromString(msg.SerializePartialAsString());
        }

        auto buffer = New<TRowBuffer>();
        TFull reconstructed;
        TUnversionedRow row = schema.MessageToRow(&src, buffer);
        schema.RowToMessage(row, &reconstructed);

        ASSERT_PROTO_EQ(src, reconstructed);
    }
}

void TTestProtoSchema::TestConvertReconstruct() {
    ReconstructConvertCheck<TZoo, TZoo>(Schema);
    ReconstructConvertCheck<TZoo, TFilteredZoo>(Schema
        .Filter({
            TZoo::kSignedFieldNumber,
            TZoo::kSigned32FieldNumber,
            TZoo::kStringFieldNumber
        })
    );
    ReconstructConvertCheck<TIntegral, TIntegral>(TProtoSchema{TIntegral::descriptor(), Primary});
    ReconstructConvertCheck<TOneOf, TOneOf>(TProtoSchema{TOneOf::descriptor(), Primary});
    ReconstructConvertCheck<TAggregated, TAggregated>(TProtoSchema{TAggregated::descriptor(), Primary});
}

void TTestProtoSchema::TestConvertCompatibility() {
    {
        TNewEnumMsg newProto;
        newProto.SetEnumField(TNewEnumMsg::VALUE_3);
        TProtoSchema newSchema{TNewEnumMsg::descriptor(), Primary};

        auto buffer = New<TRowBuffer>();
        auto row = newSchema.MessageToRow(&newProto, buffer);

        TOldEnumMsg oldProto;
        TProtoSchema oldSchema{TOldEnumMsg::descriptor(), Primary};

        ASSERT_THROW(oldSchema.RowToMessage(row, &oldProto), TErrorException);
    }
}

void TTestProtoSchema::TestConvertExceptionSafety() {
    TZoo msg;

    TUnversionedOwningRowBuilder builder;
    builder.AddValue(NTableClient::MakeUnversionedInt64Value(42, 0));

    ASSERT_NO_THROW(Schema.RowToMessage(builder.FinishRow(), &msg));
    ASSERT_TRUE(msg.GetSigned() == 42);

    msg.Clear();
    builder.AddValue(NTableClient::MakeUnversionedInt64Value(42, 0));
    builder.AddValue(NTableClient::MakeUnversionedUint64Value(42, 239));

    ASSERT_THROW(Schema.RowToMessage(builder.FinishRow(), &msg), TErrorException);
    ASSERT_FALSE(msg.HasSigned());
}

void TTestProtoSchema::TestConvertStrict() {
    TZoo msg;
    msg.SetSigned(42);
    msg.SetString("42");

    auto filtered = Schema.Filter({TZoo::kSignedFieldNumber});
    ASSERT_NO_THROW(filtered.MessageToRow(&msg, Buffer));

    TConvertRowOptions opts;
    opts.Strict = true;
    ASSERT_THROW(filtered.MessageToRow(&msg, Buffer, opts), TErrorException);

    msg.ClearString();
    ASSERT_NO_THROW(filtered.MessageToRow(&msg, Buffer));
}

void TTestProtoSchema::TestConvertPartial() {
    TConvertRowOptions opts;
    opts.Partial = true;

    TZoo src1;
    src1.SetSigned(42);
    TZoo src2;
    src2.SetString("42");

    {
        TZoo dst;
        Schema.RowToMessage(Schema.MessageToRow(&src1, Buffer, opts), &dst, opts);
        Schema.RowToMessage(Schema.MessageToRow(&src2, Buffer), &dst, opts);
        ASSERT_FALSE(dst.HasSigned());
        ASSERT_EQ(dst.GetString(), "42");
    }
    {
        TZoo dst;
        Schema.RowToMessage(Schema.MessageToRow(&src1, Buffer, opts), &dst, opts);
        Schema.RowToMessage(Schema.MessageToRow(&src2, Buffer, opts), &dst, opts);
        ASSERT_EQ(dst.GetSigned(), 42);
        ASSERT_EQ(dst.GetString(), "42");
    }
}

// LookupRows can return null rows when KeepMissingRows is true
void TTestProtoSchema::TestConvertNull() {
    TUnversionedRow null;
    TZoo empty, target;

    target.SetString("foo");
    Schema.RowToMessage(null, &target);
    ASSERT_PROTO_EQ(empty, target);

    target.SetString("foo");
    TZoo ans = target;
    Schema.RowToMessage(null, &target, {.Partial = true});
    ASSERT_PROTO_EQ(ans, target);
}

void TTestProtoSchema::TestCache() {
    auto zoo = TProtoSchema::Get(TZoo::descriptor(), Primary);
    auto zoo_write = TProtoSchema::Get(TZoo::descriptor(), ETableSchemaKind::Write);
    auto ord = TProtoSchema::Get(TOrdered::descriptor(), Primary);
    auto ord_lookup = TProtoSchema::Get(TOrdered::descriptor(), ETableSchemaKind::Lookup);

    ASSERT_TRUE((zoo->Schema() == TProtoSchema{TZoo::descriptor(), Primary}.Schema()));
    ASSERT_TRUE((zoo_write->Schema() == TProtoSchema{TZoo::descriptor(), ETableSchemaKind::Write}.Schema()));
    ASSERT_TRUE((ord->Schema() == TProtoSchema{TOrdered::descriptor(), Primary}.Schema()));
    ASSERT_TRUE((ord_lookup->Schema() == TProtoSchema{TOrdered::descriptor(), ETableSchemaKind::Lookup}.Schema()));
}

void TTestProtoSchema::TestMutation() {
    TVector<TProtoSchemaPtr> goodSchemas = {
        TProtoSchema::Get(TOrdered::descriptor(), ETableSchemaKind::Primary),
        TProtoSchema::Get(TFilteredZoo::descriptor(), ETableSchemaKind::Primary),
    };
    TVector<TProtoSchemaPtr> badSchemas = {
        TProtoSchema::Get(TBox::descriptor(), ETableSchemaKind::Primary),
        TProtoSchema::Get(TNotZoo1::descriptor(), ETableSchemaKind::Primary),
        TProtoSchema::Get(TNotZoo2::descriptor(), ETableSchemaKind::Primary),
    };
    for (auto& goodSchema : goodSchemas) {
        TProtoSchema schema{TZoo::descriptor(), goodSchema->Schema()};
        Cerr << goodSchema->Descriptor()->full_name() << " -> TZoo mutation check: OK" << Endl;
    }
    for (auto& badSchema : badSchemas) try {
        TProtoSchema schema{TZoo::descriptor(), badSchema->Schema()};
        Cerr << badSchema->Descriptor()->full_name() << " -> TZoo mutation doesn't fail" << Endl;
        ASSERT_TRUE(false);
    } catch (TErrorException& ex) {
        Cerr << badSchema->Descriptor()->full_name() << " -> TZoo mutation check: OK" << Endl;
    }
}
} // NProtoApiTest
} // NYT
