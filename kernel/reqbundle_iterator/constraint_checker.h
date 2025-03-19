#pragma once

#include <kernel/reqbundle_iterator/proto/constraint_checker.pb.h>

#include <kernel/reqbundle/reqbundle_accessors.h>
#include <kernel/reqbundle_iterator/position.h>
#include <kernel/reqbundle/constraint_contents.h>
#include <kernel/sent_lens/sent_lens.h>

#include <util/generic/algorithm.h>
#include <util/generic/array_ref.h>
#include <util/generic/hash.h>



namespace NReqBundleIterator {

    class TConstraintChecker {
    private:
        struct TConstraintInfo {
            size_t Index = 0;
            TConstConstraintAcc Constraint;

            TConstraintInfo() = default;
            TConstraintInfo(size_t index, TConstConstraintAcc constraint)
                : Index(index)
                , Constraint(constraint)
            {
            }
        };

    public:
        struct TOptions {
            bool IgnoreZeroBreak = false;
            bool CheckQuotedConstraint = false;
            bool AllBlocksAreOriginal = false; // for testing purposes TODO(danlark@) remove
        };

        TConstraintChecker(TConstReqBundleAcc reqBundle, const TOptions& options);

        // Reconstructs an object previously serialized by Serialize()
        static TConstraintChecker Deserialize(TStringBuf serialized);

        bool Validate(const TArrayRef<const TPosition>& positions, const TSentenceLengths& sentenceLengths) const;

        inline bool GetNeedSentenceLengths() const {
            return NeedSentenceLengths;
        }

        // Returns a binary blob of data, not a human-readable string
        TString Serialize() const;

    private:
        TConstraintChecker() = default;

        bool ValidateQuotedConstraint(const TArrayRef<const TPosition>& positions, const TSentenceLengths& sentenceLengths, TConstConstraintAcc constraint) const;

        TVector<TVector<TConstraintInfo>> ConstraintsByFirstBlock;
        TOptions Options;
        size_t ConstraintCount = 0;
        bool NeedSentenceLengths = false;

        // multitoken block indices
        TVector<size_t> MultitokenBlockIndex;

        // for asterisks
        TVector<bool> IsAnyBlock;

        // boolean mask to take only original blocks into consideration
        TVector<bool> IsOriginalRequestBlock;

        // needed for finding block number of words in multitoken block
        TVector<ui32> MultitokenBlockWordCount;

        // TConstraintInfo keeps non-owning references to constraints normally kept alive by
        // the ReqBundle passed in the constructor. During deserialization we don't have access
        // to the original bundle anymore, so we have to store all the constraints inside the checker.
        TVector<NReqBundle::NDetail::TConstraintData> ConstraintHolder;
    };
} // namespace NReqBundleIterator
