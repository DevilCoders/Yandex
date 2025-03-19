#include <kernel/reqbundle_iterator/constraint_checker.h>

#include <kernel/reqbundle_iterator/proto/constraint_checker.pb.h>

namespace {
    using NReqBundleIterator::TPosition;
    using NReqBundle::TConstConstraintAcc;

    static inline void IncrementExpectedPos(TPosition& expectedPos, const TSentenceLengths& sentenceLengths, bool* jumpSentence = nullptr) {
        if (expectedPos.Break() >= sentenceLengths.Size()) {
            expectedPos.TWordPosBeg::Set(expectedPos.WordPosBeg() + 1);
        } else {
            if (expectedPos.WordPosBeg() + 1 > sentenceLengths[expectedPos.Break()]) {
                if (expectedPos.Break() + 1 >= sentenceLengths.Size()) {
                    expectedPos.SetInvalid();
                } else {
                    if (jumpSentence) {
                        *jumpSentence = true;
                    }
                    expectedPos.TWordPosBeg::Set(1);
                    expectedPos.TBreak::Set(expectedPos.Break() + 1);
                }
            } else {
                expectedPos.TWordPosBeg::Set(expectedPos.WordPosBeg() + 1);
            }
        }
    }
} // anonymous namespace

namespace NReqBundleIterator {

    TConstraintChecker::TConstraintChecker(TConstReqBundleAcc reqBundle, const TOptions& options)
        : Options(options)
        , ConstraintCount(reqBundle.GetNumConstraints())
        , MultitokenBlockIndex(reqBundle.GetSequence().GetNumElems(), Max<size_t>())
        , IsAnyBlock(reqBundle.GetSequence().GetNumElems())
        , IsOriginalRequestBlock(reqBundle.GetSequence().GetNumElems())
        , MultitokenBlockWordCount(reqBundle.GetSequence().GetNumElems())
    {
        if (Options.AllBlocksAreOriginal) {
            IsOriginalRequestBlock.assign(IsOriginalRequestBlock.size(), true);
        }
        for (size_t i = 0; i < reqBundle.GetNumConstraints(); i++) {
            const TConstConstraintAcc& constraint = reqBundle.GetConstraint(i);
            const TVector<size_t>& blockIndices = constraint.GetBlockIndices();
            if (Y_UNLIKELY(blockIndices.empty())) {
                Y_ASSERT(false);
                continue;
            }
            size_t firstBlockId = blockIndices[0];
            if (firstBlockId >= ConstraintsByFirstBlock.size()) {
                ConstraintsByFirstBlock.resize(firstBlockId + 1);
            }
            ConstraintsByFirstBlock[firstBlockId].emplace_back(i, constraint);
            NeedSentenceLengths = (constraint.GetType() == NLingBoost::TConstraint::Quoted && Options.CheckQuotedConstraint);
        }

        // for any words
        for (size_t i = 0; i < reqBundle.GetSequence().GetNumElems(); ++i) {
            IsAnyBlock[i] = (reqBundle.GetSequence().GetBlock(i).GetNumWords() == 1 && reqBundle.GetSequence().GetBlock(i).GetWord(0).IsAnyWord());
        }

        auto assignWithResize = [](auto& cont, size_t index, const auto& value) {
            if (index >= cont.size()) {
                cont.resize(index + 1);
            }
            cont[index] = value;
        };

        // we find for each word its original one word block
        TVector<ui64> blockByWord;
        for (const NReqBundle::TRequest& req : reqBundle.GetRequests()) {
            if (req.HasExpansionType(NLingBoost::TExpansion::OriginalRequest)) {
                for (const NReqBundle::TConstMatchAcc& match : req.GetMatches()) {
                    if (match.GetType() == NLingBoost::TMatch::OriginalWord && match.GetNumWords() == 1) {
                        assignWithResize(blockByWord, match.GetWordIndexFirst(), match.GetBlockIndex());

                        // only original blocks are taken into consideration
                        assignWithResize(IsOriginalRequestBlock, match.GetBlockIndex(), true);
                    }
                }

                // then we try to find synonyms for these words (they are multitokens)
                for (const NReqBundle::TConstMatchAcc& match : req.GetMatches()) {
                    if (match.GetType() == NLingBoost::TMatch::Synonym && match.GetNumWords() == 1) {
                        if (match.GetWordIndexFirst() < blockByWord.size()) {
                            if (blockByWord[match.GetWordIndexFirst()] >= MultitokenBlockIndex.size()) {
                                MultitokenBlockIndex.resize(blockByWord[match.GetWordIndexFirst()] + 1, Max<size_t>());
                                MultitokenBlockWordCount.resize(MultitokenBlockIndex.size());
                            }
                            // take first synonym as multitokens as they are really the first
                            if (MultitokenBlockIndex[blockByWord[match.GetWordIndexFirst()]] == Max<size_t>()) {
                                const size_t block = blockByWord[match.GetWordIndexFirst()];
                                MultitokenBlockIndex[block] = match.GetBlockIndex();
                                MultitokenBlockWordCount[block] = reqBundle.GetSequence().GetBlock(match.GetBlockIndex()).GetNumWords();
                            }
                        }
                        // only original blocks are taken into consideration
                        assignWithResize(IsOriginalRequestBlock, match.GetBlockIndex(), true);
                    }
                }
            }
        }
    }

