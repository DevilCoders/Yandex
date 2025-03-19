package instructions

import (
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/dom0discovery"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/lockcluster"
	cms_juggler "a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/helpers/juggler"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/helpers/shipments"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/steps"
	"a.yandex-team.ru/cloud/mdb/deploy/api/pkg/deployapi"
	"a.yandex-team.ru/cloud/mdb/internal/conductor"
	"a.yandex-team.ru/cloud/mdb/internal/dbm"
	"a.yandex-team.ru/cloud/mdb/internal/juggler"
	"a.yandex-team.ru/cloud/mdb/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/mlock/pkg/mlockclient"
)

const lgFinishMessage = "let go to Wall-e"
const lgStrategyName = "letting dom0 go"

func NewLetGoStrategy(
	dom0d dom0discovery.Dom0Discovery,
	mlock mlockclient.Locker,
	locker lockcluster.Locker,
	lockHolder string,
	dbm dbm.Client,
	jglr juggler.API,
	d deployapi.Client,
	cfg shipments.AwaitShipmentConfig,
	jugglerNamespaces []string, windowSize int,
	jgrlConf cms_juggler.JugglerConfig,
) *DecisionStrategy {
	lockDom0 := steps.NewLockDom0Step(mlock, lockHolder)
	lockContainers := steps.NewLockContainersStep(locker, dom0d)
	letGoUnreachable := steps.NewActOnUnreachableStep(0, steps.AfterStepAtWalle, jglr, jgrlConf.UnreachableServiceWindowMin)
	disallowNewHosts := steps.NewAllowNewHostsStep(dbm, false)
	dom0State := steps.NewDom0StateStep(d)
	downtimes := steps.NewSetDowntimesStep(dom0d, jglr, jugglerNamespaces)
	preRestart := steps.NewPreRestartStep(d, dom0d, cfg)
	shutdownContainers := steps.NewShutdownContainersStep(d, cfg)
	atWalle := steps.NewInParticularState(
		steps.AfterStepAtWalle, lgFinishMessage)

	forContainersSwitch := steps.NewHostForContainers(dbm,
		steps.NoSteps,
		steps.ListOfSteps(
			lockContainers,
			disallowNewHosts,
			downtimes,
			letGoUnreachable,
			dom0State,
			preRestart, // TODO: some containers may still be reachable, so you can pre_restart them - learn to do it in future
			shutdownContainers,
		))

	wf := NewCommonWorkflow(lgStrategyName, []steps.DecisionStep{
		lockDom0,
		forContainersSwitch,
		atWalle,
	})

	return &DecisionStrategy{
		EntryPoint: wf,
	}
}

func NewLetGoNewHostStrategy(
	deploy deployapi.Client,
	regMinionCfg steps.RegisterMinionStepConfig,
	cnd conductor.Client,
	addToCondCfg steps.NewHostsConfig,
) *DecisionStrategy {
	addToConductor := steps.NewHostInConductorStep(cnd, addToCondCfg)
	regMinion := steps.NewRegisterMinionStep(deploy, regMinionCfg)
	atWalle := steps.NewInParticularState(
		steps.AfterStepAtWalle, "giving away")

	wf := NewCommonWorkflow("let go new host", []steps.DecisionStep{
		addToConductor,
		regMinion,
		atWalle,
	})

	return &DecisionStrategy{
		EntryPoint: wf,
	}
}

