#include "classify_experiments.h"

#include "config.h"
#include "request_context.h"
#include "request_params.h"
#include "service_param_holder.h"
#include "stat.h"

#include <util/folder/path.h>
#include <util/generic/bitmap.h>
#include <util/generic/xrange.h>
#include <util/string/builder.h>
#include <util/string/cast.h>

#include <atomic>
#include <utility>

namespace NAntiRobot {

template<class TLinearizedFactors>
class TClassifyExperiments<TLinearizedFactors>::TImpl {
public:
    static constexpr size_t ExperimentsMax = NAntiRobot::MAX_FORMULAS_NUMBER;
public:
    TImpl(const TExpDescriptions& expDescription)
        : ExperimentsNum(expDescription.size())
    {
        if (ExperimentsNum > ExperimentsMax) {
            ythrow yexception() << "Too many experimental formulas, current: " << ExperimentsNum << " formulas, max possible: " << size_t(ExperimentsMax);
        }

        Experiments.reserve(ExperimentsNum);
        for (size_t i = 0; i < ExperimentsNum; ++i) {
            TSimpleSharedPtr<TClassificator<TLinearizedFactors>> fm;
            if constexpr (std::is_same_v<TLinearizedFactors, TCacherLinearizedFactors>) {
                fm = CreateCacherClassificator(expDescription[i].first);
            } else
            if constexpr (std::is_same_v<TLinearizedFactors, TProcessorLinearizedFactors>) {
                fm = CreateProcessorClassificator(expDescription[i].first);
            } else {
                ythrow yexception() << "Unknown TLinearizedFactors in ctor of TClassifyExperiments<TLinearizedFactors>::TImpl::TImpl";
            }
            Experiments.push_back(TExperiment(fm, TFsPath(expDescription[i].first).GetName(), expDescription[i].second));
        }
    }

    template<class TFactors>
    float GetProbability(const TFactors&, EHostType, size_t) {
        ythrow yexception() << "Unknown TLinearizedFactors. Function GetProbability was called with wrong type";
    }

    float GetProbability(const TProcessorLinearizedFactors&, EHostType hostType, size_t fid) {
        return ANTIROBOT_DAEMON_CONFIG.ProcessorExpFormulasProbability[fid][hostType];
    }

    float GetProbability(const TCacherLinearizedFactors&, EHostType hostType, size_t fid) {
        return ANTIROBOT_DAEMON_CONFIG.CacherExpFormulasProbability[fid][hostType];
    }

    TFormulasResults Apply(const TRequest& req, const TLinearizedFactors& lf) {
        TFormulasResults formulasResults;
        formulasResults.reserve(ExperimentsNum);
        const float randomNumber = RandomNumber<float>();
        for (size_t i = 0; i < ExperimentsNum; ++i) {
            auto expProbability = GetProbability(lf, req.HostType, i);
            if (randomNumber > expProbability) {
                continue;
            }
            auto& exp = Experiments[i];
            const auto expRes = exp.Apply(req, lf);
            formulasResults.push_back({exp.GetFormulaName(), expRes.second});
        }
        return formulasResults;
    }

    void PrintStats(TStatsWriter& out) {
        for (size_t i = 0; i < ExperimentsNum; ++i) {
            Experiments[i].PrintStats(out, i);
        }
    }

    static TStringBuf GetFormulaTypePrefix() {
        if constexpr (std::is_same_v<TLinearizedFactors, TProcessorLinearizedFactors>) {
            return ""_sb;
        } else if constexpr (std::is_same_v<TLinearizedFactors, TCacherLinearizedFactors>) {
            return "cacher_"_sb;
        } else {
            static_assert(TDependentFalse<TLinearizedFactors>);
        }
    }

private:
    class TExperiment {
    public:
        TExperiment(TSimpleSharedPtr<TClassificator<TLinearizedFactors>> formula, const TString& formulaName, float threshold)
            : Formula(std::move(formula))
            , FormulaName(formulaName)
            , Threshold(threshold)
        {
        }

        std::pair<bool,float> Apply(const TRequest& req, const TLinearizedFactors& lf) {
            if (req.MayBanFor()) {
                const float score = CalcScore(lf);
                const bool isExpRobot = IsRobot(score);
                Stats.Update(isExpRobot);
                return {isExpRobot, score};
            }
            return {false, 0.0f};
        }

    void PrintStats(TStatsWriter& out, int number) const {
        Stats.Print(out, number);
    }

    const TString& GetFormulaName() const {
        return FormulaName;
    }
    private:
        float CalcScore(const TLinearizedFactors& lf) const {
            return (*Formula)(lf);
        }

        bool IsRobot(float score) const {
            return score > Threshold;
        }

        class TStats {
        public:
            TStats() = default;

            TStats(const TStats& rhs) {
                Count = rhs.Count.load();
            }

            TStats& operator=(const TStats& rhs) {
                Count = rhs.Count.load();
                return *this;
            }

            TStats(TStats&& rhs) noexcept
                : Count(rhs.Count.load()) {
            }

            TStats& operator=(TStats&& rhs) noexcept {
                Count = rhs.Count.load();
                return *this;
            }
                
            void Update(bool isExpRobot) {
                if (isExpRobot) {
                    ++Count;;
                }
            }

            void Print(TStatsWriter& out, int number) const {
                TStringBuilder prefix;
                prefix
                    << GetFormulaTypePrefix()
                    << "exp_formula_id_" << number
                    << "."
                    << GetFormulaTypePrefix() 
                    << "exp_robots_with_captcha";
                out.WriteScalar(prefix, Count);
            }

            std::atomic<size_t> Count = {};
        }; // class TClassifyExperiments::TImpl::TExperiment::TStats

        TSimpleSharedPtr<TClassificator<TLinearizedFactors>> Formula;
        const TString FormulaName;
        const float Threshold;
        TStats Stats;
    }; // class TClassifyExperiments::TImpl::TExperiment

    const size_t ExperimentsNum;
    TVector<TExperiment> Experiments;
}; // class TClassifyExperiments::TImpl

template<class TLinearizedFactors>
TClassifyExperiments<TLinearizedFactors>::TClassifyExperiments(const TExpDescriptions& expDescriptions)
    : Impl(new TImpl(expDescriptions))
{}

template<class TLinearizedFactors>
TClassifyExperiments<TLinearizedFactors>::~TClassifyExperiments() = default;

template<class TLinearizedFactors>
TFormulasResults TClassifyExperiments<TLinearizedFactors>::Apply(const TRequest& req, const TLinearizedFactors& lf) {
    return Impl->Apply(req, lf);
}

template<class TLinearizedFactors>
void TClassifyExperiments<TLinearizedFactors>::PrintStats(TStatsWriter& out) const {
    Impl->PrintStats(out);
}

template class TClassifyExperiments<TProcessorLinearizedFactors>;
template class TClassifyExperiments<TCacherLinearizedFactors>;
} // namespace NAntiRobot
