#pragma once

#include "reqbundle_contents.h"
#include "reqbundle_fwd.h"

namespace NReqBundle {

    template <typename DataType>
    class TReqBundleAccBase
        : public NDetail::TLightAccessor<DataType>
    {
    public:
        using NDetail::TLightAccessor<DataType>::Contents;
        using NDetail::TLightAccessor<DataType>::MutableContents;

        TReqBundleAccBase() = default;
        TReqBundleAccBase(const TReqBundleAcc& other)
            : NDetail::TLightAccessor<DataType>(other.Contents())
        {}
        TReqBundleAccBase(const TConstReqBundleAcc& other)
            : NDetail::TLightAccessor<DataType>(other.Contents())
        {}
        TReqBundleAccBase(DataType& data)
            : NDetail::TLightAccessor<DataType>(data)
        {}

        size_t GetNumRequests() const {
            return Contents().Requests.size();
        }
        TRequestAcc Request(size_t requestIndex) const {
            return TRequestAcc(*Contents().Requests[requestIndex]);
        }
        TConstRequestAcc GetRequest(size_t requestIndex) const {
            return TConstRequestAcc(*Contents().Requests[requestIndex]);
        }

        const TRequestPtr& GetRequestPtr(size_t requestIndex) const {
            return MutableContents().Requests[requestIndex];
        }

        auto Requests() const -> decltype(NDetail::MakeAccPtrContainer<TRequestAcc>(std::declval<DataType&>().Requests)) {
            return NDetail::MakeAccPtrContainer<TRequestAcc>(Contents().Requests);
        }
        auto GetRequests() const -> decltype(NDetail::MakeAccPtrContainer<TConstRequestAcc>(std::declval<DataType&>().Requests)) {
            return NDetail::MakeAccPtrContainer<TConstRequestAcc>(Contents().Requests);
        }

        size_t GetNumConstraints() const {
            return Contents().Constraints.size();
        }
        TConstraintAcc Constraint(size_t constraintIndex) const {
            return TConstraintAcc(*Contents().Constraints[constraintIndex]);
        }
        TConstConstraintAcc GetConstraint(size_t constraintIndex) const {
            return TConstConstraintAcc(*Contents().Constraints[constraintIndex]);
        }

        auto Constraints() const -> decltype(NDetail::MakeAccPtrContainer<TConstraintAcc>(std::declval<DataType&>().Constraints)) {
            return NDetail::MakeAccPtrContainer<TConstraintAcc>(Contents().Constraints);
        }
        auto GetConstraints() const -> decltype(NDetail::MakeAccPtrContainer<TConstConstraintAcc>(std::declval<DataType&>().Constraints)) {
            return NDetail::MakeAccPtrContainer<TConstConstraintAcc>(Contents().Constraints);
        }

        void AddRequest(const TRequestPtr& reqPtr) const {
            Y_ASSERT(reqPtr);
            Y_ASSERT(NDetail::IsValidRequest(*reqPtr, *Contents().Sequence));
            Contents().Requests.push_back(reqPtr);
        }

        void AddConstraint(const TConstraintPtr& constraintPtr) const {
            Y_ASSERT(constraintPtr);
            Y_ASSERT(NDetail::IsValidConstraint(*constraintPtr, *Contents().Sequence));
            Contents().Constraints.push_back(constraintPtr);
        }

        TSequenceAcc Sequence() const {
            return TSequenceAcc(*Contents().Sequence);
        }
        TConstSequenceAcc GetSequence() const {
            return TConstSequenceAcc(*Contents().Sequence);
        }

        void ReplaceSequence(const TSequencePtr& seqPtr) const {
            NDetail::ReplaceSequence(Contents(), seqPtr);
        }
        void StripRequests() const {
            Contents().Requests.clear();
        }

        template <typename KeyFunc>
        void SortRequestsBy(const KeyFunc& func) {
            SortBy(Contents().Requests, [&func](const TRequestPtr& x){
                return func(TConstRequestAcc(*x));
            });
        }

        bool HasExpansionType(EExpansionType type) const {
            for (const auto& request : GetRequests()) {
                if (request.HasExpansionType(type)) {
                    return true;
                }
            }
            return false;
        }
    };

    namespace NDetail {
        struct TValidConstraints;

        EValidType IsValidReqBundle(TConstReqBundleAcc bundle, bool needOriginal = true);
        EValidType IsValidReqBundle(TConstReqBundleAcc bundle, const TValidConstraints& constr);

        EValidType IsValidRequests(TConstReqBundleAcc bundle, const TValidConstraints& constr);
        EValidType IsValidConstraints(TConstReqBundleAcc bundle);

        class TRequestsRemapper
            : public TVector<size_t>
        {
        public:
            TRequestsRemapper(size_t numRequests = 0);
            void Reset(size_t numRequests);

            bool Has(size_t requestIndex) const;
        };
    } // NDetail
} // NReqBundle
