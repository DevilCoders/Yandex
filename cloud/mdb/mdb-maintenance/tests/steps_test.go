package tests

import (
	"database/sql"
	"encoding/json"
	"fmt"
	"io/ioutil"
	"os"
	"path/filepath"
	"regexp"
	"sort"
	"strings"
	"time"

	"github.com/DATA-DOG/godog"
	"github.com/DATA-DOG/godog/gherkin"
	"github.com/jackc/pgtype"
	"github.com/jmoiron/sqlx"
	"google.golang.org/grpc/codes"
	"gopkg.in/yaml.v2"

	apihelpers "a.yandex-team.ru/cloud/mdb/dbaas-internal-api-image/recipe/helpers"
	metadbhelpers "a.yandex-team.ru/cloud/mdb/dbaas_metadb/recipes/helpers"
	"a.yandex-team.ru/cloud/mdb/internal/config"
	"a.yandex-team.ru/cloud/mdb/internal/dbteststeps"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil/grpcmocker/expectations"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/cloud/mdb/internal/testutil/intapi"
	"a.yandex-team.ru/cloud/mdb/mdb-maintenance/internal/metadb/pg"
	"a.yandex-team.ru/cloud/mdb/mdb-maintenance/internal/models"
	"a.yandex-team.ru/library/go/core/log/nop"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/slices"
	"a.yandex-team.ru/library/go/test/yatest"
)

var (
	querySetClickHouseMinorVersion = sqlutil.Stmt{
		Name: "SetClickHouseMinorVersion",
		// language=PostgreSQL
		Query: `
WITH upd_pillar AS (
UPDATE dbaas.pillar
   SET value = jsonb_set(value, '{data,clickhouse,ch_version}', to_jsonb(CAST(:ver AS text)))
 WHERE subcid = (
    SELECT subcid
      FROM dbaas.subclusters
     WHERE cid = :cid
       AND roles && '{clickhouse_cluster}')
RETURNING subcid
)
SELECT count(*) FROM upd_pillar`,
	}
	querySetClickHouseMixedGranularityParts = sqlutil.Stmt{
		Name: "SetClickHouseMixedGranularityParts",
		// language=PostgreSQL
		Query: `
WITH upd_pillar AS (
UPDATE dbaas.pillar
   SET value = jsonb_set(value, '{data,clickhouse,config,merge_tree,enable_mixed_granularity_parts}', to_jsonb(CAST(:value AS bool)))
 WHERE subcid = (
    SELECT subcid
      FROM dbaas.subclusters
     WHERE cid = :cid
       AND roles && '{clickhouse_cluster}')
RETURNING subcid
)
SELECT count(*) FROM upd_pillar`,
	}
	querySetClickHouseCloudStorage = sqlutil.Stmt{
		Name: "SetClickHouseCloudStorage",
		// language=PostgreSQL
		Query: `
WITH upd_pillar AS (
UPDATE dbaas.pillar
   SET value = jsonb_set(value, '{data,cloud_storage,enabled}', to_jsonb(CAST(:value AS bool)))
 WHERE subcid = (
    SELECT subcid
      FROM dbaas.subclusters
     WHERE cid = :cid
       AND roles && '{clickhouse_cluster}')
RETURNING subcid
)
SELECT count(*) FROM upd_pillar`,
	}
	querySetClickHouseUserManagementV2 = sqlutil.Stmt{
		Name: "SetClickHouseUserManagementV2",
		// language=PostgreSQL
		Query: `
WITH upd_pillar AS (
UPDATE dbaas.pillar
   SET value = jsonb_set(value, '{data,clickhouse,user_management_v2}', to_jsonb(CAST(:value AS bool)))
 WHERE subcid = (
    SELECT subcid
      FROM dbaas.subclusters
     WHERE cid = :cid
       AND roles && '{clickhouse_cluster}')
RETURNING subcid
)
SELECT count(*) FROM upd_pillar`,
	}

	querySetPillarBoolValue = sqlutil.Stmt{
		Name: "SetPillarBoolValue",
		// language=PostgreSQL
		Query: `SELECT code.easy_update_pillar(:cid, :path, to_jsonb(CAST(:value AS bool)))`,
	}

	querySetPillarEmptyPath = sqlutil.Stmt{
		Name: "SetPillarEmptyPath",
		// language=PostgreSQL
		Query: `SELECT code.easy_update_pillar(:cid, :path, '{}')`,
	}

	querySetPillarStringValue = sqlutil.Stmt{
		Name: "SetPillarStringValue",
		// language=PostgreSQL
		Query: `SELECT code.easy_update_pillar(:cid, :path, to_jsonb(CAST(:value AS text)))`,
	}

	querySetPillarValue = sqlutil.Stmt{
		Name: "SetPillarValue",
		// language=PostgreSQL
		Query: `SELECT code.easy_update_pillar(:cid, :path, :value)`,
	}

	queryGetPGClusterID = sqlutil.Stmt{
		Name: "GetPGClusterID",
		// language=PostgreSQL
		Query: "SELECT cid FROM dbaas.clusters WHERE type = 'postgresql_cluster'",
	}

	querySelectMWTasksCount = sqlutil.Stmt{
		Name: "SelectMWTasksCount",
		// language=PostgreSQL
		Query: "SELECT count(*) FROM dbaas.maintenance_tasks",
	}

	querySelectMajorVersionsCount = sqlutil.Stmt{
		Name: "SelectMajorVersionsCount",
		// language=PostgreSQL
		Query: "SELECT count(DISTINCT major_version) FROM dbaas.versions WHERE cid = :cid",
	}

	queryClusterMWTaskStatus = sqlutil.Stmt{
		Name: "ClusterMWTaskStatus",
		// language=PostgreSQL
		Query: "SELECT status FROM dbaas.maintenance_tasks WHERE config_id=:config_id AND cid=:cid",
	}
	queryRejectMWTask = sqlutil.Stmt{
		Name: "RejectMWTask",
		// language=PostgreSQL
		Query: "SELECT code.reject_task('dummy', :task_id, '{}'::::jsonb, 'rejected by test', i_force => true)",
	}
	queryLockCluster = sqlutil.Stmt{
		Name: "LockCluster",
		// language=PostgreSQL
		Query: "SELECT rev FROM code.lock_cluster(:cid)",
	}
	queryCompleteClusterChange = sqlutil.Stmt{
		Name: "CompleteClusterChange",
		// language=PostgreSQL
		Query: "SELECT code.complete_cluster_change(:cid, :rev)",
	}
	queryChangeClusterDescription = sqlutil.Stmt{
		Name: "ChangeClusterDescription",
		// language=PostgreSQL
		Query: "UPDATE dbaas.clusters SET description=:description WHERE cid=:cid",
	}

	querySetClusterMWSettings = sqlutil.Stmt{
		Name: "SetClusterMWSettings",
		// language=PostgreSQL
		Query: "delete from dbaas.maintenance_window_settings where cid=:cid; insert into dbaas.maintenance_window_settings values (:cid, :mwDay, :mwHour);",
	}

	querySelectClusterMWSettings = sqlutil.Stmt{
		Name: "SelectClusterMWSettings",
		// language=PostgreSQL
		Query: "select day, hour from dbaas.maintenance_window_settings where cid = :cid",
	}

	queryGetDelayedUntil = sqlutil.Stmt{
		Name: "GetDelayedUntil",
		// language=PostgreSQL
		Query: `select delayed_until from dbaas.maintenance_tasks m join dbaas.worker_queue using (task_id) where m.cid = :cid limit 1`,
	}

	queryClusterMaintenanceTaskTimeout = sqlutil.Stmt{
		Name: "ClusterMaintenanceTaskTimeout",
		// language=PostgreSQL
		Query: "SELECT timeout FROM dbaas.worker_queue JOIN dbaas.maintenance_tasks USING (cid) WHERE cid=:cid AND result IS NULL",
	}

	querySetMajorVersion = sqlutil.Stmt{
		Name: "SetMajorVersion",
		// language=PostgreSQL
		Query: `UPDATE dbaas.versions SET major_version = :version WHERE cid = :cid`,
	}

	querySetMinorVersion = sqlutil.Stmt{
		Name: "SetMinorVersion",
		// language=PostgreSQL
		Query: `UPDATE dbaas.versions SET minor_version = :version WHERE cid = :cid AND component = :component`,
	}

	querySetEdition = sqlutil.Stmt{
		Name: "SetEdition",
		// language=PostgreSQL
		Query: `UPDATE dbaas.versions SET edition = :edition WHERE cid = :cid`,
	}

	queryCheckClusterEdition = sqlutil.Stmt{
		Name: "CheckClusterEdition",
		// language=PostgreSQL
		Query: `select edition from dbaas.versions WHERE cid = :cid limit 1`,
	}

	querySetComponentVersion = sqlutil.Stmt{
		Name: "SetComponentVersion",
		// language=PostgreSQL
		Query: `UPDATE dbaas.versions SET minor_version = :version WHERE cid = :cid and component = :component`,
	}

	querySetHostParams = sqlutil.Stmt{
		Name: "SetHostParams",
		// language=PostgreSQL
		Query: `
UPDATE dbaas.hosts SET vtype_id = :vtype WHERE fqdn = (
    SELECT fqdn FROM dbaas.hosts JOIN dbaas.subclusters USING (subcid) JOIN dbaas.clusters USING (cid)
    	WHERE clusters.name = :cname
	ORDER BY fqdn
    LIMIT 1
    );
UPDATE dbaas.hosts_revs SET vtype_id = :vtype WHERE fqdn = (
    SELECT fqdn FROM dbaas.hosts JOIN dbaas.subclusters USING (subcid) JOIN dbaas.clusters USING (cid)
    	WHERE clusters.name = :cname
    ORDER BY fqdn
    LIMIT 1
    );`,
	}

	queryGetTaskArguments = sqlutil.Stmt{
		Name: "GetTaskArguments",
		// language=PostgreSQL
		Query: `SELECT task_args FROM dbaas.worker_queue WHERE cid = :cid AND task_type = :taskType ORDER BY create_ts LIMIT 1`,
	}

	querySetClusterEnv = sqlutil.Stmt{
		Name: "SetClusterEnv",
		// language=PostgreSQL
		Query: `UPDATE dbaas.clusters SET env = :env WHERE cid = :cid`,
	}
	querySetTLSExpiration = sqlutil.Stmt{
		Name: "SetTlsExpiration",
		// language=PostgreSQL
		Query: `
INSERT INTO dbaas.pillar (fqdn, value)
SELECT
	h.fqdn,
	jsonb_build_object('cert.expiration', to_jsonb(CAST (:expiration AS TEXT)))
FROM dbaas.hosts h
JOIN dbaas.subclusters sc USING (subcid)
WHERE sc.cid = :cid
ON CONFLICT (fqdn) WHERE fqdn IS NOT NULL
DO UPDATE
	SET value = jsonb_set(excluded.value, '{cert.expiration}', to_jsonb(CAST (:expiration as text)))`,
	}

	queryLastTaskForMWConfig = sqlutil.Stmt{
		Name: "LastTaskForMWConfig",
		// language=PostgreSQL
		Query: `SELECT task_id
FROM dbaas.worker_queue
WHERE cid = :cid
  AND config_id = :config_id
ORDER BY create_ts DESC
LIMIT 1`,
	}
	queryClusterStatus = sqlutil.Stmt{
		Name: "SelectClusterStatus",
		// language=PostgreSQL
		Query: `SELECT status FROM dbaas.clusters WHERE cid = :cid`,
	}

	queryMarkAllMaintenanceTasksAsCompleted = sqlutil.Stmt{
		Name: "MarkAllMaintenanceTasksAsCompleted",
		// language=PostgreSQL
		Query: `update dbaas.maintenance_tasks set status = 'COMPLETED'`,
	}

	queryIncreaseCloudQuota = sqlutil.Stmt{
		Name: "IncreaseCloudQuota",
		// language=PostgreSQL
		Query: `update dbaas.clouds set
		cpu_quota = 1000,
		memory_quota = 1030792151040000,
		clusters_quota = 100,
		ssd_space_quota = 6442450944000000;
		update dbaas.clouds_revs set
		cpu_quota = 1000,
		memory_quota = 1030792151040000,
		clusters_quota = 100,
		ssd_space_quota = 6442450944000000; `,
	}

	queryGetClusterRev = sqlutil.Stmt{
		Name: "GetClusterRev",
		// language=PostgreSQL
		Query: `select actual_rev from dbaas.clusters where cid = :cid`,
	}

	queryGetClusterName = sqlutil.Stmt{
		Name: "GetClusterName",
		// language=PostgreSQL
		Query: `select name from dbaas.clusters where cid = :cid`,
	}

	querySetComputeFlavor = sqlutil.Stmt{
		Name: "SetComputeFlavor",
		// language=PostgreSQL
		Query: `
UPDATE dbaas.hosts
   SET flavor = (
    SELECT id
      FROM dbaas.flavors
      WHERE vtype = 'compute'
      LIMIT 1
   )
 WHERE fqdn = (
    SELECT fqdn
      FROM dbaas.hosts h
      JOIN dbaas.subclusters s using (subcid)
     WHERE s.cid = :cid);
 `,
	}

	querySetClusterCreateTimeToYearAgo = sqlutil.Stmt{
		Name: "SetClusterCreateTimeToYearAgo",
		// language=PostgreSQL
		Query: `
UPDATE dbaas.clusters
SET created_at = now() - '1 year'::::interval WHERE cid = :cid
 `,
	}

	querySetClickHouseZooKeeperVersion = sqlutil.Stmt{
		Name: "SetClickHouseZooKeeperVersion",
		// language=PostgreSQL
		Query: `
WITH upd_pillar AS (
UPDATE dbaas.pillar
   SET value = jsonb_set(value, '{data,zk,version}', to_jsonb(CAST(:value AS text)))
 WHERE subcid = (
    SELECT subcid
      FROM dbaas.subclusters
     WHERE cid = :cid
       AND roles && '{zk}')
RETURNING subcid
)
SELECT count(*) FROM upd_pillar`,
	}

	queryInsertDefaultFeatureFlag = sqlutil.Stmt{
		Name:  "InsertDefaultFeatureFlag",
		Query: "INSERT INTO dbaas.default_feature_flags (flag_name) VALUES (:flag)",
	}
)

