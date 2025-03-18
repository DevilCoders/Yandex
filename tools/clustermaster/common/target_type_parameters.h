#pragma once

#include "id_for_string.h"
#include "param_list_manager.h"
#include "printer.h"

#include <util/generic/bt_exception.h>
#include <util/generic/hash.h>
#include <util/generic/vector.h>
#include <util/stream/output.h>

class TTargetTypeParameters;

class TLevelId {
public:
    typedef ui32 TLightId;
private:
    const TTargetTypeParameters* TargetType; // integrity checking
    TLightId Value;
public:
    TLevelId()
        : TargetType(nullptr)
        , Value(Max<TLightId>())
    {}
    TLevelId(const TTargetTypeParameters* targetType, TLightId value);
    TLightId GetValue() const { return Value; }

    TLevelId operator-(ui32 that) const;
    TLevelId& operator++();

    // not sure it is a good idea to expose it
    const TTargetTypeParameters* GetTargetType() const { return TargetType; }

    bool operator<(const TLevelId& that) const {
        return this->Value < that.Value;
    }

    bool operator==(const TLevelId& other) const {
        return Value == other.Value;
    }
};

struct TParameterPathsCounts {
    ui32 Total;
    TVector<ui32> WithFirstLevelFixed;
};

class TTargetTypeParameters {
public:
    static const TLevelId::TLightId HOST_LEVEL_ID = 1;
    static const TLevelId::TLightId FIRST_PARAM_LEVEL_ID = 2;

    typedef TVector<TString> TPath;


    class TId {
    private:
        const TTargetTypeParameters* Parameters;
        ui32 N;

    public:
        TId();
        TId(const TTargetTypeParameters* parameters, ui32 n);

        const TTargetTypeParameters* GetParameters() const { return Parameters; }
        ui32 GetN() const { return N; }
    };

    typedef TIdForString::TIdSafe TParamId;
    typedef TVector<TParamId> TIdPath;

private:
    struct TLevelParameter;
    struct TLevelParameters;

    static const TLevelParameter DummyEmptyParameter;

    struct TLevelParameters {
        // indexed by task name
        TCopyPtr<TVector<TLevelParameter> > Parameters; // Pointer here helps us to save memory. Most TLevelParameter instances
                // are on last level and thus has no next level (TLevelParameters NextLevel is emtpy). Because of pointer here it
                // takes only 8 bytes (not 24 needed to store empty vector).

        bool IsEmpty() const {
            return (Parameters.Get() == nullptr);
        }

        const TLevelParameter& GetForTask(TParamId taskId) const {
            return const_cast<TLevelParameters*>(this)->GetForTask(taskId);
        }

        TLevelParameter& GetForTask(TParamId taskId) {
            if (IsEmpty() || taskId.Id >= Parameters->size()) {
                // this is weird
                return const_cast<TLevelParameter&>(DummyEmptyParameter);
            } else {
                return Parameters->at(taskId.Id);
            }
        }

        TLevelParameter& GetOrCreateForTask(TParamId taskId) {
            if (IsEmpty()) {
                Parameters.Reset(new TVector<TLevelParameter>());
            }
            if (taskId.Id >= Parameters->size()) {
                Parameters->resize(taskId.Id + 1);
            }
            TLevelParameter& parameter = Parameters->at(taskId.Id);
            parameter.Exists = true;
            return parameter;
        }

        bool Exists(TParamId taskId) const {
            return !IsEmpty() && taskId.Id < Parameters->size() && Parameters->at(taskId.Id).Exists;
        }

        ui32 Count(ui32 depthLeft) const {
            if (IsEmpty()) {
                return 0;
            } else {
                ui32 r = 0;
                for (TVector<TLevelParameter>::const_iterator i = Parameters->begin(); i != Parameters->end(); ++i) {
                    r += i->Count(depthLeft);
                }
                return r;
            }
        }
    };

public:
    struct TLevelParameterStats { // Auxiliary class to calculate memory consumed by TLevelParameter instances.
        long LastLevel;
        long NotLast;
        long Capacity;

        TLevelParameterStats()
            : LastLevel(0)
            , NotLast(0)
            , Capacity(0)
        {
        }
    };

private:
    struct TLevelParameter {
        bool Exists;
        // value only for last level
        ui32 N;
        // empty (see TLevelParameters::IsEmpty()) if it is last level
        TLevelParameters NextLevel;

