#pragma once

#include <yql/library/purecalc/common/interface.h>
#include <yql/library/purecalc/purecalc.h>
#include <ydb/library/yql/public/udf/udf_value.h>
#include <ydb/library/yql/minikql/computation/mkql_computation_node_holders.h>

#include <util/generic/hash.h>
#include <util/generic/vector.h>


namespace NAntiRobot {


class TYqlInputSpec final: public NYql::NPureCalc::TInputSpecBase {
public:
    explicit TYqlInputSpec(const TVector<TString>& keys);

    const TVector<NYT::TNode>& GetSchemas() const override {
        return Schemas;
    }

    const THashMap<TString, size_t>& GetKeyToIndex() const {
        return KeyToIndex;
    }

private:
    THashMap<TString, size_t> KeyToIndex;
    TVector<NYT::TNode> Schemas;
};


class TYqlInputConsumer final:
    public NYql::NPureCalc::IConsumer<const TVector<float>*>
{
public:
    explicit TYqlInputConsumer(
        const THashMap<TString, size_t>& keyToIndex,
        THolder<NYql::NPureCalc::IPushStreamWorker> worker
    );

    ~TYqlInputConsumer() override;

    void OnObject(const TVector<float>* features) override;
    void OnFinish() override;

private:
    THolder<NYql::NPureCalc::IPushStreamWorker> Worker;
    NKikimr::NMiniKQL::TPlainContainerCache Cache;
    TVector<size_t> FeatureIndices;
};


class TRuleYqlOutputSpec final: public NYql::NPureCalc::TOutputSpecBase {
public:
    explicit TRuleYqlOutputSpec(size_t nResults);

    const NYT::TNode& GetSchema() const override {
        return Schema;
    }

private:
    NYT::TNode Schema;
};


class TRuleYqlOutputConsumer final:
    public NYql::NPureCalc::IConsumer<const NYql::NUdf::TUnboxedValue*>
{
public:
    explicit TRuleYqlOutputConsumer(
        NYql::NPureCalc::IPushStreamWorker* worker,
        THolder<NYql::NPureCalc::IConsumer<const TVector<ui32>*>> next
    );

    void OnObject(const NYql::NUdf::TUnboxedValue* value) override;
    void OnFinish() override;

private:
    NYql::NPureCalc::IWorker* Worker;
    THolder<NYql::NPureCalc::IConsumer<const TVector<ui32>*>> Next;
    TVector<ui32> RuleIndices;
    TVector<ui32> Result;
};


}


namespace NYql {
    namespace NPureCalc {
        template <>
        struct TInputSpecTraits<NAntiRobot::TYqlInputSpec> {
            static constexpr bool IsPartial = false;
            static constexpr bool SupportPushStreamMode = true;

            using TConsumerType = THolder<IConsumer<const TVector<float>*>>;

            static TConsumerType MakeConsumer(
                const NAntiRobot::TYqlInputSpec& spec,
                THolder<IPushStreamWorker> worker
            ) {
                return MakeHolder<NAntiRobot::TYqlInputConsumer>(
                    spec.GetKeyToIndex(),
                    std::move(worker)
                );
            }
        };


        template <>
        struct TOutputSpecTraits<NAntiRobot::TRuleYqlOutputSpec> {
            static constexpr bool IsPartial = false;
            static constexpr bool SupportPushStreamMode = true;

            static void SetConsumerToWorker(
                const NAntiRobot::TRuleYqlOutputSpec& spec,
                IPushStreamWorker* worker,
                THolder<IConsumer<const TVector<ui32>*>> consumer
            ) {
                Y_UNUSED(spec);

                worker->SetConsumer(
                    MakeHolder<NAntiRobot::TRuleYqlOutputConsumer>(
                        worker, std::move(consumer)
                    )
                );
            }
        };
    }
}