func (tstc *TestContext) RegisterSteps(s *godog.Suite) {
	s.Step(`^I successfully load config with name "(.+)"$`, tstc.iSuccessfullyLoadConfig)
	s.Step(`^"([^"]*)" config$`, tstc.config)
	s.Step(`^I load all configs$`, tstc.loadAllConfigs)
	s.Step(`^I load all configs except ones that match "([^"]+)"$`, tstc.loadAllConfigsExcept)
	s.Step(`^cluster selection successfully returns$`, tstc.clusterSelectionSuccessfullyReturns)
	s.Step(`^I successfully execute pillar_change on "([^"]+)"$`, tstc.pillarChangeSuccessfully)
	s.Step(`^I maintain "([^"]+)" config$`, tstc.iMaintainConfig)
	s.Step(`^I check that "([^"]+)" cluster revision doesn't change while pillar_change$`, tstc.revisionsCheck)
	s.Step(`^I maintain all "([^"]+)" configs$`, tstc.maintainAllConfigs)
	s.Step(`^I increase cloud quota$`, tstc.increaseCloudQuota)
	s.Step(`^exists "([^"]*)" maintenance task on "([^"]*)" in "([^"]*)" status$`, tstc.existsMaintenanceTaskOnInStatus)
	s.Step(`^cluster "([^"]*)" has "(\d+)" major version$`, tstc.clusterHasNMajorVersions)
	s.Step(`^maintenance task "([^"]*)" on "([^"]*)" was REJECTED$`, tstc.rejectMaintenanceTask)
	s.Step(`^there is one maintenance task$`, tstc.thereIsOneMaintenanceTask)
	s.Step(`^there are "(\d+)" maintenance tasks$`, tstc.thereAreNMaintenanceTasks)
	s.Step(`^"([^"]+)" maintenance task has timeout="([^"]+)"$`, tstc.iClusterMaintenanceTaskHasTimeout)
	s.Step(`^I add default feature flag "([^"]+)"$`, tstc.insertDefaultFeatureFlag)
	s.Step(`^In cluster "([^"]+)" I set pillar path "([^"]+)" to bool "([^"]+)"$`, tstc.setPillarBoolValue)
	s.Step(`^In cluster "([^"]+)" I set pillar path "([^"]+)" to string "([^"]+)"$`, tstc.setPillarStringValue)
	s.Step(`^In cluster "([^"]+)" I set pillar path "([^"]+)"$`, tstc.setPillarEmptyPath)
	s.Step(`^In cluster "([^"]+)" I use compute flavor$`, tstc.setComputeFlavor)
	s.Step(`^Change cluster "([^"]+)" create time to year ago$`, tstc.setClusterCreateTimeToYearAgo)
	s.Step(`^I check config names starts with db names$`, tstc.checkConfigNames)
	s.Step(`^I add "([^"]+)" database$`, tstc.addDBName)
	s.Step(`^I set cluster "([^"]*)" mw day to "([^"]*)", mw hour to "(\d+)"$`, tstc.iSetClusterMWSettingsTo)
	s.Step(`^Cluster "([^"]*)" MW settings matches with task$`, tstc.ClusterMWSettingsMatchesWithTask)

	s.Step(`^I set cluster "([^"]*)" edition to "([^"]*)"$`, tstc.setClusterEdition)
	s.Step(`^cluster "([^"]*)" edition is equal to "([^"]*)"$`, tstc.checkClusterEdition)
	s.Step(`^clickhouse cluster with "([^"]+)" version$`, tstc.clickhouseClusterWithVersion)
	s.Step(`^clickhouse cluster with "([^"]+)" version and name "([^"]+)"$`, tstc.clickhouseClusterWithVersionAndName)
	s.Step(`^clickhouse cluster with zookeeper$`, tstc.clickhouseClusterWithZookeeper)
	s.Step(`^clickhouse cluster with zookeeper and name "([^"]+)"$`, tstc.clickhouseClusterWithZookeeperAndName)
	s.Step(`^stopped clickhouse cluster with name "([^"]+)"$`, tstc.stoppedClickHouseCluster)
	s.Step(`^I set enable_mixed_granularity_parts on cluster "([^"]+)" to "([^"]+)"$`, tstc.setEnableMixedGranularityParts)
	s.Step(`^I set zookeeper version on cluster "([^"]+)" to "([^"]+)"$`, tstc.setClickHouseZooKeeperVersion)
	s.Step(`^I set cloud_storage on cluster "([^"]+)" to "([^"]+)"$`, tstc.setClickHouseCloudStorage)
	s.Step(`^I set user_management_v2 on cluster "([^"]+)" to "([^"]+)"$`, tstc.setUserManagementV2)
	s.Step(`^mongodb cluster with "([^"]+)" major version$`, tstc.mongodbClusterWithMajorVersion)
	s.Step(`^mongodb cluster with "([^"]+)" full_num version and name "([^"]+)"$`, tstc.mongodbClusterWithFullnumVersionAndName)
	s.Step(`^mongodb cluster with "([^"]+)" full_num version$`, tstc.mongodbClusterWithFullnumVersion)
	s.Step(`^redis cluster with "([^"]+)" minor version$`, tstc.redisClusterWithMinorVersion)
	s.Step(`^redis cluster with "([^"]+)" minor version and name "([^"]+)"$`, tstc.redisClusterWithMinorVersionAndName)
	s.Step(`^mysql cluster with "([^"]+)" minor version$`, tstc.mysqlClusterWithMinorVersion)
	s.Step(`^mysql cluster with "([^"]+)" minor version and name "([^"]*)"$`, tstc.mysqlClusterWithMinorVersionAndName)
	s.Step(`^postgresql cluster$`, tstc.postgresqlCluster)
	s.Step(`^stopped postgresql cluster with name "([^"]+)"$`, tstc.stoppedPostgresqlCluster)
	s.Step(`^CMS up and running with task for cluster "([^"]+)"`, tstc.cmsUpAndRunningWithTask)
	s.Step(`^postgresql cluster with name "([^"]*)" and "([^"]*)" connection_pooler$`, tstc.postgresqlClusterWithConnectionPooler)
	s.Step(`^postgresql cluster with name "([^"]*)" and "([^"]*)" environment and "([^"]*)" connection_pooler$`, tstc.postgresqlClusterWithConnectionPoolerAndEnv)
	s.Step(`^greenplum cluster with name "([^"]*)"$`, tstc.greenplumCluster)
	s.Step(`^postgresql cluster with "([^"]+)" minor version$`, tstc.postgresqlClusterWithMinorVersion)
	s.Step(`^postgresql cluster with "([^"]+)" minor version and name "([^"]+)"$`, tstc.postgresqlClusterWithMinorVersionAndName)
	s.Step(`^elasticsearch cluster with name "([^"]*)"$`, tstc.elasticsearchCluster)
	s.Step(`^I set "([^"]+)" "([^"]+)" version to "([^"]+)"$`, tstc.setComponentVersion)
	s.Step(`^I set "([^"]+)" cluster "([^"]+)" major version to "([^"]+)", minor version to "([^"]+)"$`, tstc.setVersions)
	s.Step(`^cluster "([^"]+)" have "([^"]+)" task with argument "([^"]+)" equal to$`, tstc.taskArgumentsContains)
	s.Step(`^move cluster "([^"]+)" to "([^"]+)" env$`, tstc.moveClusterToEnv)
	s.Step(`^set tls expirations of hosts of cluster "([^"]+)" to "([^"]+)"$`, tstc.setTLSExpiration)
	s.Step(`^I initiate host creation in postgresql cluster$`, tstc.iCreateHostInPostgreSQLCluster)
	s.Step(`^I change description of cluster "(.+)" to "(.+)"$`, tstc.iChangeClusterDescription)
	s.Step(`^there is last sent multiple notification to cloud "([^"]+)"$`, tstc.thereIsLastSentMultipleNotification)
	s.Step(`^there is last sent single notification to cloud "([^"]+)" for cluster "([^"]+)"$`, tstc.thereIsLastSentSingleNotification)
	s.Step(`^there are total "(\d+)" notifications to cloud "([^"]+)" for cluster "([^"]+)"$`, tstc.thereIsTotalNotificationsForCluster)
	s.Step(`^worker task for cluster "([^"]*)" and config "([^"]*)" is acquired by worker$`, tstc.workerTaskForClusterAndConfigIsAcquiredByWorker)
	s.Step(`^worker task "([^"]*)" is acquired and completed by worker$`, tstc.workerTaskAcquiredAndCompletedByWorker)
	s.Step(`^cluster "([^"]+)" is in "([^"]+)" status$`, tstc.clusterIsInStatus)
	s.BeforeScenario(tstc.BeforeScenario)
	s.AfterScenario(dbteststeps.DumpOnErrorHook(tstc.TC, tstc.mdb.Primary(), "metadb", "dbaas"))
	s.AfterScenario(tstc.AfterScenario)
}

