package katan

import (
	"context"
	"fmt"
	"os"
	"time"

	"a.yandex-team.ru/cloud/mdb/deploy/api/pkg/deployapi"
	"a.yandex-team.ru/cloud/mdb/internal/encodingutil"
	jugglerapi "a.yandex-team.ru/cloud/mdb/internal/juggler"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/retry"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/internal/sentry"
	"a.yandex-team.ru/cloud/mdb/katan/internal/katandb"
	"a.yandex-team.ru/cloud/mdb/katan/katan/internal/cluster"
	"a.yandex-team.ru/cloud/mdb/katan/katan/internal/roller"
	healthapi "a.yandex-team.ru/cloud/mdb/mdb-health/pkg/client"
	"a.yandex-team.ru/cloud/mdb/mlock/pkg/mlockclient"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

// Config ...
type Config struct {
	Roller          roller.Config         `json:"roller" yaml:"roller"`
	PendingInterval encodingutil.Duration `json:"pending_interval" yaml:"pending_interval"`
	Parallel        int32                 `json:"parallel" yaml:"parallel"`
	MaxRollouts     int32                 `json:"max_rollouts" yaml:"max_rollouts"`
	MaxRetries      int32                 `json:"max_retries" yaml:"max_retries"`
	Name            string                `json:"name" yaml:"name"`
}

func DefaultConfig() Config {
	return Config{
		Roller:          roller.DefaultConfig(),
		PendingInterval: encodingutil.FromDuration(time.Second * 10),
		Parallel:        42,
		MaxRetries:      42,
	}
}

// Katan run rollouts
// Right now it monitor pending rollouts and spawn new Roller for it
//
// In the future, it should:
// - Support 'stop rollout'
type Katan struct {
	kdb      katandb.KatanDB
	rollApis roller.RollApis
	L        log.Logger
	cfg      Config
	rtr      *retry.BackOff
}

func New(cfg Config, kdb katandb.KatanDB, deploy deployapi.Client, juggler jugglerapi.API, health healthapi.MDBHealthClient, mlock mlockclient.Locker, L log.Logger) *Katan {
	return &Katan{
		kdb: kdb,
		rollApis: roller.RollApis{
			Deploy:  deploy,
			Juggler: juggler,
			Health:  health,
			MLock:   mlock,
		},
		rtr: retry.New(retry.Config{
			MaxRetries: uint64(cfg.MaxRetries),
		}),
		L:   L,
		cfg: cfg,
	}
}

func (kat *Katan) IsReady(ctx context.Context) error {
	return kat.kdb.IsReady(ctx)
}

func (kat *Katan) newForRollout(rollout katandb.Rollout) *roller.Roller {
	logger := log.With(kat.L, log.Int64("rollout", rollout.ID))
	return roller.New(kat.cfg.Roller, kat.kdb, kat.rollApis, logger)
}

func (kat *Katan) getName() string {
	if kat.cfg.Name != "" {
		return kat.cfg.Name
	}
	hostname, err := os.Hostname()
	if err != nil {
		kat.L.Warn("failed to get hostname", log.Error(err))
		return ""
	}
	return hostname
}

func (kat *Katan) cleanupZombieRollout(ctx context.Context, roll katandb.Rollout) error {
	var clusterRolls []katandb.ClusterRollout
	if err := kat.rtr.RetryWithLog(
		ctx,
		func() error {
			rolls, err := kat.kdb.ClusterRollouts(ctx, roll.ID)
			if err != nil {
				return xerrors.Errorf("get clusters rollouts in zombie rollout %d: %w", roll.ID, err)
			}
			clusterRolls = rolls
			return nil
		},
		fmt.Sprintf("get clusters rollouts in zombie rollout: %d", roll.ID),
		kat.L,
	); err != nil {
		return err
	}

	for _, cr := range clusterRolls {
		// We clean the:
		// - failed ones, as they may not have removed the garbage.
		// - running ones, as they probably did not remove the garbage.
		if cr.State == katandb.ClusterRolloutRunning || cr.State == katandb.ClusterRolloutFailed {
			kat.L.Warnf("found cluster rollout for cleanup: %+v", cr)
			if err := cluster.Cleanup(ctx, kat.L, kat.cfg.Roller.Rollout, kat.rollApis, roll.ID, cr.ClusterID); err != nil {
				return xerrors.Errorf("failed to cleanup zombie rollout on cluster %q: %w", cr.ClusterID, err)
			}
			kat.L.Infof("successfully cleanup zombie rollout %d on %q cluster", roll.ID, cr.ClusterID)
		}
	}

	if err := kat.rtr.RetryWithLog(
		ctx,
		func() error {
			if err := kat.kdb.FinishRollout(ctx, roll.ID, optional.String{}); err != nil {
				return xerrors.Errorf("finish zombie rollout %d: %w", roll.ID, err)
			}
			return nil
		},
		fmt.Sprintf("finish zombie rollout %d", roll.ID),
		kat.L,
	); err != nil {
		return err
	}
	kat.L.Infof("successfully cleanup zombie rollout %d", roll.ID)

	return nil
}

// cleanup lookup for zombie rollouts, clean their side effects and finish them.
// Zombie rollout is a rollout that Katan start but forgot about it (panic, kill -9 ...)
// cleanup expect that:
// - it's called before start new rollouts
// - Katan Name is unique (three are no Katans with same name)
func (kat *Katan) cleanup(ctx context.Context) error {
	name := kat.getName()
	if name == "" {
		kat.L.Warn("don't cleanup zombie rollouts, cause my Name is empty")
		return nil
	}
	var zombieRollouts []katandb.Rollout
	if err := kat.rtr.RetryWithLog(
		ctx,
		func() error {
			rolls, err := kat.kdb.UnfinishedRollouts(ctx, name)
			if err != nil {
				return xerrors.Errorf("get zombie rollouts: %w", err)
			}
			zombieRollouts = rolls
			return nil
		},
		"get zombie rollouts",
		kat.L,
	); err != nil {
		return err
	}
	if len(zombieRollouts) == 0 {
		return nil
	}

	kat.L.Infof("got %d zombie rollouts", len(zombieRollouts))

	for _, roll := range zombieRollouts {
		if err := kat.cleanupZombieRollout(ctx, roll); err != nil {
			return err
		}
	}
	return nil
}

// Run get pending rollouts and start them
func (kat *Katan) Run(ctx context.Context) {
	defer func() {
		if err := recover(); err != nil {
			sentry.CapturePanicAndWait(ctx, err, nil)
			panic(err)
		}
	}()

	if err := kat.cleanup(ctx); err != nil {
		kat.L.Error("cleanup failed. I better give up", log.Error(err))
		sentry.GlobalClient().CaptureErrorAndWait(ctx, xerrors.Errorf("cleanup: %w", err), nil)
		return
	}

	cfg := RolloutRunnerConfig{
		PendingInterval: kat.cfg.PendingInterval.Duration,
		Parallel:        kat.cfg.Parallel,
		MaxRollouts:     kat.cfg.MaxRollouts,
	}
	RunRollouts(
		ctx,
		func() (rollout katandb.Rollout, err error) {
			return kat.kdb.StartPendingRollout(ctx, kat.getName())
		},
		func(rollout katandb.Rollout) {
			var comment optional.String

			rr := kat.newForRollout(rollout)
			err := rr.Rollout(ctx, rollout)
			if err != nil {
				comment.Set(err.Error())
				switch {
				case semerr.IsUnavailable(err):
					kat.L.Warn("katandb unavailable during rollout", log.Error(err))
				case xerrors.Is(err, cluster.ErrRolloutFailed) || xerrors.Is(err, cluster.ErrUnhealthyAfterRollout):
					kat.L.Warn("rollout failed", log.Error(err))
				default:
					kat.L.Error("rollout failed unexpectedly", log.Error(err))
					sentry.GlobalClient().CaptureErrorAndWait(ctx, err, SentryTagsFromRollout(rollout))
				}
			}

			_ = kat.rtr.RetryWithLog(
				ctx,
				func() error {
					return kat.kdb.FinishRollout(ctx, rollout.ID, comment)
				},
				fmt.Sprintf("rollout '%d'", rollout.ID),
				kat.L,
			)
		},
		cfg,
		kat.L,
	)
}
