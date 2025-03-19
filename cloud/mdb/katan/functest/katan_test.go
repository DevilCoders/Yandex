package functest

import (
	"context"
	"time"

	"github.com/golang/mock/gomock"

	deploymock "a.yandex-team.ru/cloud/mdb/deploy/api/pkg/deployapi/mocks"
	"a.yandex-team.ru/cloud/mdb/deploy/api/pkg/models"
	"a.yandex-team.ru/cloud/mdb/internal/app"
	"a.yandex-team.ru/cloud/mdb/internal/juggler"
	jugglermock "a.yandex-team.ru/cloud/mdb/internal/juggler/mocks"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	kdbpg "a.yandex-team.ru/cloud/mdb/katan/internal/katandb/pg"
	katanapp "a.yandex-team.ru/cloud/mdb/katan/katan/pkg/app"
	healthapi "a.yandex-team.ru/cloud/mdb/mdb-health/pkg/client"
	healthmock "a.yandex-team.ru/cloud/mdb/mdb-health/pkg/client/mocks"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types"
	"a.yandex-team.ru/cloud/mdb/mlock/pkg/mlockclient"
	mlockmock "a.yandex-team.ru/cloud/mdb/mlock/pkg/mlockclient/mocks"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

const testKatanName = "test-katan"

type KatanContext struct {
	JugglerStatusBeforeRollout string
	JugglerStatusAfterRollout  string
	ClusterHealthBeforeRollout string
	ClusterHealthAfterRollout  string
	FirstShipmentStatus        *models.ShipmentStatus
	ShipmentsStatus            models.ShipmentStatus
	DeployGroup                string
	nextShipmentID             int64
	mockCtrl                   *gomock.Controller
}

func (kc *KatanContext) Reset(lg log.Logger) {
	kc.JugglerStatusBeforeRollout = "OK"
	kc.JugglerStatusAfterRollout = "OK"
	kc.ClusterHealthBeforeRollout = "Alive"
	kc.ClusterHealthAfterRollout = "Alive"
	kc.DeployGroup = "default"
	kc.ShipmentsStatus = models.ShipmentStatusDone
	kc.FirstShipmentStatus = nil

	if kc.mockCtrl != nil {
		kc.mockCtrl.Finish()
	}

	kc.mockCtrl = gomock.NewController(lg)
}

func (kc *KatanContext) AllShipmentsAre(status string) error {
	var ss models.ShipmentStatus
	if err := ss.UnmarshalText([]byte(status)); err != nil {
		return xerrors.Errorf("malformed shipment status: %w", err)
	}
	kc.ShipmentsStatus = ss
	return nil
}

func (kc *KatanContext) FirstShipmentAre(status string) error {
	var ss models.ShipmentStatus
	if err := ss.UnmarshalText([]byte(status)); err != nil {
		return xerrors.Errorf("malformed shipment status: %w", err)
	}
	kc.FirstShipmentStatus = &ss
	return nil
}

func (kc *KatanContext) RunKatan(ctx context.Context, kCluster *sqlutil.Cluster, lg log.Logger) error {
	dep := deploymock.NewMockClient(kc.mockCtrl)
	jug := jugglermock.NewMockAPI(kc.mockCtrl)
	hel := healthmock.NewMockMDBHealthClient(kc.mockCtrl)
	mlock := mlockmock.NewMockLocker(kc.mockCtrl)

	kdb := kdbpg.NewWithCluster(kCluster, log.With(lg, log.String("role", "katan")))
	var rolloutStarted bool

	jug.EXPECT().RawEvents(ctx, gomock.Any(), gomock.Any()).Return([]juggler.RawEvent{{ReceivedTime: time.Now()}}, nil).
		AnyTimes().
		DoAndReturn(func(_ context.Context, arg1, arg2 string) ([]juggler.RawEvent, error) {
			ev := juggler.RawEvent{
				Status:       kc.JugglerStatusBeforeRollout,
				Service:      arg1,
				Host:         arg2,
				ReceivedTime: time.Now(),
			}
			if rolloutStarted {
				ev.Status = kc.JugglerStatusAfterRollout
			}

			return []juggler.RawEvent{ev}, nil
		})

	dep.EXPECT().
		GetMinion(gomock.Any(), gomock.Any()).
		AnyTimes().
		Return(models.Minion{Group: kc.DeployGroup}, nil)

	dep.EXPECT().
		CreateShipment(gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any()).
		AnyTimes().
		DoAndReturn(func(_, _, _, _, _, _ interface{}) (models.Shipment, error) {
			rolloutStarted = true
			kc.nextShipmentID++
			return models.Shipment{ID: models.ShipmentID(kc.nextShipmentID)}, nil
		})

	var gotFirstShipment bool
	dep.EXPECT().
		GetShipment(gomock.Any(), gomock.Any()).
		Return(models.Shipment{Status: kc.ShipmentsStatus}, nil).
		AnyTimes().
		DoAndReturn(func(_, _ interface{}) (models.Shipment, error) {
			ret := models.Shipment{Status: kc.ShipmentsStatus}
			if kc.FirstShipmentStatus != nil && !gotFirstShipment {
				ret.Status = *kc.FirstShipmentStatus
			}
			gotFirstShipment = true
			return ret, nil
		})

	hel.EXPECT().
		GetHostsHealth(gomock.Any(), gomock.Any()).
		AnyTimes().
		DoAndReturn(func(_ context.Context, fqdns []string) ([]types.HostHealth, error) {
			var ret []types.HostHealth
			for i, f := range fqdns {
				role := types.ServiceRoleMaster
				if i > 0 {
					role = types.ServiceRoleReplica
				}
				repl := types.NewServiceHealth("pg_replication", time.Now(), types.ServiceStatusAlive, role, types.ServiceReplicaTypeUnknown, "", 0, nil)
				ret = append(ret, types.NewHostHealth(
					"cid",
					f,
					[]types.ServiceHealth{repl},
				))
			}
			return ret, nil
		})

	var cnt int64 = 0
	now, _ := time.Parse(time.RFC822, time.RFC822)
	hel.EXPECT().
		GetClusterHealth(gomock.Any(), gomock.Any()).
		AnyTimes().
		DoAndReturn(func(_, _ interface{}) (healthapi.ClusterHealth, error) {
			cnt++
			clusterHealth := healthapi.ClusterHealth{
				Status:             healthapi.ClusterStatus(kc.ClusterHealthBeforeRollout),
				Timestamp:          now.Add(time.Duration(cnt) * time.Minute),
				LastAliveTimestamp: time.Now(),
			}
			if rolloutStarted {
				clusterHealth.Status = healthapi.ClusterStatus(kc.ClusterHealthAfterRollout)
			}
			return clusterHealth, nil
		})

	mlock.EXPECT().
		CreateLock(gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any()).
		AnyTimes().
		Return(nil)

	mlock.EXPECT().
		GetLockStatus(gomock.Any(), gomock.Any()).
		AnyTimes().
		Return(mlockclient.LockStatus{Acquired: true}, nil)

	mlock.EXPECT().
		ReleaseLock(gomock.Any(), gomock.Any()).
		AnyTimes().
		Return(nil)

	config := katanapp.DefaultConfig()
	// don't need to sleep, cause here we works we mocks
	config.Katan.Roller.Rollout.WaitAfterDeploy.Duration = time.Millisecond
	config.Katan.Roller.Rollout.ShipmentCheckInterval.Duration = time.Millisecond
	config.Katan.Roller.Rollout.HealthCheckMaxWait.Duration = time.Second * 5
	config.Katan.Roller.Rollout.HealthCheckInterval.Duration = time.Millisecond
	config.Katan.Roller.Rollout.HealthMeasureCount = 1
	config.Katan.MaxRollouts = 1
	config.Katan.MaxRetries = 1
	config.Katan.Name = testKatanName
	config.Katan.PendingInterval.Duration = time.Millisecond
	config.Katan.Roller.Rollout.AllowedDeployGroups = []string{"default"}

	baseApp, err := app.New(
		app.WithLoggerConstructor(func(_ app.LoggingConfig) (log.Logger, error) {
			return lg, nil
		}),
	)
	if err != nil {
		return err
	}
	katApp, err := katanapp.NewAppCustom(ctx, config, kdb, dep, jug, hel, mlock, baseApp)
	if err != nil {
		return err
	}

	katApp.Run(ctx)
	return nil
}