    bool TConstraintChecker::Validate(const TArrayRef<const TPosition>& positions, const TSentenceLengths& sentenceLengths) const {
        if (ConstraintsByFirstBlock.empty()) {
            return true;
        }

        TVector<bool> constraintValidated(ConstraintCount, false);

        for (const TVector<TConstraintInfo>& infos : ConstraintsByFirstBlock) {
            for (const TConstraintInfo& info : infos) {
                switch (info.Constraint.GetType()) {
                    case NLingBoost::TConstraint::MustNot:
                        constraintValidated[info.Index] = true;
                        break;
                    case NLingBoost::TConstraint::Quoted:
                        constraintValidated[info.Index] = !Options.CheckQuotedConstraint;
                        break;
                    default:
                        break;
                }
            }
        }

        for (size_t i = 0; i < positions.size(); i++) {
            size_t firstBlockId = positions[i].BlockId();

            /*
                first word sometimes is not a word :)
                it may be of multitokens e.g. "can't reconnect until invalid transaction is rolled back" or `+h5py`
            */
            if (firstBlockId >= ConstraintsByFirstBlock.size() || ConstraintsByFirstBlock[firstBlockId].empty()) {
                for (size_t j = 0; j < MultitokenBlockIndex.size(); ++j) {
                    if (MultitokenBlockIndex[j] == firstBlockId) {
                        firstBlockId = j;
                        break;
                    }
                }
            }

            if (firstBlockId >= ConstraintsByFirstBlock.size()) {
                continue;
            }

            for (const TConstraintInfo& info : ConstraintsByFirstBlock[firstBlockId]) {
                TArrayRef<const TPosition> slice = positions.Slice(i);
                bool validated = false;
                switch (info.Constraint.GetType()) {
                    case NLingBoost::TConstraint::Quoted:
                        if (!constraintValidated[info.Index]) {
                            validated = ValidateQuotedConstraint(slice, sentenceLengths, info.Constraint);
                        } else {
                            validated = true;
                        }
                        break;
                    case NLingBoost::TConstraint::Must:
                        validated = true;
                        break;
                    case NLingBoost::TConstraint::MustNot:
                        return false;
                    default:
                        Y_ASSERT(false);
                        break;
                }
                constraintValidated[info.Index] = validated;
            }
        }

        return Find(constraintValidated.begin(), constraintValidated.end(), false) == constraintValidated.end();
    }

