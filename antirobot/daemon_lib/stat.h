#pragma once

#include "exp_bin.h"
#include "req_types.h"
#include "request_group_classifier.h"
#include "request_params.h"
#include "uid.h"

#include <antirobot/lib/ar_utils.h>
#include <antirobot/lib/param_pack.h>
#include <antirobot/lib/stats_writer.h>

#include <library/cpp/deprecated/atomic/atomic.h>

#include <array>
#include <type_traits>


namespace NAntiRobot {


enum EStatType {
    STAT_TOTAL = HOST_NUMTYPES
};

inline EStatType HostTypeToStatType(EHostType hostType) {
    return static_cast<EStatType>(hostType);
}

inline EHostType StatTypeToHostType(EStatType statType) {
    return static_cast<EHostType>(statType);
}


enum class ESimpleUidType {
    Other    /* "other" */,
    Spravka  /* "2_spravka" */,
    Lcookie  /* "7_lcookie" */,
    Icookie  /* "10_icookie" */,
    Count
};

inline ESimpleUidType UidNsToSimpleUidType(TUid::ENameSpace ns) {
    switch (ns) {
    case TUid::SPRAVKA:
        return ESimpleUidType::Spravka;
    case TUid::LCOOKIE:
        return ESimpleUidType::Lcookie;
    case TUid::ICOOKIE:
        return ESimpleUidType::Icookie;
    default:
        return ESimpleUidType::Other;
    }
}


template <typename T, typename = void>
struct THasDynamicCountImpl : public std::true_type {};

template <typename T>
struct THasDynamicCountImpl<T, decltype((void)T::Count)> : public std::false_type {};

template <typename T>
using THasDynamicCount = THasDynamicCountImpl<T>;


template <typename T>
struct TCategorizedStatsToString {};

template <>
struct TCategorizedStatsToString<EReqGroup> {
    TCategorizedStatsToString(const TRequestGroupClassifier& classifier)
        : Classifier(&classifier)
    {}

    TStringBuf operator()(EHostType service, EReqGroup group) const {
        return Classifier->GroupName(service, group);
    }

private:
    const TRequestGroupClassifier* Classifier = nullptr;
};


template <typename T, typename = void>
struct TCategorizedStatsHasToStringImpl : public std::false_type {};

template <typename T>
struct TCategorizedStatsHasToStringImpl<T, decltype((void)std::declval<TCategorizedStatsToString<T>>()(EHostType{}, EReqGroup{}))>
    : public std::true_type
{};

template <typename T>
using TCategorizedStatsHasToString = TCategorizedStatsHasToStringImpl<T>;


template <bool IsHistogram, typename TStat, typename EStat, typename... ECategories>
struct TCategorizedStatsBase {
    template <typename... EDynamicCategories>
    struct TWithDynamicCategories {
        template <typename... EToStringCategories>
        class TWithToStringCategories {
        public:
            explicit TWithToStringCategories(
                TParametrized<
                    EDynamicCategories,
                    std::array<size_t, EHostType::Count>
                >... categoryCounts
            )
                : DynamicCategoryCounts{categoryCounts...}
            {
                ProcessCategoryCounts<0>(categoryCounts...);
                Stats.resize(THelper<0, ECategories...>::GetSize(*this));
            }

            void Print(
                TCategorizedStatsToString<EToStringCategories>... toStrings,
                TStatsWriter& out,
                TString prefix = {}
            ) const {
                THelper<0, ECategories...>
                    ::Print(*this, toStrings..., out, std::move(prefix), HOST_OTHER, 0);
            }

        private:
            template <size_t I>
            size_t ProcessCategoryCounts() {
                return 0;
            }

