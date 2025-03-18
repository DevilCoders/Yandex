#include "target_type_parameters.h"

#include "make_vector.h"
#include "maybe_to_string.h"
#include "param_sort.h"
#include "vector_to_string.h"
#include "vector_to_string.h"
#include "vector_util.h"

#include <util/generic/bt_exception.h>

TLevelId::TLevelId(const TTargetTypeParameters* targetType, TLightId value)
    : TargetType(targetType)
    , Value(value)
{
    if (value > targetType->GetDepth()) {
        ythrow TWithBackTrace<yexception>() << "too large level id: " << value
                << ", max valid: " << targetType->GetDepth();
    }
}


TLevelId TLevelId::operator-(ui32 that) const {
    if (that > Value)
        ythrow yexception() << "too much to subtract";
    return TLevelId(TargetType, Value - that);
}

TLevelId& TLevelId::operator++() {
    if (Value == TargetType->GetDepth())
        ythrow yexception() << "too much to increment";
    ++Value;
    return *this;
}

TTargetTypeParameters::TTargetTypeParameters(
        const TString& targetName,
        TParamListManager* paramListManager,
        const TVector<TParamListManager::TListReference>& paramNames)
    : TargetName(targetName)
    , ParamListManager(paramListManager)
    , Levels(paramNames)
    , Complete(false)
    , DontFillIndexByN(false)
{
}

namespace {
    TVector<TParamListManager::TListReference> ComputeLevels(
            TParamListManager* paramListManager,
            ui32 depth,
            const TVector<TVector<TString> >& paramss)
    {
        TVector<TParamListManager::TListReference> r;
        for (ui32 i = 0; i < depth; ++i) {
            TVector<TString> ithParams;
            for (TVector<TVector<TString> >::const_iterator params = paramss.begin();
                    params != paramss.end(); ++params)
            {
                ithParams.push_back(params->at(i));
            }

            TVector<TString> unique = StableUnique(ithParams);

            std::sort(unique.begin(), unique.end(), TNiceLess<TString>());

            r.push_back(paramListManager->GetOrCreateList(unique));
        }
        return r;
    }
}

TTargetTypeParameters::TTargetTypeParameters(
        const TString& targetName,
        TParamListManager* paramListManager,
        ui32 depth,
        const TVector<TVector<TString> >& paramss,
        bool DontFillIndexByN)
    : TargetName(targetName)
    , ParamListManager(paramListManager)
    , Levels(ComputeLevels(paramListManager, depth, paramss))
    , Complete(false)
    , DontFillIndexByN(DontFillIndexByN)
{
    for (TVector<TVector<TString> >::const_iterator params = paramss.begin();
            params != paramss.end(); ++params)
    {
        AddTask(*params);
    }

    CompleteBuild();
}


void TTargetTypeParameters::CompleteBuild() {
    CheckIncomplete();

    TIterator it = Iterator();
    ui32 n = 0;
    while (it.Next()) {
        TIdPath path = *it;
        TLevelParameter* levelParameter = GetParameterForPath(path);
        Y_VERIFY(levelParameter->Exists, "oops");
        levelParameter->N = n;
        n++;
        if (!DontFillIndexByN) {
            IndexByN.push_back(path);
        }
    }

    PathsCounts.Total = Count();
    Y_VERIFY(n == PathsCounts.Total, "iterator based and direct counting gives different count");

    if (GetDepth() > 0) {
        const TIdForString& firstLevelList = GetNameListAtLevel(1);
        if (firstLevelList.Size() > 0) {
            PathsCounts.WithFirstLevelFixed.resize(firstLevelList.Size());
            for (TIdForString::TIterator i = firstLevelList.Iterator(); i.Next(); ) {
                PathsCounts.WithFirstLevelFixed.at(i->Id) = CountFirstLevelFixed(*i);
            }
        }
    }

    Complete = true;

    CheckState();
}

void TTargetTypeParameters::CheckComplete() const {
    Y_VERIFY(Complete, "Must be complete");
}

void TTargetTypeParameters::CheckIncomplete() const {
    Y_VERIFY(!Complete, "Must be incomplete");
}

ui32 TTargetTypeParameters::Count() const {
    return LevelParameter.Count(GetDepth());
}

