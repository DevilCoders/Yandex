package functest

import (
	"context"
	"database/sql"
	"fmt"
	"os"
	"path"
	"sort"
	"strconv"
	"strings"
	"text/template"
	"time"

	"github.com/DATA-DOG/godog"
	"github.com/DATA-DOG/godog/gherkin"
	"github.com/golang/mock/gomock"
	"github.com/jmoiron/sqlx"

	apihelpers "a.yandex-team.ru/cloud/mdb/dbaas-internal-api-image/recipe/helpers"
	metadbhelpers "a.yandex-team.ru/cloud/mdb/dbaas_metadb/recipes/helpers"
	"a.yandex-team.ru/cloud/mdb/internal/app"
	"a.yandex-team.ru/cloud/mdb/internal/compute"
	computeapi "a.yandex-team.ru/cloud/mdb/internal/compute/compute"
	compute_mock "a.yandex-team.ru/cloud/mdb/internal/compute/compute/mocks"
	stub "a.yandex-team.ru/cloud/mdb/internal/compute/marketplace/stub"
	"a.yandex-team.ru/cloud/mdb/internal/compute/resmanager"
	resmanMock "a.yandex-team.ru/cloud/mdb/internal/compute/resmanager/mocks"
	"a.yandex-team.ru/cloud/mdb/internal/compute/vpc"
	vpc_mock "a.yandex-team.ru/cloud/mdb/internal/compute/vpc/mocks"
	"a.yandex-team.ru/cloud/mdb/internal/dbteststeps"
	"a.yandex-team.ru/cloud/mdb/internal/fs/stat"
	"a.yandex-team.ru/cloud/mdb/internal/generator"
	"a.yandex-team.ru/cloud/mdb/internal/godogutil"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil"
	networkProvider "a.yandex-team.ru/cloud/mdb/internal/network"
	networkMock "a.yandex-team.ru/cloud/mdb/internal/network/mocks"
	s3file "a.yandex-team.ru/cloud/mdb/internal/s3/file"
	"a.yandex-team.ru/cloud/mdb/internal/secret"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/pgutil"
	"a.yandex-team.ru/cloud/mdb/internal/testutil"
	"a.yandex-team.ru/cloud/mdb/internal/tracing/jaeger"
	healthapi "a.yandex-team.ru/cloud/mdb/mdb-health/pkg/client"
	healthmock "a.yandex-team.ru/cloud/mdb/mdb-health/pkg/client/mocks"
	healthtypes "a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/app/datacloud"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/app/mdb"
	crypto_mock "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/crypto/mocks"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/factory"
	mongoperfdiag_mock "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/mongodb/perfdiagdb/mocks"
	myperfdiag_mock "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/mysql/perfdiagdb/mocks"
	pgperfdiag_mock "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/postgresql/perfdiagdb/mocks"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logsdb/clickhouse"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/metadb/pg"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/environment"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/pillars"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/quota"
	pillarconfig "a.yandex-team.ru/cloud/mdb/mdb-pillar-config/recipe"
	pillarsecretsclient "a.yandex-team.ru/cloud/mdb/mdb-pillar-secrets/pkg/pillarsecrets/grpc"
	pillarsecrets "a.yandex-team.ru/cloud/mdb/mdb-pillar-secrets/recipe"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/nop"
	"a.yandex-team.ru/library/go/core/log/zap"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/valid"
)

type healthResponse struct {
	Clusters []struct {
		ClusterID string                  `json:"cid"`
		Status    healthapi.ClusterStatus `json:"status"`
	} `json:"clusters"`
	Hosts []struct {
		FQDN      string `json:"fqdn"`
		ClusterID string `json:"cid"`
		Status    healthtypes.HostStatus
		Services  []struct {
			Name            string                         `json:"name"`
			Status          healthtypes.ServiceStatus      `json:"status"`
			Role            healthtypes.ServiceRole        `json:"role"`
			ReplicaType     healthtypes.ServiceReplicaType `json:"replicatype"`
			ReplicaUpstream string                         `json:"replica_upstream"`
			ReplicaLag      int64                          `json:"replica_lag"`
		}
		System *struct {
			CPU *struct {
				Timestamp int64   `json:"timestamp"`
				Used      float64 `json:"used"`
			} `json:"cpu"`
			Memory *struct {
				Timestamp int64 `json:"timestamp"`
				Used      int64 `json:"used"`
				Total     int64 `json:"total"`
			} `json:"mem"`
			Disk *struct {
				Timestamp int64 `json:"timestamp"`
				Used      int64 `json:"used"`
				Total     int64 `json:"total"`
			} `json:"disk"`
		} `json:"system"`
	} `json:"hosts"`
}

type MockData struct {
	HealthResponse healthResponse
	FeatureFlags   []string
}

func (m *MockData) Clear() {
	m.HealthResponse = healthResponse{}
	m.FeatureFlags = nil
}

