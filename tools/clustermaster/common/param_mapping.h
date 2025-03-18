#pragma once

#include "target_type_parameters.h"

#include <util/generic/ptr.h>

class TParamMapping {
private:
    TLevelId MyLevelId;
    TLevelId DepLevelId;

public:
    TParamMapping(const TLevelId& myLevelId, const TLevelId& depLevelId);

    TLevelId GetMyLevelId() const {
        return MyLevelId;
    }

    TLevelId GetDepLevelId() const {
        return DepLevelId;
    }

    bool operator<(const TParamMapping& that) const {
        // TODO: make better
        return this->MyLevelId < that.MyLevelId || this->DepLevelId < that.DepLevelId;
    }

    bool operator==(const TParamMapping& other) const {
        return MyLevelId == other.MyLevelId && DepLevelId == other.DepLevelId;
    }
};


class TParamMappings {
private:
    TVector<TParamMapping> Mappings;
public:
    TParamMappings(const TVector<TParamMapping>& mappings)
        : Mappings(mappings)
    {}

    TParamMappings()
        : Mappings()
    {}

    const TVector<TParamMapping>& GetMappings() const {
        return Mappings;
    }

    size_t Size() const { return Mappings.size(); }

    TString ToString() const;

    bool Has00() const;

    bool operator==(const TParamMappings& other) const {
        return Mappings == other.Mappings;
    }
};


class TJoinCounter {
    friend class TJoinEnumerator;
    friend class TDependEdgesEnumerator;
private:
    const TTargetTypeParameters* const MyParameters;
    const TTargetTypeParameters* const DepParameters;
    const TParamListManager* const ParamListManager;
    TParamMappings Mappings;

    struct TState {
        TVector<TIdForString::TIdSafe> Ids;
    };

    THolder<TState> State;

    bool Initialized() const;
    bool SingleStep();

public:
    TJoinCounter(
            const TTargetTypeParameters* myParameters,
            const TTargetTypeParameters* depParameters,
            const TParamMappings& mappings);
};

class TJoinEnumerator {
private:
    TJoinCounter Counter;
    TTargetTypeParameters::TProjection GetProjectionImpl(bool my) const;

public:
    TJoinEnumerator(
            const TTargetTypeParameters* myParameters,
            const TTargetTypeParameters* depParameters,
            const TParamMappings& mappings);

    bool Next();

    TTargetTypeParameters::TProjection GetMyProjection() const;
    TTargetTypeParameters::TProjection GetDepProjection() const;
};

class TPrecomputedTasksIds;
