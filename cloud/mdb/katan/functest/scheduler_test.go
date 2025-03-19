package functest

import (
	"context"
	"fmt"
	"time"

	"github.com/golang/mock/gomock"
	"github.com/jmoiron/sqlx"

	"a.yandex-team.ru/cloud/mdb/internal/holidays"
	calmocks "a.yandex-team.ru/cloud/mdb/internal/holidays/mocks"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	kdbpg "a.yandex-team.ru/cloud/mdb/katan/internal/katandb/pg"
	sherapp "a.yandex-team.ru/cloud/mdb/katan/scheduler/pkg/app"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

const deployCommands = `[{"command": "state.high"}]`

var (
	queryScheduleState = sqlutil.Stmt{
		Name: "ScheduleState",
		// language=PostgreSQL
		Query: "SELECT state FROM katan.schedules WHERE schedule_id = :id",
	}
	queryAddSchedule = sqlutil.Stmt{
		Name: "AddSchedule",
		// language=PostgreSQL
		Query: `
INSERT INTO katan.schedules
    (match_tags, commands, options, age, still_age, max_size, parallel, namespace, name)
VALUES
    (:match_tags, :commands, :options, '1day', '12hour', :max_size, 100, :namespace, 'test-name')
RETURNING schedule_id`,
	}
	queryMarkScheduleAs = sqlutil.Stmt{
		Name: "MarkScheduleAs",
		// language=PostgreSQL
		Query: `UPDATE katan.schedules SET state = :state WHERE schedule_id=:schedule_id`,
	}
)

type SchedulerContext struct {
	RolloutCreated optional.Int64
	ScheduleID     optional.Int64
	L              log.Logger
	kdb            *sqlutil.Cluster
	mockCtrl       *gomock.Controller
}

func (sc *SchedulerContext) Reset(lg log.Logger) {
	sc.RolloutCreated = optional.Int64{}
	sc.ScheduleID = optional.Int64{}
	sc.mockCtrl = gomock.NewController(lg)
}

func (sc *SchedulerContext) newScheduler(ctx context.Context, maxRolloutFails optional.Int64, importCooldown optional.Duration) (*sherapp.App, error) {
	config := sherapp.DefaultConfig()
	if maxRolloutFails.Valid {
		config.Scheduler.MaxFailsSequentially = int(maxRolloutFails.Int64)
	}
	config.Scheduler.ImportCooldown.Duration = -time.Second
	if importCooldown.Valid {
		config.Scheduler.ImportCooldown.Duration = importCooldown.Duration
	}
	cal := calmocks.NewMockCalendar(sc.mockCtrl)
	cal.EXPECT().Range(gomock.Any(), gomock.Any(), gomock.Any()).Return(
		[]holidays.Day{{Type: holidays.Weekday}, {Type: holidays.Weekday}}, nil,
	).AnyTimes()
	return sherapp.NewAppCustom(ctx, config, kdbpg.NewWithCluster(sc.kdb, sc.L), cal, sc.L)
}

func (sc *SchedulerContext) AddRolloutFor(ctx context.Context, matchTags string) error {
	app, err := sc.newScheduler(ctx, optional.Int64{}, optional.Duration{})
	if err != nil {
		return err
	}
	rolloutID, err := app.AddRollout(ctx, matchTags, deployCommands, "functest")
	if err != nil {
		return err
	}

	sc.RolloutCreated.Set(rolloutID)
	return nil
}

func (sc *SchedulerContext) MyRolloutIsFinished(ctx context.Context) error {
	app, err := sc.newScheduler(ctx, optional.Int64{}, optional.Duration{})
	if err != nil {
		return err
	}
	state, err := app.RolloutState(ctx, sc.RolloutCreated.Int64)
	if err != nil {
		return err
	}
	if !state.Rollout.FinishedAt.Valid {
		return fmt.Errorf("rollout %+v still running", state.Rollout)
	}

	return nil
}

func (sc *SchedulerContext) Run(ctx context.Context, maxRolloutFails optional.Int64, importCooldown optional.Duration) error {
	sher, err := sc.newScheduler(ctx, maxRolloutFails, importCooldown)
	if err != nil {
		return err
	}
	return sher.Run(ctx)
}

func (sc *SchedulerContext) AddSchedule(ctx context.Context, maxSize int, matchTags, options, namespace string) error {
	if options == "" {
		options = "{}"
	}
	_, err := sqlutil.QueryNode(
		ctx,
		sc.kdb.Primary(),
		queryAddSchedule,
		map[string]interface{}{
			"match_tags": matchTags,
			"max_size":   maxSize,
			"commands":   deployCommands,
			"namespace":  namespace,
			"options":    options,
		},
		func(rows *sqlx.Rows) error {
			var id int64
			if err := rows.Scan(&id); err != nil {
				return err
			}
			sc.ScheduleID.Set(id)
			return nil
		},
		sc.L,
	)
	return err
}

func (sc *SchedulerContext) MyScheduleStateIs(ctx context.Context, expected string) error {
	var actual string
	_, err := sqlutil.QueryNode(
		ctx,
		sc.kdb.Primary(),
		queryScheduleState,
		map[string]interface{}{
			"id": sc.ScheduleID.Int64,
		},
		func(rows *sqlx.Rows) error {
			return rows.Scan(&actual)
		},
		sc.L,
	)
	if err != nil {
		return err
	}

	if actual != expected {
		return xerrors.Errorf("Expect %q state. Got %q state", expected, actual)
	}

	return nil
}

func (sc *SchedulerContext) MarkMyScheduleAs(ctx context.Context, state string) error {
	_, err := sqlutil.QueryNode(
		ctx,
		sc.kdb.Primary(),
		queryMarkScheduleAs,
		map[string]interface{}{
			"schedule_id": sc.ScheduleID.Int64,
			"state":       state,
		},
		sqlutil.NopParser,
		sc.L,
	)
	return err
}
