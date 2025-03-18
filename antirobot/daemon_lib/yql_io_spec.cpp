#include "yql_io_spec.h"

#include <util/generic/algorithm.h>
#include <util/system/guard.h>


namespace NAntiRobot {


namespace {
    NYT::TNode BuildSpecFromKeys(
        const TVector<TString>& keys,
        THashMap<TString, size_t>& keyToIndex
    ) {
        auto structType = NYT::TNode::CreateList();
        const auto floatType = NYT::TNode::CreateList({"DataType", "Float"});

        for (size_t i = 0; i < keys.size(); ++i) {
            keyToIndex[keys[i]] = i;
            structType.Add(NYT::TNode::CreateList({keys[i], floatType}));
        }

        return NYT::TNode::CreateList({"StructType", structType});
    }


    TVector<size_t> GetFeatureIndices(
        const THashMap<TString, size_t>& keyToIndex,
        const NKikimr::NMiniKQL::TStructType* type
    ) {
        const ui32 nMembers = type->GetMembersCount();

        TVector<size_t> indices;
        indices.reserve(nMembers);

        for (ui32 i = 0; i < nMembers; ++i) {
            const auto name = type->GetMemberName(i);
            indices.push_back(keyToIndex.at(name));
        }

        return indices;
    }
}


TYqlInputSpec::TYqlInputSpec(const TVector<TString>& keys) {
    Schemas = {BuildSpecFromKeys(keys, KeyToIndex)};
}


TYqlInputConsumer::TYqlInputConsumer(
    const THashMap<TString, size_t>& keyToIndex,
    THolder<NYql::NPureCalc::IPushStreamWorker> worker
)
    : Worker(std::move(worker))
{
    FeatureIndices = GetFeatureIndices(keyToIndex, Worker->GetInputType());
}

TYqlInputConsumer::~TYqlInputConsumer() {
    with_lock (Worker->GetScopedAlloc()) {
        Cache.Clear();
    }
}

void TYqlInputConsumer::OnObject(const TVector<float>* features) {
    NKikimr::NMiniKQL::TBindTerminator bind(Worker->GetGraph().GetTerminator());

    with_lock (Worker->GetScopedAlloc()) {
        auto& holderFactory = Worker->GetGraph().GetHolderFactory();
        NYql::NUdf::TUnboxedValue* items = nullptr;

        NYql::NUdf::TUnboxedValue result = Cache.NewArray(
            holderFactory,
            static_cast<ui32>(FeatureIndices.size()),
            items
        );

        for (ui32 i = 0; i < FeatureIndices.size(); ++i) {
            items[i] = NYql::NUdf::TUnboxedValuePod((*features)[FeatureIndices[i]]);
        }

        Worker->Push(std::move(result));
    }
}

void TYqlInputConsumer::OnFinish() {
    NKikimr::NMiniKQL::TBindTerminator bind(Worker->GetGraph().GetTerminator());

    with_lock (Worker->GetScopedAlloc()) {
        Worker->OnFinish();
    }
}


TRuleYqlOutputSpec::TRuleYqlOutputSpec(size_t nResults) {
    auto structType = NYT::TNode::CreateList();
    const auto boolType = NYT::TNode::CreateList({"DataType", "Bool"});

    for (size_t i = 0; i < nResults; ++i) {
        structType.Add(NYT::TNode::CreateList({"Y" + ToString(i), boolType}));
    }

    Schema = NYT::TNode::CreateList({"StructType", structType});
}


TRuleYqlOutputConsumer::TRuleYqlOutputConsumer(
    NYql::NPureCalc::IPushStreamWorker* worker,
    THolder<NYql::NPureCalc::IConsumer<const TVector<ui32>*>> next
)
    : Worker(worker)
    , Next(std::move(next))
{
    const auto abstractType = worker->GetOutputType();
    Y_ENSURE(abstractType->IsStruct());

    const auto structType =
        static_cast<const NKikimr::NMiniKQL::TStructType*>(abstractType);

    const auto nMembers = structType->GetMembersCount();

    for (ui32 i = 0; i < nMembers; ++i) {
        const auto name = structType->GetMemberName(i);
        const auto index = FromString<ui32>(name.substr(1));
        RuleIndices.push_back(index);
    }
}

void TRuleYqlOutputConsumer::OnObject(const NYql::NUdf::TUnboxedValue* value) {
    auto unguard = Unguard(Worker->GetScopedAlloc());

    Result.clear();

    for (ui64 i = 0; i < value->GetListLength(); ++i) {
        if (value->GetElement(i).Get<bool>()) {
            Result.push_back(RuleIndices[i]);
        }
    }

    Next->OnObject(&Result);
}

void TRuleYqlOutputConsumer::OnFinish() {
    auto unguard = Unguard(Worker->GetScopedAlloc());
    Next->OnFinish();
}


}