        TLevelParameter()
            : Exists(false)
            , N(Max<ui32>())
        {
        }

        ui32 Count(ui32 depthLeft) const {
            if (!Exists) {
                return 0;
            }
            if (depthLeft == 0) {
                Y_ASSERT(NextLevel.IsEmpty());
                return 1;
            }
            return NextLevel.Count(depthLeft - 1);
        }

        void CalcStats(TLevelParameterStats* stats) const {
            if (NextLevel.IsEmpty()) {
                stats->LastLevel++;
            } else {
                for (TVector<TLevelParameter>::const_iterator i = NextLevel.Parameters->begin();
                        i != NextLevel.Parameters->end(); ++i)
                {
                    i->CalcStats(stats);
                }
                stats->NotLast++;
                stats->Capacity += NextLevel.Parameters->capacity();
            }
        }
    };

    TLevelParameter* GetParameterForPartialPath(const TIdPath& path);
    const TLevelParameter* GetParameterForPartialPath(const TIdPath& path) const;
    TLevelParameter* GetParameterForPath(const TIdPath& path);
    const TLevelParameter* GetParameterForPath(const TIdPath& path) const;

    bool PartialPathExists(const TIdPath& path) const {
        return GetParameterForPartialPath(path)->Exists;
    }

    void CheckComplete() const;

    void CheckIncomplete() const;

    ui32 Count() const;
    ui32 CountFirstLevelFixed(TParamId firstLevelParamId) const;

    const TString TargetName;

    TParamListManager const* ParamListManager;

    TLevelParameter LevelParameter;

    TVector<TIdPath> IndexByN;

    const TVector<TParamListManager::TListReference> Levels;

    TParameterPathsCounts PathsCounts;

    bool Complete;

    bool DontFillIndexByN;

public:
    TTargetTypeParameters(
            const TString& targetName,
            TParamListManager* paramListManager,
            const TVector<TParamListManager::TListReference>& paramNames);

    TTargetTypeParameters(
            const TString& targetName,
            TParamListManager* paramListManager,
            ui32 depth,
            const TVector<TVector<TString> >& paramss,
            bool DontFillIndexByN = false);

    void CompleteBuild();

    const TParamListManager* GetParamListManager() const {
        return ParamListManager;
    }

    TLevelId GetLevelId(TLevelId::TLightId lightId) const;

    ui32 GetDepth() const { return Levels.size(); }

    bool IsEqualTo(const TTargetTypeParameters* that) const;

    TLevelId GetMinLevelId() const {
        return GetLevelId(0);
    }

    TLevelId GetMaxLevelId() const {
        return GetLevelId(GetDepth());
    }

    bool PathExists(const TIdPath& path) const;

    bool PathExists(const TPath& path) const {
        TIdPath idPath;
        for (size_t i = 0; i < path.size(); ++i) {
            TMaybe<TParamId> id = GetNameListAtLevel(i + 1).FindIdByName(path.at(i));
            if (!id) {
                return false;
            }
            idPath.push_back(*id);
        }
        return PathExists(idPath);
    }

    TPath ResolvePath(const TIdPath& path) const;

    TIdPath ResolvePath(const TPath& path) const;

    ui32 GetCount() const;
    ui32 GetCountFirstLevelFixed(TParamId firstLevelParamId) const;

    bool Empty() const {
        return GetCount() == 0;
    }

    ui32 CheckId(const TId& id) const {
        Y_VERIFY(this == id.GetParameters(), "invalid parameters reference");
        Y_VERIFY(id.GetN() < GetCount(), "too large n: %d; count: %d", int(id.GetN()), int(GetCount()));
        return id.GetN();
    }

    TId GetId(ui32 n) const {
        return TId(this, n);
    }

    ui32 GetParamCountAtLevel(TLevelId level) const {
        if (level.GetValue() == 0) {
            return 1;
        } else {
            return GetParamNamesAtLevel(level).size();
        }
    }

    ui32 GetParamCountAtLevel(TLevelId::TLightId level) const {
        return GetParamCountAtLevel(GetLevelId(level));
    }