ui32 TTargetTypeParameters::CountFirstLevelFixed(TParamId firstLevelParamId) const {
    if (GetDepth() == 0) {
        return 0;
    }
    TIdPath path;
    path.push_back(firstLevelParamId);
    return GetParameterForPartialPath(path)->Count(GetDepth() - 1);
}


TLevelId TTargetTypeParameters::GetLevelId(TLevelId::TLightId lightId) const {
    if (lightId > GetDepth()) {
        ythrow yexception() << "level id " << lightId << " exceeds max level id " << GetDepth();
    }
    return TLevelId(this, lightId);
}

const TTargetTypeParameters::TLevelParameter* TTargetTypeParameters::GetParameterForPartialPath(const TIdPath& path) const {
    return const_cast<TTargetTypeParameters*>(this)->GetParameterForPartialPath(path);
}

TTargetTypeParameters::TLevelParameter* TTargetTypeParameters::GetParameterForPartialPath(const TIdPath& path) {
    if (path.size() > GetDepth()) {
        ythrow yexception() << "too long path: " << ToString(path) << ", depth: " << GetDepth();
    }
    TLevelParameter* levelParameter = &LevelParameter;
    for (TIdPath::const_iterator l = path.begin(); l != path.end(); ++l) {
        levelParameter = &levelParameter->NextLevel.GetForTask(*l);
    }
    return levelParameter;
}

const TTargetTypeParameters::TLevelParameter* TTargetTypeParameters::GetParameterForPath(const TIdPath& path) const {
    return const_cast<TTargetTypeParameters*>(this)->GetParameterForPath(path);
}

TTargetTypeParameters::TLevelParameter* TTargetTypeParameters::GetParameterForPath(const TIdPath& path) {
    if (path.size() != GetDepth()) {
        ythrow TWithBackTrace<yexception>() << "invalid depth";
    }
    return GetParameterForPartialPath(path);
}



bool TTargetTypeParameters::IsEqualTo(const TTargetTypeParameters* that) const {
    if (this->Levels != that->Levels)
        return false;
    if (this->IndexByN != that->IndexByN)
        return false;
    return true;
}

bool TTargetTypeParameters::PathExists(const TIdPath& path) const {
    if (path.size() != GetDepth()) {
        ythrow yexception() << "expected depth " << GetDepth() << " actual " << ToString(path) << ".";
    }
    return PartialPathExists(path);
}

TTargetTypeParameters::TPath TTargetTypeParameters::ResolvePath(const TIdPath& path) const {
    if (path.size() != GetDepth()) {
        ythrow yexception() << "too short path " << ToString(path);
    }
    TPath r;
    for (TLevelId::TLightId i = 0; i < path.size(); ++i) {
        r.push_back(GetParamNameAtLevel(i + 1, path.at(i).Id));
    }
    return r;
}

TTargetTypeParameters::TIdPath TTargetTypeParameters::ResolvePath(const TPath& path) const {
    if (path.size() != GetDepth()) {
        ythrow TWithBackTrace<yexception>() << "too short path " << ToString(path)
                << ", expecting depth " << GetDepth();
    }
    TIdPath r;
    for (TLevelId::TLightId i = 0; i < path.size(); ++i) {
        r.push_back(GetParamIdAtLevel(i + 1, path.at(i)));
    }
    return r;
}

ui32 TTargetTypeParameters::GetCount() const {
    CheckComplete();
    return PathsCounts.Total;
}

ui32 TTargetTypeParameters::GetCountFirstLevelFixed(TParamId firstLevelParamId) const {
    CheckComplete();
    if (GetDepth() > 0) {
        Y_VERIFY(firstLevelParamId.ListId == GetNameListAtLevel(1).GetListId(), "firstLevelParamId belongs to different list");
        return PathsCounts.WithFirstLevelFixed.at(firstLevelParamId.Id);
    } else {
        return 0;
    }
}

TTargetTypeParameters::TId TTargetTypeParameters::GetNForPath(const TIdPath& path) const {
    CheckComplete();

    if (path.size() != GetDepth()) {
        ythrow yexception() << "too short path " << ToString(path);
    }

    const TLevelParameter* levelParameters = &LevelParameter;
    for (ui32 i = 0; i < path.size(); ++i) {
        const TParamId& paramId = path.at(i);
        GetNameListAtLevel(i + 1).CheckListId(paramId.ListId);
        levelParameters = &levelParameters->NextLevel.GetForTask(paramId);
    }

    if (!levelParameters->Exists) {
        ythrow TWithBackTrace<yexception>()
            << "path not found " << ToString(path)
            //<< " (" << ToString(ResolvePath(path)) <<  ")"
            << " for target " << TargetName;
    }

    return TId(this, levelParameters->N);
}

