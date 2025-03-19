package functest

import (
	"context"
	"encoding/json"
	"fmt"
	"time"

	"github.com/DATA-DOG/godog"
	"github.com/DATA-DOG/godog/gherkin"
	"github.com/jmoiron/sqlx"
	"gopkg.in/yaml.v2"

	"a.yandex-team.ru/cloud/mdb/backup/internal/metadb"
	mdbpg "a.yandex-team.ru/cloud/mdb/backup/internal/metadb/pg"
	schedApp "a.yandex-team.ru/cloud/mdb/backup/scheduler/pkg/app"
	workerApp "a.yandex-team.ru/cloud/mdb/backup/worker/pkg/app"
	cliApp "a.yandex-team.ru/cloud/mdb/backup/worker/pkg/cli"
	apihelpers "a.yandex-team.ru/cloud/mdb/dbaas-internal-api-image/recipe/helpers"
	metadbhelpers "a.yandex-team.ru/cloud/mdb/dbaas_metadb/recipes/helpers"
	"a.yandex-team.ru/cloud/mdb/internal/dbteststeps"
	"a.yandex-team.ru/cloud/mdb/internal/diff"
	"a.yandex-team.ru/cloud/mdb/internal/generator"
	"a.yandex-team.ru/cloud/mdb/internal/godogutil"
	"a.yandex-team.ru/cloud/mdb/internal/ready"
	s3steps "a.yandex-team.ru/cloud/mdb/internal/s3/steps"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/pgutil"
	"a.yandex-team.ru/cloud/mdb/internal/testutil/intapi"
	"a.yandex-team.ru/cloud/mdb/internal/testutil/workeremulation"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/nop"
	"a.yandex-team.ru/library/go/core/log/zap"
	"a.yandex-team.ru/library/go/core/xerrors"
)

const (
	metadbClusterName = "metadb"
)

var (
	queryBackupsCount = sqlutil.Stmt{
		Name: "BackupCounts",
		// language=PostgreSQL
		Query: "SELECT count(*) FROM dbaas.backups",
	}
	querySelectBackupServiceEnabled = sqlutil.Stmt{
		Name: "SelectBackupServiceUsage",
		// language=PostgreSQL
		Query: `
SELECT
       COALESCE(CAST(schedule ->> 'use_backup_service' AS BOOLEAN), false)
FROM
     dbaas.backup_schedule
WHERE
      cid = :cid`,
	}
	querySetTimezone = sqlutil.Stmt{
		Name: "SetTimezone",
		// language=PostgreSQL
		Query: "ALTER DATABASE dbaas_metadb SET timezone TO :tz",
	}
	querySetMaxIncrementalSteps = sqlutil.Stmt{
		Name: "SetMaxIncrementalSteps",
		// language=PostgreSQL
		Query: "UPDATE dbaas.backup_schedule SET schedule = jsonb_set(schedule, '{max_incremental_steps}', to_jsonb(:steps)) WHERE cid = :cid",
	}
	queryChangeRetainPeriodDays = sqlutil.Stmt{
		Name: "SetMaxIncrementalSteps",
		// language=PostgreSQL
		Query: "UPDATE dbaas.backup_schedule SET schedule = jsonb_set(schedule, '{retain_period}', to_jsonb(:days)) WHERE cid = :cid",
	}

	insertDefaultFeatureFlag = sqlutil.Stmt{
		Name:  "InsertDefaultFeatureFlag",
		Query: "INSERT INTO dbaas.default_feature_flags (flag_name) VALUES (:feature_flag)",
	}

	defaultSchedConfig = schedApp.DefaultConfig()
)