func (tstc *TestContext) BeforeScenario(interface{}) {
	if err := metadbhelpers.CleanupMetaDB(tstc.TC.Context(), tstc.mdb.Primary()); err != nil {
		panic(err)
	}
	if err := os.RemoveAll(apihelpers.MustTmpRootPath("")); err != nil {
		panic(fmt.Sprintf("failed to cleanup tmp root dir: %s", err))
	}
	tstc.stepsConfig = make(map[string]models.MaintenanceTaskConfig)
}

func (tstc *TestContext) AfterScenario(interface{}, error) {
	tstc.sender.Clear()
}

func (tstc *TestContext) iSuccessfullyLoadConfig(configName string) error {
	cfg, err := tstc.App.LoadConfig(configName + ".yaml")
	if err != nil {
		tstc.LoadedMT = nil
		return err
	}
	tstc.LoadedMT = &cfg
	tstc.stepsConfig[configName] = cfg
	return nil
}

func (tstc *TestContext) config(configName string, configYaml *gherkin.DocString) error {
	cfg := models.DefaultTaskConfig()
	if err := yaml.Unmarshal([]byte(configYaml.Content), &cfg); err != nil {
		return err
	}
	cfg.ID = configName
	tstc.stepsConfig[configName] = cfg
	tstc.App.L().Debugf("config is %+v", cfg)
	return nil
}