TVector<TString> TTargetTypeParameters::GetScriptArgsOnWorkerByN(const TId& n) const {
    return Drop(GetPathForN(n), FIRST_PARAM_LEVEL_ID - 1);
}


void TTargetTypeParameters::AddTask(const TTargetTypeParameters::TIdPath& path) {
    if (path.size() != GetDepth()) {
        ythrow TWithBackTrace<yexception>() << "adding a path of wrong depth";
    }

    TLevelParameter* levelParameter = &LevelParameter;
    if (path.size() == 0) {
        if (levelParameter->Exists) {
            ythrow TWithBackTrace<yexception>() << "task already exists (empty path)" << Endl;
        }
    }
    levelParameter->Exists = true;
    for (size_t i = 0; i < path.size(); ++i) {
        TParamId taskId = path.at(i);
        if (i == path.size() - 1) {
            if (levelParameter->NextLevel.Exists(taskId)) {
                ythrow TWithBackTrace<yexception>() << "task already exists: " << ToString(path) << ".";
            }
        }
        levelParameter = &levelParameter->NextLevel.GetOrCreateForTask(taskId);
    }
}

void TTargetTypeParameters::AddTask(const TTargetTypeParameters::TPath& path) {
    AddTask(ResolvePath(path));
}

void TTargetTypeParameters::DumpState() const {
    TPrinter printer;
    DumpState(printer);
}

void TTargetTypeParameters::DumpState(TPrinter& printer) const {
    printer.Println("Parameters:");

    TPrinter p1 = printer.Next();
    p1.Println("Depth: " + ToString(GetDepth()));
    p1.Println("Count: " + ToString(GetCount()));

    TIterator it = Iterator();
    while (it.Next()) {
        p1.Println(ToString(it.CurrentPath()) + " " + ToString(it.CurrentN().GetN()));
    }

    printer.Println("Parameters.");
}

void TTargetTypeParameters::CheckState() const {
    TIterator it = Iterator();

    ui32 count = 0;
    while (it.Next()) {
        Y_VERIFY(it.CurrentN().GetN() == count,
                "expecting current n: %d, actual: %d for target type %s",
                int(count), int(it.CurrentN().GetN()), TargetName.data());
        Y_VERIFY(count == GetNForPath(*it).GetN(),
                "expecting n: %d after n -> path -> n conversion: %d for target %s",
                int(count), GetNForPath(*it).GetN(), TargetName.data());
        ++count;
    }
    Y_VERIFY(count == GetCount(),
            "count mismatch: from iterator: %d, from GetCount: %d for target %s",
            int(count), int(GetCount()), TargetName.data());
}


TTargetTypeParameters::TId::TId()
    : Parameters(nullptr), N(Max<ui32>())
{}

TTargetTypeParameters::TId::TId(const TTargetTypeParameters* parameters, ui32 n)
    : Parameters(parameters), N(n)
{
    parameters->CheckId(*this);
}



TTargetTypeParameters::TLevelEnumerator::TLevelEnumerator(const TTargetTypeParameters& parent)
    : Parent(parent)
{}

bool TTargetTypeParameters::TLevelEnumerator::Next() {
    if (Parent.GetDepth() == 0)
        return false;
    if (!CurrentLevel) {
        CurrentLevel = Parent.GetLevelId(0);
        return true;
    }
    if (CurrentLevel->GetValue() == Parent.GetDepth())
        return false;
    ++*CurrentLevel;
    return true;
}



TTargetTypeParameters::TIterator::TIterator(const TTargetTypeParameters& parent, const TProjection& pathFilter)
    : Parent(parent)
    , PathFilter(pathFilter)
    , State(S_BEGIN)
{
    if (pathFilter.GetDepth() != Parent.GetDepth()) {
        ythrow yexception()
            << "expected path of length " << Parent.GetDepth()
            << ", got " << ToString(pathFilter);
    }

    for (ui32 i = 0; i < pathFilter.GetDepth(); ++i) {
        if (!!PathFilter.GetBindings().at(i)) {
            if (PathFilter.GetBindings().at(i)->ListId != Parent.GetListReferenceAtLevel(i + 1).N) {
                ythrow yexception() << "inconsistent path filter: " << ToString(pathFilter);
            }
        }
    }
}