func contextInitializer(tc *godogutil.TestContext, s *godog.Suite) {
	tctx, err := newTestContext(tc)
	if err != nil {
		panic(err)
	}

	s.BeforeSuite(tctx.BeforeSuite)
	s.BeforeFeature(tctx.BeforeFeature)
	s.BeforeScenario(tctx.BeforeScenario)
	s.AfterScenario(tctx.AfterScenario)
	s.BeforeStep(tctx.BeforeStep)

	l, _ := zap.New(zap.KVConfig(log.DebugLevel))
	metadbSteps := &dbteststeps.DBSteps{
		L:          l,
		Migrations: make(map[string]struct{}),
		TC:         tc,
		Params: dbteststeps.Params{
			DBName: "metadb",
		},
	}
	mdb, node, _ := dbteststeps.NewReadyCluster("metadb", "dbaas_metadb")
	metadbSteps.Cluster = mdb
	metadbSteps.Node = node

	dbteststeps.RegisterSteps(metadbSteps, s)

	// Steps...
	s.Step(`^we create flag "([^"]*)"$`, tctx.weCreateFlag)
	s.Step(`^string "([^"]*)" is not in tskv log$`, tctx.stringIsNotInTskvLog)
	s.Step(`^string "([^"]*)" is in tskv log$`, tctx.stringIsInTskvLog)
	s.Step(`^body matches schema`, tctx.bodyMatchesSchema)
	s.Step(`^body matches "([^"]*)" schema`, tctx.bodyMatchesSchemaFromFile)
	s.Step(`^body is valid JSON Schema$`, tctx.bodyIsValidJSONSchema)
	s.Step(`^body at path "([^"]*)" contains$`, tctx.bodyAtPathContains)
	s.Step(`^body at path "([^"]*)" contains only$`, tctx.bodyAtPathContainsOnly)
	s.Step(`^body at path "([^"]*)" contains fields with order: (.*)$`, tctx.bodyAtPathContainsFieldsWithOrder)
	s.Step(`^body at path "([^"]*)" includes$`, tctx.bodyAtPathIncludes)
	s.Step(`^each path on body evaluates to$`, tctx.eachPathOnBodyEvaluatesTo)
	s.Step(`^body does not contain following paths$`, tctx.bodyDoesNotContainsPaths)
	s.Step(`^we get response with status (\d+) and body equals to "([^"]*)" data with following changes$`, tctx.weGetResponseWithStatusAndBodyEqualsDataWithChanges)
	s.Step(`^we get response with status (\d+) and body equals to "([^"]*)" data$`, tctx.weGetResponseWithStatusAndBodyEqualsData)
	s.Step(`^we get response with status (\d+) and body matching following values$`, tctx.weGetResponseWithStatusAndBodyMatchingFollowingValues)
	s.Step(`^headers$`, tctx.headers)
	s.Step(`^default headers$`, tctx.defaultHeaders)
	s.Step(`^default headers with "(.*)" token$`, tctx.defaultHeadersWithToken)
	s.Step(`^"([^"]*)" data$`, tctx.data)
	s.Step(`^feature flags$`, tctx.featureFlags)
	s.Step(`^health response$`, tctx.healthResponse)
	s.Step(`^dataproc manager cluster health response$`, tctx.dataProcHealthResponse)
	s.Step(`^health report is received from dataproc agent$`, tctx.healthFromDataprocAgent)
	s.Step(`^s3 response$`, tctx.s3response)
	s.Step(`^we POST "([^"]*)"$`, tctx.wePOST)
	s.Step(`^we POST "([^"]*)" with "([^"]*)" data$`, tctx.wePOSTWithNamedData)
	s.Step(`^we PATCH "([^"]*)" with data$`, tctx.wePATCHWithData)
	s.Step(`^we POST "([^"]*)" with data$`, tctx.wePOSTWithData)
	s.Step(`^we "([^"]*)" via REST at "([^"]*)"$`, tctx.weViaRESTAt)
	s.Step(`^we "([^"]*)" via REST at "([^"]*)" with params$`, tctx.weViaRESTAtWithParams)
	s.Step(`^we get response with status (\d+)$`, tctx.weGetResponseWithStatus)
	s.Step(`^we get response with status (\d+) and body contains$`, tctx.weGetResponseWithStatusAndBodyContains)
	s.Step(`^we get response with status (\d+) and body partially contains$`, tctx.weGetResponseWithStatusAndBodyPartiallyContains)
	s.Step(`^we get response with status (\d+) and body equals`, tctx.weGetResponseWithStatusAndBodyEquals)
	s.Step(`^we "([^"]*)" via REST at "([^"]*)" with data$`, tctx.weViaRESTAtWithData)
	s.Step(`^we "([^"]*)" via REST at "([^"]*)" with "([^"]*)" data$`, tctx.weViaRESTAtWithNamedData)
	s.Step(`^we error via REST`, tctx.weViaRESTError)
	s.Step(`^we DELETE "([^"]*)"$`, tctx.weDELETE)
	s.Step(`^we GET "([^"]*)"$`, tctx.weGET)
	s.Step(`^we GET "([^"]*)" with params$`, tctx.weGETWithParams)

	// MDB Pillar Config steps
	s.Step(`^we GET via PillarConfig "([^"]*)"$`, tctx.weGETPillarConfig)
	s.Step(`^PillarConfig response body contains$`, tctx.weGetPillarConfigResponseBodyContains)

	// MDB Internal API steps
	s.Step(`^we "([^"]*)" via gRPC at "([^"]*)" with data$`, tctx.weViaGRPCAtWithData)
	s.Step(`^we "([^"]*)" via gRPC at "([^"]*)" with "([^"]*)" data$`, tctx.weViaGRPCAtWithNamedData)
	s.Step(`^we "([^"]*)" via gRPC at "([^"]*)"$`, tctx.weViaGRPCAt)
	s.Step(`^we "([^"]*)" via gRPC at "([^"]*)" with data and last page token$`, tctx.weViaGRPCAtWithDataAndLastPageToken)
	s.Step(`^we get gRPC response with body$`, tctx.weGetGRPCResponseWithBody)
	s.Step(`^we get gRPC response with body ignoring empty$`, tctx.weGetGRPCResponseWithBodyOmitEmpty)
	s.Step(`^we get gRPC response OK$`, tctx.weGetGRPCResponseOK)
	s.Step(`^we get gRPC response error with code (\w+) and message "(.*)"$`, tctx.weGetGRPCResponseErrorWithCodeAndMessage)
	s.Step(`^we get gRPC response error with code (\w+) and message "(.*)" and details contain$`, tctx.weGetGRPCResponseErrorWithCodeAndMessageAndDetailsContain)
	s.Step(`^gRPC response body at path "([^"]*)" contains$`, tctx.grpcResponseBodyAtPathContains)
	s.Step(`^gRPC response body at path "([^"]*)" equals to$`, tctx.grpcResponseBodyAtPathEqualsTo)
	s.Step(`^we get gRPC response with JSON Schema$`, tctx.weGetGRPCResponseWithJSONSchema)

	// DataCloud Internal API steps
	s.Step(`^we "([^"]*)" via DataCloud at "([^"]*)" with data$`, tctx.weViaDataCloudAtWithData)
	s.Step(`^we "([^"]*)" via DataCloud at "([^"]*)" with "([^"]*)" data$`, tctx.weViaDataCloudAtWithNamedData)
	s.Step(`^we "([^"]*)" via DataCloud at "([^"]*)"$`, tctx.weViaDataCloudAt)
	s.Step(`^we "([^"]*)" via DataCloud at "([^"]*)" with data and last page token$`, tctx.weViaDataCloudAtWithDataAndLastPageToken)
	s.Step(`^we get DataCloud response with body$`, tctx.weGetDataCloudResponseWithBody)
	s.Step(`^we get DataCloud response with body ignoring empty$`, tctx.weGetDataCloudResponseWithBodyOmitEmpty)
	s.Step(`^we get DataCloud response OK$`, tctx.weGetDataCloudResponseOK)
	s.Step(`^we get DataCloud response error with code (\w+) and message "(.*)"$`, tctx.weGetDataCloudResponseErrorWithCodeAndMessage)
	s.Step(`^we get DataCloud response error with code (\w+) and message "(.*)" and details contain$`, tctx.weGetDataCloudResponseErrorWithCodeAndMessageAndDetailsContain)
	s.Step(`^we save DataCloud response body at path "([^"]*)" as "([^"]*)"$`, tctx.weSaveDataCloudResponseBodyAtPathAs)

	// LogsDB steps
	s.Step(`^logsdb response$`, tctx.logsdbResponse)
	s.Step(`^named logsdb response$`, tctx.namedLogsdbResponse)

	// Reindexer steps
	s.Step("^we run mdb-search-reindexer$", tctx.weRunMDBSearchReindexer)

	// MetaDB steps
	s.Step(`^we enable feature flag "([^"]*)" to valid resources described as$`, tctx.weEnableFFlagToValidResources)
	s.Step(`^we shrink valid resources in metadb for (flavor|disk|geo) "([^"]*)"$`, tctx.weShrinkValidResourcesByParam)
	s.Step(`^we restore valid resources after shrinking$`, tctx.restoreValidResources)
	s.Step(`^"([^"]*)" acquired and finished by worker$`, tctx.acquiredAndFinishedByWorker)
	s.Step(`^"([^"]*)" acquired and failed by worker$`, tctx.acquiredAndFailedByWorker)
	s.Step(`^worker set "([^"]*)" security groups on "([^"]*)"$`, tctx.workerSetSecurityGroupsOn)
	s.Step(`^worker clear security groups for "([^"]*)"$`, tctx.workerClearSecurityGroupsFor)
	s.Step(`^worker task "([^"]*)" created at "([^"]*)"$`, tctx.workerTaskCreatedAt)
	s.Step(`^in worker_queue exists "([^"]*)" id with args "([^"]*)" set to "([^"]*)"$`, tctx.inWorkerQueueExistsIDWithArgsSetTo)
	s.Step(`^in worker_queue exists "([^"]*)" id with args "([^"]*)" containing:`, tctx.inWorkerQueueExistsIDWithArgsContaining)
	s.Step(`^in worker_queue exists "([^"]*)" id with args "([^"]*)" containing map:`, tctx.inWorkerQueueExistsIDWithArgsContainingKeyValue)
	s.Step(`^in worker_queue exists "([^"]*)" id with args "([^"]*)" is empty list`, tctx.inWorkerQueueExistsIDWithArgsEmptyList)
	s.Step(`^in worker_queue exists "([^"]*)" id without args "([^"]*)"`, tctx.inWorkerQueueExistsIDWithoutArgs)
	s.Step(`^for "([^"]*)" exists "([^"]*)" event with$`, tctx.forExistsEventTypeWith)
	s.Step(`^for "([^"]*)" exists event$`, tctx.forExistsEvent)
	s.Step(`^that event matches "([^"]*)" schema$`, tctx.thatEventMatchesSchema)
	s.Step(`^for "([^"]*)" exists "([^"]*)" event$`, tctx.forExistsEventType)
	s.Step(`^in worker_queue exists "([^"]*)" with "([^"]*)" id$`, tctx.inWorkerQueueExistsWithID)
	s.Step(`^in worker_queue exists "([^"]*)" id and data contains$`, tctx.inWorkerQueueExistsWithIDWithDataContains)
	s.Step(`^for "([^"]*)" there are no events$`, tctx.forThereAreNoEvents)
	s.Step(`^event body for event "([^"]*)" does not contain following paths$`, tctx.eventBodyDoesNotContainsPaths)
	s.Step(`^"([^"]*)" task has valid tracing$`, tctx.taskHasValidTracing)
	s.Step(`^we run query$`, tctx.weRunQuery)
	s.Step(`^"([^"]*)" change fqdn to "([^"]*)" and geo to "([^"]*)"$`, tctx.changeFqdnToAndGeoTo)
	s.Step(`^all "([^"]*)" revs committed before "([^"]*)"$`, tctx.allRevsCommittedBefore)
	s.Step(`^last document in search_queue is$`, tctx.lastDocumentInSearchQueueIs)
	s.Step(`^last document in search_queue matches$`, tctx.lastDocumentInSearchQueueMatches)
	s.Step(`^"([^"]*)" document in search_queue is$`, tctx.documentInSearchQueueIs)
	s.Step(`^in search_queue there is one document$`, tctx.inSearchQueueThereIsOneDocument)
	s.Step(`^in search_queue there are "([^"]*)" documents$`, tctx.inSearchQueueThereAreDocuments)
	s.Step(`^that search doc matches "([^"]*)" schema$`, tctx.thatSearchDocMatchesSchema)
	s.Step(`^in hosts there are "([^"]*)" documents$`, tctx.inSearchQueueThereAreDocuments)
	s.Step(`^cluster "([^"]*)" has (\d+) host$`, tctx.clusterHasHosts)
	s.Step(`^cluster "([^"]*)" hosts match$`, tctx.clusterHasHosts)
	s.Step(`^we add default feature flag "([^"]*)"$`, tctx.weAddDefaultFeatureFlag)
	s.Step(`^we add feature flag "([^"]*)" for cloud "([^"]*)"$`, tctx.weAddFeatureFlagForCloud)
	s.Step(`^we add cloud "([^"]*)"$`, tctx.weAddCloud)
	s.Step(`^we add folder "([^"]*)" to cloud "([^"]*)"$`, tctx.weAddFolderToCloud)
	s.Step(`^cluster "([^"]*)" pillar has value '(.*)' on path "([^"]*)"$`, tctx.pillarPathMatches)
	s.Step(`^we update pillar for cluster "([^"]*)" on path "([^"]*)" with value '([^']*)'$`, tctx.weUpdatePillar)
	// backup service steps
	s.Step(`^we add managed backup$`, tctx.weAddManagedBackup)
	s.Step(`^in backups there are "([^"]*)" jobs$`, tctx.inBackupsThereAreJobs)
	s.Step(`^we set managed backup status with id "([^"]*)" to "([^"]*)"$`, tctx.weSetManagedBackupStatus)
	s.Step(`^last managed backup for cluster "([^"]*)" has status "([^"]*)"$`, tctx.thatLastBackupHaveStatus)
	s.Step(`^managed backup with id "([^"]*)" has status "([^"]*)"$`, tctx.thatBackupHaveStatus)
	s.Step(`^we enable BackupService for cluster "([^"]*)"$`, tctx.weEnableBackupServiceForCluster)
	s.Step(`^initial backup for cluster "([^"]*)" successfully created$`, tctx.initialBackupSuccessfullyCreated)
	// variables
	s.Step(`^I save "([^"]*)" as "([^"]*)"$`, tctx.stepISaveValAs)
	s.Step(`^I save NOW as "([^"]*)"$`, tctx.stepISaveNowAs)

	// Logic config
	s.Step(`^we use following default resources for "([^"]*)" with role "([^"]*)"$`, tctx.weUseFollowingDefaultResourcesForWithRole)
	s.Step(`^we allow move cluster between clouds$`, tctx.weAllowMoveClusterBetweenClouds)
	s.Step(`^we disallow move cluster between clouds$`, tctx.weDisallowMoveClusterBetweenClouds)

	// License service config
	s.Step(`^we allow create sqlserver clusters`, tctx.weAllowCreateSqlserverClusters)
	s.Step(`^we disallow create sqlserver clusters`, tctx.weDisallowCreateSqlserverClusters)
}