func ContextInitializer(tc *godogutil.TestContext, s *godog.Suite) {
	steps, err := NewSteps(tc, s)
	if err != nil {
		panic(err)
	}

	s.Step(`^I set "([^"]*)" cluster backup window at "([^"]*)" from now$`, steps.iMoveBackupWindowAt)
	s.Step(`^I create "(\d+)" (\w+) clusters$`, steps.iCreateCluster)
	s.Step(`^I add shard "([^"]*)" to "(\w+)" cluster$`, steps.iAddShard)
	s.Step(`^I enable sharding on "(\w+)" mongodb cluster$`, steps.iEnableMongoDBSharding)
	s.Step(`^I ([^"]*) backup service for all existed clusters via cli`, steps.iSwitchBackupServiceForClusters)
	s.Step(`^I set delta_max_steps for "(\w+)" to (\d+)$`, steps.iSetMaxIncrementalSteps)
	s.Step(`^I list backups for "(\w+)" cluster and got`, steps.iListClusterBackups)
	s.Step(`^backup service is "(\w+)" for "(\w+)" cluster`, steps.backupServiceUsage)
	s.Step(`^I execute mdb-backup-worker$`, steps.iExecuteWorker)
	s.Step(`^I execute mdb-backup-worker with config$`, steps.iExecuteWorkerWithConfig)
	s.Step(`^I execute mdb-backup-scheduler for ctype "(\w+)" to (\w+) with config$`, steps.iExecuteSchedulerWithConfig)
	s.Step(`^I execute mdb-backup-scheduler for ctype "(\w+)" to (\w+)$`, steps.iExecuteScheduler)
	s.Step(`^I execute mdb-backup-cli for cluster "(\w+)" to "([^"]+)"$`, steps.iExecuteCli)
	s.Step(`^I execute mdb-backup-cli to import backups for ctype "(\w+)" with batch size (\d+) and interval "(\w+)" and got$`, steps.iExecuteBatchImport)
	s.Step(`^I execute mdb-backup-cli for cluster "(\w+)" to "([^"]+)" and got$`, steps.iExecuteCliAssert)

	s.Step(`^all deploy shipments creation fails$`, steps.allShipmentsCreationFails)
	s.Step(`^all deploy shipments get status is "([^"]*)"$`, steps.allShipmentsGetStatus)
	s.Step(`^all hosts in health are "([^"]*)"$`, steps.allHostsInHealthAre)
	s.Step(`^in metadb there are "(\d+)" backups`, steps.inMetaDBThereAreNumBackups)
	s.Step(`^databases timezone set to "([^"]+)"$`, steps.setTimezone)

	s.Step(`^it succeeds$`, steps.itSucceeds)
	s.Step(`^I sleep "([^"]*)"$`, steps.iSleep)
	s.Step(`^I change retain period for "([^"]*)" to (\d+) days$`, steps.iChangeRetainPeriod)
	s.Step(`^we add default feature flag "([^"]*)"$`, steps.weAddDefaultFeatureFlag)

	s.BeforeScenario(steps.BeforeScenario)
}

type TestCluster struct {
	cid   string
	ctype string
	name  string
}

type Clusters struct {
	clusters          map[string]TestCluster
	clusterTypeCounts map[string]int
	size              int
}

func NewClusters() Clusters {
	return Clusters{
		clusters:          map[string]TestCluster{},
		clusterTypeCounts: map[string]int{},
	}
}

func (c *Clusters) Add(cluster TestCluster) error {
	if _, ok := c.clusters[cluster.name]; ok {
		return xerrors.Errorf("cluster name %q already exists", cluster.name)
	}
	c.clusters[cluster.name] = cluster
	c.clusterTypeCounts[cluster.ctype]++
	c.size++
	return nil
}

func (c *Clusters) Cluster(clusterName string) (TestCluster, error) {
	cluster, ok := c.clusters[clusterName]
	if !ok {
		return TestCluster{}, xerrors.Errorf("cluster name %q is not exists", clusterName)
	}
	return cluster, nil
}

type Steps struct {
	L log.Logger

	intAPI *intapi.Client
	worker *workeremulation.Worker
	mdb    *sqlutil.Cluster
	metadb *metadbhelpers.Client

	WC *WorkerContext
	CC *CliContext
	S3 *s3steps.S3Ctl
	TC *godogutil.TestContext

	createdClusters Clusters
}

