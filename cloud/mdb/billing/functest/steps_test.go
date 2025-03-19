package functest

import (
	"context"
	"encoding/json"
	"fmt"
	"strings"
	"time"

	"github.com/DATA-DOG/godog"
	"github.com/DATA-DOG/godog/gherkin"
	"github.com/jmoiron/sqlx"
	"gopkg.in/yaml.v2"

	bookkeeperApp "a.yandex-team.ru/cloud/mdb/billing/bookkeeper/pkg/app"
	"a.yandex-team.ru/cloud/mdb/billing/internal/billingdb"
	bdbpg "a.yandex-team.ru/cloud/mdb/billing/internal/billingdb/pg"
	"a.yandex-team.ru/cloud/mdb/billing/internal/metadb"
	mdbpg "a.yandex-team.ru/cloud/mdb/billing/internal/metadb/pg"
	senderApp "a.yandex-team.ru/cloud/mdb/billing/sender/pkg/app"
	apihelpers "a.yandex-team.ru/cloud/mdb/dbaas-internal-api-image/recipe/helpers"
	metadbhelpers "a.yandex-team.ru/cloud/mdb/dbaas_metadb/recipes/helpers"
	"a.yandex-team.ru/cloud/mdb/internal/dbteststeps"
	"a.yandex-team.ru/cloud/mdb/internal/diff"
	"a.yandex-team.ru/cloud/mdb/internal/godogutil"
	"a.yandex-team.ru/cloud/mdb/internal/ready"
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
	metadbClusterName    = "metadb"
	billingdbClusterName = "billingdb"
)

var (
	querySelectBackupsCount = sqlutil.Stmt{
		Name: "SelectBackupsCount",
		// language=PostgreSQL
		Query: "SELECT count(*) FROM dbaas.backups",
	}
	querySelectClusters = sqlutil.Stmt{
		Name: "SelectClusters",
		// language=PostgreSQL
		Query: "SELECT cid, type, name FROM dbaas.clusters",
	}
	queryEnableBilling = sqlutil.Stmt{
		Name: "EnableBilling",
		// language=PostgreSQL
		Query: `
INSERT INTO billing.tracks
    (cluster_id, cluster_type, bill_type, from_ts)
VALUES
    (:cluster_id, :cluster_type, :bill_type, :from_ts)
    `,
	}

	defaultBookkeeperConfig = bookkeeperApp.DefaultConfig()
)

func ContextInitializer(tc *godogutil.TestContext, s *godog.Suite) {
	steps, err := NewSteps(tc, s)
	if err != nil {
		panic(err)
	}

	s.Step(`^I create "(\d+)" (\w+) clusters$`, steps.iCreateCluster)
	s.Step(`^I list backups for "(\w+)" cluster and got`, steps.iListClusterBackups)
	s.Step(`^I execute mdb-billing-bookkeeper to bill (\w+) with config$`, steps.iExecuteBookkeeperWithConfig)
	s.Step(`^I execute mdb-billing-bookkeeper to bill (\w+)$`, steps.iExecuteBookkeeper)

	s.Step(`^I execute mdb-billing-sender for billing type "(\w+)"$`, steps.iExecuteSender)
	s.Step(`^I execute mdb-billing-sender for billing type "(\w+)" with config$`, steps.iExecuteSenderWithConfig)

	s.Step(`^all logbroker writes are (\w+)$`, steps.allLogbrokerWrites)
	s.Step(`^in metadb there are "(\d+)" backups`, steps.inMetaDBThereAreNumBackups)
	s.Step(`^I enable (\w+) billing for all clusters`, steps.iEnableBilling)

	s.Step(`^it succeeds$`, steps.itSucceeds)
	s.Step(`^I sleep "([^"]*)"$`, steps.iSleep)

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
	bdb    *sqlutil.Cluster

	TC *godogutil.TestContext

	createdClusters  Clusters
	lbWritesSucceeds bool
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
	mdb, node, err := dbteststeps.NewReadyCluster(metadbClusterName, defaultBookkeeperConfig.Metadb.DB)
	if err != nil {
		return nil, err
	}
	metadbSteps.Cluster = mdb
	metadbSteps.Node = node
	dbteststeps.RegisterStepsWithDBName(metadbSteps, s, metadbClusterName)

	billingdbSteps := &dbteststeps.DBSteps{
		L:          l,
		Migrations: make(map[string]struct{}),
		TC:         tc,
		Params: dbteststeps.Params{
			DBName: "billingdb",
		},
	}
	bdb, node, err := dbteststeps.NewReadyCluster(billingdbClusterName, defaultBookkeeperConfig.Billingdb.DB)
	if err != nil {
		return nil, err
	}
	billingdbSteps.Cluster = bdb
	billingdbSteps.Node = node
	dbteststeps.RegisterStepsWithDBName(billingdbSteps, s, billingdbClusterName)

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
		L:               l,
		intAPI:          intAPI,
		TC:              tc,
		mdb:             metadbSteps.Cluster,
		bdb:             billingdbSteps.Cluster,
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
	s.lbWritesSucceeds = true

	if err := metadbhelpers.CleanupMetaDB(s.TC.Context(), s.mdb.Primary()); err != nil {
		panic(err)
	}

	if err := cleanupBillingDB(s.TC.Context(), s.bdb.Primary()); err != nil {
		panic(err)
	}

	if err := apihelpers.CleanupTmpRootPath(""); err != nil {
		panic(err)
	}
}

