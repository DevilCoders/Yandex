#pragma once

#include "target.h"

template <typename PNames>
bool TTargetBase<PNames>::TConstTaskIterator::Next() {
    return ParametersIterator.Next();
}

template <typename PNames>
const typename PNames::TSpecificTaskStatus& TTargetBase<PNames>::TConstTaskIterator::operator*() const {
    return Target->Tasks.At(GetN());
}

template <typename PNames>
const typename PNames::TSpecificTaskStatus* TTargetBase<PNames>::TConstTaskIterator::operator->() const {
    return &(**this);
}

template <typename PNames>
TTargetTypeParameters::TId TTargetBase<PNames>::TConstTaskIterator::GetN() const {
    return ParametersIterator.CurrentN();
}

template <typename PNames>
TTargetTypeParameters::TPath TTargetBase<PNames>::TConstTaskIterator::GetPath() const {
    return ParametersIterator.CurrentPath();
}

template <typename PNames>
const TTargetTypeParameters::TIdPath& TTargetBase<PNames>::TConstTaskIterator::GetPathId() const {
    return *ParametersIterator;
}

template <typename PNames>
const typename PNames::TSpecificTaskStatus& TTargetBase<PNames>::TFlatConstTaskIterator::operator*() const {
    return Target->Tasks.At(GetN());
}

template <typename PNames>
const typename PNames::TSpecificTaskStatus* TTargetBase<PNames>::TFlatConstTaskIterator::operator->() const {
    return &(**this);
}

template <typename PNames>
TTargetTypeParameters::TId TTargetBase<PNames>::TFlatConstTaskIterator::GetN() const {
    return Target->Type->GetParameters().GetId(Current);
}

template <typename PNames>
TTargetTypeParameters::TPath TTargetBase<PNames>::TFlatConstTaskIterator::GetPath() const {
    return Target->Type->GetParameters().GetPathForN(GetN());
}

inline bool NextNoFilter(ui32* current, ui32 tasksCount) {
    if (*current + 1 >= tasksCount) {
        return false;
    } else {
        ++(*current);
        return true;
    }
}

template <typename PNames>
bool TTargetBase<PNames>::TFlatConstTaskIterator::Next() {
    while (true) {
        bool nextNoFilter = NextNoFilter(&Current, TasksCount);
        if (!nextNoFilter) {
            return false;
        } else {
            if (!HostId.Defined()) {
                return true;
            } else {
                TTargetTypeParameters::TParamId goodHostId = *HostId;

                const TTargetTypeParameters::TIdPath& path = Target->Type->GetParameters().GetIdPathForN(GetN());
                TTargetTypeParameters::TParamId hostId = path.at(TTargetTypeParameters::HOST_LEVEL_ID - 1);

                if (hostId == goodHostId) {
                    return true;
                }
            }
        }
    }
}