var mapStepNames = map[string]string{
	"check if primary": "CHECK_IF_PRIMARY",
}

func (tstc *TestContext) cmsMatcherByTask(task CMSTaskDescription) {
	stepNames := make([]string, len(task.ExecutedSteps))
	for ind, s := range task.ExecutedSteps {
		stepName, ok := mapStepNames[s]
		if !ok {
			panic(fmt.Sprintf("no map for %q", s))
		}
		stepNames[ind] = fmt.Sprintf("\"%s\"", stepName)
	}
	createdAt := time.Now().Add(-task.CreatedFromNow).Format(time.RFC3339)

	tstc.cmsMatcherState.Matchers["/yandex.mdb.cms.v1.InstanceOperationService/List"] = &expectations.MatcherResponder{
		Responder: &expectations.ResponderExact{Response: fmt.Sprintf(`{"operations": [{
                  "id": "test-id",
                  "instance_id": "%s",
                  "executed_steps": [%s],
                  "created_at": "%s"
}]}`, task.InstanceID, strings.Join(stepNames, ","), createdAt), Code: codes.OK},
	}
}

func (tstc *TestContext) metaDBHostVtypeID(clusterName, instanceID string) error {
	_, err := sqlutil.QueryContext(
		tstc.TC.Context(),
		tstc.mdb.PrimaryChooser(),
		querySetHostParams,
		map[string]interface{}{"vtype": instanceID, "cname": clusterName},
		func(rows *sqlx.Rows) error {
			return nil
		},
		&nop.Logger{},
	)
	return err
}

func (tstc *TestContext) cmsUpAndRunningWithTask(clusterName string, taskYAML *gherkin.DocString) error {
	taskDescription := CMSTaskDescription{}
	if err := yaml.Unmarshal([]byte(taskYAML.Content), &taskDescription); err != nil {
		return err
	}
	if err := tstc.metaDBHostVtypeID(clusterName, taskDescription.InstanceID); err != nil {
		return err
	}
	tstc.cmsMatcherByTask(taskDescription)
	return nil
}

func (tstc *TestContext) clusterSelectionSuccessfullyReturns(arg1 *gherkin.DocString) error {
	var expected []string
	if err := yaml.Unmarshal([]byte(arg1.Content), &expected); err != nil {
		return xerrors.Errorf("failed to unmarshal %q: %w", arg1.Content, err)
	}
	actualClusters, err := tstc.App.Mnt.SelectClusters(tstc.TC.Context(), *tstc.LoadedMT)
	if err != nil {
		return err
	}
	actual := make([]string, len(actualClusters))
	for i := 0; i < len(actualClusters); i++ {
		actual[i] = actualClusters[i].ID
	}
	if slices.ContainsAllStrings(actual, expected) && slices.ContainsAllStrings(expected, actual) {
		return nil
	}
	return xerrors.Errorf("actual loaded ID %v but expected %v", actual, expected)
}

func (tstc *TestContext) pillarChangeSuccessfully(cid string) error {
	return tstc.App.MDB.ChangePillar(tstc.TC.Context(), cid, *tstc.LoadedMT)
}

func (tstc *TestContext) completeTask(resp intapi.OperationResponse) error {
	return tstc.worker.AcquireAndSuccessfullyCompleteTask(tstc.TC.Context(), resp.ID)
}

func (tstc *TestContext) clickhouseClusterWithVersion(minorVer string) error {
	return tstc.clickhouseClusterWithVersionAndName(minorVer, "chtest")
}

func (tstc *TestContext) clickhouseClusterWithVersionAndName(minorVer, name string) error {
	resp, err := tstc.intAPI.CreateClickHouseCluster(intapi.CreateClickHouseClusterRequest{
		Name: name,
	})
	if err != nil {
		return err
	}

	if err := tstc.completeTask(resp); err != nil {
		return err
	}

	return tstc.callUpdateSingleRow(
		querySetClickHouseMinorVersion,
		map[string]interface{}{
			"cid": resp.Metadata.ClusterID,
			"ver": minorVer,
		},
	)
}

func (tstc *TestContext) clickhouseClusterWithZookeeper() error {
	return tstc.clickhouseClusterWithZookeeperAndName("chtest")
}

func (tstc *TestContext) clickhouseClusterWithZookeeperAndName(name string) error {
	resp, err := tstc.intAPI.CreateClickHouseCluster(intapi.CreateClickHouseClusterRequest{
		Name: name,
	})
	if err != nil {
		return err
	}

	if err := tstc.completeTask(resp); err != nil {
		return err
	}

	resp, err = tstc.intAPI.AddZooKeeper(resp.Metadata.ClusterID)
	if err != nil {
		return err
	}

	if err := tstc.completeTask(resp); err != nil {
		return err
	}

	return nil
}

func (tstc *TestContext) setEnableMixedGranularityParts(cid, value string) error {
	return tstc.callUpdateSingleRow(
		querySetClickHouseMixedGranularityParts,
		map[string]interface{}{
			"cid":   cid,
			"value": value,
		},
	)
}

func (tstc *TestContext) setClickHouseCloudStorage(cid, value string) error {
	return tstc.callUpdateSingleRow(
		querySetClickHouseCloudStorage,
		map[string]interface{}{
			"cid":   cid,
			"value": value,
		},
	)
}

func (tstc *TestContext) stoppedClickHouseCluster(name string) error {
	// currently, we implement stop only in compute
	createResp, err := tstc.intAPI.CreateClickHouseCluster(intapi.CreateClickHouseClusterRequest{
		Name:             name,
		ResourcePresetID: "s1.compute.1",
		DiskTypeID:       "network-ssd",
		NetworkID:        "network1",
		Zones:            []string{"myt"},
	})
	if err != nil {
		return err
	}
	if err := tstc.completeTask(createResp); err != nil {
		return err
	}
	stopResp, err := tstc.intAPI.StopClickHouseCluster(createResp.Metadata.ClusterID)
	if err != nil {
		return err
	}
	err = tstc.completeTask(stopResp)
	if err != nil {
		return err
	}
	return nil
}

func (tstc *TestContext) setUserManagementV2(cid, value string) error {
	return tstc.callUpdateSingleRow(
		querySetClickHouseUserManagementV2,
		map[string]interface{}{
			"cid":   cid,
			"value": value,
		},
	)
}

func (tstc *TestContext) setClickHouseZooKeeperVersion(cid, value string) error {
	return tstc.callUpdateSingleRow(
		querySetClickHouseZooKeeperVersion,
		map[string]interface{}{
			"cid":   cid,
			"value": value,
		},
	)
}

func (tstc *TestContext) insertDefaultFeatureFlag(flag string) error {
	return tstc.callUpdateSingleRow(
		queryInsertDefaultFeatureFlag,
		map[string]interface{}{
			"flag": flag,
		},
	)
}

func (tstc *TestContext) mongodbClusterWithMajorVersion(version string) error {
	resp, err := tstc.intAPI.CreateCluster("mongodb", intapi.CreateClusterRequest{
		Name: "mongotest",
	})
	if err != nil {
		return err
	}

	if err := tstc.completeTask(resp); err != nil {
		return err
	}
	versions := strings.Split(version, ".")
	for index, elem := range versions {
		if index > 0 && len(elem) < 2 {
			versions[index] = "0" + elem
		}
	}
	versionNum := strings.Join(versions, "")
	versionStr := "{\"major_human\": \"" + version + "\", \"major_num\": \"" + versionNum + "\"}"

	return tstc.callUpdate(
		querySetPillarValue,
		map[string]interface{}{
			"cid":   resp.Metadata.ClusterID,
			"path":  "{data,mongodb,version}",
			"value": versionStr,
		},
	)
}

