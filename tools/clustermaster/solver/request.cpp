#include "request.h"

#include "solver.h"

bool TClaims::Combine(const TClaims& right) {
PROFILE_ACQUIRE(PROFILE_CLAIMS_COMBINE)

    const TBase* rightBase = &right;
    THolder<TBase> tempBase;

    for (TSharedParts::const_iterator rightPart = right.SharedParts.begin(); rightPart != right.SharedParts.end(); ++rightPart) {
        const TSharedParts::iterator thisPart = SharedParts.find(rightPart->first);

        if (thisPart != SharedParts.end()) {
            if (!tempBase.Get()) {
                tempBase.Reset(new TBase);
                *tempBase = right;
                rightBase = tempBase.Get();
            }

            const TBase::iterator rightClaim = tempBase->find(rightPart->first.second);
            const TBase::iterator thisClaim = find(thisPart->first.second);

            Y_VERIFY(rightClaim != tempBase->end() && rightClaim->second >= rightPart->second, "Faulty claims");
            Y_VERIFY(thisClaim != end() && thisClaim->second >= thisPart->second, "Faulty claims");

            rightClaim->second -= Min(rightPart->second, thisPart->second);
            thisPart->second = Max(thisPart->second, rightPart->second);
        }
    }

    SharedParts.insert(right.SharedParts.begin(), right.SharedParts.end());

    bool exceeded = false;

    for (const_iterator i = rightBase->begin(); i != rightBase->end(); ++i) {
        std::pair<iterator, bool> ret = insert(*i);

        if (!ret.second) {
            ret.first->second += i->second;

            if (ret.first->second < i->second) {
                ret.first->second = Max<value_type::second_type>();
                exceeded = true;
            }
        }
    }

    return !exceeded;
}

TRequest::TRequest(TBatch* batch, const NCore::TDefineMessage& message)
    : Batch(batch)
    , Key(message.GetKey())
{
PROFILE_ACQUIRE(PROFILE_REQUEST_PARSE)

    for (google::protobuf::RepeatedPtrField<NProto::TDefinition::TClaim>::const_iterator i = message.GetDefinition().GetClaim().begin(); i != message.GetDefinition().GetClaim().end(); ++i) {
        if (i->GetKey().empty()) {
            throw TRejectException() << "empty key"sv;
        }

        if (i->GetVal() == 0.0) {
            continue;
        }

        const THashMap<TString, double>::const_iterator limit = NGlobal::KnownLimits.find(i->GetKey());

        if (limit != NGlobal::KnownLimits.end() && limit->second == 0.0) {
            throw TRejectException() << "known limit for \""sv << i->GetKey() << "\" is zero"sv;
        }

        double ratio = i->GetVal();

        if (ratio < 0.0) {
            if (limit == NGlobal::KnownLimits.end()) {
                throw TRejectException() << "no known limit for \""sv << i->GetKey() << '\"';
            }

            ratio = -ratio / limit->second;
        }

        value_type::second_type val = 0;

        if (ratio == 1.0) {
            val = Max<value_type::second_type>();
        } else if (ratio > 0.0 && ratio < 1.0) {
            val = ratio * Max<value_type::second_type>();
        } else {
            throw TRejectException() << "incorrect value of "sv << i->GetKey();
        }

        if ((operator[](NGlobal::KeyMapper[i->GetKey()]) += val) < val) {
            throw TRejectException() << "incorrect complex value of "sv << i->GetKey();
        }

        if (i->HasName() && (SharedParts[TSharedParts::value_type::first_type(NGlobal::KeyMapper[i->GetName()], NGlobal::KeyMapper[i->GetKey()])] += val) < val) {
            throw TRejectException() << "incorrect complex value of "sv << i->GetKey();
        }
    }
}