void TTargetTypeParameters::TIterator::IncAtLevel(TLevelId level) {
    if (level.GetValue() == 0 || Parent.GetParamCountAtLevel(level) == 0) {
        State = S_EOF;
        return;
    }
    for (;;) {
        if (!!(PathFilter.GetBindings().at(level.GetValue() - 1))) {
            Y_ASSERT(Path.at(level.GetValue() - 1) == *PathFilter.GetBindings().at(level.GetValue() - 1));
            IncAtLevel(level - 1);
            return;
        }

        TParamId& taskId = Path.at((level - 1).GetValue());
        if (taskId.Id + 1 == Parent.GetParamCountAtLevel(level)) {
            IncAtLevel(level - 1);
            return;
        }
        ++taskId;
        Y_ASSERT(taskId.Id < Parent.GetParamCountAtLevel(level));

        TIdPath partialPath = TIdPath(Path.begin(), Path.begin() + level.GetValue());
        if (!Parent.PartialPathExists(partialPath)) {
            continue;
        }
        for (TLevelId::TLightId j = level.GetValue() + 1; j <= Parent.GetDepth(); ++j) {
            Y_ASSERT(j <= Path.size());
            Path.at(j - 1) = PathFilter.GetBindings().at(j - 1).GetOrElse(TParamId(Parent.Levels.at(j - 1).N, 0));
            TIdPath furtherPartialPath = TIdPath(Path.begin(), Path.begin() + j);
            if (!Parent.PartialPathExists(furtherPartialPath)) {
                IncAtLevel(Parent.GetLevelId(j));
                return;
            }
        }
        return;
    }
}

void TTargetTypeParameters::TIterator::Inc() {
    if (State == S_BEGIN) {
        State = S_RUNNING;
        for (ui32 i = 0; i < Parent.GetDepth(); ++i) {
            Path.push_back(PathFilter.GetBindings().at(i).GetOrElse(TParamId(Parent.Levels.at(i).N, 0)));
        }
        if (!Parent.PathExists(Path)) {
            IncAtLevel(Parent.GetMaxLevelId());
        }
    } else {
        IncAtLevel(Parent.GetMaxLevelId());
    }
}

bool TTargetTypeParameters::TIterator::Next() {
    Inc();
    if (State == S_RUNNING) {
        Y_ASSERT(Parent.PathExists(CurrentPath()));
        return true;
    } else {
        return false;
    }
}

void TTargetTypeParameters::TIterator::Skip(ui32 n) {
    for (ui32 i = 0; i < n; ++i) {
        if (!Next()) {
            ythrow TWithBackTrace<yexception>() << "too few";
        }
    }
}

void TTargetTypeParameters::TIterator::DumpState(IOutputStream& out) const {
    out << "state = " << ui32(State) << Endl;
    out << ToString(Path) << Endl;
}


TTargetTypeParameters::TProjection::TProjection(const TVector<TMaybe<TParamId> >& pathFilter)
    : PathFilter(pathFilter)
{
}

TTargetTypeParameters::TProjection TTargetTypeParameters::TProjection::True(ui32 depth) {
    TVector<TMaybe<TParamId> > r;
    r.resize(depth);
    return TProjection(r);
}

TTargetTypeParameters::TProjection TTargetTypeParameters::TProjection::SingleParam(TLevelId levelId, TTargetTypeParameters::TParamId value, ui32 depth) {
    if (levelId.GetValue() == 0) {
        ythrow yexception() << "cannot filter at level 0";
    }
    TProjection r = True(depth);
    r.PathFilter.at((levelId - 1).GetValue()) = value;
    return r;
}

template <>
void Out<TTargetTypeParameters::TProjection>(IOutputStream& stream, TTypeTraits<TTargetTypeParameters::TProjection>::TFuncParam filter) {
    stream << ToString(filter.GetPathFilter());
}


const TTargetTypeParameters::TLevelParameter TTargetTypeParameters::DummyEmptyParameter;
