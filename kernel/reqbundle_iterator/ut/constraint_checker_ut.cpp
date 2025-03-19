#include <kernel/reqbundle/block_contents.h>
#include <kernel/reqbundle/reqbundle.h>
#include <kernel/reqbundle/serializer.h>
#include <kernel/reqbundle_iterator/constraint_checker.h>


#include <library/cpp/testing/unittest/registar.h>

using namespace NReqBundleIterator;

struct TPositionDef {
    size_t BlockId = 0;
    size_t Break = 0;
    size_t WordPosBeg = 0;

    TPositionDef() = default;
    TPositionDef(size_t blockId, size_t breakId, size_t wordPosBeg)
        : BlockId(blockId)
        , Break(breakId)
        , WordPosBeg(wordPosBeg)
    {
    }
};

static TVector<TPosition> GeneratePositions(std::initializer_list<TPositionDef> positions) {
    TVector<TPosition> result;
    for (const TPositionDef& def : positions) {
        TPosition pos;
        pos.Clear();
        pos.TBlockId::Set(def.BlockId);
        pos.TBreak::Set(def.Break);
        pos.TWordPosBeg::Set(def.WordPosBeg);
        result.push_back(pos);
    }
    return result;
}

static TSentenceLengths GenerateSentenceLenghts(std::initializer_list<size_t> sentenceLengths) {
    TSentenceLengths result;
    for (size_t sentLen : sentenceLengths) {
        result.push_back(sentLen);
    }
    return result;
}

static bool Validate(
    const NReqBundleProtocol::TReqBundle& proto,
    const TConstraintChecker::TOptions& options,
    std::initializer_list<TPositionDef> positions,
    std::initializer_list<size_t> sentenceLengths) {

    TReqBundleDeserializer::TOptions deserializerOptions;
    deserializerOptions.FailMode = TReqBundleDeserializer::EFailMode::ThrowOnError;
    TReqBundleDeserializer deserializer(deserializerOptions);


    TReqBundle bundle;
    deserializer.DeserializeProto(proto, bundle);

    TConstraintChecker checker(bundle, options);
    bool result = checker.Validate(
        GeneratePositions(positions),
        GenerateSentenceLenghts(sentenceLengths)
    );

    TConstraintChecker secondChecker = TConstraintChecker::Deserialize(checker.Serialize());

    bool secondResult = secondChecker.Validate(
        GeneratePositions(positions),
        GenerateSentenceLenghts(sentenceLengths)
    );

    UNIT_ASSERT(result == secondResult);
    return result;
}