func NewSteps(tc *godogutil.TestContext, s *godog.Suite) (*Steps, error) {

	l, err := zap.New(zap.KVConfig(log.DebugLevel))
	if err != nil {
		return nil, err
	}

	metadbSteps := &dbteststeps.DBSteps{
		L:          l,
		Migrations: make(map[string]struct{}),
		TC:         tc,
		Params: dbteststeps.Params{
			DBName: "metadb",
		},
	}
	mdb, node, err := dbteststeps.NewReadyCluster(metadbClusterName, defaultSchedConfig.Metadb.DB)
	metadbSteps.Cluster = mdb
	metadbSteps.Node = node

	dbteststeps.RegisterSteps(metadbSteps, s)

	s3ctl := s3steps.RegisterSteps(s, l)

	if err != nil {
		return nil, err
	}

	intAPI := intapi.New()

	awaitCtx, cancel := context.WithTimeout(context.Background(), time.Minute)
	defer cancel()
	_ = ready.Wait(awaitCtx, intAPI, &ready.DefaultErrorTester{
		Name:        "internal-api",
		FailOnError: false,
		L:           l,
	}, time.Second)
	if err := intAPI.IsReady(awaitCtx); err != nil {
		return nil, xerrors.Errorf("internal-api not ready: %w", err)
	}

	metadb, err := metadbhelpers.NewClient()
	if err != nil {
		return nil, err
	}

	st := &Steps{
		L:               l,
		intAPI:          intAPI,
		TC:              tc,
		WC:              &WorkerContext{},
		CC:              &CliContext{},
		S3:              s3ctl,
		mdb:             metadbSteps.Cluster,
		metadb:          metadb,
		worker:          workeremulation.New("dummy", metadbSteps.Cluster),
		createdClusters: NewClusters(),
	}

	return st, nil
}

func fillClusterAddr(clusterName string, config *pgutil.Config) {
	config.Addrs = []string{dbteststeps.DBHostPort(clusterName)}
	config.SSLMode = pgutil.AllowSSLMode
}

func (s *Steps) BeforeScenario(interface{}) {
	s.createdClusters = NewClusters()
	s.WC.Reset()
	s.CC.Reset(s.L)
	s.S3.Reset()

	if err := metadbhelpers.CleanupMetaDB(s.TC.Context(), s.mdb.Primary()); err != nil {
		panic(err)
	}
	if err := apihelpers.CleanupTmpRootPath(""); err != nil {
		panic(err)
	}
}

func (s *Steps) iExecuteSchedulerWithConfig(ctype, cmd string, cfg *gherkin.DocString) error {
	appCfg := schedApp.DefaultConfig()
	if err := yaml.Unmarshal([]byte(cfg.Content), &appCfg.Scheduler); err != nil {
		return xerrors.Errorf("failed to load scheduler config: %w", err)
	}

	return executeScheduler(s, ctype, cmd, appCfg)
}

func (s *Steps) iExecuteScheduler(ctype, cmd string) error {
	return executeScheduler(s, ctype, cmd, schedApp.DefaultConfig())
}

func executeScheduler(s *Steps, ctype, cmd string, appCfg schedApp.Config) error {
	fillClusterAddr(metadbClusterName, &appCfg.Metadb)
	mdb, err := mdbpg.New(appCfg.Metadb, log.With(s.L, log.String("cluster", mdbpg.DBName)))
	if err != nil {
		return err
	}

	awaitCtx, cancel := context.WithTimeout(s.TC.Context(), time.Minute)
	defer cancel()
	if err = ready.Wait(awaitCtx, mdb, &ready.DefaultErrorTester{Name: "metadb", L: s.L}, time.Second); err != nil {
		return xerrors.Errorf("failed to wait backend: %w", err)
	}

	clusterType, err := metadb.ParseClusterType(ctype)
	if err != nil {
		return err
	}

	sched, err := schedApp.NewSchedulerFromExtDeps(clusterType, mdb, s.L, &generator.BackupIDGenerator{}, appCfg.Scheduler)
	if err != nil {
		return err
	}

	switch cmd {
	case "plan":
		_, err = sched.PlanBackups(s.TC.Context(), false)
	case "obsolete":
		_, err = sched.ObsoleteBackups(s.TC.Context(), false)
	case "purge":
		_, err = sched.PurgeBackups(s.TC.Context(), false)
	default:
		return xerrors.Errorf("unknown command: %s", cmd)
	}
	return err
}

func (s *Steps) iExecuteCli(cname string, cmd string) error {
	return executeCli(s, cname, cmd, cliApp.DefaultConfig(), nil)
}