func (tstc *TestContext) redisClusterWithMinorVersion(version string) error {
	return tstc.redisClusterWithMinorVersionAndName(version, "redistest")
}

func (tstc *TestContext) redisClusterWithMinorVersionAndName(version, name string) error {
	resp, err := tstc.intAPI.CreateCluster("redis", intapi.CreateClusterRequest{
		Name: name,
	})
	if err != nil {
		return err
	}

	if err := tstc.completeTask(resp); err != nil {
		return err
	}

	parts := strings.SplitN(version, ".", 3)
	if len(parts) != 3 {
		return fmt.Errorf("invalid redis version syntax: %s", version)
	}
	majorVersion := parts[0] + "." + parts[1]

	return tstc.setVersions(resp.Metadata.ClusterID, "redis", majorVersion, version)
}

func (tstc *TestContext) mysqlClusterWithMinorVersion(version string) error {
	return tstc.mysqlClusterWithMinorVersionAndName(version, "mytest")
}

func (tstc *TestContext) mysqlClusterWithMinorVersionAndName(version, name string) error {
	resp, err := tstc.intAPI.CreateCluster("mysql", intapi.CreateClusterRequest{
		Name: name,
	})
	if err != nil {
		return err
	}

	if err := tstc.completeTask(resp); err != nil {
		return err
	}

	parts := strings.SplitN(version, ".", 3)
	if len(parts) != 3 {
		return fmt.Errorf("invalid mysql version syntax: %s", version)
	}
	majorVersion := parts[0] + "." + parts[1]
	majorFlat := parts[0] + parts[1] + "0"
	versionStr := fmt.Sprintf("{\"major_human\": \"%s\", \"major_num\": \"%s\"}", majorVersion, majorFlat)

	err = tstc.callUpdate(
		querySetPillarValue,
		map[string]interface{}{
			"cid":   resp.Metadata.ClusterID,
			"path":  "{data,mysql,version}",
			"value": versionStr,
		},
	)
	if err != nil {
		return err
	}
	return tstc.setVersions(resp.Metadata.ClusterID, "mysql", majorVersion, version)
}

func (tstc *TestContext) postgresqlClusterWithMinorVersion(version string) error {
	return tstc.postgresqlClusterWithMinorVersionAndName(version, "pgtest")
}

func (tstc *TestContext) setVersions(cid, component, majorVersion, minorVersion string) error {
	err := tstc.callUpdate(
		querySetMajorVersion,
		map[string]interface{}{
			"cid":     cid,
			"version": majorVersion,
		},
	)
	if err != nil {
		return err
	}

	return tstc.callUpdateSingleRow(
		querySetMinorVersion,
		map[string]interface{}{
			"cid":       cid,
			"component": component,
			"version":   minorVersion,
		},
	)
}

func (tstc *TestContext) postgresqlClusterWithMinorVersionAndName(version, name string) error {
	resp, err := tstc.intAPI.CreateCluster("postgresql", intapi.CreateClusterRequest{
		Name: name,
	})
	if err != nil {
		return err
	}

	if err := tstc.completeTask(resp); err != nil {
		return err
	}

	majorVersion := strings.SplitN(version, ".", 2)[0]

	return tstc.setVersions(resp.Metadata.ClusterID, "postgres", majorVersion, version)
}

func (tstc *TestContext) setComponentVersion(cid, component, version string) error {
	return tstc.callUpdateSingleRow(
		querySetComponentVersion,
		map[string]interface{}{
			"cid":       cid,
			"version":   version,
			"component": component,
		},
	)
}

func (tstc *TestContext) taskArgumentsContains(cid, taskType, key string, arg1 *gherkin.DocString) error {
	expected := arg1.Content
	var taskArgs string
	_, err := sqlutil.QueryContext(
		tstc.TC.Context(),
		tstc.mdb.PrimaryChooser(),
		queryGetTaskArguments,
		map[string]interface{}{"cid": cid, "taskType": taskType},
		func(rows *sqlx.Rows) error {
			return rows.Scan(&taskArgs)
		},
		&nop.Logger{},
	)
	if err != nil {
		return xerrors.Errorf("unable %v: %w", queryGetTaskArguments, err)
	}

	var dat map[string]interface{}
	err = json.Unmarshal([]byte(taskArgs), &dat)
	if err != nil {
		return xerrors.Errorf("failed to unmarshal task_args %q: %w", taskArgs, err)
	}

	actual, err := json.Marshal(dat[key])
	if err != nil {
		return err
	}

	if string(actual) != expected {
		return xerrors.Errorf("expect task args contains key %q equal to %q, but actual value is %q", key, expected,
			actual)
	}

	return nil
}

func (tstc *TestContext) moveClusterToEnv(cid, env string) error {
	return tstc.callUpdateSingleRow(
		querySetClusterEnv,
		map[string]interface{}{
			"cid": cid,
			"env": env,
		},
	)
}

func (tstc *TestContext) setTLSExpiration(cid, expiration string) error {
	return tstc.callUpdateSingleRow(
		querySetTLSExpiration,
		map[string]interface{}{
			"cid":        cid,
			"expiration": expiration,
		},
	)
}

func (tstc *TestContext) mongodbClusterWithFullnumVersion(version string) error {
	return tstc.mongodbClusterWithFullnumVersionAndName(version, "mongotest")
}

func (tstc *TestContext) mongodbClusterWithFullnumVersionAndName(version, name string) error {
	resp, err := tstc.intAPI.CreateCluster("mongodb", intapi.CreateClusterRequest{
		Name: name,
	})
	if err != nil {
		return err
	}

	if err := tstc.completeTask(resp); err != nil {
		return err
	}

	versionNum := version[:len(version)-2]
	major := versionNum[:len(versionNum)-2]
	middle := strings.TrimLeft(versionNum[len(versionNum)-2:], "0")
	if middle == "" {
		middle = "0"
	}
	versionStr := "{\"major_human\": \"" + major + "." + middle + "\", \"major_num\": \"" + versionNum + "\", \"full_num\": \"" + version + "\"}"

	return tstc.callUpdate(
		querySetPillarValue,
		map[string]interface{}{
			"cid":   resp.Metadata.ClusterID,
			"path":  "{data,mongodb,version}",
			"value": versionStr,
		},
	)
}

func (tstc *TestContext) callUpdateSingleRow(query sqlutil.Stmt, params map[string]interface{}) error {
	return tstc.callUpdateCountUpdated(query, params, 1)
}

func (tstc *TestContext) callUpdate(query sqlutil.Stmt, params map[string]interface{}) error {
	return tstc.callUpdateCountUpdated(query, params, 0)
}

func (tstc *TestContext) callUpdateCountUpdated(query sqlutil.Stmt, params map[string]interface{}, expectUpdatedRows int) error {
	if _, err := sqlutil.QueryContext(
		tstc.TC.Context(),
		tstc.mdb.PrimaryChooser(),
		query,
		params,
		func(rows *sqlx.Rows) error {
			if expectUpdatedRows == 0 {
				return nil
			}
			var updatedRows int
			if err := rows.Scan(&updatedRows); err != nil {
				return err
			}
			if updatedRows != expectUpdatedRows {
				return xerrors.Errorf(
					"expect one row updated, but %d rows updated",
					updatedRows,
				)
			}
			return nil
		},
		&nop.Logger{},
	); err != nil {
		return xerrors.Errorf("unable (%v : %v): %w", query, params, err)
	}

	return nil
}

func (tstc *TestContext) createPostgresqlCluster(name, env string) (string, error) {
	resp, err := tstc.intAPI.CreatePGCluster(intapi.CreatePostgresqlClusterRequest{
		Name:        name,
		Environment: env,
	})
	if err != nil {
		return "", err
	}

	if err := tstc.completeTask(resp); err != nil {
		return "", err
	}

	return resp.Metadata.ClusterID, nil
}

func (tstc *TestContext) postgresqlCluster() error {
	_, err := tstc.createPostgresqlCluster("name", "PRESTABLE")
	return err
}

