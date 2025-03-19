package instructions

import (
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/dom0discovery"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/lockcluster"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/steps"
	"a.yandex-team.ru/cloud/mdb/internal/dbm"
	"a.yandex-team.ru/cloud/mdb/mlock/pkg/mlockclient"
)

func NewCleanupStrategy(
	mlock mlockclient.Locker,
	locker lockcluster.Locker,
	dom0d dom0discovery.Dom0Discovery,
	lockHolder string,
	dbm dbm.Client,
) *DecisionStrategy {
	unlockDom0 := steps.NewUnlockDom0Step(mlock, lockHolder)
	unlockContainers := steps.NewUnlockClusterNodesStep(locker, dom0d)
	allowNewHosts := steps.NewAllowNewHostsStep(dbm, true)
	finish := steps.NewInParticularState(steps.AfterStepNext, "clean finished")

	forContainersSwitch := steps.NewHostForContainers(dbm,
		steps.NoSteps,
		steps.ListOfSteps(
			allowNewHosts,
		))

	wf := NewCommonWorkflow(
		"cleanup",
		[]steps.DecisionStep{
			forContainersSwitch,
			unlockContainers,
			unlockDom0,
			finish,
		})

	return &DecisionStrategy{
		EntryPoint: wf,
	}
}