    inline bool TConstraintChecker::ValidateQuotedConstraint(
        const TArrayRef<const TPosition>& positions,
        const TSentenceLengths& sentenceLengths,
        TConstConstraintAcc constraint) const
    {

        const TVector<size_t>& blockIndices = constraint.GetBlockIndices();

        Y_ASSERT(!blockIndices.empty());

        size_t currentBlock = 0;
        if (Options.IgnoreZeroBreak && positions[0].Break() == 0) {
            return false;
        }

        TPosition expectedPos = positions[0];

        size_t i = 0;
        while (currentBlock < blockIndices.size() && i < positions.size()) {
            // the end of index
            if (!expectedPos.Valid()) {
                return false;
            }

            TPosition pos = positions[i];

            /*
                inlalid positions are skipped, if there are many the same positions with different blocks, it will be skipped later
            */
            if (pos.BlockId() >= IsOriginalRequestBlock.size() || !IsOriginalRequestBlock[pos.BlockId()]) {
                ++i;
                // be sure that every continue does some forward moving (++i or ++currentBlock)
                continue;
            }

            if (pos.Break() != expectedPos.Break() || pos.WordPosBeg() != expectedPos.WordPosBeg()) {
                if (!IsAnyBlock[blockIndices[currentBlock]]) {
                    // no chance for the word, something is completely missed
                    return false;
                } else {
                    /*
                       unknown word in the positions, should continue
                       no way to fix multitokens inside it, we don't know how many positions multitoken will have
                       because it depends on the language and false positive result may happen, :sadface:
                    */
                    IncrementExpectedPos(expectedPos, sentenceLengths);
                    ++currentBlock;
                    // be sure that every continue does some forward moving (++i or ++currentBlock)
                    continue;
                }
            }

            // any block can be here because
            bool meetCurrentBlock = false;
            ui32 incremented = 0;
            auto blockChecker = [&](ui32 blockId, bool anyBlock) noexcept {
                // see all hits for the query with same break and position
                while (i < positions.size() && pos.Break() == expectedPos.Break() && pos.WordPosBeg() == expectedPos.WordPosBeg()) {
                    if (anyBlock || pos.BlockId() == blockId) {
                        meetCurrentBlock = true;
                    }
                    ++i;
                    ++incremented;
                    if (i >= positions.size()) {
                        break;
                    }
                    pos = positions[i];
                }
            };

            // known word in terms of known blocks in reqbundle, should just skip all the positions
            blockChecker(blockIndices[currentBlock], IsAnyBlock[blockIndices[currentBlock]]);

            bool hasTokens = blockIndices[currentBlock] < MultitokenBlockIndex.size() && MultitokenBlockIndex[blockIndices[currentBlock]] != Max<size_t>();

            // invariant for the any block and meeting the block, we always meet it
            Y_ASSERT(!IsAnyBlock[blockIndices[currentBlock]] || meetCurrentBlock);

            // we may not find the block with word -> we guess they are multitokens
            if (!meetCurrentBlock) {
                // it can happen that multitokens can find the way)
                if (hasTokens) {
                    i -= incremented;
                    pos = positions[i];
                    blockChecker(MultitokenBlockIndex[blockIndices[currentBlock]], /*no matter*/false);
                }
            }
            // if we haven't found word or its multitokens -> quit
            if (!meetCurrentBlock) {
                return false;
            }

            /*
                `can't` takes two positions iff it is not the end of the sentence, so if we jumped sentence,
                we should not increase the multitokens advancing
            */
            bool jumpSentence = false;
            if (hasTokens) {
                ui32 advanceNumber = MultitokenBlockWordCount[blockIndices[currentBlock]];
                for (size_t j = 1; j < advanceNumber; ++j) {
                    IncrementExpectedPos(expectedPos, sentenceLengths, &jumpSentence);
                    // validness is checked after the incrementing and strongly before the jump sentence break
                    if (!expectedPos.Valid()) {
                        return false;
                    }
                    if (jumpSentence) {
                        break;
                    }
                }
            }

            /*
                if we jumped sentence, we are on a correct position, so no need to jump
                e.g. "can't can't can't can't"
            */
            if (!jumpSentence) {
                IncrementExpectedPos(expectedPos, sentenceLengths);
            }

            ++currentBlock;
        }

        /*
            we must find all the blocks, if `return true;` here we may reach the end of the text
            and do not find all the blocks, e.g. "вечерние платья wildberries"
        */
        return currentBlock == blockIndices.size();
    }

