package functest

import (
	"context"
	"math/rand"
	"time"

	"github.com/jmoiron/sqlx"

	writerdummy "a.yandex-team.ru/cloud/mdb/internal/logbroker/writer/dummy"
	mdbpg "a.yandex-team.ru/cloud/mdb/internal/metadb/pg"
	"a.yandex-team.ru/cloud/mdb/internal/monrun"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/pgutil"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/pgutil/recipeconfig"
	"a.yandex-team.ru/cloud/mdb/internal/testutil/appwrap"
	"a.yandex-team.ru/cloud/mdb/mdb-event-producer/internal/app"
	"a.yandex-team.ru/cloud/mdb/mdb-event-producer/internal/monitoring"
	"a.yandex-team.ru/cloud/mdb/mdb-event-producer/internal/producer"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
	"a.yandex-team.ru/library/go/core/xerrors"
	pgxutil "a.yandex-team.ru/library/go/x/sql/pgx"
	"a.yandex-team.ru/library/go/x/yandex/hasql/tracers"
)

func init() {
	rand.Seed(time.Now().UnixNano())
}

var letterRunes = []rune("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ")

func randID() string {
	b := make([]rune, 16)
	for i := range b {
		b[i] = letterRunes[rand.Intn(len(letterRunes))]
	}
	return string(b)
}

type testContext struct {
	mdb              *sqlutil.Cluster
	apps             []*appwrap.Wrap
	mdbCfg           pgutil.Config
	ctx              context.Context
	cancel           context.CancelFunc
	tasksCreated     []string
	monitoringResult *monrun.Result
}

var storedGlobalContext *testContext

func initTestContext() (*testContext, error) {
	if storedGlobalContext == nil {
		logger, err := zap.New(zap.KVConfig(log.DebugLevel))
		if err != nil {
			return nil, xerrors.Errorf("failed to init logger: %w", err)
		}

		mdbCfg, err := recipeconfig.New("metadb", "dbaas_metadb", "")
		if err != nil {
			return nil, err
		}

		mdb, err := pgutil.NewCluster(mdbCfg, sqlutil.WithTracer(tracers.Log(logger)))
		if err != nil {
			return nil, xerrors.Errorf("failed to initialize metadb backend: %w", err)
		}

		if _, err = mdb.WaitForPrimary(context.Background()); err != nil {
			_ = mdb.Close()
			return nil, xerrors.Errorf("no master: %w", err)
		}

		storedGlobalContext = &testContext{mdb: mdb, mdbCfg: mdbCfg}

	}
	return storedGlobalContext, nil
}

func (tc *testContext) BeforeScenario(arg interface{}) {
	ctx := context.Background()
	ctx, cancel := context.WithTimeout(ctx, time.Minute)
	tc.ctx = ctx
	tc.cancel = cancel
}

func (tc *testContext) AfterScenario(arg interface{}, err error) {
	for _, a := range tc.apps {
		a.Stop()
	}
	tc.apps = nil
	tc.cancel()
}

func (tc *testContext) getDB() (*sqlx.DB, error) {
	master := tc.mdb.Primary()
	if master == nil {
		return nil, xerrors.New("master not available at that moment")
	}
	return master.DBx(), nil
}

func (tc *testContext) onMaster(handler func(db *sqlx.DB) error) error {
	db, err := tc.getDB()
	if err != nil {
		return err
	}
	return handler(db)
}

func (tc *testContext) execScript(query string, args ...interface{}) error {
	db, err := tc.getDB()
	if err != nil {
		return err
	}
	result, err := db.Exec(query, args...)
	if err != nil {
		return err
	}
	_, err = result.RowsAffected()
	return err
}

