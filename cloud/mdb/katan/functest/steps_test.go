package functest

import (
	"context"
	"fmt"
	"time"

	"github.com/DATA-DOG/godog"
	"github.com/DATA-DOG/godog/gherkin"
	"github.com/google/go-cmp/cmp"
	"github.com/jmoiron/sqlx"
	"gopkg.in/yaml.v2"

	metadbhelpers "a.yandex-team.ru/cloud/mdb/dbaas_metadb/recipes/helpers"
	"a.yandex-team.ru/cloud/mdb/internal/dbteststeps"
	"a.yandex-team.ru/cloud/mdb/internal/godogutil"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/ready"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/pgutil"
	"a.yandex-team.ru/cloud/mdb/internal/testutil/intapi"
	"a.yandex-team.ru/cloud/mdb/internal/testutil/workeremulation"
	"a.yandex-team.ru/cloud/mdb/katan/imp/pkg/app"
	"a.yandex-team.ru/cloud/mdb/katan/imp/pkg/imp"
	kdbpg "a.yandex-team.ru/cloud/mdb/katan/internal/katandb/pg"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/nop"
	"a.yandex-team.ru/library/go/core/log/zap"
	"a.yandex-team.ru/library/go/core/xerrors"
)

const (
	metadbClusterName  = "metadb"
	katandbClusterName = "katandb"
)

var (
	queryHostsCount = sqlutil.Stmt{
		Name: "HostCounts",
		// language=PostgreSQL
		Query: "SELECT count(*) FROM katan.hosts",
	}
	queryClustersCount = sqlutil.Stmt{
		Name: "ClustersCounts",
		// language=PostgreSQL
		Query: "SELECT count(*) FROM katan.clusters",
	}
	queryClustersTagsVersion = sqlutil.Stmt{
		Name: "ClusterTagsVersion",
		// language=PostgreSQL
		Query: "SELECT tags->>'version' FROM katan.clusters",
	}
	querySetClustersTagsVersion = sqlutil.Stmt{
		Name: "ClusterTagsVersion",
		// language=PostgreSQL
		Query: `
UPDATE katan.clusters
   SET tags = jsonb_set(tags, '{version}', to_jsonb(CAST(:version AS integer)))
`,
	}
	queryRolloutShipmentsCount = sqlutil.Stmt{
		Name: "RolloutShipmentsCount",
		// language=PostgreSQL
		Query: "SELECT count(*) FROM katan.rollout_shipments",
	}
	queryClusterRolloutsStat = sqlutil.Stmt{
		Name: "ClusterRolloutsStat",
		// language=PostgreSQL
		Query: "SELECT CAST(state AS text), count(*) FROM katan.cluster_rollouts GROUP BY state",
	}
	queryRolloutsCount = sqlutil.Stmt{
		Name: "RolloutsCount",
		// language=PostgreSQL
		Query: "SELECT count(*) FROM katan.rollouts",
	}
	queryStartOnePendingRollout = sqlutil.Stmt{
		Name: "StartRollout",
		// language=PostgreSQL
		Query: `
WITH run_roll AS (
	UPDATE katan.rollouts
	   SET started_at = now(),
	       rolled_by = :rolled_by
	 WHERE rollout_id IN (
	     SELECT rollout_id
	       FROM katan.rollouts
	      WHERE started_at IS NULL
	      FETCH FIRST ROW ONLY)
	RETURNING rollout_id
), run_cluster_roll AS (
    UPDATE katan.cluster_rollouts
       SET state = 'running',
           updated_at = now()
     WHERE (rollout_id, cluster_id) = (
           SELECT rollout_id, cluster_id
             FROM katan.cluster_rollouts
            WHERE rollout_id = (SELECT rollout_id FROM run_roll)
            FETCH FIRST ROW ONLY)
    RETURNING 1)
SELECT * FROM run_cluster_roll`,
	}
	defaultImpConfig = app.DefaultConfig()
)

