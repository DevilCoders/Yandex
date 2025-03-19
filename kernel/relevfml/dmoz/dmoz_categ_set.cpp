#include <util/generic/algorithm.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/generic/set.h>
#include <util/string/cast.h>
#include <kernel/groupattrs/iterator.h>
#include <kernel/search_types/search_types.h>

#include "dmoz_categ_set.h"
#include "dmoz_categ_set.inc"

typedef std::pair<TString, float> TCategSetValue;

static const TVector<TCateg> DMOZCategories = {3, 1, 13, 2, 76, 4, 9, 11, 12, 75, 6, 14};
static const TSet<TCateg> DMOZCategoriesSet(DMOZCategories.begin(), DMOZCategories.end());

static const TVector<TCategSetValue> DMOZQueryMapList = {
    {"3", 1},
    {"4", 0.8f},
    {"9", 0.6f},
    {"1", 0.4f},
    {"1|75", 0.266f},
    {"1|76", 0.133f}
};

static const TMap<TString, float> DMOZQueryMap(DMOZQueryMapList.begin(), DMOZQueryMapList.end());

static inline TString MakeCategSetString(TVector<TCateg>& categories) {
    TString categString;
    TCateg categ(0), prevCateg(0);

    Sort(categories.begin(), categories.end());

    for (TVector<TCateg>::iterator it = categories.begin(); it != categories.end(); ++it) {
        categ = *it;
        if (categ != prevCateg) {
            if (categString.length() > 0)
                categString += "|";
            categString += ToString<int>(categ);
        }
        prevCateg = categ;
    }

    return categString;
}

static inline float GetCategSetValue(const TMap<TString, float>& valuesMap, TVector<TCateg>& categories) {
    if (categories.size() <= 0)
        return 0.0f;

    TString sKey = MakeCategSetString(categories);
    TMap<TString, float>::const_iterator mapIter = valuesMap.find(sKey);
    return mapIter == valuesMap.end() ? 0.0f :  mapIter->second;
}

void DmozAddParentThemes(const NGroupingAttrs::TAttrWeights& themes, TMap<int,float>& out) {
    out = TMap<int,float>();
    TCateg categ(0);
    float probability = 0.0f;
    for (NGroupingAttrs::TAttrWeights::const_iterator it = themes.begin(); it != themes.end(); ++it) {
        categ = it->AttrId;
        probability = it->Weight;

        TMap<int,int>::const_iterator parentIt;
        for(; categ != 0; ) {
            if (out.find(categ) == out.end())
                out[categ] = probability;
            else
                out[categ] += probability;

            parentIt = DmozParents.find(categ);
            if (parentIt == DmozParents.end())
                break;
            categ = parentIt->second;
        }
    }
}

float DmozQueryBestThemeWithMapping(const TMap<int,float>& themes) {
    if (themes.size() <= 0)
        return 0.0f;

    TCateg categ(0), bestCateg(0);
    float maxProbability = 0.0f, probability;
    for (TMap<int,float>::const_iterator it = themes.begin(); it != themes.end(); ++it) {
        categ = it->first;
        if (DMOZCategoriesSet.find(categ) != DMOZCategoriesSet.end()) {
            probability = it->second;
            if (probability > maxProbability) {
                maxProbability = probability;
                bestCateg = categ;
            }
        }
    }

    if (bestCateg < 1 || maxProbability <= 0.0f)
        return 0.0f;

    TVector<TCateg>::const_iterator it = std::find(DMOZCategories.begin(), DMOZCategories.end(), bestCateg);
    if (it == DMOZCategories.end())
        return 0.001f;

    return 1.0f - (it-DMOZCategories.begin() + maxProbability*0.5f)/DMOZCategories.size();
}

float DmozQueryThemeCombination(const TMap<int,float>& themes) {
    if (themes.size() <= 0)
        return 0.0f;

    TVector<TCateg> queryCategories;
    for (TMap<int,float>::const_iterator it = themes.begin(); it != themes.end(); ++it)
        if (DMOZCategoriesSet.find(it->first) != DMOZCategoriesSet.end())
            queryCategories.push_back(it->first);

    return GetCategSetValue(DMOZQueryMap, queryCategories);
}
