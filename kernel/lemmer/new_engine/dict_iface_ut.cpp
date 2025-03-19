#include "iface/dict_iface.h"

#include <library/cpp/langs/langs.h>
#include <kernel/lemmer/dictlib/grammar_index.h>
#include <kernel/lemmer/new_engine/binary_dict/binary_dict.h>
#include <kernel/lemmer/new_engine/proto_dict/proto_dict.h>
#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/streams/factory/factory.h>
#include <util/charset/wide.h>
#include <util/memory/blob.h>
#include <util/stream/output.h>

using namespace NNewLemmer;

extern "C" {
    extern const unsigned char ExProtoDict[];
    extern const ui32 ExProtoDictSize;
    extern const unsigned char ExBinaryDict[];
    extern const ui32 ExBinaryDictSize;
};

enum EDictType {
    BinaryDict,
    ProtoDict
};

using IDictDataPtr = THolder<IDictData>;

template <EDictType Type>
struct TDictDataHolder {
    THolder<IDictData> DataPtr;
    THolder<IPattern> Pattern0;
    THolder<IPattern> Pattern5;
    THolder<NLemmerProto::TLemmerDict> DataProto;
    THolder<TData> DataBinary;
    TBlob Blob;
    TDictDataHolder() {
        switch(Type) {
            case ProtoDict: {
                DataProto = MakeHolder<NLemmerProto::TLemmerDict>();
                Y_PROTOBUF_SUPPRESS_NODISCARD DataProto->ParseFromArray(ExProtoDict, ExProtoDictSize);
                TBlob Blob = TBlob::NoCopy(ExProtoDict, ExProtoDictSize);
                DataPtr = MakeHolder<TProtoDictData>(Blob);
                Pattern0 = MakeHolder<TProtoPattern>(&*DataProto, 0);
                Pattern5 = MakeHolder<TProtoPattern>(&*DataProto, 5);
                break;
            }
            case BinaryDict: {
                TBlob Blob = TBlob::NoCopy(ExBinaryDict, ExBinaryDictSize);
                DataBinary = MakeHolder<TData>(Blob.AsCharPtr(), Blob.Length());
                DataPtr = MakeHolder<TBinaryDictData>(Blob.AsCharPtr(), Blob.Length());
                Pattern0 = MakeHolder<TBinaryPattern>(&*DataBinary, 0);
                Pattern5 = MakeHolder<TBinaryPattern>(&*DataBinary, 5);
                break;
            }
            default: {
                Y_FAIL("unknown type");
            }
        }
    }
};

template<EDictType Type>
class TDictDataTestSuite : public TTestBase {
private:
    UNIT_TEST_SUITE_DEMANGLE(TDictDataTestSuite<Type>);
    UNIT_TEST(TestScheme);
    UNIT_TEST(TestBlock);
    UNIT_TEST(TestGrammarStringIterator);
    UNIT_TEST(TestPattern);
    UNIT_TEST_SUITE_END();
private:
    TDictDataHolder<Type> dictDataHolder;
public:
    void TestScheme() {
        const ISchemePtr Scheme0 = dictDataHolder.DataPtr->GetScheme(0);
        const ISchemePtr Scheme1 = dictDataHolder.DataPtr->GetScheme(1);
        const ISchemePtr Scheme2 = dictDataHolder.DataPtr->GetScheme(2);
        UNIT_ASSERT(Scheme0 != nullptr);
        UNIT_ASSERT(Scheme1 != nullptr);
        UNIT_ASSERT_VALUES_EQUAL(Scheme0->GetId(), 0);
        UNIT_ASSERT_VALUES_EQUAL(Scheme1->GetId(), 1);
        UNIT_ASSERT_VALUES_EQUAL(Scheme0->GetFrequency(), 0);
        UNIT_ASSERT_VALUES_EQUAL(Scheme1->GetFrequency(), 0);
        UNIT_ASSERT_VALUES_EQUAL(sprint_grammar(Scheme1->GetGrammarString().AsCharPtr()), "S");
        UNIT_ASSERT_VALUES_EQUAL(sprint_grammar(Scheme0->GetGrammarString().AsCharPtr()), "S");
        UNIT_ASSERT_VALUES_EQUAL(TWtringBuf(Scheme0->GetLemmaFlex()), TWtringBuf(u"ler"));
        UNIT_ASSERT_VALUES_EQUAL(TWtringBuf(Scheme1->GetLemmaFlex()), TWtringBuf(u"er"));
    }

