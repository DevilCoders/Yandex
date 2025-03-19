package kafka

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/kafka/kfmodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/console"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/environment"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/kafka"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/operations"
	modelsoptional "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/optional"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/pagination"
)

//go:generate ../../../../scripts/mockgen.sh Kafka

type Kafka interface {
	MDBCluster(ctx context.Context, cid string) (kfmodels.MDBCluster, error)
	DataCloudCluster(ctx context.Context, cid string, sensitive bool) (kfmodels.DataCloudCluster, error)
	MDBClusters(ctx context.Context, folderExtID string, limit, offset int64) ([]kfmodels.MDBCluster, error)
	DataCloudClusters(ctx context.Context, folderExtID string, limit, offset int64) ([]kfmodels.DataCloudCluster, error)
	CreateMDBCluster(ctx context.Context, args CreateMDBClusterArgs) (operations.Operation, error)
	CreateDataCloudCluster(ctx context.Context, args CreateDataCloudClusterArgs) (operations.Operation, error)
	ModifyMDBCluster(ctx context.Context, args ModifyMDBClusterArgs) (operations.Operation, error)
	ModifyDataCloudCluster(ctx context.Context, args ModifyDataCloudClusterArgs) (operations.Operation, error)
	DeleteCluster(ctx context.Context, cid string) (operations.Operation, error)
	StartCluster(ctx context.Context, cid string) (operations.Operation, error)
	StopCluster(ctx context.Context, cid string) (operations.Operation, error)
	MoveCluster(ctx context.Context, cid, destinationFolderID string) (operations.Operation, error)
	ListHosts(ctx context.Context, cid string, limit, offset int64) ([]hosts.HostExtended, error)
	RescheduleMaintenance(ctx context.Context, cid string, rescheduleType clusters.RescheduleType, delayedUntil optional.Time) (operations.Operation, error)

	User(ctx context.Context, cid, name string) (kfmodels.User, error)
	Users(ctx context.Context, cid string, limit, offset int64) ([]kfmodels.User, pagination.OffsetPageToken, error)
	CreateUser(ctx context.Context, cid string, spec kfmodels.UserSpec) (operations.Operation, error)
	UpdateUser(ctx context.Context, spec kfmodels.UpdateUserArgs) (operations.Operation, error)
	GrantUserPermission(ctx context.Context, cid string, name string, permission kfmodels.Permission) (operations.Operation, error)
	RevokeUserPermission(ctx context.Context, cid string, name string, permission kfmodels.Permission) (operations.Operation, error)
	DeleteUser(ctx context.Context, cid string, name string) (operations.Operation, error)
	ResetCredentials(ctx context.Context, cid string) (operations.Operation, error)

	Topic(ctx context.Context, cid, name string) (kfmodels.Topic, error)
	Topics(ctx context.Context, cid string, pageSize int64, pageToken kafka.TopicPageToken) ([]kfmodels.Topic, kafka.TopicPageToken, error)
	CreateTopic(ctx context.Context, cid string, spec kfmodels.TopicSpec) (operations.Operation, error)
	UpdateTopic(ctx context.Context, args UpdateTopicArgs) (operations.Operation, error)
	DeleteTopic(ctx context.Context, cid string, name string) (operations.Operation, error)

	CreateConnector(ctx context.Context, cid string, spec kfmodels.ConnectorSpec) (operations.Operation, error)
	UpdateConnector(ctx context.Context, args UpdateConnectorArgs) (operations.Operation, error)
	Connector(ctx context.Context, cid string, name string) (kfmodels.Connector, error)
	Connectors(ctx context.Context, cid string, limit, offset int64) ([]kfmodels.Connector, error)
	DeleteConnector(ctx context.Context, cid string, name string) (operations.Operation, error)
	PauseConnector(ctx context.Context, cid string, name string) (operations.Operation, error)
	ResumeConnector(ctx context.Context, cid string, name string) (operations.Operation, error)

	TopicsToSync(ctx context.Context, cid string) (TopicsToSync, error)
	SyncTopics(ctx context.Context, cid string, revision int64, updatedTopics []string, deletedTopics []string) (bool, error)

	EstimateCreateCluster(ctx context.Context, args CreateMDBClusterArgs) (console.BillingEstimate, error)
	EstimateCreateDCCluster(ctx context.Context, args CreateDataCloudClusterArgs) (console.BillingEstimate, error)

	Version(ctx context.Context, metadb metadb.Backend, cid string) (string, error)
}

type CreateMDBClusterArgs struct {
	FolderExtID        string
	Name               string
	Description        string
	Labels             map[string]string
	Environment        environment.SaltEnv
	NetworkID          string
	SubnetID           []string
	SecurityGroupIDs   []string
	HostGroupIDs       []string
	UserSpecs          []kfmodels.UserSpec
	TopicSpecs         []kfmodels.TopicSpec
	ConfigSpec         kfmodels.MDBClusterSpec
	DeletionProtection bool
	MaintenanceWindow  clusters.MaintenanceWindow
}

type CreateDataCloudClusterArgs struct {
	ProjectID   string
	Name        string
	Description string
	CloudType   environment.CloudType
	RegionID    string
	ClusterSpec kfmodels.DataCloudClusterSpec
	NetworkID   optional.String
}

type ModifyMDBClusterArgs struct {
	ClusterID          string
	Name               optional.String
	Description        optional.String
	SecurityGroupIDs   optional.Strings
	Labels             modelsoptional.Labels
	ConfigSpec         kfmodels.ClusterConfigSpecMDBUpdate
	DeletionProtection optional.Bool
	MaintenanceWindow  modelsoptional.MaintenanceWindow
}

type ModifyDataCloudClusterArgs struct {
	ClusterID   string
	Name        optional.String
	Description optional.String
	Labels      modelsoptional.Labels
	ConfigSpec  kfmodels.ClusterConfigSpecDataCloudUpdate
}

type UpdateTopicArgs struct {
	ClusterID string
	Name      string
	TopicSpec kfmodels.TopicSpecUpdate
}

type UpdateConnectorArgs struct {
	ClusterID     string
	Name          string
	ConnectorSpec kfmodels.UpdateConnectorSpec
}

type TopicsToSync struct {
	UpdateAllowed              bool
	Revision                   int64
	Topics                     []string
	KnownTopicConfigProperties []string
}
