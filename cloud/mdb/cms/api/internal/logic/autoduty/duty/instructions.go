package duty

import (
	"a.yandex-team.ru/cloud/mdb/cms/api/generated/swagger/models"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/dom0discovery"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/healthiness"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/lockcluster"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/instructions"
	"a.yandex-team.ru/cloud/mdb/deploy/api/pkg/deployapi"
	"a.yandex-team.ru/cloud/mdb/internal/conductor"
	"a.yandex-team.ru/cloud/mdb/internal/dbm"
	"a.yandex-team.ru/cloud/mdb/internal/juggler"
	"a.yandex-team.ru/cloud/mdb/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/client"
	"a.yandex-team.ru/cloud/mdb/mlock/pkg/mlockclient"
)

func newAnalysisInstructions(
	jugglerHealth healthiness.Healthiness,
	dom0d dom0discovery.Dom0Discovery,
	jgrl juggler.API,
	dbm dbm.Client,
	cncl conductor.Client,
	hlthcl client.MDBHealthClient,
	cfg CmsDom0DutyConfig,
	mdb metadb.MetaDB,
) *instructions.Instructions {
	maybeDataLoss := instructions.NewAnalysePossibleDataLossStrategy(jugglerHealth, dom0d, jgrl, dbm, cncl, hlthcl, cfg.PgIntTestAllowAtOnce, cfg.PgIntTestCGroup, mdb, cfg.Steps.Drills, cfg.Juggler)
	unconditionalOK := instructions.NewUnconditionalOKAnalyse()
	explicit := map[string]*instructions.DecisionStrategy{
		models.ManagementRequestActionPrepare:  unconditionalOK,
		models.ManagementRequestActionProfile:  maybeDataLoss,
		models.ManagementRequestActionRedeploy: maybeDataLoss,
	}
	return &instructions.Instructions{
		Default:  instructions.NewAnalyseDefaultStrategy(jugglerHealth, dom0d, jgrl, dbm, cncl, hlthcl, cfg.PgIntTestAllowAtOnce, cfg.PgIntTestCGroup, mdb, cfg.Steps.Drills, cfg.Juggler),
		Explicit: explicit,
	}
}

func newLetGoInstructions(
	cnd conductor.Client,
	dom0d dom0discovery.Dom0Discovery,
	mlock mlockclient.Locker,
	locker lockcluster.Locker,
	lockHolder string,
	dbm dbm.Client,
	jglr juggler.API,
	d deployapi.Client,
	mdb metadb.MetaDB,
	cfg CmsDom0DutyConfig,
) *instructions.Instructions {
	newHost := instructions.NewLetGoNewHostStrategy(d, cfg.Steps.RegisterMinion, cnd, cfg.Steps.NewHosts)
	withoutShutdown := instructions.NewLetGoNoShutdownStrategy(dom0d, mlock, locker, lockHolder, dbm, jglr, cfg.JugglerNamespaces, cfg.Juggler)
	redeploy := instructions.NewLetGoRedeployStrategy(dom0d, mlock, locker, lockHolder, dbm, jglr, d, cfg.Steps.Shipments, cfg.JugglerNamespaces, cfg.Juggler)
	noPrimary := instructions.NewLetGoWhipPrimariesAway(
		dom0d, mlock, locker, lockHolder, dbm, jglr, cfg.JugglerNamespaces, d, mdb)
	explicit := map[string]*instructions.DecisionStrategy{
		models.ManagementRequestActionPrepare:                  newHost,
		models.ManagementRequestActionChangeDashDisk:           withoutShutdown,
		models.ManagementRequestActionRedeploy:                 redeploy,
		models.ManagementRequestActionTemporaryDashUnreachable: noPrimary,
	}
	return &instructions.Instructions{
		Default:  instructions.NewLetGoStrategy(dom0d, mlock, locker, lockHolder, dbm, jglr, d, cfg.Steps.Shipments, cfg.JugglerNamespaces, cfg.MutatingDom0Limit, cfg.Juggler),
		Explicit: explicit,
	}
}

func newToReturnInstructions(jglr juggler.API, dom0d dom0discovery.Dom0Discovery, d deployapi.Client) *instructions.Instructions {
	return &instructions.Instructions{
		Default:  instructions.NewToReturnFromWalleStrategy(jglr, dom0d, d),
		Explicit: map[string]*instructions.DecisionStrategy{},
	}
}

func newAfterWalleInstruction(
	dom0d dom0discovery.Dom0Discovery,
	mdb metadb.MetaDB,
	dbm dbm.Client,
	d deployapi.Client,
	jglr juggler.API,
	cfg CmsDom0DutyConfig,
) *instructions.Instructions {
	prepareHost := instructions.NewAfterPrepareStrategy()
	withoutShutdown := instructions.NewAfterWalleReturnedNoShutdownStrategy()
	explicit := map[string]*instructions.DecisionStrategy{
		models.ManagementRequestActionPrepare:                  prepareHost,
		models.ManagementRequestActionChangeDashDisk:           withoutShutdown,
		models.ManagementRequestActionTemporaryDashUnreachable: withoutShutdown,
	}
	return &instructions.Instructions{
		Default:  instructions.NewAfterWalleReturnedStrategy(dom0d, mdb, dbm, d, jglr, cfg.Steps.Shipments, cfg.Juggler),
		Explicit: explicit,
	}
}

func newCleanupInstruction(
	mlock mlockclient.Locker,
	locker lockcluster.Locker,
	dom0d dom0discovery.Dom0Discovery,
	lockHolder string,
	dbm dbm.Client,
) *instructions.Instructions {
	return &instructions.Instructions{
		Default:  instructions.NewCleanupStrategy(mlock, locker, dom0d, lockHolder, dbm),
		Explicit: map[string]*instructions.DecisionStrategy{},
	}
}
