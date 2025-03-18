#pragma once

#include "param_mapping.h"
#include "precomputed_task_ids_one_side.h"

#include <util/generic/singleton.h>

class TDependEdgesEnumeratorRealFakeStates {
    class TStateCollection {
    private:
        TVector<ui32> States;

    public:
        void Append(ui32 value);
        bool Has(ui32 value) const;
    };

private:
    bool AllStatesAreReal;
    TStateCollection RealStates;
    size_t StatesCount;

public:
    TDependEdgesEnumeratorRealFakeStates();

    bool IsStateReal(ui32 state) const;
    bool IsStateFake(ui32 state) const;

    void RegisterStateReal(ui32 state);
    void RegisterStateFake(ui32 state);

    size_t GetStatesCount() const;
};

class TPrecomputedTasksIds : private TNonCopyable {
    template <typename PTypes, typename PParamsByType>
    friend class TPrecomputedTaskIdsInitializer;

    template <typename PTypes, typename PParamsByType>
    friend class TPrecomputedTaskIdsInitializerGroup;

public:
    const TPrecomputedTaskIdsForOneSide* GetDepTaskIds() const {
        return &Dep;
    }

    const TPrecomputedTaskIdsForOneSide* GetMyTaskIds() const {
        return &My;
    }

    const TDependEdgesEnumeratorRealFakeStates* GetEnumeratorRealFakeStates() const {
        return &EnumeratorRealFakeStates;
    }

    TDependEdgesEnumeratorRealFakeStates* GetEnumeratorRealFakeStates() {
        return &EnumeratorRealFakeStates;
    }

private:
    TPrecomputedTaskIdsForOneSide Dep;
    TPrecomputedTaskIdsForOneSide My;
    TDependEdgesEnumeratorRealFakeStates EnumeratorRealFakeStates;
};

/* I'm not sure about this. Rationale behind this is to save allocations and to protect us from memory defragmentation. */
struct TPrecomputedTaskIdsInitializerBuffers {
    TPrecomputedTaskIdsInitializerBuffers() {
        Dep.reserve(1024 * 1024);
        My.reserve(1024 * 1024);
    }

    TPrecomputedTaskIdsForOneSide::TTaskIdsList Dep;
    TPrecomputedTaskIdsForOneSide::TTaskIdsList My;
};

template <typename TTargetType>
class TDependSourceTypeTargetTypeAndMappings {
public:
    TDependSourceTypeTargetTypeAndMappings(const TParamMappings& joinParamMappings, TTargetType* sourceType,
            TTargetType* targetType)
        : JoinParamMappings(joinParamMappings)
        , SourceType(sourceType)
        , TargetType(targetType)
    {
    }

    TDependSourceTypeTargetTypeAndMappings(const TDependSourceTypeTargetTypeAndMappings& dstttm)
        : JoinParamMappings(dstttm.JoinParamMappings)
        , SourceType(dstttm.SourceType)
        , TargetType(dstttm.TargetType)
    {
    }

    const TParamMappings& GetJoinParamMappings() const {
        return JoinParamMappings;
    }

    const TTargetType* GetSourceType() const {
        return SourceType;
    }

    const TTargetType* GetTargetType() const {
        return TargetType;
    }

    bool operator==(const TDependSourceTypeTargetTypeAndMappings& other) const {
        return GetJoinParamMappings() == other.GetJoinParamMappings() &&
                GetSourceType()->GetName() == other.GetSourceType()->GetName() &&
                GetTargetType()->GetName() == other.GetTargetType()->GetName();
    }

private:
    TParamMappings JoinParamMappings;
    TTargetType* SourceType;
    TTargetType* TargetType;
};

class IPrecomputedTaskIdsInitializer {
public:
    virtual void Initialize() = 0;
    virtual const TPrecomputedTasksIds& GetIds() const = 0;
    virtual ~IPrecomputedTaskIdsInitializer() {}
};

template <typename PTypes, typename PParamsByType>
class TPrecomputedTaskIdsInitializer : private TNonCopyable, public IPrecomputedTaskIdsInitializer {
private:
    typedef typename PTypes::TTarget TTarget;
    typedef typename PTypes::TTargetType TTargetType;

public:
    TPrecomputedTaskIdsInitializer(const TDependSourceTypeTargetTypeAndMappings<TTargetType>& dstttm)
        : Dstttm(dstttm)
        , Ids()
    {
    }