func ContextInitializer(tc *godogutil.TestContext, s *godog.Suite) {
	steps, err := NewSteps(tc)
	if err != nil {
		panic(err)
	}

	s.Step(`^I execute mdb-katan-imp$`, steps.iExecuteKatanImp)
	s.Step(`^mdb-katan-imp import (them|it)$`, steps.iExecuteKatanImp)
	s.Step(`^it succeeds$`, steps.itSucceeds)

	s.Step(`^I execute mdb-katan$`, steps.iExecuteMdbKatan)

	s.Step(`^I add rollout for$`, steps.iAddRolloutFor)
	s.Step(`^my rollout is finished$`, steps.myRolloutIsFinished)

	s.Step(`^I create "(\d+)" postgresql clusters$`, steps.iCreatePGClusters)
	s.Step(`^I create hadoop cluster$`, steps.iCreateHadoopCluster)
	s.Step(`^"(\d+)" postgresql clusters in metadb$`, steps.iCreatePGClusters)
	s.Step(`^postgresql cluster in metadb$`, steps.OnePGCluster)
	s.Step(`^I delete one postgresql cluster$`, steps.iDeleteOnePGCluster)
	s.Step(`^I create host in postgresql cluster$`, steps.iCreateHostInPGCluster)

	s.Step(`^in katandb there are "(\d+)" hosts$`, steps.inKatandbThereAreNumHosts)
	s.Step(`^in katandb there are "(\d+)" clusters$`, steps.inKatandbThereAreNumClusters)
	s.Step(`^in katandb there is one cluster$`, steps.inKatandbThereIsOneCluster)
	s.Step(`^there are "(\d+)" katan\.rollout_shipments$`, steps.thereAreKatanRolloutShipments)
	s.Step(`^there is one katan\.rollout_shipments$`, steps.thereIsOneKatanRolloutShipments)
	s.Step(`^katan\.cluster_rollouts statistics is$`, steps.clusterRolloutsStatisticsIs)
	s.Step(`^there are no rows in katan\.rollouts$`, steps.thereAreNoRowsInKatanRollouts)
	s.Step(`^empty databases$`, steps.emptyDatabases)
	s.Step(`^in katandb there is one cluster with latest tags version$`, steps.inKatandbThereIsOneClusterWithLatestTagsVersion)
	s.Step(`^I set tags it\'s tags version to (\d+)$`, steps.iSetTagsItsTagsVersionTo)
	s.Step(`^I start one pending rollout$`, steps.iStartOnePendingRollout)

	s.Step(`^all deploy shipments are "([^"]+)"$`, steps.allDeployShipmentsAre)
	s.Step(`^all deploy shipments are "([^"]*)" after "([^"]*)"$`, steps.allDeployShipmentsAreAfter)
	s.Step(`^all deploy minions in "([^"]*)" group$`, steps.allDeployMinionsInGroup)
	s.Step(`^all host in juggler are "([^"]+)"$`, steps.allHostInJugglerAre)
	s.Step(`^all host in juggler are "([^"]+)" before rollout$`, steps.allHostInJugglerBeforeRolloutAre)
	s.Step(`^all host in juggler are "([^"]+)" after rollout$`, steps.allHostInJugglerAfterRolloutAre)
	s.Step(`^all clusters in health are "([^"]*)"$`, steps.allClusterInHealthAre)
	s.Step(`^all clusters in health are "([^"]*)" before rollout$`, steps.allClusterInHealthAreBeforeRollout)
	s.Step(`^all clusters in health are "([^"]*)" after rollout$`, steps.allClusterInHealthAreAfterRollout)

	s.Step(`^katan\.schedule with maxSize=(\d+) for$`, steps.katanScheduleWithMaxSizeFor)
	s.Step(`^"([^"]+)" katan\.schedule with maxSize=(\d+) for$`, steps.katanScheduleInNamespaceWithMaxSizeFor)
	s.Step(`^katan\.schedule with options=({.*}) for$`, steps.katanScheduleWithOptionsFor)
	s.Step(`^my schedule in "([^"]*)" state$`, steps.myScheduleInState)
	s.Step(`^I execute mdb-katan-scheduler$`, steps.iExecuteMdbKatanScheduler)
	s.Step(`^I execute mdb-katan-scheduler with MaxRolloutFails=(\d+)$`, steps.iExecuteMdbKatanSchedulerWithMaxRolloutFails)
	s.Step(`^I execute mdb-katan-scheduler with ImportCooldown=(\w+)$`, steps.iExecuteMdbKatanSchedulerWithImportCooldown)
	s.Step(`^I mark my schedule as "([^"]*)"$`, steps.iMarkMyScheduleAs)

	s.Step(`^I sleep "([^"]*)"$`, steps.iSleep)
	s.Step(`^I run mdb-katan-zombie-rollouts with warn="([^"]*)" crit="([^"]*)"$`, steps.iRunMdbKatanZombieRollouts)
	s.Step(`^I run mdb-katan-broken-schedules with namespace="([^"]*)"$`, steps.iRunMdbKatanBrokenSchedulesWithNamespace)
	s.Step(`^monitoring returns "([^"]*)"$`, steps.monitoringReturns)
	s.Step(`^monitoring message matches$`, steps.monitoringMessageMatches)
	s.BeforeScenario(steps.BeforeScenario)
}

