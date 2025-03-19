package workflow

import (
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/cmsdb"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/lockcluster"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/instances/duty/config"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/instances/steps"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/shipments"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/tasksclient"
	"a.yandex-team.ru/cloud/mdb/internal/fqdn"
	"a.yandex-team.ru/cloud/mdb/internal/juggler"
	"a.yandex-team.ru/cloud/mdb/internal/metadb"
	healthapi "a.yandex-team.ru/cloud/mdb/mdb-health/pkg/client"
	"a.yandex-team.ru/library/go/core/log"
)

func NewMoveInstanceWorkflow(
	cmsdb cmsdb.Client,
	l log.Logger,
	api juggler.API,
	locker lockcluster.Locker,
	client tasksclient.Client,
	db metadb.MetaDB,
	health healthapi.MDBHealthClient,
	shipmentsClient shipments.ShipmentClient,
	cfg config.Config,
	fqdnConverter fqdn.Converter,
) Workflow {
	return NewBaseWorkflow(
		cmsdb,
		l,
		"move instance",
		[]steps.InstanceStep{
			steps.NewInstanceToFqdnConverterStep(db, cfg.IsCompute),
			steps.NewCheckIsMaster(cfg.IsCompute, health, false, cfg.EnabledMW),
			steps.NewLockAcquire(locker),
			steps.NewWaitForHealthy(health, db),
			steps.NewCreateDowntimes(api, fqdnConverter),
			steps.NewWhipMaster(shipmentsClient),
			steps.NewMoveInstance(client, cfg.IsCompute),
		},
		[]steps.InstanceStep{
			steps.NewRemoveDowntimes(api),
			steps.NewLockRelease(locker),
		},
	)
}