    void TestBlock() {
        const ISchemePtr Scheme0 = dictDataHolder.DataPtr->GetScheme(0);
        const ISchemePtr Scheme2 = dictDataHolder.DataPtr->GetScheme(2);
        const IBlockPtr Block0_4 = Scheme0->GetBlock(4);
        const IBlockPtr Block2_1 = Scheme2->GetBlock(1);
        const IBlockPtr BlockEnd = Scheme0->GetBlock(7);
        auto Iter = Block2_1->GetGrammarStringIterator();
        UNIT_ASSERT(Block0_4 != nullptr);
        UNIT_ASSERT(Block2_1 != nullptr);
        UNIT_ASSERT_VALUES_EQUAL(Block0_4->GetFrequency(), 0);
        UNIT_ASSERT_VALUES_EQUAL(Block2_1->GetFrequency(), 0);
        UNIT_ASSERT_VALUES_EQUAL(TWtringBuf(Block0_4->GetFormFlex()), TWtringBuf(u"ler"));
        UNIT_ASSERT(Block0_4->HasNext());
        UNIT_ASSERT(!BlockEnd->HasNext());
    }

    void TestGrammarStringIterator() {
        const ISchemePtr Scheme2 = dictDataHolder.DataPtr->GetScheme(2);
        const IBlockPtr Block2_1 = Scheme2->GetBlock(1);
        auto Iter = Block2_1->GetGrammarStringIterator();
        UNIT_ASSERT(Iter != nullptr);
        UNIT_ASSERT(Iter->IsValid());
        UNIT_ASSERT_VALUES_EQUAL(sprint_grammar(Iter->Get().AsCharPtr()), "praes,indic");
        ++(*Iter);
        UNIT_ASSERT(Iter != nullptr);
        UNIT_ASSERT(Iter->IsValid());
        UNIT_ASSERT_VALUES_EQUAL(sprint_grammar(Iter->Get().AsCharPtr()), "inf");
        ++(*Iter);
        UNIT_ASSERT(!Iter->IsValid());
    }

    void TestPattern() {
        UNIT_ASSERT(dictDataHolder.Pattern0->GetScheme() != nullptr);
        UNIT_ASSERT(dictDataHolder.Pattern5->GetScheme() != nullptr);
        UNIT_ASSERT_VALUES_EQUAL(dictDataHolder.Pattern0->GetScheme()->GetId(), 0);
        UNIT_ASSERT_VALUES_EQUAL(dictDataHolder.Pattern5->GetScheme()->GetId(), 2);
        UNIT_ASSERT_VALUES_EQUAL(size_t(dictDataHolder.Pattern0->GetStemLen()), 7);
        UNIT_ASSERT(!dictDataHolder.Pattern0->HasPrefix());
        UNIT_ASSERT(dictDataHolder.Pattern0->IsFinal());
        UNIT_ASSERT_VALUES_EQUAL(dictDataHolder.Pattern0->GetRest(), 0);
        UNIT_ASSERT(!dictDataHolder.Pattern0->IsUseAlways());
        UNIT_ASSERT_VALUES_EQUAL(dictDataHolder.Pattern0->GetDiaMask(), 0);
        UNIT_ASSERT_VALUES_EQUAL(dictDataHolder.Pattern5->GetDiaMask(), 0);
        UNIT_ASSERT(dictDataHolder.Pattern0->HasFlexTrie());
        UNIT_ASSERT(dictDataHolder.Pattern5->HasFlexTrie());
    }
};


UNIT_TEST_SUITE_REGISTRATION(TDictDataTestSuite<BinaryDict>);
UNIT_TEST_SUITE_REGISTRATION(TDictDataTestSuite<ProtoDict>);