func (tstc *TestContext) greenplumCluster(name string) error {
	_, err := tstc.intAPI.CreateClusterGoAPI(
		"greenplum",
		intapi.CreateClusterRequest{
			Name: name,
		},
		tstc.TC,
		tstc.GRPCCtx.RawClient,
	)
	return err
}

func (tstc *TestContext) elasticsearchCluster(name string) error {
	_, err := tstc.intAPI.CreateClusterGoAPI(
		"elasticsearch",
		intapi.CreateClusterRequest{Name: name},
		tstc.TC,
		tstc.GRPCCtx.RawClient,
	)
	return err
}

func (tstc *TestContext) stoppedPostgresqlCluster(name string) error {
	// currently, we implement stop only in compute
	createResp, err := tstc.intAPI.CreatePGCluster(intapi.CreatePostgresqlClusterRequest{
		Name:             name,
		ResourcePresetID: "s1.compute.1",
		DiskTypeID:       "network-ssd",
		NetworkID:        "network1",
		Zones:            []string{"myt"},
	})
	if err != nil {
		return err
	}
	if err := tstc.completeTask(createResp); err != nil {
		return err
	}

	stopResp, err := tstc.intAPI.StopPGCluster(createResp.Metadata.ClusterID)
	if err != nil {
		return err
	}

	if err := tstc.completeTask(stopResp); err != nil {
		return err
	}

	return nil
}

func (tstc *TestContext) postgresqlClusterWithConnectionPoolerAndEnv(name, env, pooler string) error {
	cid, err := tstc.createPostgresqlCluster(name, env)
	if err != nil {
		return err
	}
	return tstc.callUpdate(
		querySetPillarStringValue,
		map[string]interface{}{
			"cid":   cid,
			"path":  "{data,connection_pooler}",
			"value": pooler,
		},
	)
}

func (tstc *TestContext) postgresqlClusterWithConnectionPooler(name, pooler string) error {
	return tstc.postgresqlClusterWithConnectionPoolerAndEnv(name, "PRESTABLE", pooler)
}

func (tstc *TestContext) setClusterEdition(cid, edition string) error {
	return tstc.queryWithClusterLock(
		cid,
		querySetEdition,
		map[string]interface{}{
			"cid":     cid,
			"edition": edition,
		},
	)
}

func (tstc *TestContext) checkClusterEdition(cid, expectedEdition string) error {
	var actualEdition string
	_, err := sqlutil.QueryContext(
		tstc.TC.Context(),
		tstc.mdb.PrimaryChooser(),
		queryCheckClusterEdition,
		map[string]interface{}{"cid": cid},
		func(rows *sqlx.Rows) error {
			return rows.Scan(&actualEdition)
		},
		&nop.Logger{},
	)
	if err != nil {
		return err
	}
	if actualEdition != expectedEdition {
		return xerrors.Errorf("Actual edition is %s, but expected %s", actualEdition, expectedEdition)
	}
	return nil
}

func (tstc *TestContext) setPillarEmptyPath(cid, path string) error {
	return tstc.callUpdate(
		querySetPillarEmptyPath,
		map[string]interface{}{
			"cid":  cid,
			"path": path,
		},
	)
}

func (tstc *TestContext) setPillarBoolValue(cid, path, value string) error {
	return tstc.callUpdate(
		querySetPillarBoolValue,
		map[string]interface{}{
			"cid":   cid,
			"path":  path,
			"value": value,
		},
	)
}

func (tstc *TestContext) setPillarStringValue(cid, path, value string) error {
	return tstc.callUpdate(
		querySetPillarStringValue,
		map[string]interface{}{
			"cid":   cid,
			"path":  path,
			"value": value,
		},
	)
}

func (tstc *TestContext) setClusterCreateTimeToYearAgo(cid string) error {
	return tstc.queryWithClusterLock(
		cid,
		querySetClusterCreateTimeToYearAgo,
		map[string]interface{}{
			"cid": cid,
		},
	)
}

func (tstc *TestContext) iCreateHostInPostgreSQLCluster() error {
	var cid string
	count, err := sqlutil.QueryContext(
		tstc.TC.Context(),
		tstc.mdb.PrimaryChooser(),
		queryGetPGClusterID,
		nil,
		func(rows *sqlx.Rows) error {
			return rows.Scan(&cid)
		},
		&nop.Logger{},
	)
	if err != nil {
		return xerrors.Errorf("unable %v: %w", queryGetPGClusterID, err)
	}
	if count != 1 {
		return xerrors.Errorf("that step expect one Postgre cluster, but find: %d", count)
	}

	_, err = tstc.intAPI.CreatePGHost(cid)
	return err
}

func (tstc *TestContext) loadAllConfigs() error {
	configPath := yatest.SourcePath("cloud/mdb/mdb-maintenance/configs")
	files, err := ioutil.ReadDir(configPath)
	if err != nil {
		return err
	}

	for _, f := range files {
		if f.IsDir() {
			continue
		}
		cfg := models.DefaultTaskConfig()
		configID := strings.TrimSuffix(f.Name(), filepath.Ext(f.Name()))
		err := config.LoadFromAbsolutePath(filepath.Join(configPath, f.Name()), &cfg)
		if err != nil {
			return err
		}
		cfg.ID = configID

		tstc.stepsConfig[configID] = cfg
		tstc.App.L().Debugf("                 \n\n\nconfig is %+v", cfg)
	}
	return nil
}

func (tstc *TestContext) loadAllConfigsExcept(ignorePattern string) error {
	ignoreRegexp, err := regexp.Compile(ignorePattern)
	if err != nil {
		return err
	}

	configPath := yatest.SourcePath("cloud/mdb/mdb-maintenance/configs")
	files, err := ioutil.ReadDir(configPath)
	if err != nil {
		return err
	}

	for _, file := range files {
		if file.IsDir() {
			continue
		}

		cfg := models.DefaultTaskConfig()
		configID := strings.TrimSuffix(file.Name(), filepath.Ext(file.Name()))

		if ignoreRegexp.MatchString(configID) {
			tstc.App.L().Debugf("Skipping config %s as it matches ignore pattern \"%s\"", configID, ignorePattern)
			continue
		}

		err := config.LoadFromAbsolutePath(filepath.Join(configPath, file.Name()), &cfg)
		if err != nil {
			return err
		}
		cfg.ID = configID

		tstc.stepsConfig[configID] = cfg
		tstc.App.L().Debugf("                 \n\n\nconfig is %+v", cfg)
	}

	return nil
}

func (tstc *TestContext) markAllMaintenanceTasksAsCompleted() error {
	return tstc.callUpdateSingleRow(
		queryMarkAllMaintenanceTasksAsCompleted,
		map[string]interface{}{},
	)
}

func (tstc *TestContext) increaseCloudQuota() error {
	return tstc.callUpdateSingleRow(
		queryIncreaseCloudQuota,
		map[string]interface{}{},
	)
}

func (tstc *TestContext) getClusterRev(clusterID string) (string, error) {
	var rev string
	_, err := sqlutil.QueryContext(
		tstc.TC.Context(),
		tstc.mdb.PrimaryChooser(),
		queryGetClusterRev,
		map[string]interface{}{"cid": clusterID},
		func(rows *sqlx.Rows) error {
			return rows.Scan(&rev)
		},
		tstc.App.L(),
	)

	if err != nil {
		return "0", xerrors.Errorf("unable %v: %w", queryGetClusterRev, err)
	}

	return rev, nil
}

func (tstc *TestContext) getClusterName(clusterID string) (string, error) {
	var rev string
	_, err := sqlutil.QueryContext(
		tstc.TC.Context(),
		tstc.mdb.PrimaryChooser(),
		queryGetClusterName,
		map[string]interface{}{"cid": clusterID},
		func(rows *sqlx.Rows) error {
			return rows.Scan(&rev)
		},
		tstc.App.L(),
	)

	if err != nil {
		return "0", xerrors.Errorf("unable %v: %w", queryGetClusterName, err)
	}

	return rev, nil
}

func (tstc *TestContext) setComputeFlavor(cid string) error {
	return tstc.queryWithClusterLock(
		cid,
		querySetComputeFlavor,
		map[string]interface{}{
			"cid": cid,
		},
	)
}