            template <size_t I, typename T, typename... Ts>
            size_t ProcessCategoryCounts(T serviceToCount, Ts... serviceToCounts) {
                MaxDynamicCategoryCounts[I] = *MaxElement(serviceToCount.begin(), serviceToCount.end());

                Y_ENSURE(
                    Find(serviceToCount, 0) == serviceToCount.end(),
                    "dynamic category count cannot be zero"
                );

                return Max(MaxDynamicCategoryCounts[I], ProcessCategoryCounts<I + 1>(serviceToCounts...));
            }

        protected:
            using TCategoryPack = TParamPack<ECategories...>;

            static_assert(
                !TCategoryPack::template ContainsIf<THasDynamicCount> ||
                (
                    TCategoryPack::template Contains<EHostType> &&
                    TCategoryPack::template IndexOf<EHostType> <
                        TCategoryPack::template IndexIf<THasDynamicCount>
                ),
                "EHostType categorization must preceed any dynamically sized categorization"
            );

            static constexpr bool CategorizedByService = TCategoryPack::template Contains<EHostType>;
            static constexpr bool CategorizedByReqGroup = TCategoryPack::template Contains<EReqGroup>;

            static constexpr std::array<EHostType, 8> DeadServices = {
                HOST_AUTO,
                HOST_HILIGHTER,
                HOST_KPAPI,
                HOST_MARKETAPI_BLUE,
                HOST_MARKETRED,
                HOST_TECH,
                HOST_SLOVARI,
                HOST_RCA,
            };

            static constexpr std::array<TUid::ENameSpace, 7> ReportedNameSpaces = {
                TUid::IP,
                TUid::SPRAVKA,
                TUid::FUID,
                TUid::LCOOKIE,
                TUid::IP6,
                TUid::ICOOKIE,
                TUid::PREVIEW,
            };

            template <typename T>
            struct TCategoryTag {};

            template <>
            struct TCategoryTag<EHostType> {
                static constexpr auto Value = "service_type"_sb;
            };

            template <>
            struct TCategoryTag<TUid::ENameSpace> {
                static constexpr auto Value = "id_type"_sb;
            };

            template <>
            struct TCategoryTag<ESimpleUidType> {
                static constexpr auto Value = "id_type"_sb;
            };

            template <>
            struct TCategoryTag<EReqGroup> {
                static constexpr auto Value = "req_group"_sb;
            };

            template <>
            struct TCategoryTag<EExpBin> {
                static constexpr auto Value = "exp_bin"_sb;
            };

            template <size_t DynamicIndex, typename... ECats>
            struct THelper;

            template <size_t DynamicIndex>
            struct THelper<DynamicIndex> {
                static size_t GetCount(const TWithToStringCategories&, EHostType) {
                    return static_cast<size_t>(EStat::Count);
                }

                static size_t GetMaxCount(const TWithToStringCategories&) {
                    return static_cast<size_t>(EStat::Count);
                }

                static size_t GetSize(const TWithToStringCategories& stats) {
                    return GetMaxCount(stats);
                }

                static size_t GetOffset(const TWithToStringCategories&, EStat stat) {
                    return static_cast<size_t>(stat);
                }

                static void Print(
                    const TWithToStringCategories& stats,
                    TCategorizedStatsToString<EToStringCategories>...,
                    TStatsWriter& out,
                    TString prefix,
                    EHostType service,
                    size_t offset
                ) {
                    if (prefix.empty()) {
                        Print(stats, out, service, offset);
                    } else {
                        auto prefixedOut = out.WithPrefix(std::move(prefix));
                        Print(stats, prefixedOut, service, offset);
                    }
                }

                static void Print(const TWithToStringCategories& stats, TStatsWriter& out, EHostType, size_t offset) {
                    for (size_t i = 0; i < static_cast<size_t>(EStat::Count); ++i) {
                        const auto stat = static_cast<EStat>(i);
                        const auto statStr = ToString(stat);
                        const auto& wrapper = stats.Stats[offset + i];
                        if constexpr (IsHistogram) {
                            out.WriteHistogram(statStr, wrapper.Value);
                        } else {
                            out.WriteScalar(statStr, wrapper.Value);
                        }
                    }
                }
            };

