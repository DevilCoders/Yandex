package scheduler

import (
	"context"
	"encoding/json"
	"reflect"
	"time"

	deploymodels "a.yandex-team.ru/cloud/mdb/deploy/api/pkg/models"
	"a.yandex-team.ru/cloud/mdb/internal/encodingutil"
	"a.yandex-team.ru/cloud/mdb/internal/holidays"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/katan/internal/katandb"
	"a.yandex-team.ru/cloud/mdb/katan/internal/tags"
	"a.yandex-team.ru/cloud/mdb/katan/scheduler/internal/workingtime"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type Config struct {
	MaxFailsSequentially    int                   `json:"max_fails_sequentially" yaml:"max_fails_sequentially"`
	MaxFailsInWindow        int                   `json:"max_fails_in_window" yaml:"max_fails_in_window"`
	InspectionWindow        encodingutil.Duration `json:"inspection_window" yaml:"inspection_window"`
	InspectionRolloutsLimit int                   `json:"inspection_rollouts_limit" yaml:"inspection_rollouts_limit"`
	ProcessRetries          int                   `json:"process_retries" yaml:"process_retries"`
	ImportCooldown          encodingutil.Duration `json:"import_cooldown" yaml:"import_cooldown"`
}

func DefaultConfig() Config {
	return Config{
		MaxFailsSequentially:    3,
		MaxFailsInWindow:        10,
		InspectionWindow:        encodingutil.FromDuration(time.Hour * 24 * 3),
		InspectionRolloutsLimit: 50,
		ProcessRetries:          5,
		ImportCooldown:          encodingutil.FromDuration(time.Hour * 24 * 2),
	}
}

// Scheduler process schedule and plan updates
type Scheduler struct {
	kdb katandb.KatanDB
	cal holidays.Calendar
	cfg Config
	L   log.Logger
}

func New(kdb katandb.KatanDB, cal holidays.Calendar, cfg Config, L log.Logger) *Scheduler {
	return &Scheduler{
		kdb: kdb,
		cal: cal,
		cfg: cfg,
		L:   L,
	}
}

func ValidTagsQuery(matchTags string) error {
	t, err := tags.UnmarshalClusterTags(matchTags)
	if err != nil {
		return err
	}

	if reflect.DeepEqual(t, tags.ClusterTags{}) {
		return xerrors.New("empty tags is not valid")
	}

	return nil
}

func ValidDeployCommands(commands string) error {
	var cmds []deploymodels.CommandDef
	if err := json.Unmarshal([]byte(commands), &cmds); err != nil {
		return xerrors.Errorf("malformed deploy commands: %s", err)
	}
	if len(cmds) == 0 {
		return xerrors.New("emtpy deploy commands are not valid")
	}
	return nil
}

func (sher *Scheduler) IsReady(ctx context.Context) error {
	return sher.kdb.IsReady(ctx)
}

// PruneSchedules remove not active schedules and schedules with non active dependencies
func PruneSchedules(schedules []katandb.Schedule) []katandb.Schedule {
	var ret []katandb.Schedule
	for _, s := range schedules {
		if s.State != katandb.ScheduleStateActive {
			continue
		}
		ret = append(ret, s)
	}
	return ret
}

// ProcessSchedules process schedules
func (sher *Scheduler) ProcessSchedules(ctx context.Context) error {
	if ok, err := workingtime.Check(ctx, sher.cal, time.Now()); !ok {
		if err != nil {
			return xerrors.Errorf("check working time: %w", err)
		}
		sher.L.Debug("The current time is not a working time. Do nothing")
		return nil
	}
	schedules, err := sher.kdb.Schedules(ctx)
	if err != nil {
		return xerrors.Errorf("get schedules: %w", err)
	}
	sher.L.Debugf("got %d schedules from db", len(schedules))

	schedules = PruneSchedules(schedules)
	sher.L.Debugf("after pruning we have %d schedules", len(schedules))

	for _, sh := range schedules {
		ok, err := sher.isProcessableOrMarkBroken(ctx, sh)
		if err != nil {
			return err
		}
		if !ok {
			continue
		}

		for try := 1; ; try++ {
			// Sometimes it fails.
			// Mostly on foreign key constraint from katan.cluster_rollouts to katan.clusters.
			// Caused by imported cluster delete by `katan-imp`.
			// It was a fancy, to retry only such errors, but it's not a problem if we retry all errors here.
			err = sher.process(ctx, sh)
			if err != nil {
				if try >= sher.cfg.ProcessRetries {
					return err
				}
				sher.L.Warnf("schedule process fail: %s. It was %d try", err, try)
				continue
			}
			break
		}
	}

	return nil
}

func (sher *Scheduler) isProcessableOrMarkBroken(ctx context.Context, schedule katandb.Schedule) (bool, error) {
	// define can we process or not
	// get last rollouts InspectionRolloutsLimit for that schedule.
	// - Skip it - if have running schedules
	// - Mark it broken, if MaxFailsSequentially cluster rollouts are failed sequentially,
	// 	 or overall fails >= MaxFailsInWindow
	rollouts, err := sher.kdb.LastRolloutsBySchedule(
		ctx,
		schedule.ID,
		time.Now().Add(-sher.cfg.InspectionWindow.Duration),
		sher.cfg.InspectionRolloutsLimit,
	)
	if err != nil {
		return false, xerrors.Errorf("fail to get rollouts: %w", err)
	}

	for _, roll := range rollouts {
		if !roll.FinishedAt.Valid {
			sher.L.Infof("skipping %d schedule, cause it have running rollout: %d", schedule.ID, roll.ID)
			return false, nil
		}
	}

	overallFailedClusters := make(map[string]int64)
	sequentiallyFailedClusters := make(map[string]struct{})
	var itsBroken bool
	var lastExaminedRolloutID int64
	for _, roll := range rollouts {
		if schedule.ExaminedRolloutID.Int64 >= roll.ID {
			sher.L.Debugf("don't count %d rollout, cause we already examined it", roll.ID)
			continue
		}
		if lastExaminedRolloutID < roll.ID {
			lastExaminedRolloutID = roll.ID
		}
		clusterRolls, err := sher.kdb.ClusterRollouts(ctx, roll.ID)
		if err != nil {
			return false, xerrors.Errorf("fail to get cluster rollouts: %w", err)
		}
		for _, cr := range clusterRolls {
			switch cr.State {
			case katandb.ClusterRolloutSucceeded:
				if len(sequentiallyFailedClusters) > 0 {
					sequentiallyFailedClusters = map[string]struct{}{}
				}
			case katandb.ClusterRolloutFailed:
				sher.L.Infof("cluster %q failed in schedule %d rollout %d cause: %s", cr.ClusterID, schedule.ID, roll.ID, cr.Comment.String)
				overallFailedClusters[cr.ClusterID] = roll.ID
				sequentiallyFailedClusters[cr.ClusterID] = struct{}{}

				if len(sequentiallyFailedClusters) >= sher.cfg.MaxFailsSequentially {
					sher.L.Infof("mark schedule %d as broken, cause it failed on %d clusters sequentially: %q", schedule.ID, sher.cfg.MaxFailsSequentially, sequentiallyFailedClusters)
					itsBroken = true
				}
				if len(overallFailedClusters) >= sher.cfg.MaxFailsInWindow {
					sher.L.Infof("mark schedule %d as broken, cause it failed on %d clusters: %q", schedule.ID, sher.cfg.MaxFailsInWindow, overallFailedClusters)
					itsBroken = true
				}
			}
			if itsBroken {
				break
			}
		}
		if itsBroken {
			break
		}
	}

	sher.L.Debugf("schedule %d fails on %d clusters, on %d cluster sequentially", schedule.ID, len(overallFailedClusters), len(sequentiallyFailedClusters))

	if itsBroken {
		fails := make([]katandb.ScheduleFail, 0, len(overallFailedClusters))
		// store overallFailedClusters as fails, cause overall include sequentiallyFailedClusters
		for clusterID, rolloutID := range overallFailedClusters {
			fails = append(fails, katandb.ScheduleFail{
				RolloutID: rolloutID,
				ClusterID: clusterID,
			})
		}
		err = sher.kdb.MarkSchedule(ctx, schedule.ID, katandb.ScheduleStateBroken, lastExaminedRolloutID, fails)
		if err != nil {
			return false, xerrors.Errorf("fail to mark schedule as broken")
		}
		return false, nil
	}

	return true, nil
}

func (sher *Scheduler) process(ctx context.Context, schedule katandb.Schedule) error {
	ids, err := sher.kdb.AutoUpdatedClustersBySchedule(
		ctx,
		schedule.MatchTags,
		schedule.ID,
		schedule.Age,
		schedule.StillAge,
		sher.cfg.ImportCooldown.Duration,
		schedule.MaxSize,
	)
	if err != nil {
		return xerrors.Errorf("fail to get cluster ids for given rollout: %w", err)
	}
	if len(ids) == 0 {
		return nil
	}

	sher.L.Debugf("got %d clusters", len(ids))
	roll, err := sher.kdb.AddRollout(ctx, schedule.Commands, schedule.Options, "scheduler", schedule.Parallel, optional.NewInt64(schedule.ID), ids)
	if err != nil {
		return xerrors.Errorf("fail to add rollout by %+v: %w", schedule, err)
	}

	sher.L.Infof("create rollout %d for schedule %d", roll.ID, schedule.ID)
	return nil

}

func (sher *Scheduler) AddRollout(ctx context.Context, matchTags, commands string, createdBy string, parallel int32) (int64, error) {
	if err := ValidTagsQuery(matchTags); err != nil {
		return 0, err
	}
	if err := ValidDeployCommands(commands); err != nil {
		return 0, err
	}
	ids, err := sher.kdb.AutoUpdatedClustersIDsByQuery(ctx, matchTags)
	if err != nil {
		return 0, xerrors.Errorf("fail to get cluster ids for given rollout: %w", err)
	}

	if len(ids) == 0 {
		return 0, xerrors.Errorf("no clusters found that match your query")
	}

	sher.L.Debugf("got %d clusters", len(ids))
	roll, err := sher.kdb.AddRollout(ctx, commands, "{}", createdBy, parallel, optional.Int64{}, ids)
	if err != nil {
		return 0, err
	}
	return roll.ID, nil
}

type RolloutStateResponse struct {
	Rollout          katandb.Rollout
	ClustersRollouts []katandb.ClusterRollout
}

// RolloutState return state of given rollout
func (sher *Scheduler) RolloutState(ctx context.Context, id int64) (RolloutStateResponse, error) {
	rollout, err := sher.kdb.Rollout(ctx, id)
	if err != nil {
		return RolloutStateResponse{}, err
	}
	clusters, err := sher.kdb.ClusterRollouts(ctx, id)
	if err != nil {
		return RolloutStateResponse{}, err
	}
	return RolloutStateResponse{
		Rollout:          rollout,
		ClustersRollouts: clusters,
	}, nil
}