var (
	queryCleanup = sqlutil.Stmt{
		Name:  "Cleanup",
		Query: "TRUNCATE TABLE billing.%s RESTART IDENTITY",
	}
)

func cleanupBillingDB(ctx context.Context, master sqlutil.Node) error {
	binding, err := sqlutil.BeginOnNode(ctx, master, nil)
	if err != nil {
		panic(err)
	}
	defer binding.Rollback(ctx)

	for _, name := range []string{
		"tracks",
		"metrics_queue",
	} {
		_, err := sqlutil.QueryTxBinding(ctx, binding, queryCleanup.Format(name), nil, sqlutil.NopParser, &nop.Logger{})
		if err != nil {
			return xerrors.Errorf("failed to cleanup table %s: %w", name, err)
		}
	}

	if err := binding.Commit(ctx); err != nil {
		return xerrors.Errorf("failed to commit billingdb cleanup: %w", err)
	}
	return nil
}

func (s *Steps) iExecuteBookkeeperWithConfig(cmd string, cfg *gherkin.DocString) error {
	appCfg := bookkeeperApp.DefaultConfig()
	if err := yaml.Unmarshal([]byte(cfg.Content), &appCfg.Bookkeeper); err != nil {
		return xerrors.Errorf("load bookkeeper config: %w", err)
	}

	return executeBookkeeper(s, cmd, appCfg)
}

func (s *Steps) iExecuteBookkeeper(cmd string) error {
	return executeBookkeeper(s, cmd, bookkeeperApp.DefaultConfig())
}

func executeBookkeeper(s *Steps, cmd string, appCfg bookkeeperApp.Config) error {
	fillClusterAddr(metadbClusterName, &appCfg.Metadb)
	mdb, err := mdbpg.New(appCfg.Metadb, log.With(s.L, log.String("cluster", mdbpg.DBName)))
	if err != nil {
		return err
	}

	fillClusterAddr(billingdbClusterName, &appCfg.Billingdb)
	bdb, err := bdbpg.New(appCfg.Billingdb, log.With(s.L, log.String("cluster", bdbpg.DBName)))
	if err != nil {
		return err
	}

	mdbAwaitCtx, cancel := context.WithTimeout(s.TC.Context(), time.Minute)
	defer cancel()
	if err = ready.Wait(mdbAwaitCtx, mdb, &ready.DefaultErrorTester{Name: metadbClusterName, L: s.L}, time.Second); err != nil {
		return xerrors.Errorf("wait backend: %w", err)
	}

	bdbAwaitCtx, cancel := context.WithTimeout(s.TC.Context(), time.Minute)
	defer cancel()
	if err = ready.Wait(bdbAwaitCtx, bdb, &ready.DefaultErrorTester{Name: billingdbClusterName, L: s.L}, time.Second); err != nil {
		return xerrors.Errorf("wait backend: %w", err)
	}

	s.L.Debugf("Starting sender bookkeeper config: %+v", appCfg)
	switch cmd {
	case "backups":
		bookkeeper, err := bookkeeperApp.NewBackupBookkeeperFromExtDeps(mdb, bdb, s.L, appCfg.Bookkeeper)
		if err != nil {
			return err
		}
		return bookkeeper.Bill(s.TC.Context())
	default:
		return xerrors.Errorf("unknown command: %s", cmd)
	}
}