    TConstraintChecker TConstraintChecker::Deserialize(TStringBuf serialized) {
        NReqBundleIteratorProto::TConstraintChecker proto;
        Y_ENSURE(proto.ParseFromArray(serialized.data(), serialized.size()));

        TConstraintChecker result;

        auto deserializeConstraintInfo = [](const NReqBundleIteratorProto::TConstraintInfo& info) {
            NReqBundle::NDetail::TConstraintData result;
            result.Type = static_cast<NLingBoost::EConstraintType>(info.GetType());
            result.BlockIndices.assign(info.GetBlockIndices().begin(), info.GetBlockIndices().end());
            return result;
        };

        // It's crucial that it never reallocates
        result.ConstraintHolder.resize(proto.GetConstraintCount());
        size_t constraintHolderIndex = 0;

        result.ConstraintsByFirstBlock.reserve(proto.GetConstraintsByFirstBlock().size());
        for (const auto& block : proto.GetConstraintsByFirstBlock()) {
            TVector<TConstraintInfo> newBlock(Reserve(block.GetConstraints().size()));
            for (const auto& constraint : block.GetConstraints()) {
                result.ConstraintHolder.at(constraintHolderIndex) = deserializeConstraintInfo(constraint);
                newBlock.emplace_back(constraint.GetIndex(), result.ConstraintHolder.at(constraintHolderIndex));
                ++constraintHolderIndex;
            }
            result.ConstraintsByFirstBlock.push_back(std::move(newBlock));
        }

        result.Options.IgnoreZeroBreak = proto.GetOptions().GetIgnoreZeroBreak();
        result.Options.CheckQuotedConstraint = proto.GetOptions().GetCheckQuotedConstraint();

        result.ConstraintCount = proto.GetConstraintCount();
        result.NeedSentenceLengths = proto.GetNeedSentenceLengths();

        result.MultitokenBlockIndex.assign(proto.GetMultitokenBlockIndex().begin(), proto.GetMultitokenBlockIndex().end());
        result.IsAnyBlock.assign(proto.GetIsAnyBlock().begin(), proto.GetIsAnyBlock().end());
        result.IsOriginalRequestBlock.assign(proto.GetIsOriginalRequestBlock().begin(), proto.GetIsOriginalRequestBlock().end());
        result.MultitokenBlockWordCount.assign(proto.GetMultitokenBlockWordCount().begin(), proto.GetMultitokenBlockWordCount().end());

        return result;
    }

    TString TConstraintChecker::Serialize() const {
        NReqBundleIteratorProto::TConstraintChecker proto;

        auto serializeConstraintInfo = [](const TConstraintInfo& info) {
            NReqBundleIteratorProto::TConstraintInfo proto;
            proto.SetIndex(info.Index);
            proto.SetType(info.Constraint.GetType());
            proto.MutableBlockIndices()->Reserve(info.Constraint.GetBlockIndices().size());
            for (const auto& index : info.Constraint.GetBlockIndices()) {
                *proto.MutableBlockIndices()->Add() = index;
            }
            return proto;
        };

        proto.MutableConstraintsByFirstBlock()->Reserve(ConstraintsByFirstBlock.size());
        for (const auto& block : ConstraintsByFirstBlock) {
            auto& newBlock = *proto.MutableConstraintsByFirstBlock()->Add();
            newBlock.MutableConstraints()->Reserve(block.size());
            for (const auto& constraintInfo : block) {
                *newBlock.MutableConstraints()->Add() = serializeConstraintInfo(constraintInfo);
            }
        }

        proto.MutableOptions()->SetIgnoreZeroBreak(Options.IgnoreZeroBreak);
        proto.MutableOptions()->SetCheckQuotedConstraint(Options.CheckQuotedConstraint);

        proto.SetConstraintCount(ConstraintCount);
        proto.SetNeedSentenceLengths(NeedSentenceLengths);

        proto.MutableMultitokenBlockIndex()->Reserve(MultitokenBlockIndex.size());
        for (const auto index : MultitokenBlockIndex) {
            *proto.MutableMultitokenBlockIndex()->Add() = index;
        }

        proto.MutableIsAnyBlock()->Reserve(IsAnyBlock.size());
        for (const auto isAny : IsAnyBlock) {
            *proto.MutableIsAnyBlock()->Add() = isAny;
        }

        proto.MutableIsOriginalRequestBlock()->Reserve(IsOriginalRequestBlock.size());
        for (const auto isOriginal : IsOriginalRequestBlock) {
            *proto.MutableIsOriginalRequestBlock()->Add() = isOriginal;
        }

        proto.MutableMultitokenBlockWordCount()->Reserve(MultitokenBlockWordCount.size());
        for (const auto wordCount : MultitokenBlockWordCount) {
            *proto.MutableMultitokenBlockWordCount()->Add() = wordCount;
        }

        return proto.SerializeAsString();
    }
}