func (tstc *TestContext) getCIDOfClusterInConfigSelection(name string, cfg models.MaintenanceTaskConfig) (string, error) {
	clusters, err := tstc.App.Mnt.SelectClusters(tstc.TC.Context(), cfg)
	if err != nil {
		return "", err
	}
	for i := 0; i < len(clusters); i++ {
		clusterName, err := tstc.getClusterName(clusters[i].ID)
		if err != nil {
			return "", err
		}
		if name == clusterName {
			return clusters[i].ID, nil
		}
	}
	return "", nil
}

func (tstc *TestContext) revisionsCheck(dbName string) error {
	configsWithoutClusterMatch := []string{}
	for configID, cfg := range tstc.stepsConfig {
		if cfg.Disabled || strings.Split(configID, "_")[0] != dbName {
			continue
		}
		tstc.App.L().Debugf("Revisions check for config %s", configID)

		cid, err := tstc.getCIDOfClusterInConfigSelection(configID, cfg)
		if err != nil {
			return err
		}
		if cid == "" {
			configsWithoutClusterMatch = append(configsWithoutClusterMatch, configID)
			continue
		}

		rev, err := tstc.getClusterRev(cid)
		if err != nil {
			return err
		}
		err = tstc.App.MDB.ChangePillar(tstc.TC.Context(), cid, cfg)
		if err != nil {
			return err
		}
		newRev, err := tstc.getClusterRev(cid)
		if err != nil {
			return err
		}
		if rev != newRev {
			return xerrors.Errorf("Config %s changed revision of cluster %s while in pillar_change",
				configID, cid)
		}
	}
	if len(configsWithoutClusterMatch) != 0 {
		return xerrors.Errorf(`There is no clusters matches with configs: %v.
		Add clusters (with name = config_name) that matches with config to %s_all.feature`,
			configsWithoutClusterMatch, dbName)
	}
	return nil
}

func (tstc *TestContext) addDBName(dbName string) error {
	tstc.databases = append(tstc.databases, dbName)
	return nil
}

func (tstc *TestContext) checkConfigNames() error {
	for configID := range tstc.stepsConfig {
		configPrefix := strings.Split(configID, "_")[0]
		found := false
		for _, dbName := range tstc.databases {
			if configPrefix == dbName {
				found = true
				break
			}
		}
		if !found {
			return xerrors.Errorf(`Prefix %s of config name %s not found in %v`, configPrefix, configID, tstc.databases)
		}
	}
	return nil
}

func (tstc *TestContext) SortConfigsAndGetPriorities() ([]string, map[string]int) {
	priorities := map[string]int{}
	var configs []string
	for configID, cfg := range tstc.stepsConfig {
		priorities[cfg.ID] = cfg.Priority
		configs = append(configs, configID)
	}

	sort.Slice(configs, func(i, j int) bool {
		return priorities[tstc.stepsConfig[configs[j]].ID] < priorities[tstc.stepsConfig[configs[i]].ID]
	})
	return configs, priorities
}

func (tstc *TestContext) maintainAllConfigs(dbName string) error {
	configsWithoutClusterMatch := []string{}
	configs, priorities := tstc.SortConfigsAndGetPriorities()

	for _, configID := range configs {
		cfg := tstc.stepsConfig[configID]
		if cfg.Disabled || strings.Split(configID, "_")[0] != dbName {
			continue
		}
		tstc.App.L().Debugf("Run task %s", configID)
		cid, err := tstc.getCIDOfClusterInConfigSelection(configID, cfg)
		if err != nil {
			return err
		}
		if cid == "" {
			configsWithoutClusterMatch = append(configsWithoutClusterMatch, configID)
			continue
		}

		// Run every config for only one cluster (only when cluster name matches with config name)
		oneClusterCfg := cfg
		oneClusterCfg.ClustersSelection.DB = "SELECT '" + cid + "'"

		err = tstc.App.MaintainConfig(tstc.TC.Context(), oneClusterCfg, priorities)
		if err != nil {
			return err
		}
	}
	if len(configsWithoutClusterMatch) != 0 {
		return xerrors.Errorf(`There is no clusters matches with configs: %v.
		Add clusters (with name = config_name) that matches with config to %s_all.feature`,
			configsWithoutClusterMatch, dbName)
	}
	return nil
}

func (tstc *TestContext) iMaintainConfig(configName string) error {
	cfg, ok := tstc.stepsConfig[configName]
	if !ok {
		return xerrors.Errorf("There are no %s in loaded configs", configName)
	}
	_, priorities := tstc.SortConfigsAndGetPriorities()
	return tstc.App.MaintainConfig(tstc.TC.Context(), cfg, priorities)
}

func (tstc *TestContext) getDelayedUntil(clusterID string) (string, error) {
	var delayedUntil string
	_, err := sqlutil.QueryContext(
		tstc.TC.Context(),
		tstc.mdb.PrimaryChooser(),
		queryGetDelayedUntil,
		map[string]interface{}{"cid": clusterID},
		func(rows *sqlx.Rows) error {
			return rows.Scan(&delayedUntil)
		},
		&nop.Logger{},
	)
	if err != nil {
		return "", err
	}
	return delayedUntil, nil
}

func (tstc *TestContext) ClusterMWSettingsMatchesWithTask(clusterID string) error {
	var dayString string
	var hour int
	_, err := sqlutil.QueryContext(
		tstc.TC.Context(),
		tstc.mdb.PrimaryChooser(),
		querySelectClusterMWSettings,
		map[string]interface{}{"cid": clusterID},
		func(rows *sqlx.Rows) error {
			return rows.Scan(&dayString, &hour)
		},
		&nop.Logger{},
	)
	if err != nil {
		return err
	}
	day, err := pg.ConvertMWDay(dayString)
	if err != nil {
		return err
	}

	delayedUntilString, err := tstc.getDelayedUntil(clusterID)
	if err != nil {
		return err
	}

	ts, err := time.Parse(time.RFC3339, delayedUntilString)
	if err != nil {
		return err
	}

	if !models.IsTimeInMaintenanceWindow(day, hour, ts) {
		return xerrors.Errorf("Task delayed_until %s not matches with cluster MW settings (day: %s, hour: %d)", delayedUntilString, dayString, hour)
	}
	return nil
}

func (tstc *TestContext) iSetClusterMWSettingsTo(clusterID, mwDay string, upperBoundMWHour int) error {
	return tstc.queryWithClusterLock(clusterID, querySetClusterMWSettings,
		map[string]interface{}{"cid": clusterID, "mwDay": mwDay, "mwHour": upperBoundMWHour})
}

func (tstc *TestContext) mwTasksCount() (int, error) {
	var count int
	if _, err := sqlutil.QueryContext(
		tstc.TC.Context(),
		tstc.mdb.PrimaryChooser(),
		querySelectMWTasksCount,
		nil,
		func(rows *sqlx.Rows) error {
			return rows.Scan(&count)
		},
		&nop.Logger{},
	); err != nil {
		return 0, xerrors.Errorf("unable %v: %w", querySelectMWTasksCount, err)
	}
	return count, nil
}

func (tstc *TestContext) thereAreNMaintenanceTasks(expected int) error {
	count, err := tstc.mwTasksCount()
	if err != nil {
		return err
	}
	if count != expected {
		return xerrors.Errorf("expect there are %d maintenance tasks, but there are %d", expected, count)
	}
	return nil
}

func (tstc *TestContext) thereIsOneMaintenanceTask() error {
	return tstc.thereAreNMaintenanceTasks(1)
}

func (tstc *TestContext) thereIsLastSentMultipleNotification(cloudID string) error {
	n, ok := tstc.sender.LastNotification()
	if !ok {
		return xerrors.New("the notification history is empty")
	}
	if n.CloudID != cloudID {
		return xerrors.Errorf("expect last notification cloud id %s, but it's %s", cloudID, n.CloudID)
	}
	_, ok = n.TemplateData["resources"]
	if !ok {
		return xerrors.New("the last notification is not multiple")
	}
	return nil
}