type Steps struct {
	L log.Logger

	intAPI *intapi.Client
	worker *workeremulation.Worker
	mdb    *sqlutil.Cluster
	kdb    *sqlutil.Cluster
	KC     *KatanContext
	SC     *SchedulerContext
	MC     *MonitoringContext

	TC                *godogutil.TestContext
	createdPGClusters []string
}

func NewSteps(tc *godogutil.TestContext) (*Steps, error) {
	l, err := zap.New(zap.KVConfig(log.DebugLevel))
	if err != nil {
		return nil, err
	}

	mdb, _, err := dbteststeps.NewReadyCluster(metadbClusterName, defaultImpConfig.Metadb.DB)
	if err != nil {
		return nil, err
	}
	kdb, _, err := dbteststeps.NewReadyCluster(katandbClusterName, defaultImpConfig.Katandb.DB)
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

	st := &Steps{
		L:      l,
		intAPI: intAPI,
		TC:     tc,
		mdb:    mdb,
		kdb:    kdb,
		KC:     &KatanContext{},
		SC: &SchedulerContext{
			kdb: kdb,
			L:   l,
		},
		MC: &MonitoringContext{
			kdb: kdbpg.NewWithCluster(kdb, l),
			L:   l,
		},
		worker: workeremulation.New("dummy", mdb),
	}

	return st, nil
}

func fillClusterAddr(clusterName string, config *pgutil.Config) {
	config.Addrs = []string{dbteststeps.DBHostPort(clusterName)}
	config.SSLMode = pgutil.AllowSSLMode
}

func (s *Steps) BeforeScenario(interface{}) {
	s.KC.Reset(s.L)
	s.SC.Reset(s.L)
}

func (s *Steps) iExecuteKatanImp() error {
	config := app.DefaultConfig()
	fillClusterAddr(metadbClusterName, &config.Metadb)
	fillClusterAddr(katandbClusterName, &config.Katandb)
	config.Timeout.Duration = time.Second * 30

	return app.RunWithContext(s.TC.Context(), config, s.L)
}

func (s *Steps) itSucceeds() error {
	return nil
}

func (s *Steps) iCreatePGClusters(clustersCount int) error {
	for i := 0; i < clustersCount; i++ {
		resp, err := s.intAPI.CreatePGCluster(intapi.CreatePostgresqlClusterRequest{Name: fmt.Sprintf("cluster-%d", i)})
		if err != nil {
			return err
		}
		err = s.worker.AcquireAndSuccessfullyCompleteTask(s.TC.Context(), resp.ID)
		if err != nil {
			return xerrors.Errorf("unable to finish task: %w", err)
		}
		s.createdPGClusters = append(s.createdPGClusters, resp.Metadata.ClusterID)
	}
	return nil
}

func (s *Steps) OnePGCluster() error {
	return s.iCreatePGClusters(1)
}

func (s *Steps) iCreateHadoopCluster() error {
	_, err := s.intAPI.CreateCluster("hadoop", intapi.CreateClusterRequest{Name: "hadoop"})
	return err
}

func (s *Steps) countQuery(stmt sqlutil.Stmt, expected int, kind string) error {
	var count int
	parser := func(rows *sqlx.Rows) error {
		return rows.Scan(&count)
	}
	_, err := sqlutil.QueryNode(s.TC.Context(), s.kdb.Primary(), stmt, nil, parser, &nop.Logger{})
	if err != nil {
		return err
	}
	if count != expected {
		return xerrors.Errorf("expect %d %s, but there are %d %s in katandb", expected, kind, count, kind)
	}
	return nil
}

func (s *Steps) inKatandbThereAreNumHosts(expected int) error {
	return s.countQuery(queryHostsCount, expected, "hosts")
}

func (s *Steps) inKatandbThereAreNumClusters(expected int) error {
	return s.countQuery(queryClustersCount, expected, "clusters")
}

func (s *Steps) inKatandbThereIsOneCluster() error {
	return s.inKatandbThereAreNumClusters(1)
}

func (s *Steps) thereAreKatanRolloutShipments(expected int) error {
	return s.countQuery(queryRolloutShipmentsCount, expected, "rollout_shipments")
}

func (s *Steps) thereIsOneKatanRolloutShipments() error {
	return s.thereAreKatanRolloutShipments(1)
}