func InitInternalAPI(licenseMock stub.LicenseMock, mockdata *MockData, logsdb *sql.DB, l log.Logger) (*mdb.App, *datacloud.App, error) {
	const tries = 3
	var errs []error

	for i := 0; i < tries; i++ {
		iapi, dapi, err := initInternalAPIImpl(licenseMock, mockdata, logsdb, l)
		if err != nil {
			fmt.Printf("failed to initialize internal api: %s\n", err)
			errs = append(errs, err)
			continue
		}

		fmt.Printf("Using %d port for MDB internal API\n", iapi.GRPCPort())
		go func() {
			iapi.WaitForStop()
		}()

		fmt.Printf("Using %d port for DataCloud internal API\n", dapi.GRPCPort())
		go func() {
			dapi.WaitForStop()
		}()

		if err = os.Setenv("MDB_INTERNAL_API_PORT", strconv.FormatInt(int64(iapi.GRPCPort()), 10)); err != nil {
			return nil, nil, err
		}

		if err = os.Setenv("MDB_DATACLOUD_API_PORT", strconv.FormatInt(int64(dapi.GRPCPort()), 10)); err != nil {
			return nil, nil, err
		}

		return iapi, dapi, nil
	}

	// TODO: use multierr when available
	return nil, nil, xerrors.Errorf("failed to initialize internal api with random port %d times: %+v", tries, errs)
}