func NewLetGoRedeployStrategy(
	dom0d dom0discovery.Dom0Discovery,
	mlock mlockclient.Locker,
	locker lockcluster.Locker,
	lockHolder string,
	dbm dbm.Client,
	jglr juggler.API,
	d deployapi.Client,
	cfg shipments.AwaitShipmentConfig,
	jugglerNamespaces []string,
	jgrlConf cms_juggler.JugglerConfig,
) *DecisionStrategy {
	lockDom0 := steps.NewLockDom0Step(mlock, lockHolder)
	lockContainers := steps.NewLockContainersStep(locker, dom0d)
	disallowNewHosts := steps.NewAllowNewHostsStep(dbm, false)
	letGoUnreachable := steps.NewActOnUnreachableStep(0, steps.AfterStepAtWalle, jglr, jgrlConf.UnreachableServiceWindowMin)
	downtimes := steps.NewSetDowntimesStep(dom0d, jglr, jugglerNamespaces)
	preRestart := steps.NewPreRestartStep(d, dom0d, cfg)
	shutdownContainers := steps.NewShutdownContainersStep(d, cfg)
	atWalle := steps.NewInParticularState(
		steps.AfterStepAtWalle, lgFinishMessage)

	forContainersSwitch := steps.NewHostForContainers(dbm,
		steps.NoSteps,
		steps.ListOfSteps(
			lockContainers,
			disallowNewHosts,
			downtimes,
			letGoUnreachable,
			preRestart,
			shutdownContainers,
		))

	wf := NewCommonWorkflow(lgStrategyName, []steps.DecisionStep{
		lockDom0,
		forContainersSwitch,
		atWalle,
	})

	return &DecisionStrategy{
		EntryPoint: wf,
	}
}

func NewLetGoNoShutdownStrategy(
	dom0d dom0discovery.Dom0Discovery,
	mlock mlockclient.Locker,
	locker lockcluster.Locker,
	lockHolder string,
	dbm dbm.Client,
	jglr juggler.API,
	jugglerNamespaces []string,
	jgrlCfg cms_juggler.JugglerConfig,
) *DecisionStrategy {
	lockDom0 := steps.NewLockDom0Step(mlock, lockHolder)
	lockContainers := steps.NewLockContainersStep(locker, dom0d)
	letGoUnreachable := steps.NewActOnUnreachableStep(0, steps.AfterStepAtWalle, jglr, jgrlCfg.UnreachableServiceWindowMin)
	disallowNewHosts := steps.NewAllowNewHostsStep(dbm, false)
	downtimes := steps.NewSetDowntimesStep(dom0d, jglr, jugglerNamespaces)
	atWalle := steps.NewInParticularState(
		steps.AfterStepAtWalle, lgFinishMessage)

	forContainersSwitch := steps.NewHostForContainers(dbm,
		steps.NoSteps,
		steps.ListOfSteps(
			lockContainers,
			disallowNewHosts,
			downtimes,
			letGoUnreachable,
		))

	wf := NewCommonWorkflow(lgStrategyName, []steps.DecisionStep{
		lockDom0,
		forContainersSwitch,
		atWalle,
	})

	return &DecisionStrategy{
		EntryPoint: wf,
	}
}

func NewLetGoWhipPrimariesAway(
	dom0d dom0discovery.Dom0Discovery,
	mlock mlockclient.Locker,
	locker lockcluster.Locker,
	lockHolder string,
	dbm dbm.Client,
	jglr juggler.API,
	jugglerNamespaces []string,
	d deployapi.Client,
	mdb metadb.MetaDB,
) *DecisionStrategy {
	lockDom0 := steps.NewLockDom0Step(mlock, lockHolder)
	lockContainers := steps.NewLockContainersStep(locker, dom0d)
	ensureNoPrimaries := steps.NewEnsureNoPrimariesStep(d, dom0d, mdb)
	disallowNewHosts := steps.NewAllowNewHostsStep(dbm, false)
	downtimes := steps.NewSetDowntimesStep(dom0d, jglr, jugglerNamespaces)
	atWalle := steps.NewInParticularState(
		steps.AfterStepAtWalle, lgFinishMessage)

	forContainersSwitch := steps.NewHostForContainers(dbm,
		steps.NoSteps,
		steps.ListOfSteps(
			lockContainers,
			disallowNewHosts,
			downtimes,
			ensureNoPrimaries,
		))

	wf := NewCommonWorkflow(lgStrategyName, []steps.DecisionStep{
		lockDom0,
		forContainersSwitch,
		atWalle,
	})

	return &DecisionStrategy{
		EntryPoint: wf,
	}
}