func (s *Steps) iDeleteOnePGCluster() error {
	if len(s.createdPGClusters) == 0 {
		return xerrors.Errorf("there are no created clusters")
	}
	clusterID := s.createdPGClusters[0]
	_, err := s.intAPI.DeletePGCluster(clusterID)
	if err != nil {
		return err
	}
	s.createdPGClusters = s.createdPGClusters[1:]
	return nil
}

func (s *Steps) iCreateHostInPGCluster() error {
	if len(s.createdPGClusters) == 0 {
		return xerrors.Errorf("there are no created clusters")
	}
	clusterID := s.createdPGClusters[0]
	_, err := s.intAPI.CreatePGHost(clusterID)
	return err
}

func (s *Steps) emptyDatabases() error {
	master := s.kdb.Primary()
	if master == nil {
		return xerrors.New("katandb not available")
	}
	res, err := master.DBx().Exec(`
		DO $$
		    BEGIN
		        DELETE FROM katan.rollouts;
		        DELETE FROM katan.hosts;
		        DELETE FROM katan.schedules;
			END;
		$$`)
	if err != nil {
		return err
	}
	_, err = res.RowsAffected()
	if err != nil {
		return err
	}

	return metadbhelpers.CleanupMetaDB(s.TC.Context(), s.mdb.Primary())
}

func (s *Steps) inKatandbThereIsOneClusterWithLatestTagsVersion() error {
	if err := s.inKatandbThereAreNumClusters(1); err != nil {
		return err
	}

	var version int64
	if _, err := sqlutil.QueryNode(
		s.TC.Context(),
		s.kdb.Primary(),
		queryClustersTagsVersion,
		nil,
		func(rows *sqlx.Rows) error {
			return rows.Scan(&version)
		},
		&nop.Logger{}); err != nil {
		return err
	}

	if version != imp.TagsVersion {
		return fmt.Errorf("actual IMP's tag version is %d, from katandb we got: %d", imp.TagsVersion, version)
	}
	return nil
}

func (s *Steps) iSetTagsItsTagsVersionTo(version int) error {
	_, err := sqlutil.QueryNode(
		s.TC.Context(),
		s.kdb.Primary(),
		querySetClustersTagsVersion,
		map[string]interface{}{"version": version},
		sqlutil.NopParser,
		&nop.Logger{})
	return err
}

func (s *Steps) allDeployShipmentsAre(status string) error {
	return s.KC.AllShipmentsAre(status)
}

func (s *Steps) allDeployShipmentsAreAfter(toStatus, fromStatus string) error {
	if err := s.KC.AllShipmentsAre(toStatus); err != nil {
		return err
	}
	return s.KC.FirstShipmentAre(fromStatus)
}

func (s *Steps) allHostInJugglerAre(status string) error {
	s.KC.JugglerStatusBeforeRollout = status
	s.KC.JugglerStatusAfterRollout = status
	return nil
}

func (s *Steps) allHostInJugglerBeforeRolloutAre(status string) error {
	s.KC.JugglerStatusBeforeRollout = status
	return nil
}

func (s *Steps) allHostInJugglerAfterRolloutAre(status string) error {
	s.KC.JugglerStatusAfterRollout = status
	return nil
}

func (s *Steps) allClusterInHealthAre(status string) error {
	s.KC.ClusterHealthBeforeRollout = status
	s.KC.ClusterHealthAfterRollout = status
	return nil
}

func (s *Steps) allClusterInHealthAreBeforeRollout(status string) error {
	s.KC.ClusterHealthBeforeRollout = status
	return nil
}

func (s *Steps) allClusterInHealthAreAfterRollout(status string) error {
	s.KC.ClusterHealthAfterRollout = status
	return nil
}

func (s *Steps) allDeployMinionsInGroup(group string) error {
	s.KC.DeployGroup = group
	return nil
}

func (s *Steps) iExecuteMdbKatan() error {
	return s.KC.RunKatan(s.TC.Context(), s.kdb, s.L)
}

func (s *Steps) iAddRolloutFor(tags *gherkin.DocString) error {
	return s.SC.AddRolloutFor(s.TC.Context(), tags.Content)
}

func (s *Steps) iStartOnePendingRollout() error {
	rowsUpdated, err := sqlutil.QueryNode(
		s.TC.Context(),
		s.kdb.Primary(),
		queryStartOnePendingRollout,
		map[string]interface{}{
			"rolled_by": testKatanName,
		},
		sqlutil.NopParser,
		&nop.Logger{})
	if err != nil {
		return xerrors.Errorf("start rollout: %w", err)
	}
	if rowsUpdated != 1 {
		return xerrors.Errorf("expect one row update by StartRollout query, but %d updated", rowsUpdated)
	}
	return nil
}