// language=PostgreSQL
const createCloudAndFolderAndClustersAndTasks = `DO $$
DECLARE
	i_cloud_ext_id  text   := $1;
	i_folder_ext_id text   := $2;
	i_cids 		    text[] := $3;
    i_tasks         text[] := $4;
    i               integer;
	v_cloud_id      bigint;
	v_folder_id     bigint;
	v_rev           bigint;
    c_create_event  constant jsonb = '{"event_status": "new", "event_metadata": {}}';
BEGIN
	v_cloud_id := (code.add_cloud(
		i_cloud_ext_id => i_cloud_ext_id,
		i_quota => code.make_quota(
			i_cpu      => 10,
			i_memory   => 10,
			i_network  => 10,
			i_io       => 10,
			i_clusters => 10),
	    i_x_request_id => 'test'
	)).cloud_id;

	INSERT INTO dbaas.folders (folder_ext_id, cloud_id)
	VALUES (i_folder_ext_id, v_cloud_id)
	RETURNING folder_id INTO v_folder_id;

	FOR i IN 1 .. cardinality(i_cids) LOOP
		v_rev := (code.create_cluster(
			i_cid         => i_cids[i],
			i_name        => 'test cluster ' || (i::text),
			i_type        => 'redis_cluster',
			i_env         => 'qa',
			i_public_key  => NULL,
			i_network_id  => '',
			i_folder_id   => v_folder_id,
			i_description => NULL
		)).rev;
		PERFORM code.add_operation(
			i_operation_id => i_tasks[i],
		    i_cid          => i_cids[i],
		    i_folder_id    => v_folder_id,
		    i_operation_type => 'redis_cluster_create',
		    i_task_type      => 'redis_cluster_create',
		    i_task_args      => '{}',
		    i_metadata       => '{}',
		    i_user_id        => 'tester',
		    i_version        => 42,
		    i_rev            => v_rev
		);

		INSERT INTO dbaas.worker_queue_events
		    (task_id, data)
		VALUES
			(i_tasks[i], c_create_event);

		PERFORM code.complete_cluster_change(
		    i_cid => i_cids[i],
		    i_rev => v_rev
		);
	END LOOP;
END;
$$`

func (tc *testContext) cloudFolderAndClustersAndTasks(clustersCount int) error {
	cids := make([]string, clustersCount)
	tasks := make([]string, clustersCount)
	for i := 0; i < clustersCount; i++ {
		cids[i] = randID()
		tasks[i] = randID()
	}
	tc.tasksCreated = tasks
	return tc.execScript(
		createCloudAndFolderAndClustersAndTasks,
		randID(),
		randID(),
		pgxutil.Array(cids),
		pgxutil.Array(tasks),
	)
}

// language=PostgreSQL
const finishMyTasksQuery = `DO $$
DECLARE
    v_worker       constant  text := 'test-worker';
    v_worker_queue dbaas.worker_queue;
BEGIN
    FOR v_worker_queue IN (
        SELECT *
          FROM dbaas.worker_queue
         WHERE end_ts IS NULL
           AND task_id = ANY($1)) LOOP
        PERFORM code.acquire_task(
            i_worker_id => v_worker,
            i_task_id   => v_worker_queue.task_id
        );
        PERFORM code.finish_task(
          	i_worker_id => v_worker,
            i_task_id   => v_worker_queue.task_id,
            i_result    => $2,
            i_changes   => '{}',
            i_comment   => ''
        );
	END LOOP;
END;
$$`

func (tc *testContext) iFinishMyTasks() error {
	return tc.execScript(finishMyTasksQuery, pgxutil.Array(tc.tasksCreated), true)
}

func (tc *testContext) iFallMyTasks() error {
	return tc.execScript(finishMyTasksQuery, pgxutil.Array(tc.tasksCreated), false)
}

type eventsStat struct {
	count       int64
	startUnsent int64
	doneUnsent  int64
}

func (tc *testContext) getEventsStat() (eventsStat, error) {
	var stat eventsStat

	err := tc.onMaster(func(db *sqlx.DB) error {
		row := db.QueryRow(`
			SELECT count(*),
			       count(*) FILTER (WHERE start_sent_at IS NULL),
			       count(*) FILTER (WHERE done_sent_at IS NULL)
		      FROM dbaas.worker_queue_events
             WHERE task_id = ANY($1)`, pgxutil.Array(tc.tasksCreated))
		if err := row.Scan(&stat.count, &stat.startUnsent, &stat.doneUnsent); err != nil {
			return err
		}
		return nil
	})

	return stat, err
}