func initInternalAPIImpl(licenseMock stub.LicenseMock, mockdata *MockData, logsdb *sql.DB, l log.Logger) (*mdb.App, *datacloud.App, error) {
	// Build config
	cfg := mdb.DefaultConfig()
	cfg.API.ExposeErrorDebug = true
	cfg.App.Logging.Level = log.DebugLevel
	cfg.GRPC.Logging.LogRequestBody = true
	cfg.GRPC.Logging.LogResponseBody = true
	var dataCloudLogPath string
	cfg.App.Logging.File = godogutil.TestOutputPath("logs/mdb-internal-api.log")
	dataCloudLogPath = godogutil.TestOutputPath("logs/datacloud-internal-api.log")
	cfg.SLBCloseFile.FilePath = apihelpers.MustTmpRootPath("close")
	cfg.ReadOnlyFile.FilePath = apihelpers.MustTmpRootPath("read-only")
	cfg.Logic.ResourceValidation.DecommissionedZones = []string{
		"ugr", "eu-central-1a", "eu-central-1b", "eu-central-1c", "us-east-2a", "us-east-2b", "us-east-2c",
	}
	cfg.Logic.ResourceValidation.DecommissionedResourcePresets = []string{"s1.porto.legacy", "s2.porto.0.legacy"}
	cfg.Logic.ResourceValidation.MinimalDiskUnit = 4 * 1024 * 1024
	// Do not perform any request retries in tests
	cfg.API.Retry.MaxRetries = 0
	cfg.Logic.Logs.Retry.MaxRetries = 0
	cfg.Logic.E2E.FolderID = "folder3"
	cfg.Logic.E2E.ClusterName = "dbaas_e2e_func_tests"
	cfg.Logic.ClusterStopSupported = true

	// This will make listener take any available port
	port, err := testutil.GetFreePort()
	if err != nil {
		return nil, nil, xerrors.Errorf("failed to get free port: %w", err)
	}
	cfg.GRPC.Addr = ":" + port

	metaDBHost, err := metadbhelpers.Host()
	if err != nil {
		return nil, nil, err
	}
	metaDBPort, err := metadbhelpers.Port()
	if err != nil {
		return nil, nil, err
	}

	cfg.MetaDB.DB.Addrs = []string{fmt.Sprintf("%s:%s", metaDBHost, metaDBPort)}
	cfg.MetaDB.DB.SSLMode = pgutil.DisableSSLMode

	cfg.Logic.DefaultResources = logic.DefaultResourcesConfig{
		ByClusterType: map[string]map[string]logic.DefaultResourcesRecord{
			"clickhouse_cluster": {
				"clickhouse_cluster": {
					ResourcePresetExtID: "s1.compute.1",
					DiskTypeExtID:       "network-ssd",
					DiskSize:            10 * 1024 * 1024 * 1024,
					Generation:          1,
				},
				"zk": {
					ResourcePresetExtID: "s1.compute.1",
					DiskTypeExtID:       "network-ssd",
					DiskSize:            10 * 1024 * 1024 * 1024,
					Generation:          1,
				},
			},
			"sqlserver_cluster": {
				"sqlserver_cluster": {
					ResourcePresetExtID: "s1.micro",
					DiskTypeExtID:       "network-ssd",
					DiskSize:            10 * 1024 * 1024 * 1024,
					Generation:          1,
				},
				"windows_witness": {
					ResourcePresetExtID: "s1.compute.1",
					DiskTypeExtID:       "network-ssd",
					DiskSize:            10 * 1024 * 1024 * 1024,
					Generation:          1,
				},
			},
			"kafka_cluster": {
				"kafka_cluster": {
					ResourcePresetExtID: "s2.compute.1",
					DiskTypeExtID:       "network-ssd",
					DiskSize:            30 * 1024 * 1024 * 1024,
					Generation:          1,
				},
				"zk": {
					ResourcePresetExtID: "s2.compute.1",
					DiskTypeExtID:       "network-ssd",
					DiskSize:            10 * 1024 * 1024 * 1024,
					Generation:          1,
				},
			},
		},
	}

	cfg.Logic.DefaultResourcesDefaultValue = &logic.DefaultResourcesRecord{
		ResourcePresetExtID: "test_preset_id",
		DiskTypeExtID:       "test_disk_type_id",
		DiskSize:            1024,
		Generation:          1,
	}

	cfg.Logic.ClickHouse.MinimalZkResources = []logic.ZkResource{
		{
			ChSubclusterCPU: 16,
			ZKHostCPU:       2,
		},
		{
			ChSubclusterCPU: 40,
			ZKHostCPU:       4,
		},
		{
			ChSubclusterCPU: 160,
			ZKHostCPU:       8,
		},
	}
	cfg.Logic.ClickHouse.ShardCountLimit = 3

	{
		clusters := int64(2)
		hosts := int64(3)
		something := int64(4) // TODO: pick a good name, I have no clue what this is (taken from py API)
		cfg.Logic.DefaultCloudQuota = quota.Resources{
			CPU:      float64(clusters * hosts * something),
			Memory:   clusters * hosts * something * 4 * 1024 * 1024 * 1024,
			SSDSpace: clusters * hosts * 100 * 1024 * 1024 * 1024,
			HDDSpace: clusters * hosts * 100 * 1024 * 1024 * 1024,
			Clusters: clusters,
		}
	}

	cfg.Logic.Elasticsearch.EnableAutoBackups = true
	cfg.Logic.Elasticsearch.ExtensionAllowedDomain = "storage.yandexcloud.net"
	cfg.Logic.Elasticsearch.ExtensionPrevalidation = false

	cfg.Logic.EnvironmentVType = environment.VTypeCompute

	cfg.Logic.SaltEnvs.Production = environment.SaltEnvProd
	cfg.Logic.SaltEnvs.Prestable = environment.SaltEnvQA

	cfg.Logic.Console.URI = "https://console"
	cfg.Logic.Monitoring.Charts = make(map[clusters.Type][]logic.MonitoringChartConfig)
	for _, typ := range clusters.Types() {
		cfg.Logic.Monitoring.Charts[typ] = []logic.MonitoringChartConfig{
			{Name: "YASM", Description: "YaSM (Golovan) charts", Link: "https://yasm/cid={cid}"},
			{Name: "Solomon", Description: "Solomon charts", Link: "https://solomon/cid={cid}&fExtID={folderExtID}"},
			{Name: "Console", Description: "Console charts", Link: "{console}/cid={cid}&fExtID={folderExtID}"},
		}
	}

	cfg.Logic.Kafka.ZooKeeperZones = []string{"myt", "sas", "vla"}

	vctx := valid.NewValidationCtx()
	if verr := valid.Struct(vctx, cfg); verr != nil {
		return nil, nil, xerrors.Errorf("failed to validate config: %w", verr)
	}

	// Create base app with human-friendly logger
	baseApp, err := app.New(
		app.WithConfig(&cfg),
		app.WithLoggerConstructor(app.DefaultToolLoggerConstructor()),
	)
	if err != nil {
		return nil, nil, err
	}

	ctrl := gomock.NewController(l)

	// Create access service mock
	accessservice := AccessServiceStub()

	healthClient := healthmock.NewMockMDBHealthClient(ctrl)
	healthClient.EXPECT().GetClusterHealth(gomock.Any(), gomock.Any()).AnyTimes().
		DoAndReturn(func(_ context.Context, cid string) (healthapi.ClusterHealth, error) {
			for _, cluster := range mockdata.HealthResponse.Clusters {
				if cluster.ClusterID == cid {
					return healthapi.ClusterHealth{Status: cluster.Status}, nil
				}
			}

			return healthapi.ClusterHealth{Status: healthapi.ClusterStatusUnknown}, nil
		})
	healthClient.EXPECT().GetHostsHealth(gomock.Any(), gomock.Any()).AnyTimes().
		DoAndReturn(func(_ context.Context, fqdns []string) ([]healthtypes.HostHealth, error) {
			var res []healthtypes.HostHealth
			for _, fqdn := range fqdns {
				for _, host := range mockdata.HealthResponse.Hosts {
					if host.FQDN == fqdn {
						var shs []healthtypes.ServiceHealth
						for _, s := range host.Services {
							shs = append(shs, healthtypes.NewServiceHealth(s.Name, time.Time{},
								s.Status, s.Role, s.ReplicaType, s.ReplicaUpstream, s.ReplicaLag, nil))
						}
						var system *healthtypes.SystemMetrics = nil
						if host.System != nil {
							var cpuMetric *healthtypes.CPUMetric = nil
							if host.System.CPU != nil {
								cpuMetric = &healthtypes.CPUMetric{
									BaseMetric: healthtypes.BaseMetric{Timestamp: host.System.CPU.Timestamp},
									Used:       host.System.CPU.Used,
								}
							}

							var memoryMetric *healthtypes.MemoryMetric = nil
							if host.System.Memory != nil {
								memoryMetric = &healthtypes.MemoryMetric{
									BaseMetric: healthtypes.BaseMetric{Timestamp: host.System.Memory.Timestamp},
									Used:       host.System.Memory.Used,
									Total:      host.System.Memory.Total,
								}
							}

							var diskMetric *healthtypes.DiskMetric = nil
							if host.System.Disk != nil {
								diskMetric = &healthtypes.DiskMetric{
									BaseMetric: healthtypes.BaseMetric{Timestamp: host.System.Disk.Timestamp},
									Used:       host.System.Disk.Used,
									Total:      host.System.Disk.Total,
								}
							}

							system = &healthtypes.SystemMetrics{
								CPU:    cpuMetric,
								Memory: memoryMetric,
								Disk:   diskMetric,
							}
						}
						res = append(res, healthtypes.NewHostHealthWithSystemAndStatus(
							host.ClusterID, fqdn, shs, system, host.Status))
					}
				}
			}
			return res, nil
		})

	resman := resmanMock.NewMockClient(ctrl)
	resman.EXPECT().ResolveFolders(gomock.Any(), []string{"folder1"}).Return([]resmanager.ResolvedFolder{{CloudExtID: "cloud1", FolderExtID: "folder1"}}, nil).AnyTimes()
	resman.EXPECT().ResolveFolders(gomock.Any(), []string{"folder2"}).Return([]resmanager.ResolvedFolder{{CloudExtID: "cloud2", FolderExtID: "folder2"}}, nil).AnyTimes()
	resman.EXPECT().ResolveFolders(gomock.Any(), []string{"folder3"}).Return([]resmanager.ResolvedFolder{{CloudExtID: "cloud3", FolderExtID: "folder3"}}, nil).AnyTimes()
	resman.EXPECT().ResolveFolders(gomock.Any(), []string{"folder4"}).Return([]resmanager.ResolvedFolder{{CloudExtID: "cloud1", FolderExtID: "folder4"}}, nil).AnyTimes()
	resman.EXPECT().ResolveFolders(gomock.Any(), gomock.Any()).Return(nil, resmanager.ErrFolderNotFound).AnyTimes()
	resman.EXPECT().PermissionStages(gomock.Any(), gomock.Any()).DoAndReturn(func(interface{}, interface{}) ([]string, error) {
		return mockdata.FeatureFlags, nil
	}).AnyTimes()

	vpcClient := vpc_mock.NewMockClient(ctrl)
	networkClient := networkMock.NewMockClient(ctrl)
	network := networkProvider.Network{
		ID:       "network1",
		FolderID: "folder1",
		Name:     "network1",
	}
	subnet := networkProvider.Subnet{
		ID:        "subnet1",
		FolderID:  "folder1",
		Name:      "subnet1",
		NetworkID: "network1",
	}
	sg1InNetwork1 := vpc.SecurityGroup{
		ID:        "sg_id1",
		Name:      "sg_name1",
		FolderID:  "folder1",
		NetworkID: "network1",
	}
	sg2InNetwork1 := vpc.SecurityGroup{
		ID:        "sg_id2",
		Name:      "sg_name2",
		FolderID:  "folder1",
		NetworkID: "network1",
	}
	networkClient.EXPECT().GetSubnet(gomock.Any(), gomock.Any()).Return(subnet, nil).AnyTimes()
	networkClient.EXPECT().GetNetwork(gomock.Any(), "network1").Return(network, nil).AnyTimes()
	networkClient.EXPECT().GetNetwork(gomock.Any(), "IN-PORTO-NO-NETWORK-API").Return(networkProvider.Network{}, nil).AnyTimes()
	networkClient.EXPECT().GetNetwork(gomock.Any(), "").Return(networkProvider.Network{}, nil).AnyTimes()
	networkClient.EXPECT().GetNetworks(gomock.Any(), "folder1", gomock.Any()).Return([]networkProvider.Network{network}, nil).AnyTimes()
	subnets := []networkProvider.Subnet{
		{
			ID:        "network1-myt",
			FolderID:  "folder1",
			Name:      "network1-myt",
			NetworkID: "network1",
			ZoneID:    "myt",
		},
		{
			ID:           "network1-sas",
			FolderID:     "folder1",
			Name:         "network1-sas",
			NetworkID:    "network1",
			ZoneID:       "sas",
			V4CIDRBlocks: []string{"'192.168.0.0/16'"},
		},
		{
			ID:        "network1-vla",
			FolderID:  "folder1",
			Name:      "network1-vla",
			NetworkID: "network1",
			ZoneID:    "vla",
		},
	}
	networkClient.EXPECT().GetSubnets(gomock.Any(), network).Return(subnets, nil).AnyTimes()
	vpcClient.EXPECT().GetSecurityGroup(gomock.Any(), sg1InNetwork1.ID).Return(sg1InNetwork1, nil).AnyTimes()
	vpcClient.EXPECT().GetSecurityGroup(gomock.Any(), sg2InNetwork1.ID).Return(sg2InNetwork1, nil).AnyTimes()
	vpcClient.EXPECT().GetSecurityGroup(gomock.Any(), "non_existed_sg_id").Return(vpc.SecurityGroup{}, semerr.NotFound("that sg not found")).AnyTimes()

	network2 := networkProvider.Network{
		ID:       "network2",
		FolderID: "folder1",
		Name:     "network2",
	}
	networkClient.EXPECT().GetNetwork(gomock.Any(), "network2").Return(network2, nil).AnyTimes()
	subnets2 := []networkProvider.Subnet{
		{
			ID:        "network2-myt",
			FolderID:  "folder1",
			Name:      "network2-myt",
			NetworkID: "network2",
			ZoneID:    "myt",
		},
		{
			ID:        "network2-vla",
			FolderID:  "folder1",
			Name:      "network2-vla",
			NetworkID: "network2",
			ZoneID:    "vla",
		},
		{
			ID:        "network2-sas",
			FolderID:  "folder1",
			Name:      "network2-sas",
			NetworkID: "network2",
			ZoneID:    "sas",
		},
		{
			ID:        "network2-vla2",
			FolderID:  "folder1",
			Name:      "network2-vla2",
			NetworkID: "network2",
			ZoneID:    "vla",
		},
		{
			ID:        "network2-man",
			FolderID:  "folder1",
			Name:      "network2-man",
			NetworkID: "network2",
			ZoneID:    "man",
		},
		{
			ID:        "network2-iva",
			FolderID:  "folder1",
			Name:      "network2-iva",
			NetworkID: "network2",
			ZoneID:    "iva",
		},
	}
	networkClient.EXPECT().GetSubnets(gomock.Any(), network2).Return(subnets2, nil).AnyTimes()

	network3 := networkProvider.Network{
		ID:       "network3",
		FolderID: "folder3",
		Name:     "network3",
	}
	networkClient.EXPECT().GetNetwork(gomock.Any(), "network3").Return(network3, nil).AnyTimes()
	subnets3 := []networkProvider.Subnet{
		{
			ID:        "network3-myt",
			FolderID:  "folder3",
			Name:      "network3-myt",
			NetworkID: "network3",
			ZoneID:    "myt",
		},
		{
			ID:        "network3-vla",
			FolderID:  "folder3",
			Name:      "network3-vla",
			NetworkID: "network3",
			ZoneID:    "vla",
		},
		{
			ID:        "network3-sas",
			FolderID:  "folder3",
			Name:      "network3-sas",
			NetworkID: "network3",
			ZoneID:    "sas",
		},
		{
			ID:        "network3-vla2",
			FolderID:  "folder3",
			Name:      "network3-vla2",
			NetworkID: "network3",
			ZoneID:    "vla",
		},
	}
	networkClient.EXPECT().GetSubnets(gomock.Any(), network3).Return(subnets3, nil).AnyTimes()

	hostGroupClient := compute_mock.NewMockHostGroupService(ctrl)
	hg1 := computeapi.HostGroup{ID: "hg1", FolderID: "folder1", ZoneID: "myt", TypeID: "intel-6230-c16-m64"}
	hg2 := computeapi.HostGroup{ID: "hg2", FolderID: "folder2", ZoneID: "sas", TypeID: "intel-6230-c16-m64"}
	hg3 := computeapi.HostGroup{ID: "hg3", FolderID: "folder1", ZoneID: "vla", TypeID: "intel-6230-c16-m64"}
	hg4 := computeapi.HostGroup{ID: "hg4", FolderID: "folder1", ZoneID: "vla", TypeID: "intel-6230-c16-m64"}
	hg5 := computeapi.HostGroup{ID: "hg5", FolderID: "folder1", ZoneID: "vla", TypeID: "intel-6230r-c16-m128-n1600x4"}
	hg6 := computeapi.HostGroup{ID: "hg6", FolderID: "folder1", ZoneID: "vla", TypeID: "intel-6338-c1-m128"}
	hg7 := computeapi.HostGroup{ID: "hg7", FolderID: "folder1", ZoneID: "vla", TypeID: "intel-6338-c1-m0"}
	hostGroupClient.EXPECT().Get(gomock.Any(), "hg1").Return(hg1, nil).AnyTimes()
	hostGroupClient.EXPECT().Get(gomock.Any(), "hg2").Return(hg2, nil).AnyTimes()
	hostGroupClient.EXPECT().Get(gomock.Any(), "hg3").Return(hg3, nil).AnyTimes()
	hostGroupClient.EXPECT().Get(gomock.Any(), "hg4").Return(hg4, nil).AnyTimes()
	hostGroupClient.EXPECT().Get(gomock.Any(), "hg5").Return(hg5, nil).AnyTimes()
	hostGroupClient.EXPECT().Get(gomock.Any(), "hg6").Return(hg6, nil).AnyTimes()
	hostGroupClient.EXPECT().Get(gomock.Any(), "hg7").Return(hg7, nil).AnyTimes()

	hostTypeClient := compute_mock.NewMockHostTypeService(ctrl)
	type1 := computeapi.HostType{ID: "intel-6230-c16-m64", Cores: 16, Memory: 68719476736, Disks: 4, DiskSize: 17179869184}
	type2 := computeapi.HostType{ID: "intel-6230r-c16-m128-n1600x4", Cores: 16, Memory: 137438953472, Disks: 4, DiskSize: 34359738368}
	type3 := computeapi.HostType{ID: "intel-6338-c1-m128", Cores: 1, Memory: 137438953472, Disks: 4, DiskSize: 34359738368}
	type4 := computeapi.HostType{ID: "intel-6338-c1-m0", Cores: 1, Memory: 137438, Disks: 4, DiskSize: 34359738368}
	hostTypeClient.EXPECT().Get(gomock.Any(), "intel-6230-c16-m64").Return(type1, nil).AnyTimes()
	hostTypeClient.EXPECT().Get(gomock.Any(), "intel-6230r-c16-m128-n1600x4").Return(type2, nil).AnyTimes()
	hostTypeClient.EXPECT().Get(gomock.Any(), "intel-6338-c1-m128").Return(type3, nil).AnyTimes()
	hostTypeClient.EXPECT().Get(gomock.Any(), "intel-6338-c1-m0").Return(type4, nil).AnyTimes()

	crypto := crypto_mock.NewMockCrypto(ctrl)
	crypto.EXPECT().GenerateRandomString(gomock.Any(), gomock.Any()).Return(secret.NewString("dummy"), nil).AnyTimes()
	crypto.EXPECT().Encrypt(gomock.Any()).DoAndReturn(func(arg0 []byte) (pillars.CryptoKey, error) {
		return pillars.CryptoKey{Data: string(arg0), EncryptionVersion: 0}, nil
	}).AnyTimes()

	secretGenerator := generator.NewFileSequenceIDGenerator("", path.Join(apihelpers.MustTmpRootPath(""), "cluster-secret"))
	clusterSecrets := factory.NewClusterSecretsGeneratorMock(ctrl)
	clusterSecrets.EXPECT().Generate().DoAndReturn(func() ([]byte, []byte, error) {
		id, err := secretGenerator.Generate()
		if err != nil {
			return nil, nil, err
		}
		return []byte(id), []byte(id), nil
	}).AnyTimes()

	// Create metadb client
	metaDB, err := pg.New(cfg.MetaDB.DB, baseApp.L())
	if err != nil {
		return nil, nil, xerrors.Errorf("failed to initialize metadb backend: %w", err)
	}

	// S3 client mock
	s3client := s3file.New(apihelpers.MustTmpRootPath("s3response.json"))

	slbc, err := stat.NewFileWatcher(cfg.SLBCloseFile.FilePath)
	if err != nil {
		return nil, nil, xerrors.Errorf("failed to initialize slb close file watcher: %w", err)
	}

	ro, err := stat.NewFileWatcher(cfg.ReadOnlyFile.FilePath)
	if err != nil {
		return nil, nil, xerrors.Errorf("failed to initialize read only file watcher: %w", err)
	}
	pgPerfDiagDB := pgperfdiag_mock.NewMockBackend(ctrl)
	pgPerfDiagDB.EXPECT().SessionsStats(gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any()).Return(pgPerfDiagSessionsStatResponse, true, nil).AnyTimes()
	pgPerfDiagDB.EXPECT().SessionsAtTime(gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any()).Return(pgPerfDiagSessionsAtTimeResponse, false, nil).AnyTimes()
	pgPerfDiagDB.EXPECT().StatementsAtTime(gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any()).Return(pgPerfDiagStatementsAtTimeResponse, false, nil).AnyTimes()
	pgPerfDiagDB.EXPECT().StatementsDiff(gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any()).Return(pgPerfDiagStatementsDiff, false, nil).AnyTimes()
	pgPerfDiagDB.EXPECT().StatementsInterval(gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any()).Return(pgPerfDiagStatementsIntervalResponse, false, nil).AnyTimes()
	pgPerfDiagDB.EXPECT().StatementsStats(gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any()).Return(pgPerfDiagStatementsStatsResponse, false, nil).AnyTimes()

	myPerfDiagDB := myperfdiag_mock.NewMockBackend(ctrl)
	myPerfDiagDB.EXPECT().SessionsStats(gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any()).Return(myPerfDiagSessionsStatResponse, true, nil).AnyTimes()
	myPerfDiagDB.EXPECT().SessionsAtTime(gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any()).Return(myPerfDiagSessionsAtTimeResponse, false, nil).AnyTimes()
	myPerfDiagDB.EXPECT().StatementsAtTime(gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any()).Return(myPerfDiagStatementsAtTimeResponse, false, nil).AnyTimes()
	myPerfDiagDB.EXPECT().StatementsDiff(gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any()).Return(myPerfDiagStatementsDiff, false, nil).AnyTimes()
	myPerfDiagDB.EXPECT().StatementsInterval(gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any()).Return(myPerfDiagStatementsIntervalResponse, false, nil).AnyTimes()
	myPerfDiagDB.EXPECT().StatementsStats(gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any()).Return(myPerfDiagStatementsStatsResponse, false, nil).AnyTimes()

	mongoPerfDiagDB := mongoperfdiag_mock.NewMockBackend(ctrl)
	mongoPerfDiagDB.EXPECT().ProfilerStats(gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any()).Return(mongoGetProfilerStatsResponse, false, nil).AnyTimes()
	mongoPerfDiagDB.EXPECT().ProfilerRecs(gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any()).Return(mongoGetProfilerRecsAtTimeResponse, false, nil).AnyTimes()
	mongoPerfDiagDB.EXPECT().ProfilerTopForms(gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any()).Return(mongoGetProfilerTopFormsByStat, false, nil).AnyTimes()
	mongoPerfDiagDB.EXPECT().PossibleIndexes(gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any()).Return(mongoGetPossibleIndexes, false, nil).AnyTimes()

	generators := generator.NewPlatformTestHostnameGenerator(map[compute.Platform]generator.HostnameGenerator{
		compute.Ubuntu:  generator.NewFileSequenceHostnameGenerator("hostname", apihelpers.MustTmpRootPath("")),
		compute.Windows: generator.NewFileSequenceHostnameGenerator("hostname", apihelpers.MustTmpRootPath("")),
	})
	iapi, err := mdb.NewApp(
		baseApp,
		cfg,
		mdb.AppComponents{
			AccessService:                     accessservice,
			ResourceManager:                   resman,
			MetaDB:                            metaDB,
			S3Client:                          s3client,
			S3SecureBackupsClient:             s3client,
			LogsDB:                            clickhouse.NewWithDB(logsdb, cfg.LogsDB.Config, baseApp.L()),
			PgPerfDiagDB:                      pgPerfDiagDB,
			MyPerfDiagDB:                      myPerfDiagDB,
			MongoPerfDiagDB:                   mongoPerfDiagDB,
			SLBCloseFile:                      slbc,
			ReadOnlyFile:                      ro,
			ClusterIDGenerator:                generator.NewFileSequenceIDGenerator("cid", apihelpers.MustTmpRootPath("")),
			PillarIDGenerator:                 generator.NewFileSequenceIDGenerator("pillar_id", apihelpers.MustTmpRootPath("")),
			SubClusterIDGenerator:             generator.NewFileSequenceIDGenerator("subcid", apihelpers.MustTmpRootPath("")),
			ShardIDGenerator:                  generator.NewFileSequenceIDGenerator("shard_id", apihelpers.MustTmpRootPath("")),
			TaskIDGenerator:                   generator.NewFileSequenceIDGenerator("worker_task_id", apihelpers.MustTmpRootPath("")),
			EventIDGenerator:                  generator.NewFileSequenceIDGenerator("event_id", apihelpers.MustTmpRootPath("")),
			BackupIDGenerator:                 generator.NewFileSequenceIDGenerator("backup_id", apihelpers.MustTmpRootPath("")),
			PlatformHostnameGenerator:         generators,
			ElasticsearchExtensionIDGenerator: generator.NewFileSequenceIDGenerator("extension_id", apihelpers.MustTmpRootPath("")),
		},
		healthClient,
		vpcClient,
		networkClient,
		crypto,
		clusterSecrets,
		hostGroupClient,
		hostTypeClient,
		licenseMock,
	)
	if err != nil {
		_ = metaDB.Close()
		return nil, nil, err
	}

	// This will make listener take any available port
	dataCloudPort, err := testutil.GetFreePort()
	if err != nil {
		return nil, nil, xerrors.Errorf("failed to get free port: %w", err)
	}

	port, ok := os.LookupEnv("MDB_PILLAR_SECRETS_PORT")
	if !ok {
		return nil, nil, fmt.Errorf("MDB_PILLAR_SECRETS_PORT is not set")
	}

	cfg.API.Domain = api.DomainConfig{
		Public:  "yadc.io",
		Private: "private.yadc.io",
	}

	datacloudCfg := datacloud.Config{
		App: app.Config{
			Logging: app.LoggingConfig{
				Level: cfg.App.Logging.Level,
				File:  dataCloudLogPath,
			},
		},
		API: cfg.API,
		GRPC: grpcutil.ServeConfig{
			Addr:            ":" + dataCloudPort,
			ShutdownTimeout: cfg.GRPC.ShutdownTimeout,
			Logging:         cfg.GRPC.Logging,
		},
		MetaDB:        cfg.MetaDB,
		LogsDB:        cfg.LogsDB,
		S3:            cfg.S3,
		Health:        cfg.Health,
		AccessService: cfg.AccessService,
		SLBCloseFile:  cfg.SLBCloseFile,
		ReadOnlyFile:  cfg.ReadOnlyFile,
		Logic:         cfg.Logic,
		Crypto:        cfg.Crypto,
	}

	datacloudCfg.Logic.EnvironmentVType = environment.VTypeAWS
	datacloudCfg.Logic.ResourceValidation.DecommissionedZones = []string{"ugr"}
	datacloudCfg.Logic.ResourceModel = environment.ResourceModelDataCloud

	datacloudCfg.Logic.DefaultResources = logic.DefaultResourcesConfig{
		ByClusterType: map[string]map[string]logic.DefaultResourcesRecord{
			"kafka_cluster": {
				"kafka_cluster": {
					ResourcePresetExtID: "a1.aws.1",
					DiskTypeExtID:       "local-ssd",
					DiskSize:            32 * 1024 * 1024 * 1024,
					Generation:          1,
				},
				"zk": {
					ResourcePresetExtID: "a1.aws.1",
					DiskTypeExtID:       "local-ssd",
					DiskSize:            32 * 1024 * 1024 * 1024,
					Generation:          1,
				},
			},
		},
	}

	// Create base app with human-friendly logger
	baseDatacloudApp, err := app.New(
		app.WithConfig(&datacloudCfg),
		app.WithLoggerConstructor(app.DefaultToolLoggerConstructor()),
	)
	if err != nil {
		return nil, nil, err
	}

	datacloudCrypto := crypto_mock.NewMockCrypto(ctrl)
	cryptoGenerator := generator.NewFileSequenceIDGenerator("random_string", apihelpers.MustTmpRootPath(""))
	datacloudCrypto.EXPECT().GenerateRandomString(gomock.Any(), gomock.Any()).DoAndReturn(func(int, []int32) (secret.String, error) {
		str, err := cryptoGenerator.Generate()
		return secret.NewString(str), err
	}).AnyTimes()
	datacloudCrypto.EXPECT().Encrypt(gomock.Any()).DoAndReturn(func(arg0 []byte) (pillars.CryptoKey, error) {
		return pillars.CryptoKey{Data: string(arg0), EncryptionVersion: 0}, nil
	}).AnyTimes()

	pillarSecretsClient, err := pillarsecretsclient.New(baseDatacloudApp.ShutdownContext(), "[::1]:"+port, "intapi", grpcutil.ClientConfig{
		Security: grpcutil.SecurityConfig{
			Insecure: true,
		},
	}, baseDatacloudApp.L())
	if err != nil {
		return nil, nil, err
	}

	dcNetworkClient := networkMock.NewMockClient(ctrl)
	dcNetwork := networkProvider.Network{
		ID:       "network1",
		FolderID: "folder1",
		Name:     "default in region",
		V4CIDRBlocks: []string{
			"1.2.3.4/5",
		},
		V6CIDRBlocks: []string{
			"::/16",
		},
	}
	dcNetworkCustom := networkProvider.Network{
		ID:       "some-network-id",
		FolderID: "folder1",
		Name:     "some custom network",
		V4CIDRBlocks: []string{
			"1.2.3.4/5",
		},
		V6CIDRBlocks: []string{
			"::/16",
		},
	}
	dcSubnets := []networkProvider.Subnet{
		{
			ID:       "subnet-eu1a",
			FolderID: "folder1",
			ZoneID:   "eu-central-1a",
		},
		{
			ID:       "subnet-eu1b",
			FolderID: "folder1",
			ZoneID:   "eu-central-1b",
		},
		{
			ID:       "subnet-eu1c",
			FolderID: "folder1",
			ZoneID:   "eu-central-1c",
		},
	}
	dcNetworkClient.EXPECT().GetNetwork(gomock.Any(), "network1").Return(dcNetwork, nil).AnyTimes()
	dcNetworkClient.EXPECT().GetNetwork(gomock.Any(), "some-network-id").Return(dcNetworkCustom, nil).AnyTimes()
	dcNetworkClient.EXPECT().GetNetworks(gomock.Any(), "folder1", gomock.Any()).Return([]networkProvider.Network{dcNetwork}, nil).AnyTimes()
	dcNetworkClient.EXPECT().GetSubnets(gomock.Any(), dcNetwork).Return(dcSubnets, nil).AnyTimes()
	dcNetworkClient.EXPECT().GetSubnets(gomock.Any(), dcNetworkCustom).Return(dcSubnets, nil).AnyTimes()

	dapi, err := datacloud.NewApp(
		baseDatacloudApp,
		datacloudCfg,
		datacloud.AppComponents{
			AccessService:             accessservice,
			ResourceManager:           resman,
			MetaDB:                    metaDB,
			S3Client:                  s3client,
			LogsDB:                    clickhouse.NewWithDB(logsdb, cfg.LogsDB.Config, baseApp.L()),
			SLBCloseFile:              slbc,
			ReadOnlyFile:              ro,
			ClusterIDGenerator:        generator.NewFileSequenceIDGenerator("cid", apihelpers.MustTmpRootPath("")),
			PillarIDGenerator:         generator.NewFileSequenceIDGenerator("pillar_id", apihelpers.MustTmpRootPath("")),
			SubClusterIDGenerator:     generator.NewFileSequenceIDGenerator("subcid", apihelpers.MustTmpRootPath("")),
			ShardIDGenerator:          generator.NewFileSequenceIDGenerator("shard_id", apihelpers.MustTmpRootPath("")),
			TaskIDGenerator:           generator.NewFileSequenceIDGenerator("worker_task_id", apihelpers.MustTmpRootPath("")),
			EventIDGenerator:          generator.NewFileSequenceIDGenerator("event_id", apihelpers.MustTmpRootPath("")),
			BackupIDGenerator:         generator.NewFileSequenceIDGenerator("backup_id", apihelpers.MustTmpRootPath("")),
			PlatformHostnameGenerator: generators,
			PillarSecretsClient:       pillarSecretsClient,
		},
		healthClient,
		vpcClient,
		dcNetworkClient,
		datacloudCrypto,
		clusterSecrets,
	)

	if err != nil {
		_ = metaDB.Close()
	}

	return iapi, dapi, err
}

