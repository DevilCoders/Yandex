package instructions

import (
	"time"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/dom0discovery"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/healthiness"
	juggler2 "a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/helpers/juggler"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/steps"
	"a.yandex-team.ru/cloud/mdb/internal/conductor"
	"a.yandex-team.ru/cloud/mdb/internal/dbm"
	"a.yandex-team.ru/cloud/mdb/internal/juggler"
	"a.yandex-team.ru/cloud/mdb/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/client"
)

func NewUnconditionalOKAnalyse() *DecisionStrategy {
	approveUnconditionally := steps.NewInParticularState(
		steps.AfterStepApprove, "it's safe to let go")
	wf := NewCommonWorkflow("always allow", []steps.DecisionStep{
		approveUnconditionally,
	})
	return &DecisionStrategy{
		EntryPoint: wf,
	}
}

func NewAnalyseDefaultStrategy(
	jugglerHealth healthiness.Healthiness,
	dom0d dom0discovery.Dom0Discovery,
	jgrl juggler.API,
	dbm dbm.Client,
	cnd conductor.Client,
	hlthcl client.MDBHealthClient,
	takeNoMoreNum int,
	pgIntTestCGroup string,
	mdb metadb.MetaDB,
	drillsConfig steps.DrillsConfig,
	jgrlConf juggler2.JugglerConfig,
) *DecisionStrategy {
	nonDatabaseCluster := steps.NewNonDatabaseClusterStep(cnd, takeNoMoreNum, pgIntTestCGroup)
	emptyDom0 := steps.NewEmptyDom0Step(dom0d)
	letGoUnreachable := steps.NewActOnUnreachableStep(time.Minute*5, steps.AfterStepApprove, jgrl, jgrlConf.UnreachableServiceWindowMin)
	clusterHasHealthyLegs := steps.NewWaitAllHealthyStep(dom0d, hlthcl, jugglerHealth)
	approveUnconditionally := steps.NewInParticularState(
		steps.AfterStepApprove, "it's safe to let go")
	checksDrills := steps.NewTodayCheckDrills(drillsConfig, mdb, dom0d, cnd)

	forContainersCond := steps.NewHostForContainers(dbm,
		steps.ListOfSteps(
			nonDatabaseCluster,
		), steps.ListOfSteps(
			emptyDom0,
			letGoUnreachable,
			clusterHasHealthyLegs,
		))

	wf := NewCommonWorkflow("allow when possible", []steps.DecisionStep{
		forContainersCond,
		checksDrills,
		approveUnconditionally,
	})

	return &DecisionStrategy{
		EntryPoint: wf,
	}
}