func (tstc *TestContext) thereIsLastSentSingleNotification(cloudID, clusterID string) error {
	n, ok := tstc.sender.LastNotification()
	if !ok {
		return xerrors.New("the notification history is empty")
	}
	if n.CloudID != cloudID {
		return xerrors.Errorf("expect last notification cloud id %s, but it's %s", cloudID, n.CloudID)
	}
	cid, ok := n.TemplateData["clusterId"]
	if !ok {
		if clusters, okCl := n.TemplateData["resources"]; okCl {
			return xerrors.Errorf("the last notification is not single, but contains: %+v", clusters)
		}
		return xerrors.New("the last notification is not single")
	}
	if cid != clusterID {
		return xerrors.Errorf("expect last notification cluster id %s, but it's %s", clusterID, cid)
	}
	return nil
}

func (tstc *TestContext) thereIsTotalNotificationsForCluster(n int, cloudID, clusterID string) error {
	ns := tstc.sender.Notifications()
	var count int
	for _, n := range ns {
		if n.CloudID != cloudID {
			continue
		}
		cid, ok := n.TemplateData["clusterId"]
		if !ok {
			continue
		}
		if cid != clusterID {
			continue
		}
		count++
	}

	if count != n {
		return xerrors.Errorf("expected %d notifications for cloud %q, cluster %q, but got %d", n, cloudID, clusterID, count)
	}

	return nil
}

func (tstc *TestContext) clusterHasNMajorVersions(clusterID string, n int) error {
	var actual int
	_, err := sqlutil.QueryContext(
		tstc.TC.Context(),
		tstc.mdb.PrimaryChooser(),
		querySelectMajorVersionsCount,
		map[string]interface{}{"cid": clusterID},
		func(rows *sqlx.Rows) error {
			return rows.Scan(&actual)
		},
		tstc.App.L(),
	)

	if err != nil {
		return xerrors.Errorf("unable %v: %w", querySelectMajorVersionsCount, err)
	}

	if actual != n {
		return xerrors.Errorf("Expected %d major version in dbaas.versions of cluster %q, but there is %q.", n, clusterID, actual)
	}

	return nil
}

func (tstc *TestContext) existsMaintenanceTaskOnInStatus(configID, clusterID, status string) error {
	var actual string
	count, err := sqlutil.QueryContext(
		tstc.TC.Context(),
		tstc.mdb.PrimaryChooser(),
		queryClusterMWTaskStatus,
		map[string]interface{}{"config_id": configID, "cid": clusterID},
		func(rows *sqlx.Rows) error {
			return rows.Scan(&actual)
		},
		tstc.App.L(),
	)

	if err != nil {
		return xerrors.Errorf("unable %v: %w", queryClusterMWTaskStatus, err)
	}

	if count != 1 {
		return xerrors.Errorf("Looks like there are no %q task on cluster %q", configID, clusterID)
	}

	if actual != status {
		return xerrors.Errorf("expect maintenance tasks in %q status, but actual is %q", status, actual)
	}
	return nil
}

func (tstc *TestContext) rejectMaintenanceTask(configID, clusterID string) error {
	taskID, err := tstc.workerTaskForMWTask(clusterID, configID)
	if err != nil {
		return err
	}

	return tstc.callUpdate(queryRejectMWTask, map[string]interface{}{"task_id": taskID})
}

func (tstc *TestContext) queryWithClusterLock(clusterID string, stm sqlutil.Stmt, args map[string]interface{}) error {
	binding, err := sqlutil.Begin(tstc.TC.Context(), tstc.mdb, sqlutil.Primary, nil)
	if err != nil {
		return err
	}
	defer binding.Rollback(tstc.TC.Context())

	var rev int64
	if _, err := sqlutil.QueryTxBinding(
		tstc.TC.Context(),
		binding,
		queryLockCluster,
		map[string]interface{}{"cid": clusterID},
		func(rows *sqlx.Rows) error {
			return rows.Scan(&rev)
		},
		tstc.App.L()); err != nil {
		return xerrors.Errorf("LockCluster failed with %w", err)
	}
	if _, err := sqlutil.QueryTxBinding(
		tstc.TC.Context(),
		binding,
		stm,
		args,
		sqlutil.NopParser,
		tstc.App.L()); err != nil {
		return xerrors.Errorf("Query failed with %w", err)
	}
	if _, err := sqlutil.QueryTxBinding(
		tstc.TC.Context(),
		binding,
		queryCompleteClusterChange,
		map[string]interface{}{"cid": clusterID, "rev": rev},
		sqlutil.NopParser,
		tstc.App.L()); err != nil {
		return xerrors.Errorf("Complete cluster change failed with %w", err)
	}

	return binding.Commit(tstc.TC.Context())
}

func (tstc *TestContext) iChangeClusterDescription(clusterID, description string) error {
	return tstc.queryWithClusterLock(clusterID, queryChangeClusterDescription,
		map[string]interface{}{"cid": clusterID, "description": description})
}

func (tstc *TestContext) iClusterMaintenanceTaskHasTimeout(clusterID, timeoutStr string) error {
	expected, err := time.ParseDuration(timeoutStr)
	if err != nil {
		return xerrors.Errorf("malformed timeout: %w", err)
	}
	var actual time.Duration
	count, err := sqlutil.QueryContext(
		tstc.TC.Context(),
		tstc.mdb.PrimaryChooser(),
		queryClusterMaintenanceTaskTimeout,
		map[string]interface{}{"cid": clusterID},
		func(rows *sqlx.Rows) error {
			var dur pgtype.Interval
			if err := rows.Scan(&dur); err != nil {
				return xerrors.Errorf("scan timeout into pgtype.Interval: %w", err)
			}
			if err := dur.AssignTo(&actual); err != nil {
				return xerrors.Errorf("assign pgtype.Interval to duration: %w", err)
			}
			return nil
		},
		tstc.App.L(),
	)
	if err != nil {
		return err
	}
	if count != 1 {
		return xerrors.Errorf("expect one task, but %d found", count)
	}
	if actual != expected {
		return xerrors.Errorf("expected %s timeout, but actual is %s", expected, actual)
	}
	return nil
}

func (tstc *TestContext) workerTaskForClusterAndConfigIsAcquiredByWorker(cid, configID string) error {
	taskID, err := tstc.workerTaskForMWTask(cid, configID)
	if err != nil {
		return err
	}
	return tstc.worker.AcquireTask(tstc.TC.Context(), taskID)
}

func (tstc *TestContext) workerTaskAcquiredAndCompletedByWorker(taskID string) error {
	return tstc.worker.AcquireAndSuccessfullyCompleteTask(tstc.TC.Context(), taskID)
}

func (tstc *TestContext) workerTaskForMWTask(cid, configID string) (string, error) {
	var taskID sql.NullString
	_, err := sqlutil.QueryContext(
		tstc.TC.Context(),
		tstc.mdb.PrimaryChooser(),
		queryLastTaskForMWConfig,
		map[string]interface{}{"cid": cid, "config_id": configID},
		func(rows *sqlx.Rows) error {
			if err := rows.Scan(&taskID); err != nil {
				return err
			}
			return nil
		},
		tstc.App.L(),
	)
	if err != nil {
		return "", err
	}
	if !taskID.Valid {
		return "", xerrors.Errorf("worker task not found for cid %q and config %q", cid, configID)
	}
	return taskID.String, nil
}

func (tstc *TestContext) clusterIsInStatus(cid, desired string) error {
	var actualStatus sql.NullString
	_, err := sqlutil.QueryContext(
		tstc.TC.Context(),
		tstc.mdb.PrimaryChooser(),
		queryClusterStatus,
		map[string]interface{}{"cid": cid},
		func(rows *sqlx.Rows) error {
			if err := rows.Scan(&actualStatus); err != nil {
				return err
			}
			return nil
		},
		tstc.App.L(),
	)
	if err != nil {
		return err
	}
	if !actualStatus.Valid {
		return xerrors.Errorf("cluster %q not found", cid)
	}
	if actualStatus.String == desired {
		return nil
	}
	return xerrors.Errorf("cluster %q is in status %q, expected %q", cid, actualStatus.String, desired)
}