func (s *Steps) iExecuteSenderWithConfig(btype string, cfg *gherkin.DocString) error {
	appCfg := senderApp.DefaultConfig()
	if err := yaml.Unmarshal([]byte(cfg.Content), &appCfg.Sender); err != nil {
		return xerrors.Errorf("load sender config: %w", err)
	}
	return executeSender(s, btype, appCfg)
}

func (s *Steps) iExecuteSender(btype string) error {
	return executeSender(s, btype, senderApp.DefaultConfig())
}

func executeSender(s *Steps, rawBillType string, appCfg senderApp.Config) error {
	fillClusterAddr(billingdbClusterName, &appCfg.Billingdb)
	bdb, err := bdbpg.New(appCfg.Billingdb, log.With(s.L, log.String("cluster", bdbpg.DBName)))
	if err != nil {
		return err
	}

	bdbAwaitCtx, cancel := context.WithTimeout(s.TC.Context(), time.Minute)
	defer cancel()
	if err = ready.Wait(bdbAwaitCtx, bdb, &ready.DefaultErrorTester{Name: billingdbClusterName, L: s.L}, time.Second); err != nil {
		return xerrors.Errorf("wait backend: %w", err)
	}

	billType, err := billingdb.ParseBillType(rawBillType)
	if err != nil {
		return err
	}

	s.L.Debugf("Starting sender with config: %+v", appCfg)
	senderImpl, err := senderApp.NewMockMetricsSenderFromExtDeps(billType, bdb, s.L, appCfg.Sender, s.lbWritesSucceeds)
	if err != nil {
		return err
	}

	return senderImpl.Serve(s.TC.Context())
}

func (s *Steps) allLogbrokerWrites(result string) error {
	switch result {
	case "successful":
		s.lbWritesSucceeds = true
	case "failed":
		s.lbWritesSucceeds = false
	default:
		return xerrors.Errorf("unknown lb write result: %s", result)
	}
	return nil
}

func (s *Steps) itSucceeds() error {
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
			return xerrors.Errorf("finish task: %w", err)
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
		return xerrors.Errorf("unmarshal body %s: %w", expectedDoc.Content, err)
	}

	backupsbytes, err := json.Marshal(backups)
	if err != nil {
		return xerrors.Errorf("marshall backup listing result: %+v", backupsbytes)
	}
	actual := make(map[string]interface{})
	if err := json.Unmarshal(backupsbytes, &actual); err != nil {
		return xerrors.Errorf("unmarshal response body %s: %w", string(backupsbytes), err)
	}
	return diff.Full(expected, actual)
}

func (s *Steps) inMetaDBThereAreNumBackups(expected int) error {
	return s.countQuery(querySelectBackupsCount, expected, "backups")
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

func (s *Steps) iSleep(arg string) error {
	d, err := time.ParseDuration(arg)
	if err != nil {
		return err
	}
	time.Sleep(d)
	return nil
}

type cluster struct {
	ClusterID string             `db:"cid"`
	Name      string             `db:"name"`
	Type      metadb.ClusterType `db:"type"`
}

func metadbClusters(ctx context.Context, mdb *sqlutil.Cluster) ([]cluster, error) {
	var clusters []cluster

	parser := func(rows *sqlx.Rows) error {
		var row cluster
		if err := rows.StructScan(&row); err != nil {
			return err
		}
		clusters = append(clusters, row)
		return nil
	}

	_, err := sqlutil.QueryNode(
		ctx,
		mdb.Primary(),
		querySelectClusters,
		nil,
		parser,
		&nop.Logger{},
	)

	if err != nil {
		return nil, err
	}
	return clusters, nil

}

func (s *Steps) iEnableBilling(btype string) error {
	clusters, err := metadbClusters(s.TC.Context(), s.mdb)
	if err != nil {
		return err
	}
	for _, cluster := range clusters {
		_, err := sqlutil.QueryNode(
			s.TC.Context(),
			s.bdb.Primary(),
			queryEnableBilling,
			map[string]interface{}{
				"cluster_id":   cluster.ClusterID,
				"cluster_type": cluster.Type,
				"bill_type":    strings.ToUpper(btype),
				"from_ts":      time.Now(),
			},
			sqlutil.NopParser,
			&nop.Logger{},
		)
		if err != nil {
			return err
		}
	}

	return nil
}
