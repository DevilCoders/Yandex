package cluster

import (
	"context"
	"testing"
	"time"

	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/require"

	deploymock "a.yandex-team.ru/cloud/mdb/deploy/api/pkg/deployapi/mocks"
	deploymodels "a.yandex-team.ru/cloud/mdb/deploy/api/pkg/models"
	"a.yandex-team.ru/cloud/mdb/internal/encodingutil"
	"a.yandex-team.ru/cloud/mdb/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/internal/retry"
	"a.yandex-team.ru/cloud/mdb/katan/internal/tags"
	mlocklocker "a.yandex-team.ru/cloud/mdb/katan/katan/internal/cluster/locker/mocks"
	"a.yandex-team.ru/cloud/mdb/katan/katan/internal/models"
	healthapi "a.yandex-team.ru/cloud/mdb/mdb-health/pkg/client"
	healthmock "a.yandex-team.ru/cloud/mdb/mdb-health/pkg/client/mocks"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types"
	"a.yandex-team.ru/library/go/core/log/nop"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/slices"
)

func TestRoll(t *testing.T) {
	assertRollSkipped := func(err error, t *testing.T) {
		require.Error(t, err)
		require.Truef(t, xerrors.Is(err, ErrRolloutSkipped), "err should be a ErrRolloutSkipped. but it is %+v", err)
	}

	assertRollCanceled := func(err error, t *testing.T) {
		require.Error(t, err)
		require.Truef(t, xerrors.Is(err, ErrRolloutCanceled), "err should be a ErrRolloutCanceled, but it is %+v", err)
	}

	assertRollFailed := func(err error, t *testing.T) {
		require.Error(t, err)
		require.Truef(t, xerrors.Is(err, ErrRolloutFailed), "err should be a ErrRolloutFailed, but it is %+v", err)
	}

	healthC1isReplicaC2isMaster := []types.HostHealth{
		types.NewHostHealthWithStatus(
			"cid1",
			"c1",
			[]types.ServiceHealth{types.NewServiceHealth(
				"pg_replication",
				time.Now(),
				types.ServiceStatusAlive,
				types.ServiceRoleReplica,
				types.ServiceReplicaTypeSync,
				"",
				0,
				nil,
			)},
			types.HostStatusAlive,
		),
		types.NewHostHealthWithStatus(
			"cid1",
			"c2",
			[]types.ServiceHealth{types.NewServiceHealth(
				"pg_replication",
				time.Now(),
				types.ServiceStatusAlive,
				types.ServiceRoleMaster,
				types.ServiceReplicaTypeUnknown,
				"",
				0,
				nil,
			)},
			types.HostStatusAlive,
		),
	}
	cfg := DefaultConfig()
	cfg.AllowedDeployGroups = []string{"default"}
	cfg.HealthCheckInterval = encodingutil.FromDuration(time.Nanosecond)
	cfg.HealthCheckMaxWait = encodingutil.FromDuration(time.Second * 5)
	cfg.HealthMeasureCount = 2
	cfg.Locker.CheckAttempts = 1
	cfg.Locker.CheckInterval = encodingutil.FromDuration(time.Nanosecond)
	cfg.DeployMaxRetries = 1
	cfg.WaitAfterDeploy = encodingutil.FromDuration(time.Nanosecond)
	cfg.ShipmentCheckInterval = encodingutil.FromDuration(time.Nanosecond)
	genClusterHealth := func(status healthapi.ClusterStatus) func(_ context.Context, cid string) (healthapi.ClusterHealth, error) {
		var cnt int64 = 0
		now, _ := time.Parse(time.RFC822, time.RFC822)
		return func(_ context.Context, cid string) (healthapi.ClusterHealth, error) {
			cnt++
			return healthapi.ClusterHealth{
				Status:    status,
				Timestamp: now.Add(time.Duration(cnt) * time.Minute),
			}, nil
		}
	}
	pgCluster := models.Cluster{
		ID: "cid1",
		Tags: tags.ClusterTags{Meta: tags.ClusterMetaTags{
			Type: metadb.PostgresqlCluster,
		}},
		Hosts: map[string]tags.HostTags{
			"c1": {},
			"c2": {},
		},
	}
	hsCommands := []deploymodels.CommandDef{{Type: "hs"}}

	cases := []struct {
		name   string
		setup  func(*deploymock.MockClient, *healthmock.MockMDBHealthClient, *mlocklocker.MockLocker)
		assert func(error, *testing.T)
	}{
		{
			"skip cluster with hosts on personal",
			func(dep *deploymock.MockClient, hel *healthmock.MockMDBHealthClient, mlocker *mlocklocker.MockLocker) {
				dep.EXPECT().
					GetMinion(gomock.Any(), gomock.Any()).
					Return(deploymodels.Minion{Group: "personal"}, nil)
			},
			assertRollSkipped,
		},
		{
			"skip cluster if failed to get it's minions",
			func(dep *deploymock.MockClient, hel *healthmock.MockMDBHealthClient, mlocker *mlocklocker.MockLocker) {
				dep.EXPECT().
					GetMinion(gomock.Any(), gomock.Any()).
					AnyTimes().
					Return(deploymodels.Minion{}, xerrors.New("something bad happens"))
			},
			assertRollSkipped,
		},
		{
			"skip cluster if it unhealthy",
			func(dep *deploymock.MockClient, hel *healthmock.MockMDBHealthClient, mlocker *mlocklocker.MockLocker) {
				dep.EXPECT().
					GetMinion(gomock.Any(), gomock.Any()).
					AnyTimes().
					Return(deploymodels.Minion{Group: "default"}, nil)
				hel.EXPECT().
					GetClusterHealth(gomock.Any(), gomock.Any()).
					DoAndReturn(genClusterHealth(healthapi.ClusterStatusDegraded)).
					AnyTimes()
			},
			assertRollSkipped,
		},
		{
			"skip cluster if failed to get hosts health",
			func(dep *deploymock.MockClient, hel *healthmock.MockMDBHealthClient, mlocker *mlocklocker.MockLocker) {
				dep.EXPECT().
					GetMinion(gomock.Any(), gomock.Any()).
					AnyTimes().
					Return(deploymodels.Minion{Group: "default"}, nil)
				hel.EXPECT().
					GetClusterHealth(gomock.Any(), gomock.Any()).
					DoAndReturn(genClusterHealth(healthapi.ClusterStatusAlive)).
					Times(cfg.HealthMeasureCount)
				hel.EXPECT().
					GetHostsHealth(gomock.Any(), gomock.Any()).
					Return(nil, xerrors.New("test deploy error"))
			},
			assertRollSkipped,
		},
		{

			"skip cluster if got less then expected hosts health",
			func(dep *deploymock.MockClient, hel *healthmock.MockMDBHealthClient, mlocker *mlocklocker.MockLocker) {
				dep.EXPECT().
					GetMinion(gomock.Any(), gomock.Any()).
					AnyTimes().
					Return(deploymodels.Minion{Group: "default"}, nil)
				hel.EXPECT().
					GetClusterHealth(gomock.Any(), gomock.Any()).
					DoAndReturn(genClusterHealth(healthapi.ClusterStatusAlive)).
					Times(cfg.HealthMeasureCount)
				hel.EXPECT().
					GetHostsHealth(gomock.Any(), gomock.Any()).
					Return(nil, nil)
			},
			assertRollSkipped,
		},
		{
			"skip cluster if it locked",
			func(dep *deploymock.MockClient, hel *healthmock.MockMDBHealthClient, mlocker *mlocklocker.MockLocker) {
				dep.EXPECT().
					GetMinion(gomock.Any(), gomock.Any()).
					AnyTimes().
					Return(deploymodels.Minion{Group: "default"}, nil)
				hel.EXPECT().
					GetClusterHealth(gomock.Any(), gomock.Any()).
					DoAndReturn(genClusterHealth(healthapi.ClusterStatusAlive)).
					AnyTimes()
				hel.EXPECT().
					GetHostsHealth(gomock.Any(), gomock.Any()).
					Return(healthC1isReplicaC2isMaster, nil)
				mlocker.EXPECT().
					Acquire(gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any()).
					Return(xerrors.New("it's locked!"))
			},
			assertRollSkipped,
		},
		{
			"cancel cluster if second node is locked",
			func(dep *deploymock.MockClient, hel *healthmock.MockMDBHealthClient, mlocker *mlocklocker.MockLocker) {
				dep.EXPECT().
					GetMinion(gomock.Any(), gomock.Any()).
					AnyTimes().
					Return(deploymodels.Minion{Group: "default"}, nil)
				dep.EXPECT().
					CreateShipment(gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any()).
					Return(deploymodels.Shipment{}, nil)
				dep.EXPECT().
					GetShipment(gomock.Any(), gomock.Any()).
					Return(deploymodels.Shipment{Status: deploymodels.ShipmentStatusDone}, nil)
				hel.EXPECT().
					GetClusterHealth(gomock.Any(), gomock.Any()).
					DoAndReturn(genClusterHealth(healthapi.ClusterStatusAlive)).
					AnyTimes()
				hel.EXPECT().
					GetHostsHealth(gomock.Any(), gomock.Any()).
					Return(healthC1isReplicaC2isMaster, nil)
				mlocker.EXPECT().
					Acquire(gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any()).
					DoAndReturn(func(_, _ interface{}, fqdns []string, _ interface{}) error {
						if slices.ContainsString(fqdns, "c2") {
							return xerrors.New("rolling on master. Should stop here")
						}
						return nil
					}).
					Times(2)
				mlocker.EXPECT().
					Release(gomock.Any(), gomock.Any()).
					Return(nil).
					AnyTimes()
			},
			assertRollCanceled,
		},
		{
			"fail cluster if shipment failed",
			func(dep *deploymock.MockClient, hel *healthmock.MockMDBHealthClient, mlocker *mlocklocker.MockLocker) {
				dep.EXPECT().
					GetMinion(gomock.Any(), gomock.Any()).
					AnyTimes().
					Return(deploymodels.Minion{Group: "default"}, nil)
				dep.EXPECT().
					CreateShipment(gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any()).
					Return(deploymodels.Shipment{}, nil)
				dep.EXPECT().
					GetShipment(gomock.Any(), gomock.Any()).
					Return(deploymodels.Shipment{Status: deploymodels.ShipmentStatusError}, nil)
				hel.EXPECT().
					GetClusterHealth(gomock.Any(), gomock.Any()).
					DoAndReturn(genClusterHealth(healthapi.ClusterStatusAlive)).
					AnyTimes()
				hel.EXPECT().
					GetHostsHealth(gomock.Any(), gomock.Any()).
					Return(healthC1isReplicaC2isMaster, nil)
				mlocker.EXPECT().
					Acquire(gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any()).
					Return(nil)
				mlocker.EXPECT().
					Release(gomock.Any(), gomock.Any()).
					Return(nil)
			},
			assertRollFailed,
		},
	}
	for _, c := range cases {
		t.Run(c.name, func(t *testing.T) {
			mockCtrl := gomock.NewController(t)
			defer mockCtrl.Finish()
			dep := deploymock.NewMockClient(mockCtrl)
			hel := healthmock.NewMockMDBHealthClient(mockCtrl)
			mlocker := mlocklocker.NewMockLocker(mockCtrl)
			c.setup(dep, hel, mlocker)

			rr := &roller{
				onCluster:  func(change models.ClusterChange) {},
				onShipment: func(shipment models.Shipment) {},
				deploy:     dep,
				health:     hel,
				locker:     mlocker,
				lg:         &nop.Logger{},
				juggler:    nil,
				config:     cfg,
				healthRetry: retry.New(retry.Config{
					MaxRetries: cfg.HealthMaxRetries,
				}),
				deployRetry: retry.New(retry.Config{
					MaxRetries: cfg.DeployMaxRetries,
				}),
			}

			err := rr.Roll(
				context.Background(),
				42,
				pgCluster,
				hsCommands,
				RollOptions{},
			)

			c.assert(err, t)
		})
	}

	t.Run("call locker.Release on panic", func(t *testing.T) {
		mockCtrl := gomock.NewController(t)
		defer mockCtrl.Finish()
		dep := deploymock.NewMockClient(mockCtrl)
		hel := healthmock.NewMockMDBHealthClient(mockCtrl)
		mlocker := mlocklocker.NewMockLocker(mockCtrl)

		dep.EXPECT().
			GetMinion(gomock.Any(), gomock.Any()).
			AnyTimes().
			Return(deploymodels.Minion{Group: "default"}, nil)
		dep.EXPECT().
			CreateShipment(gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any()).
			Return(deploymodels.Shipment{}, nil)
		hel.EXPECT().
			GetClusterHealth(gomock.Any(), gomock.Any()).
			DoAndReturn(genClusterHealth(healthapi.ClusterStatusAlive)).
			AnyTimes()
		hel.EXPECT().
			GetHostsHealth(gomock.Any(), gomock.Any()).
			Return(healthC1isReplicaC2isMaster, nil)

		mlocker.EXPECT().
			Acquire(gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any()).
			Return(nil)
		mlocker.EXPECT().
			Release(gomock.Any(), gomock.Any()).
			Return(nil)

		rr := &roller{
			onCluster: func(change models.ClusterChange) {},
			onShipment: func(shipment models.Shipment) {
				panic(xerrors.New("something bad happens"))
			},
			deploy:  dep,
			health:  hel,
			locker:  mlocker,
			lg:      &nop.Logger{},
			juggler: nil,
			config:  cfg,
			healthRetry: retry.New(retry.Config{
				MaxRetries: cfg.HealthMaxRetries,
			}),
			deployRetry: retry.New(retry.Config{
				MaxRetries: cfg.DeployMaxRetries,
			}),
		}

		require.Panics(
			t,
			func() {
				_ = rr.Roll(
					context.Background(),
					42,
					pgCluster,
					hsCommands,
					RollOptions{},
				)
			},
		)
	})

	t.Run("don't call planner and ask for hosts-health for one node cluster", func(t *testing.T) {
		mockCtrl := gomock.NewController(t)
		defer mockCtrl.Finish()
		dep := deploymock.NewMockClient(mockCtrl)
		hel := healthmock.NewMockMDBHealthClient(mockCtrl)
		mlocker := mlocklocker.NewMockLocker(mockCtrl)

		dep.EXPECT().
			GetMinion(gomock.Any(), gomock.Any()).
			AnyTimes().
			Return(deploymodels.Minion{Group: "default"}, nil)
		hel.EXPECT().
			GetClusterHealth(gomock.Any(), gomock.Any()).
			DoAndReturn(genClusterHealth(healthapi.ClusterStatusAlive)).
			AnyTimes()

		mlocker.EXPECT().
			Acquire(gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any()).
			Return(xerrors.New("Can't get a lock"))

		rr := &roller{
			onCluster: func(change models.ClusterChange) {},
			onShipment: func(shipment models.Shipment) {
				panic(xerrors.New("something bad happens"))
			},
			deploy:  dep,
			health:  hel,
			locker:  mlocker,
			lg:      &nop.Logger{},
			juggler: nil,
			config:  cfg,
			healthRetry: retry.New(retry.Config{
				MaxRetries: cfg.HealthMaxRetries,
			}),
			deployRetry: retry.New(retry.Config{
				MaxRetries: cfg.DeployMaxRetries,
			}),
		}

		_ = rr.Roll(
			context.Background(),
			42,
			models.Cluster{
				ID: "cid1",
				Tags: tags.ClusterTags{Meta: tags.ClusterMetaTags{
					Type: metadb.MongodbCluster,
				}},
				Hosts: map[string]tags.HostTags{
					"c1": {},
				},
			},
			hsCommands,
			RollOptions{},
		)
		// here we check that we don't request host health,
		// that is why there are no additional assertions here
	})
}

func TestUnmarshalRollOptions(t *testing.T) {
	t.Run("empty json OK and return defaults", func(t *testing.T) {
		ret, err := UnmarshalRollOptions("{}")
		require.NoError(t, err)
		require.Equal(t, ret, RollOptions{ShipmentRetries: 2})
	})

	t.Run("options with shipment_retries", func(t *testing.T) {
		ret, err := UnmarshalRollOptions("{\"shipment_retries\": 42}")
		require.NoError(t, err)
		require.Equal(t, ret, RollOptions{ShipmentRetries: 42})
	})

	t.Run("invalid json", func(t *testing.T) {
		_, err := UnmarshalRollOptions("<foo/>")
		require.Error(t, err)
	})

	t.Run("unknown options", func(t *testing.T) {
		_, err := UnmarshalRollOptions("{\"wat\": 42}")
		require.Error(t, err)
	})
}