    TId GetNForPath(const TIdPath& path) const;

    // TODO: inline
    TId GetNForPath(const TVector<ui32>& path) const {
        TIdPath properPath;
        for (ui32 i = 0; i < path.size(); ++i) {
            properPath.push_back(TParamId(GetListReferenceAtLevel(i + 1).N, path.at(i)));
        }
        return GetNForPath(properPath);
    }

    TId GetNForPath(const TPath& path) const {
        return GetNForPath(ResolvePath(path));
    }

    const TIdPath& GetIdPathForN(const TId& n) const {
        Y_VERIFY(!DontFillIndexByN, "GetIdPathForN was called while DontFillIndexByN == true");
        return IndexByN.at(CheckId(n));
    }

    TPath GetPathForN(const TId& n) const {
        return ResolvePath(GetIdPathForN(n));
    }

    TPath GetPathForN(ui32 n) const {
        return GetPathForN(GetId(n));
    }

    TVector<TString> GetScriptArgsOnWorkerByN(const TId& n) const;

    const TParamListManager::TListReference& GetListReferenceAtLevel(TLevelId levelId) const {
        if (levelId.GetTargetType() != this) {
            ythrow TWithBackTrace<yexception>() << "levelId belongs to different target type parameters";
        }
        return Levels.at(levelId.GetValue() - 1);
    }

    const TParamListManager::TListReference& GetListReferenceAtLevel(TLevelId::TLightId levelId) const {
        return GetListReferenceAtLevel(GetLevelId(levelId));
    }

    const TIdForString& GetNameListAtLevel(TLevelId levelId) const {
        return ParamListManager->GetList(GetListReferenceAtLevel(levelId));
    }

    const TIdForString& GetNameListAtLevel(TLevelId::TLightId lightId) const {
        return GetNameListAtLevel(GetLevelId(lightId));
    }

    const TVector<TString>& GetParamNamesAtLevel(TLevelId level) const {
        return GetNameListAtLevel(level).GetNames();
    }

    const TVector<TString>& GetParamNamesAtLevel(TLevelId::TLightId level) const {
        return GetParamNamesAtLevel(GetLevelId(level));
    }

    const TString& GetParamNameAtLevel(TLevelId level, const TParamId taskId) const {
        return GetNameListAtLevel(level).GetNameById(taskId);
    }

    // deprecated
    const TString& GetParamNameAtLevel(ui32 level, ui32 taskId) const {
        return GetNameListAtLevel(level).GetNameById(taskId);
    }

    TParamId GetParamIdAtLevel(TLevelId level, const TString& paramName) const {
        return GetNameListAtLevel(level).GetIdByName(paramName);
    }

    TParamId GetParamIdAtLevel(TLevelId::TLightId level, const TString& paramName) const {
        return GetParamIdAtLevel(GetLevelId(level), paramName);
    }

    void AddTask(const TIdPath& path);
    void AddTask(const TPath& path);

    class TLevelEnumerator {
        friend class TTargetTypeParameters;
    private:
        const TTargetTypeParameters& Parent;
        TMaybe<TLevelId> CurrentLevel;
        TLevelEnumerator(const TTargetTypeParameters& parent);
    public:
        bool Next();
        const TLevelId& operator*() const { return *CurrentLevel; }
        const TLevelId* operator->() const { return &**this; }
    };

    TLevelEnumerator LevelEnumerator() const { return TLevelEnumerator(*this); }

    TLevelEnumerator LevelEnumeratorSkipGround() const {
        TLevelEnumerator r = LevelEnumerator();
        r.Next();
        return r;
    }

    class TProjection {
        friend class TIterator;
    private:
        TVector<TMaybe<TParamId> > PathFilter;
    public:
        TProjection(const TVector<TMaybe<TParamId> >&);
        static TProjection True(ui32 depth);
        static TProjection SingleParam(TLevelId levelId, TParamId value, ui32 depth);

        const TVector<TMaybe<TParamId> >& GetBindings() const {
            return PathFilter;
        }

        TMaybe<TParamId> GetBindingAtLevel(TLevelId levelId) const {
            return GetBindings().at(levelId.GetValue() - 1);
        }