type testContext struct {
	L                    log.Logger
	InternalAPI          *mdb.App
	DataCloudInternalAPI *datacloud.App

	ModifyModeAPI API
	ReadModeAPI   API

	LastAPI      API
	MetaDBCtx    metaDBContext
	MetaDB       *metadbhelpers.Client
	LogsDBCtx    *LogsDBContext
	RESTCtx      *restContext
	GRPCCtx      *GRPCContext
	DataCloudCtx *GRPCContext
	MockData     *MockData
	LicenseMock  stub.LicenseMock

	TC        *godogutil.TestContext
	variables map[string]interface{}
}

func newTestContext(tc *godogutil.TestContext) (*testContext, error) {
	l, err := zap.New(zap.KVConfig(log.DebugLevel))
	if err != nil {
		return nil, err
	}

	mockdata := &MockData{}

	jaegerConfig := jaeger.DefaultConfig()
	jaegerConfig.ServiceName = "infra-tests"
	// jaeger.New init global tracer
	_, err = jaeger.New(jaegerConfig, &nop.Logger{})
	if err != nil {
		return nil, xerrors.Errorf("failed to initialize a tracer: %w", err)
	}

	licenseService := LicenseStub(true)

	logsdbCtx, err := NewLogsDBContext(l)
	if err != nil {
		return nil, err
	}

	if err := pillarsecrets.StartPillarSecretsAPI(AccessServiceStub()); err != nil {
		return nil, err
	}

	iapi, dapi, err := InitInternalAPI(licenseService, mockdata, logsdbCtx.Mock.DB, l)
	if err != nil {
		return nil, err
	}

	if err := pillarconfig.StartPillarConfigAPI(); err != nil {
		return nil, err
	}

	metadb, err := metadbhelpers.NewClient()
	if err != nil {
		return nil, err
	}

	grpcCtx, err := NewgRPCContext(iapi.GRPCPort(), l)
	if err != nil {
		return nil, err
	}

	datacloudGrpcCtx, err := NewgRPCContext(dapi.GRPCPort(), l)
	if err != nil {
		return nil, err
	}

	modifyMode, err := modifyModeAPIFromEnv()
	if err != nil {
		return nil, err
	}

	readMode, err := readModeAPIFromEnv()
	if err != nil {
		return nil, err
	}

	l.Debugf("modify mode %q, read mode %q", modifyMode, readMode)

	return &testContext{
		L:                    l,
		InternalAPI:          iapi,
		DataCloudInternalAPI: dapi,
		ModifyModeAPI:        modifyMode,
		ReadModeAPI:          readMode,
		MetaDB:               metadb,
		LogsDBCtx:            logsdbCtx,
		RESTCtx:              newRESTContext(),
		GRPCCtx:              grpcCtx,
		DataCloudCtx:         datacloudGrpcCtx,
		MockData:             mockdata,
		LicenseMock:          licenseService,
		TC:                   tc,
		variables:            make(map[string]interface{}),
	}, nil
}