            template <size_t DynamicIndex, typename ECat, typename... ECats>
            struct THelper<DynamicIndex, ECat, ECats...> {
                using TNext = THelper<DynamicIndex + THasDynamicCount<ECat>::value, ECats...>;

                static size_t GetCount(const TWithToStringCategories& stats, EHostType service) {
                    if constexpr (THasDynamicCount<ECat>::value) {
                        return stats.DynamicCategoryCounts[DynamicIndex][service];
                    } else {
                        return static_cast<size_t>(ECat::Count);
                    }
                }

                static size_t GetMaxCount(const TWithToStringCategories& stats) {
                    if constexpr (THasDynamicCount<ECat>::value) {
                        return stats.MaxDynamicCategoryCounts[DynamicIndex];
                    } else {
                        return static_cast<size_t>(ECat::Count);
                    }
                }

                static size_t GetSize(const TWithToStringCategories& stats) {
                    return GetMaxCount(stats) * TNext::GetSize(stats);
                }

                static size_t GetOffset(const TWithToStringCategories& stats, EStat stat, ECat category, ECats... categories) {
                    return
                        static_cast<size_t>(category) * TNext::GetSize(stats) +
                        TNext::GetOffset(stats, stat, categories...);
                }

                static void Print(
                    const TWithToStringCategories& stats,
                    TCategorizedStatsToString<EToStringCategories>... toString,
                    TStatsWriter& out,
                    TString prefix,
                    EHostType service,
                    size_t offset
                ) {
                    for (
                        size_t i = 0; i < GetCount(stats, service);
                        ++i, offset += TNext::GetSize(stats)
                    ) {
                        if constexpr (std::is_same_v<ECat, EHostType>) {
                            service = static_cast<EHostType>(i);
                        }

                        const auto tagValue = static_cast<ECat>(i);

                        if (!ShouldPrint(tagValue)) {
                            continue;
                        }

                        const auto tag = TCategoryTag<ECat>::Value;
                        const auto tagValueStr = TagValue(toString..., service, tagValue);

                        auto taggedOut = out.WithTag(tag, tagValueStr);
                        TNext::Print(stats, toString..., taggedOut, prefix, service, offset);
                    }
                }

                static auto TagValue(EHostType service, ECat category) {
                    Y_UNUSED(service);
                    return ToString(category);
                }

                template <typename EToStringCat, typename... EToStringCats>
                static auto TagValue(
                    TCategorizedStatsToString<EToStringCat> toString,
                    TCategorizedStatsToString<EToStringCats>... toStrings,
                    EHostType service,
                    ECat category
                ) {
                    if constexpr (std::is_same_v<ECat, EToStringCat>) {
                        return toString(service, category);
                    } else {
                        return TagValue(toStrings..., service, category);
                    }
                }

                static bool ShouldPrint(ECat category) {
                    if constexpr (std::is_same_v<ECat, EHostType> && sizeof...(ECategories) > 1) {
                        return !IsIn(DeadServices, category) && (category == HOST_OTHER || !ANTIROBOT_DAEMON_CONFIG.AsCaptchaApiService);
                    } else if constexpr (std::is_same_v<ECat, EHostType>) {
                        return category == HOST_OTHER || !ANTIROBOT_DAEMON_CONFIG.AsCaptchaApiService;
                    } else if constexpr (std::is_same_v<ECat, TUid::ENameSpace>) {
                        return IsIn(ReportedNameSpaces, category);
                    } else {
                        return true;
                    }
                }
            };

            template <typename T>
            struct TStatWrapper {
                T Value{};
            };

            template <typename T>
            struct TStatWrapper<std::atomic<T>> {
                TStatWrapper() = default;

                TStatWrapper(const TStatWrapper& that) {
                    Value = that.Value.load();
                }