    void Initialize() override /*override*/ {
        if (Ids.Get() != nullptr) { // already initialized
            return;
        }

        Ids.Reset(new TPrecomputedTasksIds());

        int N = 0;

        TPrecomputedTaskIdsForOneSide::TTaskIdsList& depTaskBuffer = Singleton<TPrecomputedTaskIdsInitializerBuffers>()->Dep;
        TPrecomputedTaskIdsForOneSide::TTaskIdsList& myTaskBuffer = Singleton<TPrecomputedTaskIdsInitializerBuffers>()->My;

        for (TJoinEnumerator en(&PParamsByType::GetParams(*Dstttm.GetSourceType()), &PParamsByType::GetParams(*Dstttm.GetTargetType()), Dstttm.GetJoinParamMappings()); en.Next(); N++) {
            depTaskBuffer.clear();
            myTaskBuffer.clear();

            for (TTargetTypeParameters::TIterator depTask = PParamsByType::GetParams(*Dstttm.GetTargetType()).Iterator(en.GetDepProjection()); depTask.Next(); )
                depTaskBuffer.push_back(depTask.CurrentN().GetN());
            for (TTargetTypeParameters::TIterator myTask = PParamsByType::GetParams(*Dstttm.GetSourceType()).Iterator(en.GetMyProjection()); myTask.Next(); )
                myTaskBuffer.push_back(myTask.CurrentN().GetN());

            if (depTaskBuffer.size() == 0 || myTaskBuffer.size() == 0) {
                Ids->EnumeratorRealFakeStates.RegisterStateFake(N);
                continue;
            }

            Ids->EnumeratorRealFakeStates.RegisterStateReal(N);

            if (depTaskBuffer.size() == 1)
                Ids->Dep.AddSingleTaskId(depTaskBuffer[0]);
            else {
                Ids->Dep.AddTaskIdsList(depTaskBuffer);
            }

            if (myTaskBuffer.size() == 1)
                Ids->My.AddSingleTaskId(myTaskBuffer[0]);
            else {
                Ids->My.AddTaskIdsList(myTaskBuffer);
            }
        }

        Ids->Dep.ShrinkToFit();
        Ids->My.ShrinkToFit();
    }

    const TPrecomputedTasksIds& GetIds() /*override*/ const override {
        Y_VERIFY(Ids.Get() != nullptr, "Not initialized");
        return *Ids;
    }

private:
    const TDependSourceTypeTargetTypeAndMappings<TTargetType> Dstttm;

    THolder<TPrecomputedTasksIds> Ids;
};

template <typename PTypes>
class TPrecomputedTaskIdsMaybe : private TNonCopyable {
public:
    static TPrecomputedTaskIdsMaybe* Empty() {
        return new TPrecomputedTaskIdsMaybe();
    }

    static TPrecomputedTaskIdsMaybe* Defined(TSimpleSharedPtr<IPrecomputedTaskIdsInitializer> initializer)
    {
        return new TPrecomputedTaskIdsMaybe(initializer);
    }

    IPrecomputedTaskIdsInitializer* Get() {
        Y_VERIFY(Maybe.Defined());
        return Maybe.GetRef().Get();
    }

private:
    TPrecomputedTaskIdsMaybe(TSimpleSharedPtr<IPrecomputedTaskIdsInitializer> initializer)
        : Maybe(initializer)
    {
    }

    TPrecomputedTaskIdsMaybe()
        : Maybe()
    {
    }

    TMaybe<TSimpleSharedPtr<IPrecomputedTaskIdsInitializer> > Maybe;
};

template<>
struct THash<TParamMapping> {

    size_t operator()(const TParamMapping& paramMapping) {
        return CombineHashes(NumericHash(paramMapping.GetMyLevelId().GetValue()),
                NumericHash(paramMapping.GetDepLevelId().GetValue()));
    }
};

template<typename PTypes>
struct THash<TDependSourceTypeTargetTypeAndMappings<PTypes> > {

    size_t operator()(const TDependSourceTypeTargetTypeAndMappings<PTypes>& v) const {
        size_t a = CombineHashes(THash<TString>()(v.GetSourceType()->GetName()),
                THash<TString>()(v.GetTargetType()->GetName()));
        return CombineHashes(a, TSimpleRangeHash()(v.GetJoinParamMappings().GetMappings()));
    }
};

template <typename PTypes, typename PParamsByType>
class TPrecomputedTaskIdsContainer {
private:
    typedef typename PTypes::TTarget TTarget;
    typedef typename PTypes::TTargetType TTargetType;
    typedef TPrecomputedTaskIdsInitializer<PTypes, PParamsByType> TInitializer;

    THashMap<TDependSourceTypeTargetTypeAndMappings<TTargetType>, TSimpleSharedPtr<TInitializer> > Map;

public:
    TSimpleSharedPtr<TInitializer> GetOrCreate(TDependSourceTypeTargetTypeAndMappings<TTargetType> dstttm) {
        TSimpleSharedPtr<TInitializer>& ref = Map[dstttm];
        if (ref.Get() == nullptr) {
            ref = TSimpleSharedPtr<TInitializer>(new TInitializer(dstttm));
        }
        return ref;
    }
};

/**
 * See TParamsByTypeHyperspace for explanation why we need PParamsByType template argument (this
 * struct is one of two 'implementations of PParamsByType).
 */
struct TParamsByTypeOrdinary {
    template <class TTargetType>
    static const TTargetTypeParameters& GetParams(const TTargetType& type) {
        return type.GetParameters();
    }

    template <typename PTypes>
    static TPrecomputedTaskIdsMaybe<PTypes>& GetPrecomputedTaskIdsMaybe(const typename PTypes::TDepend& dep) {
        return *dep.GetPrecomputedTaskIdsMaybe();
    }
};

class TDependEdgesEnumerator {
private:
    const TPrecomputedTasksIds& PrecomputedTaskIds;
    ui32 NAnyState; // Including fake
    ui32 N;

public:
    TDependEdgesEnumerator(const TPrecomputedTasksIds& precomputedTaskIds);

    ui32 GetNAnyState() const;
    ui32 GetN() const;
    bool Next();

    TTaskIdsForEnumeratorState GetDepTaskIds() const;
    TTaskIdsForEnumeratorState GetMyTaskIds() const;
};