func (tctx *testContext) BeforeSuite() {}

func (tctx *testContext) BeforeFeature(f *gherkin.Feature) {
	// Reset all contexts so 'old' values don't leak
	// TODO: do it BeforeScenario?
	tctx.MetaDBCtx.Reset()
	tctx.LogsDBCtx.Reset()
	tctx.RESTCtx.Reset()
	tctx.GRPCCtx.Reset()
	tctx.DataCloudCtx.Reset()
}

func (tctx *testContext) BeforeScenario(_ interface{}) {
	if err := metadbhelpers.CleanupMetaDB(tctx.TC.Context(), tctx.MetaDB.Node); err != nil {
		panic(err)
	}
	// This can go after we remove old internal api since new api uses inprocess mocks and synchronization
	if err := apihelpers.CleanupTmpRootPath(""); err != nil {
		panic(err)
	}

	tctx.MockData.Clear()
	tctx.variables = make(map[string]interface{})
}

func (tctx *testContext) BeforeStep(step *gherkin.Step) {
	err := tctx.templateStep(step)
	if err != nil {
		panic(fmt.Errorf("failed to render tempate in step %v: %w", step, err))
	}
}

func (tctx *testContext) AfterScenario(fn interface{}, err error) {
	dump := dbteststeps.DumpOnErrorHook(tctx.TC, tctx.MetaDB.Node, "metadb", "dbaas")

	// Dump if error in scenario
	if err != nil {
		dump(fn, err)
		return
	}

	// Validate quota
	query := sqlutil.Stmt{
		Name:  "ValidateCloudQuotaUsage",
		Query: "SELECT * from code.verify_clouds_quota_usage()",
	}
	var clouds []InvalidCloudQuota
	parser := func(rows *sqlx.Rows) error {
		var cloud InvalidCloudQuota
		if err := rows.StructScan(&cloud); err != nil {
			return err
		}

		clouds = append(clouds, cloud)
		return nil
	}

	// TODO: fix context invalidation in AfterScenario but before this callback
	if _, err = sqlutil.QueryNode(context.Background(), tctx.MetaDB.Node, query, nil, parser, &nop.Logger{}); err != nil {
		// Dump quota validation failure
		dump(fn, err)
		names := tctx.TC.LastExecutedNames()
		where := fmt.Sprintf("%s/%s", names.FeatureName, names.ScenarioName)
		panic(fmt.Sprintf("error validating cloud used quota after %q: %s", where, err))
	}

	if len(clouds) == 0 {
		return
	}

	var fails []string
	for _, cloud := range clouds {
		fails = append(fails, fmt.Sprintf("%+v", cloud))
	}
	names := tctx.TC.LastExecutedNames()
	where := fmt.Sprintf("%s/%s", names.FeatureName, names.ScenarioName)
	err = xerrors.Errorf("invalid used quota after %q: %s", where, fails)

	dump(fn, err)
	panic(err)
}