func (s *Steps) iExecuteBatchImport(rawClusterType string, batchSize int, importInterval string, expectedDoc *gherkin.DocString) error {
	ctype, err := metadb.ParseClusterType(rawClusterType)
	if err != nil {
		return err
	}

	interval, err := time.ParseDuration(importInterval)
	if err != nil {
		return err
	}

	s3client, err := s.S3.Mock(s.CC.mockCtrl)
	if err != nil {
		return err
	}

	cli, err := s.CC.NewCli(s.TC.Context(), s3client, cliApp.DefaultConfig(), s.L)
	if err != nil {
		return err
	}

	stats, err := cli.BatchImportBackups(s.TC.Context(), []metadb.ClusterType{ctype}, batchSize, interval, false, false, false)
	if err != nil {
		return err
	}

	return assertStatsMatchExpected(stats, expectedDoc)
}

func (s *Steps) iExecuteCliAssert(cname string, cmd string, expectedDoc *gherkin.DocString) error {
	return executeCli(s, cname, cmd, cliApp.DefaultConfig(), expectedDoc)
}

func executeCli(s *Steps, cname string, cmd string, appCfg cliApp.Config, expectedDoc *gherkin.DocString) error {
	cluster, ok := s.createdClusters.clusters[cname]
	if !ok {
		return xerrors.Errorf("cluster %q was not found", cname)
	}
	cid := cluster.cid

	s3client, err := s.S3.Mock(s.CC.mockCtrl)
	if err != nil {
		return err
	}

	cli, err := s.CC.NewCli(s.TC.Context(), s3client, appCfg, s.L)
	if err != nil {
		return err
	}

	switch cmd {
	case "enable backup-service":
		return cli.SetBackupServiceEnabled(s.TC.Context(), cid, true)
	case "disable backup-service":
		return cli.SetBackupServiceEnabled(s.TC.Context(), cid, false)
	case "roll metadata":
		return cli.RollMetadata(s.TC.Context(), cid, false)
	case "import s3 backups":
		return executeCliImport(s.TC.Context(), cli, cid, false, false, false, expectedDoc)
	case "import s3 backups and complete failed":
		return executeCliImport(s.TC.Context(), cli, cid, false, true, false, expectedDoc)
	case "import s3 backups ignoring sched date dups":
		return executeCliImport(s.TC.Context(), cli, cid, true, false, false, expectedDoc)
	case "import s3 backups with dry run":
		return executeCliImport(s.TC.Context(), cli, cid, false, false, true, expectedDoc)
	default:
		return xerrors.Errorf("unknown command: %s", cmd)
	}
}

func executeCliImport(ctx context.Context, cli *cliApp.Cli, cid string, skipSchedDateDups, completeFailed, dryrun bool, expectedDoc *gherkin.DocString) error {
	stats, err := cli.ImportBackups(ctx, cid, skipSchedDateDups, completeFailed, dryrun)
	if err != nil {
		return err
	}

	return assertStatsMatchExpected(stats, expectedDoc)
}

func assertStatsMatchExpected(stats interface{}, expectedDoc *gherkin.DocString) error {
	if expectedDoc == nil {
		return nil
	}

	expected := make(map[string]interface{})
	if err := json.Unmarshal([]byte(expectedDoc.Content), &expected); err != nil {
		return xerrors.Errorf("unmarshal error for expected output %s: %w", expectedDoc.Content, err)
	}
	bytesstats, err := json.Marshal(stats)
	if err != nil {
		return xerrors.Errorf("can not marshall stats result: %+v", bytesstats)
	}
	actual := make(map[string]interface{})
	if err := json.Unmarshal(bytesstats, &actual); err != nil {
		return xerrors.Errorf("unmarshal stats bytes %s: %w", string(bytesstats), err)
	}

	return diff.Full(expected, actual)
}

func (s *Steps) weAddDefaultFeatureFlag(featureFlag string) error {
	_, err := sqlutil.QueryNode(s.TC.Context(), s.metadb.Node, insertDefaultFeatureFlag,
		map[string]interface{}{"feature_flag": featureFlag}, sqlutil.NopParser, &nop.Logger{})
	return err
}

func (s *Steps) itSucceeds() error {
	return nil
}

