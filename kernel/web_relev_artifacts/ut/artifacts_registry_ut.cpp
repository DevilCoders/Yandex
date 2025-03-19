#include "artifacts_registry.h"
#include <library/cpp/testing/unittest/registar.h>

Y_UNIT_TEST_SUITE(TestRegistry) {
    Y_UNIT_TEST(JustGet) {
        auto reg = NRelevArtifacts::GetWebSearchRegistry();
        UNIT_ASSERT(reg.RelevArtifactSize() > 0);
        UNIT_ASSERT(reg.ObsolescenceMetricaSize() > 0);
    }

    Y_UNIT_TEST(CheckFormat) {
        auto reg = NRelevArtifacts::GetWebSearchRegistry();

        for(const NRelevArtifacts::TRelevArtifact& art: reg.GetRelevArtifact()) {
            for(const NRelevArtifacts::TDependencyDeclaration& c : art.GetDependsOn()) {
                TStringBuf d = c.GetAgeSla();
                if (d) {
                    TDuration val;
                    TryFromString(d, val);
                    UNIT_ASSERT(val < TDuration::Days(1000));
                    UNIT_ASSERT(val > TDuration::Days(1));
                }
                if (c.HasSilentVersionDate()) {
                    TInstant s = TInstant::ParseIso8601(c.GetSilentVersionDate());
                    UNIT_ASSERT(s < Now());
                }

                if (c.HasSlaViolationPenalty()) {
                    UNIT_ASSERT(c.GetSlaViolationPenalty() > 0);
                    UNIT_ASSERT(c.GetSlaViolationPenalty() < 1);
                }
            }

            if (art.HasFirstVersionDateTag()) {
                TInstant s = TInstant::ParseIso8601(art.GetFirstVersionDateTag());
                UNIT_ASSERT(s < Now());
            }
        }

        for(const NRelevArtifacts::TArtifactObsolescenceMetrica& m: reg.GetObsolescenceMetrica()) {
            for(const NRelevArtifacts::TObsolescenceMetricaComponent& d : m.GetComponent()) {
                TDuration val = FromString(d.GetDesiredUpdateRate());
                UNIT_ASSERT(val < TDuration::Days(1000));
                UNIT_ASSERT(val > TDuration::Days(1));
                TInstant{}.ParseIso8601(d.GetAddedInMetricaDate());
            }
        }
    }

    Y_UNIT_TEST(CheckDeps) {//NOTE: cyclic deps are not prohibited
        auto reg = NRelevArtifacts::GetWebSearchRegistry();

        THashSet<TString> arts;
        for(const NRelevArtifacts::TRelevArtifact& art: reg.GetRelevArtifact()) {
            arts.insert(art.GetName());
        }
        UNIT_ASSERT_VALUES_EQUAL(arts.size(), reg.GetRelevArtifact().size());

        for(const NRelevArtifacts::TRelevArtifact& art: reg.GetRelevArtifact()) {
            for(const NRelevArtifacts::TDependencyDeclaration& c : art.GetDependsOn()) {
                if (c.HasArtifact()) {
                    UNIT_ASSERT(arts.contains(c.GetArtifact()));
                }
            }
        }

        THashSet<TString> metrics;
        for(const NRelevArtifacts::TArtifactObsolescenceMetrica& m: reg.GetObsolescenceMetrica()) {
            metrics.insert(m.GetMetricaName());
            THashSet<TString> comps;
            for(const NRelevArtifacts::TObsolescenceMetricaComponent& d : m.GetComponent()) {
                comps.insert(d.GetArtifact());
                UNIT_ASSERT(arts.contains(d.GetArtifact()));
            }
            UNIT_ASSERT_VALUES_EQUAL(comps.size(), m.GetComponent().size());
        }
        UNIT_ASSERT_VALUES_EQUAL(metrics.size(), reg.GetObsolescenceMetrica().size());

    }

}