func (tctx *testContext) templateStep(step *gherkin.Step) error {
	var err error
	step.Text, err = tctx.templateString(step.Text)
	if err != nil {
		return err
	}

	if v, ok := step.Argument.(*gherkin.DocString); ok {
		v.Content, err = tctx.templateString(v.Content)
		if err != nil {
			return err
		}
		step.Argument = v
	}
	if v, ok := step.Argument.(*gherkin.DataTable); ok {
		if v.Rows != nil {
			for _, row := range v.Rows {
				for _, cell := range row.Cells {
					cell.Value, err = tctx.templateString(cell.Value)
					if err != nil {
						return err
					}
				}
			}
		}
	}
	return nil
}

func (tctx *testContext) templateString(data string) (string, error) {
	if !strings.Contains(data, "{{") {
		return data, nil
	}

	tpl, err := template.New(data).Funcs(templateFuncs()).Parse(data)
	if err != nil {
		return data, err
	}
	var res strings.Builder
	err = tpl.Execute(&res, tctx.variables)
	if err != nil {
		return data, err
	}
	return res.String(), nil
}

func templateFuncs() template.FuncMap {
	return template.FuncMap{
		"sortStrings":   sortStringSlice,
		"mkStringSlice": mkStringSlice,
	}
}

func sortStringSlice(strings []string) []string {
	sort.Strings(strings)
	return strings
}

