package functest

import (
	"context"
	"time"

	"github.com/golang/mock/gomock"

	mdbpg "a.yandex-team.ru/cloud/mdb/backup/internal/metadb/pg"
	"a.yandex-team.ru/cloud/mdb/backup/worker/pkg/app"
	deploymock "a.yandex-team.ru/cloud/mdb/deploy/api/pkg/deployapi/mocks"
	deploymodels "a.yandex-team.ru/cloud/mdb/deploy/api/pkg/models"
	"a.yandex-team.ru/cloud/mdb/internal/ready"
	s3steps "a.yandex-team.ru/cloud/mdb/internal/s3/steps"
	healthmock "a.yandex-team.ru/cloud/mdb/mdb-health/pkg/client/mocks"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type WorkerContext struct {
	HostsServiceHealth types.ServiceStatus

	ShipmentsCreationSucceeds bool
	nextShipmentID            int64
	ShipmentGetStatus         deploymodels.ShipmentStatus
}

func (wc *WorkerContext) Reset() {
	wc.HostsServiceHealth = types.ServiceStatusAlive
	wc.ShipmentsCreationSucceeds = true
	wc.ShipmentGetStatus = deploymodels.ShipmentStatusUnknown
	wc.nextShipmentID = 0
}

func (wc *WorkerContext) AllShipmentsCreationFails() {
	wc.ShipmentsCreationSucceeds = false
}

func (wc *WorkerContext) AllShipmentsGetStatus(status string) {
	wc.ShipmentGetStatus = deploymodels.ParseShipmentStatus(status)
}

func (wc *WorkerContext) AllHostsServiceHealthIs(status string) error {
	var ss types.ServiceStatus
	if err := ss.UnmarshalText([]byte(status)); err != nil {
		return xerrors.Errorf("malformed service health status: %w", err)
	}
	wc.HostsServiceHealth = ss

	return nil
}

func (wc *WorkerContext) RunWorker(ctx context.Context, s3ctl *s3steps.S3Ctl, lg log.Logger, cfg app.Config) (rerr error) {
	mockCtrl := gomock.NewController(lg)
	defer mockCtrl.Finish()

	s3client, err := s3ctl.Mock(mockCtrl)
	if err != nil {
		return err
	}
	deploy := deploymock.NewMockClient(mockCtrl)
	health := healthmock.NewMockMDBHealthClient(mockCtrl)

	fillClusterAddr(metadbClusterName, &cfg.Metadb)
	mdb, err := mdbpg.New(cfg.Metadb, log.With(lg, log.String("cluster", mdbpg.DBName)))
	if err != nil {
		return err
	}

	deploy.EXPECT().
		CreateShipment(gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any()).
		AnyTimes().
		DoAndReturn(func(_, _, _, _, _, _ interface{}) (deploymodels.Shipment, error) {
			if !wc.ShipmentsCreationSucceeds {
				return deploymodels.Shipment{}, xerrors.Errorf("shipment creation failed")
			}
			wc.nextShipmentID++
			return deploymodels.Shipment{ID: deploymodels.ShipmentID(wc.nextShipmentID)}, nil
		})

	deploy.EXPECT().
		GetShipment(gomock.Any(), gomock.Any()).
		AnyTimes().
		Return(
			deploymodels.Shipment{
				ID:     1,
				FQDNs:  []string{"fqdn"},
				Status: wc.ShipmentGetStatus,
			}, nil)

	health.EXPECT().
		GetHostsHealth(gomock.Any(), gomock.Any()).
		AnyTimes().
		DoAndReturn(func(_ context.Context, fqdns []string) ([]types.HostHealth, error) {
			var ret []types.HostHealth
			for i, f := range fqdns {
				role := types.ServiceRoleMaster
				if i > 0 {
					role = types.ServiceRoleReplica
				}
				repl := types.NewServiceHealth("service_health", time.Now(), wc.HostsServiceHealth, role, types.ServiceReplicaTypeUnknown, "", 0, nil)
				ret = append(ret, types.NewHostHealth(
					"cid",
					f,
					[]types.ServiceHealth{repl},
				))
			}
			return ret, nil
		})

	lg.Debugf("starting worker with config: %+v", cfg)
	worker, err := app.NewAppFromExtDeps(cfg, mdb, deploy, health, s3client, lg)
	if err != nil {
		return err
	}
	awaitCtx, cancel := context.WithTimeout(ctx, 15*time.Second)
	defer cancel()
	if err = ready.Wait(awaitCtx, worker, &ready.DefaultErrorTester{Name: "worker", L: lg}, time.Second); err != nil {
		return xerrors.Errorf("failed to wait backend: %w", err)
	}

	return worker.Run(ctx)
}
