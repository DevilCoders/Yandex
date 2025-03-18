#include "param_mapping_detect.h"

#include <util/generic/bt_exception.h>

namespace {
    struct TData {
        TVector<TLevelId> DepLevels;
        TVector<TLevelId> MyLevels;
    };
}


TParamMappings DetectMappings(const TTargetTypeParameters& dependent, const TTargetTypeParameters& my) {

    if (my.IsEqualTo(&dependent)) {
        TVector<TParamMapping> paramMappings;

        for (ui32 i = 0; i < my.GetDepth(); ++i) {
            paramMappings.push_back(TParamMapping(
                    my.GetLevelId(i + 1),
                    dependent.GetLevelId(i + 1)));
        }

        return TParamMappings(paramMappings);
    }

    typedef THashMap<TIdForString::TListId, TData> TIntermediate;
    TIntermediate intermediate;

    for (TTargetTypeParameters::TLevelEnumerator level = dependent.LevelEnumeratorSkipGround(); level.Next(); ) {
        intermediate[dependent.GetListReferenceAtLevel(*level).N].DepLevels.push_back(*level);
    }

    for (TTargetTypeParameters::TLevelEnumerator level = my.LevelEnumeratorSkipGround(); level.Next(); ) {
        intermediate[my.GetListReferenceAtLevel(*level).N].MyLevels.push_back(*level);
    }

    TVector<TParamMapping> mappings;

    for (TIntermediate::const_iterator e = intermediate.begin();
            e != intermediate.end();
            ++e)
    {
        if (e->second.DepLevels.size() == 1 && e->second.MyLevels.size() == 1) {
            mappings.push_back(TParamMapping(e->second.MyLevels.front(), e->second.DepLevels.front()));
        }
    }


    std::sort(mappings.begin(), mappings.end());

    return TParamMappings(mappings);
}