func (s *Steps) iMoveBackupWindowAt(clusterName string, timedelta string) error {
	dur, err := time.ParseDuration(timedelta)
	if err != nil {
		return xerrors.Errorf("can not parse duration %q: %w", timedelta, err)
	}
	cluster, err := s.createdClusters.Cluster(clusterName)
	if err != nil {
		return err
	}

	backupStart := time.Now().UTC().Add(dur)
	backupWindow := intapi.UpdateBackupWindowRequest{
		Hours:   backupStart.Hour(),
		Minutes: backupStart.Minute(),
	}

	resp, err := s.intAPI.UpdateBackupWindow(cluster.cid, cluster.ctype, backupWindow)
	if err != nil {
		return err
	}
	err = s.worker.AcquireAndSuccessfullyCompleteTask(s.TC.Context(), resp.ID)
	if err != nil {
		return xerrors.Errorf("unable to finish task: %w", err)
	}
	return err
}

func (s *Steps) iEnableMongoDBSharding(clusterName string) error {
	cluster, err := s.createdClusters.Cluster(clusterName)
	if err != nil {
		return err
	}

	resp, err := s.intAPI.EnableMongoDBSharding(cluster.cid)
	if err != nil {
		return err
	}
	err = s.worker.AcquireAndSuccessfullyCompleteTask(s.TC.Context(), resp.ID)
	if err != nil {
		return xerrors.Errorf("unable to finish task: %w", err)
	}
	return err
}

func (s *Steps) iAddShard(shardName, clusterName string) error {
	cluster, err := s.createdClusters.Cluster(clusterName)
	if err != nil {
		return err
	}

	resp, err := s.intAPI.AddShard(cluster.cid, cluster.ctype, intapi.AddShardRequest{
		ShardName: shardName,
	})
	if err != nil {
		return err
	}
	err = s.worker.AcquireAndSuccessfullyCompleteTask(s.TC.Context(), resp.ID)
	if err != nil {
		return xerrors.Errorf("unable to finish task: %w", err)
	}
	return nil
}

func (s *Steps) iCreateCluster(clustersCount int, clusterType string) error {
	for i := 0; i < clustersCount; i++ {
		resp, err := s.intAPI.CreateCluster(clusterType, intapi.CreateClusterRequest{Name: fmt.Sprintf("%s_%02d", clusterType, s.createdClusters.size+1)})
		if err != nil {
			return err
		}
		err = s.worker.AcquireAndSuccessfullyCompleteTask(s.TC.Context(), resp.ID)
		if err != nil {
			return xerrors.Errorf("unable to finish task: %w", err)
		}
		cluster, err := s.intAPI.GetCluster(resp.Metadata.ClusterID, clusterType)
		if err != nil {
			return err
		}

		err = s.createdClusters.Add(TestCluster{
			cid:   resp.Metadata.ClusterID,
			ctype: clusterType,
			name:  cluster.Name})
		if err != nil {
			return err
		}
	}
	return nil
}

func (s *Steps) iListClusterBackups(clusterName string, expectedDoc *gherkin.DocString) error {
	cluster, ok := s.createdClusters.clusters[clusterName]
	if !ok {
		return xerrors.Errorf("cluster %q was not found", clusterName)
	}
	cid := cluster.cid

	backups, err := s.intAPI.ListBackups(cid, cluster.ctype)
	if err != nil {
		return err
	}

	expected := make(map[string]interface{})
	if err := json.Unmarshal([]byte(expectedDoc.Content), &expected); err != nil {
		return xerrors.Errorf("unmarshal error for body %s: %w", expectedDoc.Content, err)
	}

	backupsbytes, err := json.Marshal(backups)
	if err != nil {
		return xerrors.Errorf("can not marshall backup listing result: %+v", backupsbytes)
	}
	actual := make(map[string]interface{})
	if err := json.Unmarshal(backupsbytes, &actual); err != nil {
		return xerrors.Errorf("response body json unmarshal error for body %s: %w", string(backupsbytes), err)
	}
	return diff.Full(expected, actual)
}

func (s *Steps) inMetaDBThereAreNumBackups(expected int) error {
	return s.countQuery(queryBackupsCount, expected, "backups")
}

