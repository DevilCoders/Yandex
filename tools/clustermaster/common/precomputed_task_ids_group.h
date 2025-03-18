#pragma once

#include "precomputed_task_ids.h"

template <typename TTargetType>
class TDependSourceTypeTargetType {
public:
    TDependSourceTypeTargetType(TTargetType* sourceType, TTargetType* targetType)
        : SourceType(sourceType)
        , TargetType(targetType)
    {
    }

    const TTargetType* GetSourceType() const {
        return SourceType;
    }

    const TTargetType* GetTargetType() const {
        return TargetType;
    }

private:
    TTargetType* SourceType;
    TTargetType* TargetType;
};

template <typename TTargetType>
bool operator==(const TDependSourceTypeTargetType<TTargetType>& a, const TDependSourceTypeTargetType<TTargetType>& b) {
    return a.GetSourceType() == b.GetSourceType() && a.GetTargetType() == b.GetTargetType();
}

template <typename TTargetType>
struct THash<TDependSourceTypeTargetType<TTargetType> > {
    size_t operator()(const TDependSourceTypeTargetType<TTargetType>& v) const {
        return CombineHashes(THash<TString>()(v.GetSourceType()->GetName()), THash<TString>()(v.GetTargetType()->GetName()));
    }
};

template <typename PTypes, typename PParamsByType>
class TPrecomputedTaskIdsInitializerGroup: private TNonCopyable, public IPrecomputedTaskIdsInitializer {
    typedef typename PTypes::TTarget TTarget;
    typedef typename PTypes::TTargetType TTargetType;
    typedef typename PTypes::TGraph TGraph;

private:
    void IndexTasksByGroups(const TTargetType& type, const TGraph* graph,
        THashMap<int, TPrecomputedTaskIdsForOneSide::TTaskIdsList>* tasksByGroup)
    {
        const TTargetTypeParameters& p = PParamsByType::GetParams(type);

        TIdForString workers = p.GetNameListAtLevel(TTargetTypeParameters::HOST_LEVEL_ID);

        for (TIdForString::TIterator w = workers.Iterator(); w.Next(); ) {
            int group = graph->GetListManager()->GetGroupId(w.GetName());

            TTargetTypeParameters::TProjection proj =
                    TTargetTypeParameters::TProjection::SingleParam(
                            p.GetLevelId(TTargetTypeParameters::HOST_LEVEL_ID),
                            *w,
                            p.GetDepth()
                    );

            TPrecomputedTaskIdsForOneSide::TTaskIdsList& tasks = (*tasksByGroup)[group];

            for (TTargetTypeParameters::TIterator i(p, proj); i.Next(); )
                tasks.push_back(i.CurrentN().GetN());
        }
    }

public:
    TPrecomputedTaskIdsInitializerGroup(const TDependSourceTypeTargetType<TTargetType>& st)
        : Dsttt(st)
        , Ids()
    {
    }

    void Initialize() override /*override*/ {
        if (Ids.Get() != nullptr) { // already initialized
            return;
        }

        Ids.Reset(new TPrecomputedTasksIds());

        Y_VERIFY(Dsttt.GetSourceType()->Graph == Dsttt.GetTargetType()->Graph,
               "TMasterSourceTarget: source and target belong to different graphs"); // Just in case. Isn't possible at all as far as I know

        const TGraph* graph = Dsttt.GetSourceType()->Graph;

        THashMap<int, TPrecomputedTaskIdsForOneSide::TTaskIdsList> depTasksByGroup;
        THashMap<int, TPrecomputedTaskIdsForOneSide::TTaskIdsList> myTasksByGroup;

        IndexTasksByGroups(*Dsttt.GetTargetType(), graph, &depTasksByGroup);
        IndexTasksByGroups(*Dsttt.GetSourceType(), graph, &myTasksByGroup);

        TSet<int> groups;
        for (THashMap<int, TPrecomputedTaskIdsForOneSide::TTaskIdsList>::const_iterator i = depTasksByGroup.begin(); i != depTasksByGroup.end(); ++i) {
            groups.insert(i->first);
        }
        for (THashMap<int, TPrecomputedTaskIdsForOneSide::TTaskIdsList>::const_iterator i = myTasksByGroup.begin(); i != myTasksByGroup.end(); ++i) {
            groups.insert(i->first);
        }

        int N = 0;

        for (TSet<int>::const_iterator i = groups.begin(); i != groups.end(); ++i) {
            int group = *i;

            if (depTasksByGroup.find(group) != depTasksByGroup.end() && myTasksByGroup.find(group) != myTasksByGroup.end()) {
                Ids->EnumeratorRealFakeStates.RegisterStateReal(N);
                Ids->Dep.AddTaskIdsList(depTasksByGroup[group]);
                Ids->My.AddTaskIdsList(myTasksByGroup[group]);
            } else {
                Ids->EnumeratorRealFakeStates.RegisterStateFake(N);
            }
            N++;
        }

        Ids->Dep.ShrinkToFit();
        Ids->My.ShrinkToFit();
    }

    const TPrecomputedTasksIds& GetIds() /*override*/ const override {
        Y_VERIFY(Ids.Get() != nullptr, "Not initialized");
        return *Ids;
    }

private:
    const TDependSourceTypeTargetType<TTargetType> Dsttt;
    THolder<TPrecomputedTasksIds> Ids;
};

template <typename PTypes, typename PParamsByType>
class TPrecomputedTaskIdsContainerGroup {
    typedef typename PTypes::TTarget TTarget;
    typedef typename PTypes::TTargetType TTargetType;
    typedef TPrecomputedTaskIdsInitializerGroup<PTypes, PParamsByType> TInitializer;

public:
    TSimpleSharedPtr<TInitializer> GetOrCreate(const TDependSourceTypeTargetType<TTargetType>& dsttt) {
        TSimpleSharedPtr<TInitializer>& ref = Map[dsttt];
        if (ref.Get() == nullptr) {
            ref = TSimpleSharedPtr<TInitializer> (new TInitializer(dsttt));
        }
        return ref;
    }

private:
    THashMap<TDependSourceTypeTargetType<TTargetType>, TSimpleSharedPtr<TInitializer> > Map;
};
