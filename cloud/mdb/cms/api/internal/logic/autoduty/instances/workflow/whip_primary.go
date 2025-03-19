package workflow

import (
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/cmsdb"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/lockcluster"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/instances/duty/config"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/instances/steps"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/shipments"
	"a.yandex-team.ru/cloud/mdb/internal/metadb"
	healthapi "a.yandex-team.ru/cloud/mdb/mdb-health/pkg/client"
	"a.yandex-team.ru/library/go/core/log"
)

func NewWhipPrimaryWorkflow(
	cmsdb cmsdb.Client,
	l log.Logger,
	locker lockcluster.Locker,
	db metadb.MetaDB,
	health healthapi.MDBHealthClient,
	shipmentsClient shipments.ShipmentClient,
	cfg config.Config,
) Workflow {
	return NewBaseWorkflow(
		cmsdb,
		l,
		"whip primary",
		[]steps.InstanceStep{
			steps.NewInstanceToFqdnConverterStep(db, cfg.IsCompute),
			steps.NewCheckIsMaster(cfg.IsCompute, health, true, cfg.EnabledMW),
			steps.NewLockAcquire(locker),
			steps.NewWaitForHealthy(health, db),
			steps.NewWhipMaster(shipmentsClient),
		},
		[]steps.InstanceStep{
			steps.NewLockRelease(locker),
		},
	)
}