func (s *Steps) countQuery(stmt sqlutil.Stmt, expected int, kind string) error {
	var count int
	parser := func(rows *sqlx.Rows) error {
		return rows.Scan(&count)
	}
	_, err := sqlutil.QueryNode(s.TC.Context(), s.mdb.Primary(), stmt, nil, parser, &nop.Logger{})
	if err != nil {
		return err
	}
	if count != expected {
		return xerrors.Errorf("expect %d %s, but there are %d %s in metadb", expected, kind, count, kind)
	}
	return nil
}

func (s *Steps) backupServiceUsage(usage string, cname string) error {
	cluster, ok := s.createdClusters.clusters[cname]
	if !ok {
		return xerrors.Errorf("cluster %q was not found", cname)
	}

	var enabled bool
	parser := func(rows *sqlx.Rows) error {
		return rows.Scan(&enabled)
	}

	count, err := sqlutil.QueryNode(
		s.TC.Context(),
		s.mdb.Primary(),
		querySelectBackupServiceEnabled,
		map[string]interface{}{
			"cid": cluster.cid,
		},
		parser,
		s.L,
	)
	if err != nil {
		return err
	}
	if count == 0 {
		return metadb.ErrDataNotFound
	}

	var expected bool
	switch usage {
	case "enabled":
		expected = true
	case "disabled":
		expected = false
	default:
		return xerrors.Errorf("unexpected backup service state: %s", usage)
	}

	if enabled != expected {
		return xerrors.Errorf("backup service is expected to be %t, but current status is %t", expected, enabled)
	}

	return nil
}

func (s *Steps) iSwitchBackupServiceForClusters(action string) error {
	for cname := range s.createdClusters.clusters {
		if err := executeCli(s, cname, fmt.Sprintf("%s backup-service", action), cliApp.DefaultConfig(), nil); err != nil {
			return xerrors.Errorf("can not %s backup-service for cluster %q: %w", action, cname, err)
		}
	}
	return nil
}

func (s *Steps) iSetMaxIncrementalSteps(clusterID string, val int) error {
	_, err := sqlutil.QueryNode(s.TC.Context(), s.mdb.Primary(), querySetMaxIncrementalSteps, map[string]interface{}{"cid": clusterID, "steps": val}, sqlutil.NopParser, &nop.Logger{})
	return err
}

func (s *Steps) iSleep(arg string) error {
	d, err := time.ParseDuration(arg)
	if err != nil {
		return err
	}
	time.Sleep(d)
	return nil
}

func (s *Steps) iExecuteWorkerWithConfig(cfg *gherkin.DocString) error {
	appCfg := workerApp.DefaultConfig()
	if err := yaml.Unmarshal([]byte(cfg.Content), &appCfg.Cycles); err != nil {
		return xerrors.Errorf("failed to load worker config: %w", err)
	}

	return executeWorker(s, appCfg)
}

func (s *Steps) iExecuteWorker() error {
	return executeWorker(s, workerApp.DefaultConfig())
}

func executeWorker(s *Steps, appCfg workerApp.Config) error {
	return s.WC.RunWorker(s.TC.Context(), s.S3, s.L, appCfg)
}

func (s *Steps) allHostsInHealthAre(status string) error {
	if err := s.WC.AllHostsServiceHealthIs(status); err != nil {
		return err
	}
	return s.CC.AllHostsServiceHealthIs(status)
}

func (s *Steps) allShipmentsCreationFails() error {
	s.WC.AllShipmentsCreationFails()
	s.CC.AllShipmentsCreationFails()
	return nil
}

func (s *Steps) allShipmentsGetStatus(status string) error {
	s.WC.AllShipmentsGetStatus(status)
	s.CC.AllShipmentsGetStatus(status)
	return nil
}

func (s *Steps) setTimezone(tz string) error {
	_, err := sqlutil.QueryNode(s.TC.Context(), s.mdb.Primary(), querySetTimezone, map[string]interface{}{"tz": tz}, sqlutil.NopParser, &nop.Logger{})
	return err
}

func (s *Steps) iChangeRetainPeriod(cid string, days int) error {
	if _, err := sqlutil.QueryNode(s.TC.Context(), s.mdb.Primary(), queryChangeRetainPeriodDays, map[string]interface{}{"cid": cid, "days": days}, sqlutil.NopParser, &nop.Logger{}); err != nil {
		return err
	}

	return nil
}
