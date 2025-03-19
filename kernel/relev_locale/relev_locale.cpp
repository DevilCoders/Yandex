#include "relev_locale.h"

#include <util/generic/algorithm.h>
#include <util/generic/singleton.h>
#include <util/generic/vector.h>
#include <util/generic/xrange.h>

#include <array>

using namespace NRl;

ERelevLocale NRl::GetLocaleParent(const ERelevLocale value) {
    switch (value) {
        case RL_RU:
        case RL_UA:
        case RL_BY:
        case RL_KZ:
        case RL_UZ:
        case RL_AZ:
        case RL_AM:
        case RL_GE:
        case RL_KG:
        case RL_LV:
        case RL_LT:
        case RL_MD:
        case RL_TJ:
        case RL_TM:
        case RL_EE:
        case RL_IL:
            return RL_XUSSR;
        case RL_US:
        case RL_CA:
        case RL_IN:
        case RL_SA:
        case RL_VN:
        case RL_ID:
        case RL_CN:
        case RL_KR:
        case RL_TH:
        case RL_PH:
        case RL_JP:
        case RL_AU:
        case RL_BR:
        case RL_EG:
        case RL_MX:
        case RL_NZ:
        case RL_PR:
        case RL_CH:
        case RL_MY:
            return RL_SPOK;
        case RL_PL:
        case RL_RO:
        case RL_DE:
        case RL_FR:
        case RL_GB:
        case RL_HU:
        case RL_BG:
        case RL_CZ:
        case RL_IT:
        case RL_DK:
        case RL_IE:
        case RL_ES:
        case RL_NL:
        case RL_NO:
        case RL_PT:
        case RL_BE:
        case RL_AT:
        case RL_HR:
        case RL_CY:
        case RL_FI:
        case RL_GR:
        case RL_IS:
        case RL_LI:
        case RL_LU:
        case RL_MT:
        case RL_SK:
        case RL_SI:
        case RL_SE:
            return RL_EU;
        case RL_SPOK:
        case RL_WORLD:
            return RL_XCOM;
        case RL_TR:
        case RL_XUSSR:
        case RL_XCOM:
        case RL_UNIVERSE:
        case RL_RS:
        case RL_CI:
        case RL_EU:
        case RL_AO:
            return RL_UNIVERSE;
        case RL_CM:  [[fallthrough]];
        case RL_SN:
            return RL_FR;
    }
    return RL_UNIVERSE; // make compiler happy
}

bool NRl::IsLocaleDescendantOf(const ERelevLocale child, const ERelevLocale parent,
                                        size_t* parentLevel) {
    ERelevLocale rl = child;
    for (size_t level = 0; level < 32; ++level) {
        if (rl == parent) {
            if (parentLevel) {
                *parentLevel = level;
            }
            return true;
        }
        if (rl == RL_UNIVERSE) {
            break;
        }
        rl = GetLocaleParent(rl);
    }
    return false;
}

namespace {
    class TRelevLocaleHelper {
    public:
        TRelevLocaleHelper() {
            for (int index = 0; index < ERelevLocale_ARRAYSIZE; ++index) {
                Y_ASSERT(ERelevLocale_IsValid(index));
                const ERelevLocale relevLocale = static_cast<ERelevLocale>(index);
                All.push_back(relevLocale);

                ERelevLocale parent = GetLocaleParent(relevLocale);

                TVector<ERelevLocale>& curr = Children[parent];

                if (RL_UNIVERSE != parent || RL_UNIVERSE != relevLocale) {
                    curr.push_back(relevLocale);
                }

                TVector<ERelevLocale>& hier = Hierarchies[index];

                hier.push_back(relevLocale);

                if (RL_UNIVERSE != relevLocale) {
                    while (parent != RL_UNIVERSE) {
                        hier.push_back(parent);
                        parent = GetLocaleParent(parent);
                    }

                    hier.push_back(RL_UNIVERSE);
                }
            }

            for (size_t i = 0, sz = Children.size(); i < sz; ++i) {
                TVector<ERelevLocale>& curr = Children[i];
                Sort(curr.begin(), curr.end());
            }

            for (const ERelevLocale relevLocale: All) {
                if (GetChildren(relevLocale).empty()) {
                    Basic.push_back(relevLocale);
                }
            }

            Sort(Basic.begin(), Basic.end());
        }

        inline const TVector<ERelevLocale>& GetChildren(const ERelevLocale parent) const {
            return Children.at(parent);
        }

        inline const TVector<ERelevLocale>& GetHierarchy(const ERelevLocale self) const {
            return Hierarchies.at(self);
        }

        inline const TVector<ERelevLocale>& GetAll() const {
            return All;
        }

        inline const TVector<ERelevLocale>& GetBasic() const {
            return Basic;
        }

        static const TRelevLocaleHelper& Self() {
            return Default<TRelevLocaleHelper>();
        }

    private:
        std::array<TVector<ERelevLocale>, ERelevLocale_ARRAYSIZE> Children;
        std::array<TVector<ERelevLocale>, ERelevLocale_ARRAYSIZE> Hierarchies;

        TVector<ERelevLocale> All;

        TVector<ERelevLocale> Basic;
    };

}  // namespace


const TVector<ERelevLocale>& NRl::GetLocaleWithAllParents(const ERelevLocale self) {
    return TRelevLocaleHelper::Self().GetHierarchy(self);
}

const TVector<ERelevLocale>& NRl::GetLocaleChildren(const ERelevLocale parent) {
    return TRelevLocaleHelper::Self().GetChildren(parent);
}

const TVector<ERelevLocale>& NRl::GetAllLocales() {
    return TRelevLocaleHelper::Self().GetAll();
}

const TVector<ERelevLocale>& NRl::GetBasicLocales() {
    return TRelevLocaleHelper::Self().GetBasic();
}

template <>
void Out<ERelevLocale>(IOutputStream& o, TTypeTraits<ERelevLocale>::TFuncParam v) {
    o.Write(ToString(v));
}

