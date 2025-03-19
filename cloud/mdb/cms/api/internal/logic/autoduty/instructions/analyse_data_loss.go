// 0. Check drills
//   * if date matched
//     * if dom0 has clusters that got replicas in zone will be in maintenance
//         escalate
// 1. dom0 for containers?
//   * if NOT
//  	* if it lives in CONDUCTOR groups that we know of
//			July 7, 2020: it's a very small subset of all possible group
//			* if TRUE - give away no more than 1 dom0 from group at once or wait
//			* if NOT - escalate
//   * if TRUE
//     1. if no containers - give away
// 2. escalate

package instructions

import (
	"time"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/dom0discovery"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/healthiness"
	juggler2 "a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/helpers/juggler"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/steps"
	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/models"
	"a.yandex-team.ru/cloud/mdb/internal/conductor"
	"a.yandex-team.ru/cloud/mdb/internal/dbm"
	"a.yandex-team.ru/cloud/mdb/internal/juggler"
	"a.yandex-team.ru/cloud/mdb/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/client"
)

func NewAnalysePossibleDataLossStrategy(
	jugglerHealth healthiness.Healthiness,
	dom0d dom0discovery.Dom0Discovery,
	jgrl juggler.API,
	dbm dbm.Client,
	cnd conductor.Client,
	hlthcl client.MDBHealthClient,
	pgIntTestAllowAtOnce int,
	pgIntTestCGroup string,
	mdb metadb.MetaDB,
	drillsConfig steps.DrillsConfig,
	jglrConfig juggler2.JugglerConfig,
) *DecisionStrategy {
	nonDatabaseCluster := steps.NewNonDatabaseClusterStep(cnd, pgIntTestAllowAtOnce, pgIntTestCGroup)
	emptyDom0 := steps.NewEmptyDom0Step(dom0d)
	clusterHasHealthyLegs := steps.NewWaitAllHealthyStep(dom0d, hlthcl, jugglerHealth)
	letGoUnreachable := steps.NewActOnUnreachableStep(time.Minute*29, steps.AfterStepApprove, jgrl, jglrConfig.UnreachableServiceWindowMin)
	letGoMemoryChange := steps.NewLetGoOnSafePartChange()
	terminate := steps.NewInParticularState(steps.AfterStepEscalate,
		"escalated, because data can be lost. Use move_container.py (MDB-9446 for automation) or if you see it's safe - approve it ",
		models.DecisionEscalate, models.DecisionApprove, models.DecisionWait, models.DecisionReject)
	checksDrills := steps.NewTodayCheckDrills(drillsConfig, mdb, dom0d, cnd)

	forContainersCond := steps.NewHostForContainers(dbm,
		steps.ListOfSteps(
			nonDatabaseCluster,
		), steps.ListOfSteps(
			emptyDom0,
			letGoUnreachable,
			clusterHasHealthyLegs,
		))

	wf := NewCommonWorkflow("possible data loss", []steps.DecisionStep{
		forContainersCond,
		checksDrills,
		letGoMemoryChange,
		terminate,
	})

	return &DecisionStrategy{
		EntryPoint: wf,
	}
}