func (tc *testContext) myEventsAreSent(kind string, tst string) error {
	ts, err := time.ParseDuration(tst)
	if err != nil {
		return xerrors.Errorf("unable to parse duration: %w", err)
	}
	startAt := time.Now()
	var stat eventsStat

	for time.Since(startAt) < ts {
		stat, err = tc.getEventsStat()
		if err != nil {
			return err
		}
		switch kind {
		case "start":
			if stat.startUnsent == 0 {
				return nil
			}
		case "done":
			if stat.doneUnsent == 0 {
				return nil
			}
		default:
			return xerrors.Errorf("Unexpected stat type: %q", kind)
		}
		time.Sleep(time.Second)
	}
	return xerrors.Errorf("Expect 0 unsent %s-events, by %+v at last iteration found", kind, stat)
}

func (tc *testContext) myDoneEventsAreNotSent(tst string) error {
	ts, err := time.ParseDuration(tst)
	if err != nil {
		return xerrors.Errorf("unable to parse duration: %w", err)
	}
	// I have no good solutions fot that check.
	// At least it shouldn't be flaky.
	time.Sleep(ts)

	stat, err := tc.getEventsStat()
	if err != nil {
		return err
	}
	if stat.doneUnsent == 0 {
		return xerrors.Errorf("After %s  there are not unset done events: %+v", ts, stat)
	}
	return nil
}

func getProducerUser() string {
	return app.DefaultConfig().MetaDB.User
}

func makeAppCfg(metaCfg pgutil.Config) app.Config {
	cfg := app.DefaultConfig()
	cfg.MetaDB = metaCfg
	cfg.MetaDB.User = getProducerUser()
	cfg.Producer.Sleeps.From.Duration = time.Millisecond * 100
	cfg.Producer.Sleeps.To.Duration = time.Millisecond * 300
	return cfg
}

func (tc *testContext) iRunOneProducer() error {
	appConfig := makeAppCfg(tc.mdbCfg)
	wrap, err := appwrap.New(
		tc.ctx,
		appConfig.MetaDB,
		func(_ context.Context, logger log.Logger, cluster *sqlutil.Cluster) (appwrap.App, error) {
			meta := mdbpg.NewWithCluster(cluster, logger)
			pr, err := producer.New(logger, meta, writerdummy.New(), appConfig.Producer)
			if err != nil {
				return nil, err
			}
			return app.NewAppCustom(
				logger,
				pr,
				appConfig,
				nil,
			), nil
		},
	)
	if err != nil {
		return err
	}
	tc.apps = append(tc.apps, wrap)

	return nil
}

func (tc *testContext) iRunProducers(count int) error {
	for i := 1; i <= count; i++ {
		err := tc.iRunOneProducer()
		if err != nil {
			return xerrors.Errorf("fail while try run %d-th producer: %w", i, err)
		}
	}
	return nil
}

func (tc *testContext) iSleep(duration string) error {
	dur, err := time.ParseDuration(duration)
	if err != nil {
		return xerrors.Errorf("malformed duration: %s", err)
	}
	time.Sleep(dur)
	return nil
}

func (tc *testContext) iRunMonitoring(warnStr, critStr string) error {

	logger, err := zap.New(zap.KVConfig(log.DebugLevel))
	if err != nil {
		return xerrors.Errorf("Unable to init logger: %w", err)
	}
	warn, err := time.ParseDuration(warnStr)
	if err != nil {
		return xerrors.Errorf("Bad warn format: %w", err)
	}
	crit, err := time.ParseDuration(critStr)
	if err != nil {
		return xerrors.Errorf("Bad crit format: %w", err)
	}

	monCtx, monCancel := context.WithCancel(tc.ctx)

	// Use addition cancel, cause monitoring create new pg cluster.
	// We should stop it.
	defer monCancel()
	mon := monitoring.NewFromAppConfig(logger, makeAppCfg(tc.mdbCfg))
	res := mon.CheckEventsAge(monCtx, warn, crit)
	tc.monitoringResult = &res
	return nil
}

func (tc *testContext) monitoringReturns(codeStr string) error {
	monrunCode, err := monrun.ResultCodeFromString(codeStr)
	if err != nil {
		return err
	}
	if tc.monitoringResult == nil {
		return xerrors.New("There are no monitoring results")
	}

	if tc.monitoringResult.Code != monrunCode {
		return xerrors.Errorf("Monitoring return %s. Expected %s. Message: %s", tc.monitoringResult.Code, monrunCode, tc.monitoringResult.Message)
	}
	return nil
}