func mkStringSlice(args ...string) []string {
	return args
}

type InvalidCloudQuota struct {
	CloudID    int64   `db:"o_cloud_id"`
	CloudExtID string  `db:"o_cloud_ext_id"`
	CPU        float64 `db:"o_cpu"`
	GPU        int64   `db:"o_gpu"`
	Memory     int64   `db:"o_memory"`
	Network    int64   `db:"o_network"`
	IO         int64   `db:"o_io"`
	SSDSpace   int64   `db:"o_ssd_space"`
	HDDSpace   int64   `db:"o_hdd_space"`
	Clusters   int64   `db:"o_clusters"`
}

func (tctx *testContext) isModeEnabled(mode Mode, api API) bool {
	/*tctx.L.Debugf(
		"isModeEnabled, mode %q, api %q, modifyMode %q, readMode %q, last api %q",
		mode.String(), api.String(), tctx.ModifyModeAPI.String(), tctx.ReadModeAPI.String(), tctx.LastAPI.String(),
	)*/

	// Invalidate last API. If we don't and this is not our mode, we might leak state of previous step.
	tctx.LastAPI = APIInvalid

	var match bool
	switch mode {
	case ModeRead:
		match = tctx.ReadModeAPI == api
	case ModeModify:
		match = tctx.ModifyModeAPI == api
	default:
		panic(fmt.Sprintf("invalid mode: %s", mode))
	}

	return match
}

func (tctx *testContext) isLastAPI(api API) bool {
	//tctx.L.Debugf("isLastAPI, api %q, modifyMode %q, readMode %q, last api %q", api.String(), tctx.ModifyModeAPI.String(), tctx.ReadModeAPI.String(), tctx.LastAPI.String())
	return tctx.LastAPI == api
}
