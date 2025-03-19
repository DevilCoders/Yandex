package duty

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/cmsdb"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/dom0discovery"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/healthiness"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/lockcluster"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/instructions"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/statemachine"
	"a.yandex-team.ru/cloud/mdb/deploy/api/pkg/deployapi"
	"a.yandex-team.ru/cloud/mdb/internal/conductor"
	"a.yandex-team.ru/cloud/mdb/internal/dbm"
	"a.yandex-team.ru/cloud/mdb/internal/juggler"
	"a.yandex-team.ru/cloud/mdb/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/internal/ready"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/client"
	"a.yandex-team.ru/cloud/mdb/mlock/pkg/mlockclient"
	"a.yandex-team.ru/library/go/core/log"
)

type AutoDuty struct {
	ready.Checker
	log        log.Logger
	cmsdb      cmsdb.Client
	sm         statemachine.StateMachine
	analysis   *instructions.Instructions
	letGo      *instructions.Instructions
	toReturn   *instructions.Instructions
	afterWalle *instructions.Instructions
	cleanup    *instructions.Instructions
	mustReview bool

	maxSimultaneousLetGoRequests int
}

func (ad *AutoDuty) IsReady(ctx context.Context) error {
	return ad.cmsdb.IsReady(ctx)
}

func NewCustomDuty(
	logger log.Logger,
	cmsdb cmsdb.Client,
	an *instructions.Instructions,
	lg *instructions.Instructions,
	tr *instructions.Instructions,
	fin *instructions.Instructions,
	cleanup *instructions.Instructions,
	mustReview bool,
	maxSimultaneousLetGoRequests int,
) AutoDuty {
	return AutoDuty{
		log:        logger,
		cmsdb:      cmsdb,
		analysis:   an,
		letGo:      lg,
		toReturn:   tr,
		afterWalle: fin,
		cleanup:    cleanup,
		sm:         statemachine.NewStateMachine(logger, cmsdb),
		mustReview: mustReview,

		maxSimultaneousLetGoRequests: maxSimultaneousLetGoRequests,
	}
}

func NewDuty(
	jugglerHealth healthiness.Healthiness,
	dom0d dom0discovery.Dom0Discovery,
	logger log.Logger,
	mlock mlockclient.Locker,
	locker lockcluster.Locker,
	appName string,
	cmsdb cmsdb.Client,
	dbm dbm.Client,
	d deployapi.Client,
	jglr juggler.API,
	cncl conductor.Client,
	hlthcl client.MDBHealthClient,
	mdb metadb.MetaDB,
	cfg CmsDom0DutyConfig,
) AutoDuty {
	return NewCustomDuty(
		logger,
		cmsdb,
		newAnalysisInstructions(jugglerHealth, dom0d, jglr, dbm, cncl, hlthcl, cfg, mdb),
		newLetGoInstructions(cncl, dom0d, mlock, locker, appName, dbm, jglr, d, mdb, cfg),
		newToReturnInstructions(jglr, dom0d, d),
		newAfterWalleInstruction(dom0d, mdb, dbm, d, jglr, cfg),
		newCleanupInstruction(mlock, locker, dom0d, appName, dbm),
		!cfg.RespectADDecision,
		cfg.MutatingDom0Limit,
	)
}
