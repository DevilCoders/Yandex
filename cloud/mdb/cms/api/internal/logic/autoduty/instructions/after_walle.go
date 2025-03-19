package instructions

import (
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/dom0discovery"
	juggler2 "a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/helpers/juggler"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/helpers/shipments"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/steps"
	"a.yandex-team.ru/cloud/mdb/deploy/api/pkg/deployapi"
	"a.yandex-team.ru/cloud/mdb/internal/dbm"
	"a.yandex-team.ru/cloud/mdb/internal/juggler"
	"a.yandex-team.ru/cloud/mdb/internal/metadb"
)

const awFinishMessage = "let clean cms stuff"
const awStrategyName = "finishing"

func NewAfterWalleReturnedStrategy(
	dom0 dom0discovery.Dom0Discovery,
	mdb metadb.MetaDB, dbm dbm.Client,
	d deployapi.Client,
	jglr juggler.API,
	asc shipments.AwaitShipmentConfig,
	jglrConf juggler2.JugglerConfig,
) *DecisionStrategy {
	postRestart := steps.NewPostRestartStep(d, asc, dom0)
	metadataOnCluster := steps.NewMetadataOnClusterNodesStep(dom0, d, asc, jglrConf, jglr, dbm)
	metadataOnFQDNs := steps.NewMetadataOnFQDNSStepStep(d, asc, dom0)
	waitDom0Reachable := steps.NewWaitDom0ReachableStep(jglr, jglrConf)
	waitContainersReachable := steps.NewWaitContainersAndDom0ReachableStep(jglr, dom0, jglrConf)
	waitOtherLegsReachable := steps.NewWaitOtherLegsReachableStep(jglr, dom0, mdb, jglrConf)
	dom0StateCond := steps.NewDom0StateConditionStep(
		d,
		steps.ListOfSteps(
			waitOtherLegsReachable,
			metadataOnCluster,
			postRestart,
			metadataOnFQDNs,
		),
		steps.ListOfSteps(
			postRestart,
		),
	)
	finish := steps.NewInParticularState(
		steps.AfterStepNext,
		awFinishMessage)

	metadataForNonempty := steps.NewMetadataNeededForNonempty(dom0, func() []steps.DecisionStep {
		return []steps.DecisionStep{
			waitDom0Reachable,
			waitContainersReachable,
			dom0StateCond,
		}
	})

	forContainersSwitch := steps.NewHostForContainers(dbm,
		steps.NoSteps,
		steps.ListOfSteps(
			metadataForNonempty,
		))

	wf := NewCommonWorkflow(awStrategyName, []steps.DecisionStep{
		forContainersSwitch,
		finish,
	})

	return &DecisionStrategy{
		EntryPoint: wf,
	}
}

func NewAfterPrepareStrategy() *DecisionStrategy {
	finish := steps.NewInParticularState(
		steps.AfterStepNext, "prepared successfully")

	wf := NewCommonWorkflow(awStrategyName, []steps.DecisionStep{
		finish,
	})

	return &DecisionStrategy{
		EntryPoint: wf,
	}
}

func NewAfterWalleReturnedNoShutdownStrategy() *DecisionStrategy {
	finish := steps.NewInParticularState(
		steps.AfterStepNext,
		"nothing to do, "+awFinishMessage)

	wf := NewCommonWorkflow(awStrategyName, []steps.DecisionStep{
		finish,
	})

	return &DecisionStrategy{
		EntryPoint: wf,
	}
}