Y_UNIT_TEST_SUITE(TestConstraintChecker) {
    Y_UNIT_TEST(AnythingEmpty) {
        TReqBundle bundle;
        NReqBundleProtocol::TReqBundle proto;
        TReqBundleDeserializer().DeserializeProto(proto, bundle);
        UNIT_ASSERT(Validate({}, {}, {}, {}));
    }

    Y_UNIT_TEST(EmptyConstraints) {
        TReqBundle bundle;
        NReqBundleProtocol::TReqBundle proto;
        TReqBundleDeserializer().DeserializeProto(proto, bundle);

        UNIT_ASSERT(Validate(proto, {}, {
            TPositionDef(0, 1, 1),
            TPositionDef(1, 1, 2),
            TPositionDef(2, 1, 3)
        }, { 0, 3 }));
    }

    Y_UNIT_TEST(SingleQuotedConstraint) {
        NReqBundleProtocol::TReqBundle proto;
        proto.AddBlocks();
        proto.AddBlocks();
        NReqBundleProtocol::TConstraint* constraint = proto.AddConstraints();

        constraint->SetType(NLingBoost::TConstraint::Quoted);
        constraint->AddBlockIndices(0);

        TConstraintChecker::TOptions options;
        options.CheckQuotedConstraint = true;
        options.AllBlocksAreOriginal = true;

        UNIT_ASSERT(Validate(proto, options, {
            TPositionDef(0, 1, 1)
        }, { 0, 1 }));

        constraint->SetBlockIndices(0, 1);

        UNIT_ASSERT(!Validate(proto, options, {
            TPositionDef(0, 1, 1),
        }, { 0, 1 }));
    }

    Y_UNIT_TEST(ManyWordsQuotedConstraint) {
        NReqBundleProtocol::TReqBundle proto;
        proto.AddBlocks();
        proto.AddBlocks();
        proto.AddBlocks();
        proto.AddBlocks();
        NReqBundleProtocol::TConstraint* constraint = proto.AddConstraints();

        constraint->SetType(NLingBoost::TConstraint::Quoted);
        constraint->AddBlockIndices(0);
        constraint->AddBlockIndices(1);
        constraint->AddBlockIndices(3);

        TConstraintChecker::TOptions options;
        options.CheckQuotedConstraint = true;
        options.AllBlocksAreOriginal = true;

        UNIT_ASSERT(Validate(proto, options, {
            TPositionDef(0, 1, 1),
            TPositionDef(1, 1, 2),
            TPositionDef(3, 2, 1),
        }, { 0, 2, 2 }));

        UNIT_ASSERT(!Validate(proto, options, {
            TPositionDef(0, 1, 1),
            TPositionDef(1, 1, 2),
            TPositionDef(3, 2, 2),
        }, { 0, 2, 2 }));
    }

    Y_UNIT_TEST(IgnoreZeroBreakQuotedConstraint) {
        NReqBundleProtocol::TReqBundle proto;
        proto.AddBlocks();
        proto.AddBlocks();
        proto.AddBlocks();
        proto.AddBlocks();
        NReqBundleProtocol::TConstraint* constraint = proto.AddConstraints();

        constraint->SetType(NLingBoost::TConstraint::Quoted);
        constraint->AddBlockIndices(0);
        constraint->AddBlockIndices(1);
        constraint->AddBlockIndices(3);

        TConstraintChecker::TOptions options;
        options.CheckQuotedConstraint = true;
        options.AllBlocksAreOriginal = true;

        UNIT_ASSERT(Validate(proto, options, {
            TPositionDef(0, 0, 1),
            TPositionDef(1, 0, 2),
            TPositionDef(3, 1, 1),
        }, { 2, 1 }));

        options.IgnoreZeroBreak = true;
        UNIT_ASSERT(!Validate(proto, options, {
            TPositionDef(0, 0, 1),
            TPositionDef(1, 0, 2),
            TPositionDef(3, 1, 1),
        }, { 2, 1 }));

        UNIT_ASSERT(!Validate(proto, options, {
            TPositionDef(0, 0, 1),
            TPositionDef(0, 0, 2),
            TPositionDef(0, 0, 3),
        }, { 3 }));
    }

    Y_UNIT_TEST(RepeatedWordsQuotedConstraint) {
        NReqBundleProtocol::TReqBundle proto;
        proto.AddBlocks();
        proto.AddBlocks();
        proto.AddBlocks();
        proto.AddBlocks();
        NReqBundleProtocol::TConstraint* constraint = proto.AddConstraints();

        constraint->SetType(NLingBoost::TConstraint::Quoted);
        constraint->AddBlockIndices(0);
        constraint->AddBlockIndices(1);

        TConstraintChecker::TOptions options;
        options.CheckQuotedConstraint = true;
        options.AllBlocksAreOriginal = true;

        UNIT_ASSERT(Validate(proto, options, {
            TPositionDef(0, 1, 1),
            TPositionDef(1, 1, 1),
            TPositionDef(1, 1, 2),
            TPositionDef(3, 2, 1),
        }, { 0, 2, 2 }));

        UNIT_ASSERT(!Validate(proto, options, {
            TPositionDef(0, 1, 1),
            TPositionDef(1, 1, 1),
            TPositionDef(3, 2, 1),
        }, { 0, 2, 2 }));
    }

    Y_UNIT_TEST(ManyWordsMustConstraint) {
        NReqBundleProtocol::TReqBundle proto;
        proto.AddBlocks();
        proto.AddBlocks();
        proto.AddBlocks();
        proto.AddBlocks();
        NReqBundleProtocol::TConstraint* constraint = proto.AddConstraints();

        constraint->SetType(NLingBoost::TConstraint::Must);
        constraint->AddBlockIndices(0);

        UNIT_ASSERT(Validate(proto, {}, {
            TPositionDef(0, 1, 1),
            TPositionDef(1, 1, 2),
            TPositionDef(3, 2, 1),
        }, { 0, 2, 2 }));

        UNIT_ASSERT(!Validate(proto, {}, {
            TPositionDef(1, 1, 2),
            TPositionDef(3, 2, 2),
        }, { 0, 2, 2 }));
    }

    Y_UNIT_TEST(ManyWordsMustNotConstraint) {
        NReqBundleProtocol::TReqBundle proto;
        proto.AddBlocks();
        proto.AddBlocks();
        proto.AddBlocks();
        proto.AddBlocks();
        NReqBundleProtocol::TConstraint* constraint = proto.AddConstraints();

        constraint->SetType(NLingBoost::TConstraint::MustNot);
        constraint->AddBlockIndices(0);

        UNIT_ASSERT(Validate(proto, {}, {
            TPositionDef(1, 1, 2),
            TPositionDef(3, 2, 2),
        }, { 0, 2, 2 }));

        UNIT_ASSERT(!Validate(proto, {}, {
            TPositionDef(0, 1, 1),
            TPositionDef(1, 1, 2),
            TPositionDef(3, 2, 1),
        }, { 0, 2, 2 }));
    }

    Y_UNIT_TEST(MultipleQuotedConstraints) {
        NReqBundleProtocol::TReqBundle proto;
        proto.AddBlocks();
        proto.AddBlocks();
        proto.AddBlocks();
        proto.AddBlocks();
        NReqBundleProtocol::TConstraint* constraint1 = proto.AddConstraints();
        NReqBundleProtocol::TConstraint* constraint2 = proto.AddConstraints();

        constraint1->SetType(NLingBoost::TConstraint::Quoted);
        constraint1->AddBlockIndices(0);
        constraint1->AddBlockIndices(1);
        constraint1->AddBlockIndices(2);

        constraint2->SetType(NLingBoost::TConstraint::Quoted);
        constraint2->AddBlockIndices(1);
        constraint2->AddBlockIndices(2);
        constraint2->AddBlockIndices(3);

        TConstraintChecker::TOptions options;
        options.CheckQuotedConstraint = true;
        options.AllBlocksAreOriginal = true;

        UNIT_ASSERT(Validate(proto, options, {
            TPositionDef(0, 1, 1),
            TPositionDef(1, 1, 2),
            TPositionDef(2, 2, 1),
            TPositionDef(3, 2, 2),
        }, { 0, 2, 2 }));

        UNIT_ASSERT(!Validate(proto, options, {
            TPositionDef(0, 1, 1),
            TPositionDef(1, 1, 2),
            TPositionDef(2, 2, 1),
        }, { 0, 2, 2 }));

        UNIT_ASSERT(!Validate(proto, options, {
            TPositionDef(1, 1, 1),
            TPositionDef(2, 1, 2),
            TPositionDef(3, 2, 2),
        }, { 0, 2, 2 }));
    }

    Y_UNIT_TEST(TestOneStarInQuotedConstraint) {
        NReqBundleProtocol::TReqBundle proto;
        proto.AddBlocks();
        NReqBundleProtocol::TBlock* block2 = proto.AddBlocks();

        NReqBundleProtocol::TBlockWordInfo wordInfo;
        wordInfo.SetAnyWord(true);
        NReqBundleProtocol::TBlockWordInfo* info = block2->MutableWords()->Add();
        *info = wordInfo;
        block2->SetType(NReqBundle::NDetail::EBlockType::ExactOrdered);

        proto.AddBlocks();
        proto.AddBlocks();

        NReqBundleProtocol::TConstraint* constraint1 = proto.AddConstraints();
        constraint1->SetType(NLingBoost::TConstraint::Quoted);
        constraint1->AddBlockIndices(0);
        constraint1->AddBlockIndices(1);
        constraint1->AddBlockIndices(2);
        constraint1->AddBlockIndices(3);

        TConstraintChecker::TOptions options;
        options.CheckQuotedConstraint = true;
        options.AllBlocksAreOriginal = true;

        // with some position
        UNIT_ASSERT(Validate(proto, options, {
            TPositionDef(0, 1, 1),
            TPositionDef(1, 1, 2),
            TPositionDef(2, 2, 1),
            TPositionDef(3, 2, 2),
        }, { 0, 2, 2 }));

        // with some bad position
        UNIT_ASSERT(Validate(proto, options, {
            TPositionDef(0, 1, 1),
            TPositionDef(3, 1, 2),
            TPositionDef(2, 2, 1),
            TPositionDef(3, 2, 2),
        }, { 0, 2, 2 }));

        // without block number 1
        UNIT_ASSERT(Validate(proto, options, {
            TPositionDef(0, 1, 1),
            TPositionDef(2, 2, 1),
            TPositionDef(3, 2, 2),
        }, { 0, 2, 2 }));

        // many blocks are ommited
        UNIT_ASSERT(!Validate(proto, options, {
            TPositionDef(0, 1, 1),
            TPositionDef(3, 2, 2),
        }, { 0, 2, 2 }));
    }

    Y_UNIT_TEST(TestManyStarsInQuotedConstraint) {
        NReqBundleProtocol::TReqBundle proto;
        proto.AddBlocks();
        NReqBundleProtocol::TBlock* block2 = proto.AddBlocks();

        NReqBundleProtocol::TBlockWordInfo wordInfo;
        wordInfo.SetAnyWord(true);
        *(block2->MutableWords()->Add()) = wordInfo;
        block2->SetType(NReqBundle::NDetail::EBlockType::ExactOrdered);

        NReqBundleProtocol::TBlock* block3 = proto.AddBlocks();
        *(block3->MutableWords()->Add()) = wordInfo;
        block3->SetType(NReqBundle::NDetail::EBlockType::ExactOrdered);
        proto.AddBlocks();

        NReqBundleProtocol::TConstraint* constraint1 = proto.AddConstraints();
        constraint1->SetType(NLingBoost::TConstraint::Quoted);
        constraint1->AddBlockIndices(0);
        constraint1->AddBlockIndices(1);
        constraint1->AddBlockIndices(2);
        constraint1->AddBlockIndices(3);

        TConstraintChecker::TOptions options;
        options.CheckQuotedConstraint = true;
        options.AllBlocksAreOriginal = true;

        UNIT_ASSERT(Validate(proto, options, {
            TPositionDef(0, 1, 1),
            TPositionDef(1, 1, 2),
            TPositionDef(2, 2, 1),
            TPositionDef(3, 2, 2),
        }, { 0, 2, 2 }));

        UNIT_ASSERT(Validate(proto, options, {
            TPositionDef(0, 1, 1),
            TPositionDef(3, 1, 2),
            TPositionDef(0, 1, 2),
            TPositionDef(3, 2, 2),
        }, { 0, 2, 2 }));

        UNIT_ASSERT(Validate(proto, options, {
            TPositionDef(0, 1, 1),
            TPositionDef(2, 2, 1),
            TPositionDef(3, 2, 2),
        }, { 0, 2, 2 }));

        UNIT_ASSERT(Validate(proto, options, {
            TPositionDef(0, 1, 1),
            TPositionDef(1, 1, 2),
            TPositionDef(3, 2, 2),
        }, { 0, 2, 2 }));

        UNIT_ASSERT(Validate(proto, options, {
            TPositionDef(0, 1, 1),
            TPositionDef(0, 1, 1),
            TPositionDef(3, 2, 2),
        }, { 0, 2, 2 }));

        UNIT_ASSERT(Validate(proto, options, {
            TPositionDef(0, 1, 1),
            TPositionDef(3, 2, 2),
        }, { 0, 2, 2 }));

        UNIT_ASSERT(!Validate(proto, options, {
            TPositionDef(3, 1, 1),
            TPositionDef(3, 2, 2),
        }, { 0, 2, 2 }));

        UNIT_ASSERT(!Validate(proto, options, {
            TPositionDef(0, 1, 1),
            TPositionDef(2, 2, 1),
        }, { 0, 2, 2 }));
    }
}