                TStatWrapper& operator=(const TStatWrapper& that) {
                    Value = that.Value.load();
                    return *this;
                }

                TStatWrapper(TStatWrapper&& that) noexcept
                    : Value(that.Value.load())
                {}

                TStatWrapper& operator=(TStatWrapper&& that) noexcept {
                    Value = that.Value.load();
                    return *this;
                }

                std::atomic<T> Value{};
            };

            std::array<
                std::array<size_t, EHostType::Count>,
                sizeof...(EDynamicCategories)
            > DynamicCategoryCounts;
            std::array<size_t, sizeof...(EDynamicCategories)> MaxDynamicCategoryCounts;
            TVector<TStatWrapper<TStat>> Stats;
        };
    };
};


template <bool IsHistogram, typename TStat, typename EStat, typename... ECategories>
using TCategorizedStatsBaseType = typename TParamPack<ECategories...>
    ::template TFilter<TCategorizedStatsHasToString>
    ::template TApply<
        TParamPack<ECategories...>
            ::template TFilter<THasDynamicCount>
            ::template TApply<
                TCategorizedStatsBase<IsHistogram, TStat, EStat, ECategories...>
                    ::template TWithDynamicCategories
            >
            ::template TWithToStringCategories
    >;


template <bool IsHistogram, typename TStat, typename EStat, typename... ECategories>
class TCustomCategorizedStats : private TCategorizedStatsBaseType<IsHistogram, TStat, EStat, ECategories...> {
private:
    using TSuper = TCategorizedStatsBaseType<IsHistogram, TStat, EStat, ECategories...>;

    template <typename T, typename = void>
    struct TRequestCategorizer {};

    template <>
    struct TRequestCategorizer<EHostType, void> {
        static EHostType Get(const TRequest& req) {
            return req.HostType;
        }
    };

    template <>
    struct TRequestCategorizer<TUid::ENameSpace, void> {
        static TUid::ENameSpace Get(const TRequest& req) {
            return req.Uid.GetNameSpace();
        }
    };

    template <>
    struct TRequestCategorizer<ESimpleUidType, void> {
        static ESimpleUidType Get(const TRequest& req) {
            return UidNsToSimpleUidType(req.Uid.GetNameSpace());
        }
    };

    template <>
    struct TRequestCategorizer<EReqGroup, void> {
        static EReqGroup Get(const TRequest& req) {
            return req.ReqGroup;
        }
    };

    template <>
    struct TRequestCategorizer<EExpBin, void> {
        static EExpBin Get(const TRequest& req) {
            return req.ExperimentBin();
        }
    };

public:
    using TSuper::TSuper;

    [[nodiscard]]
    TStat& Get(ECategories... categories, EStat stat) {
        return TSuper::Stats[TSuper::template THelper<0, ECategories...>::GetOffset(*this, stat, categories...)].Value;
    }

    [[nodiscard]]
    const TStat& Get(ECategories... categories, EStat stat) const {
        return TSuper::Stats[TSuper::template THelper<0, ECategories...>::GetOffset(*this, stat, categories...)].Value;
    }

    [[nodiscard]]
    TStat& Get(const TRequest& req, EStat stat) {
        return Get(TRequestCategorizer<ECategories>::Get(req)..., stat);
    }

    void Inc(ECategories... categories, EStat stat) {
        ++Get(categories..., stat);
    }

    void Inc(const TRequest& req, EStat stat) {
        ++Get(req, stat);
    }

    void Reset() {
        TSuper::Stats.assign(TSuper::Stats.size(), {});
    }

    using TSuper::Print;
};


template <typename TStat, typename EStat, typename... ECategories>
using TCategorizedStats = TCustomCategorizedStats<false, TStat, EStat, ECategories...>;

template <typename TStat, typename EStat, typename... ECategories>
using TCategorizedHistograms = TCustomCategorizedStats<true, TStat, EStat, ECategories...>;


} // namespace NAntiRobot