func (s *Steps) myRolloutIsFinished() error {
	return s.SC.MyRolloutIsFinished(s.TC.Context())
}

func (s *Steps) clusterRolloutsStatisticsIs(stats *gherkin.DocString) error {
	expectedStat := make(map[string]int64)
	if err := yaml.Unmarshal([]byte(stats.Content), &expectedStat); err != nil {
		return xerrors.Errorf("failed to load expected status: %w", err)
	}

	realStat := make(map[string]int64)

	parser := func(rows *sqlx.Rows) error {
		var state string
		var count int64
		if err := rows.Scan(&state, &count); err != nil {
			return err
		}
		realStat[state] = count
		return nil
	}
	count, err := sqlutil.QueryNode(s.TC.Context(), s.kdb.Primary(), queryClusterRolloutsStat, nil, parser, &nop.Logger{})
	if err != nil {
		return err
	}
	if count == 0 {
		return xerrors.Errorf("got no rows. Looks like katan.cluster_rollouts is empty")
	}

	if !cmp.Equal(realStat, expectedStat) {
		return xerrors.Errorf("expect %+v, but got %+v. Diff: %s", expectedStat, realStat, cmp.Diff(expectedStat, realStat))
	}
	return nil
}

func (s *Steps) thereAreNoRowsInKatanRollouts() error {
	var count int64

	parser := func(rows *sqlx.Rows) error {
		return rows.Scan(&count)
	}
	_, err := sqlutil.QueryNode(s.TC.Context(), s.kdb.Primary(), queryRolloutsCount, nil, parser, &nop.Logger{})
	if err != nil {
		return err
	}
	if count != 0 {
		return xerrors.Errorf("Expect that there are no rows in katan.rollouts, by %d rows found", count)
	}
	return nil
}

func (s *Steps) katanScheduleWithMaxSizeFor(maxSize int, matchTags *gherkin.DocString) error {
	return s.SC.AddSchedule(s.TC.Context(), maxSize, matchTags.Content, "{}", "core")
}

func (s *Steps) katanScheduleInNamespaceWithMaxSizeFor(namespace string, maxSize int, matchTags *gherkin.DocString) error {
	return s.SC.AddSchedule(s.TC.Context(), maxSize, matchTags.Content, "{}", namespace)
}

func (s *Steps) katanScheduleWithOptionsFor(options string, matchTags *gherkin.DocString) error {
	return s.SC.AddSchedule(s.TC.Context(), 42, matchTags.Content, options, "core")
}

func (s *Steps) iExecuteMdbKatanScheduler() error {
	return s.SC.Run(s.TC.Context(), optional.Int64{}, optional.Duration{})
}

func (s *Steps) iExecuteMdbKatanSchedulerWithMaxRolloutFails(maxRolloutFails int) error {
	return s.SC.Run(s.TC.Context(), optional.NewInt64(int64(maxRolloutFails)), optional.Duration{})
}

func (s *Steps) iExecuteMdbKatanSchedulerWithImportCooldown(importCooldown string) error {
	dur, err := time.ParseDuration(importCooldown)
	if err != nil {
		return xerrors.Errorf("malformed ImportCooldown parameter: %w", err)
	}
	return s.SC.Run(s.TC.Context(), optional.Int64{}, optional.NewDuration(dur))
}

func (s *Steps) myScheduleInState(state string) error {
	return s.SC.MyScheduleStateIs(s.TC.Context(), state)
}

func (s *Steps) iMarkMyScheduleAs(state string) error {
	return s.SC.MarkMyScheduleAs(s.TC.Context(), state)
}

func (s *Steps) iSleep(arg string) error {
	d, err := time.ParseDuration(arg)
	if err != nil {
		return err
	}
	time.Sleep(d)
	return nil
}

func (s *Steps) iRunMdbKatanZombieRollouts(warn, crit string) error {
	w, err := time.ParseDuration(warn)
	if err != nil {
		return err
	}
	c, err := time.ParseDuration(crit)
	if err != nil {
		return err
	}
	return s.MC.iExecuteMDBKatanZombieRollouts(s.TC.Context(), w, c)
}

func (s *Steps) iRunMdbKatanBrokenSchedulesWithNamespace(namespace string) error {
	return s.MC.iExecuteMDBKataBrokenSchedules(s.TC.Context(), namespace)
}

func (s *Steps) monitoringReturns(code string) error {
	return s.MC.lastResultIs(code)
}

func (s *Steps) monitoringMessageMatches(rs *gherkin.DocString) error {
	return s.MC.lastMessageMatches(rs.Content)
}