        const TVector<TMaybe<TParamId> > GetPathFilter() const { return PathFilter; }
        ui32 GetDepth() const { return PathFilter.size(); }

        bool operator==(const TProjection& that) const {
            return PathFilter == that.PathFilter;
        }

        bool operator!=(const TProjection& that) const {
            return !(*this == that);
        }
    };

    TProjection TrueProjection() const {
        return TProjection::True(GetDepth());
    }

    TProjection SingleParamProjection(TLevelId levelId, const TParamId& value) const {
        return TProjection::SingleParam(levelId, value, GetDepth());
    }

    TProjection MasterHostProjection(const TParamId& value) const {
        return SingleParamProjection(GetLevelId(HOST_LEVEL_ID), value);
    }

    TProjection MasterFirstParamProjection(const TParamId& value) const {
        return SingleParamProjection(GetLevelId(FIRST_PARAM_LEVEL_ID), value);
    }

    TProjection PathProjection(const TIdPath& path) const {
        if (path.size() != GetDepth()) {
            ythrow yexception() << "incorrect depth";
        }
        TVector<TMaybe<TParamId> > filter;
        for (TIdPath::const_iterator item = path.begin(); item != path.end(); ++item) {
            filter.push_back(*item);
        }
        return TProjection(filter);
    }

    TProjection NProjection(const TId& n) const {
        return PathProjection(GetIdPathForN(n));
    }

    TProjection FirstTwoProjection(const TParamId& value1, const TParamId& value2) const {
        TVector<TMaybe<TParamId> > proj;
        proj.resize(GetDepth());
        proj.at(0) = value1;
        proj.at(1) = value2;
        return TProjection(proj);
    }

    class TIterator {
    public:
    private:
        const TTargetTypeParameters& Parent;

        TVector<TParamId> Path;
        TProjection PathFilter;

        enum EState {
            S_BEGIN,
            S_RUNNING,
            S_EOF
        };

        EState State;
    public:
        TIterator(const TTargetTypeParameters& parent, const TProjection& pathFilter);

        void DumpState(IOutputStream& out = Cerr) const;
    private:
        void IncAtLevel(TLevelId level);
        void Inc();

    public:
        bool Next();

        void Skip(ui32 n);

        const TIdPath& operator*() const {
            if (State != S_RUNNING)
                ythrow TWithBackTrace<yexception>() << "EOF";
            return Path;
        }

        const TIdPath* operator->() const {
            return &**this;
        }

        const TPath CurrentPath() const {
            TPath r;
            for (TLevelId::TLightId levelId = 1; levelId <= Parent.GetMaxLevelId().GetValue(); ++levelId) {
                r.push_back(Parent.GetParamNameAtLevel(levelId, (**this).at(levelId - 1).Id));
            }
            return r;
        }

        TId CurrentN() const {
            return Parent.GetNForPath(**this);
        }

    };

    TIterator Iterator(const TProjection& filter) const {
        return TIterator(*this, filter);
    }

    TIterator Iterator() const {
        return Iterator(TProjection::True(GetDepth()));
    }

    TIterator IteratorParamFixed(TLevelId levelId, TParamId value) const {
        return Iterator(TProjection::SingleParam(levelId, value, GetDepth()));
    }

    TIterator IteratorParamFixed(ui32 levelId, TParamId value) const {
        return IteratorParamFixed(GetLevelId(levelId), value);
    }

    TIterator IteratorParamFixed(TLevelId levelId, ui32 value) const {
        // TODO: inline
        return IteratorParamFixed(levelId, TParamId(GetListReferenceAtLevel(levelId).N, value));
    }

    TIterator IteratorParamFixed(TLevelId levelId, const TString& value) const {
        TParamId paramId = GetParamIdAtLevel(levelId, value);
        return IteratorParamFixed(levelId, paramId);
    }

    TIterator IteratorParamFixed(TLevelId::TLightId levelId, const TString& value) const {
        return IteratorParamFixed(GetLevelId(levelId), value);
    }

    void DumpState() const;
    void DumpState(TPrinter& printer) const;

    void CheckState() const;

};


template <>
struct THash<TTargetTypeParameters::TProjection> {

    size_t operator()(const TTargetTypeParameters::TProjection& v) const {
        return TSimpleRangeHash()(v.GetBindings());
    }

};

