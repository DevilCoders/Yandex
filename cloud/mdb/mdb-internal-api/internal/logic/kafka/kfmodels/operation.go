package kfmodels

import (
	"time"

	"github.com/golang/protobuf/proto"
	"google.golang.org/protobuf/types/known/timestamppb"

	kfv1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/kafka/v1"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/operations"
)

const (
	OperationTypeClusterCreate        operations.Type = "kafka_cluster_create"
	OperationTypeClusterModify        operations.Type = "kafka_cluster_modify"
	OperationTypeMetadataUpdate       operations.Type = "kafka_metadata_update"
	OperationTypeClusterDelete        operations.Type = "kafka_cluster_delete"
	OperationTypeClusterStart         operations.Type = "kafka_cluster_start"
	OperationTypeClusterStop          operations.Type = "kafka_cluster_stop"
	OperationTypeClusterMove          operations.Type = "kafka_cluster_move"
	OperationTypeClusterUpgrade       operations.Type = "kafka_cluster_upgrade"
	OperationTypeOnlineResetup        operations.Type = "kafka_cluster_online_resetup"
	OperationTypeUserCreate           operations.Type = "kafka_user_create"
	OperationTypeUserModify           operations.Type = "kafka_user_modify"
	OperationTypeUserGrantPermission  operations.Type = "kafka_user_grant_permission"
	OperationTypeUserRevokePermission operations.Type = "kafka_user_revoke_permission"
	OperationTypeUserDelete           operations.Type = "kafka_user_delete"
	OperationTypeTopicAdd             operations.Type = "kafka_topic_add"
	OperationTypeTopicModify          operations.Type = "kafka_topic_modify"
	OperationTypeTopicDelete          operations.Type = "kafka_topic_delete"
	OperationTypeConnectorCreate      operations.Type = "kafka_connector_create"
	OperationTypeConnectorUpdate      operations.Type = "kafka_connector_update"
	OperationTypeConnectorDelete      operations.Type = "kafka_connector_delete"
	OperationTypeConnectorPause       operations.Type = "kafka_connector_pause"
	OperationTypeConnectorResume      operations.Type = "kafka_connector_resume"
	OperationTypeHostDelete           operations.Type = "kafka_host_delete"

	OperationTypeMaintenanceReschedule operations.Type = "kafka_maintenance_reschedule"
)

func init() {
	operations.Register(
		OperationTypeClusterCreate,
		"Create Apache Kafka cluster",
		MetadataCreateCluster{},
	)

	operations.Register(
		OperationTypeClusterModify,
		"Modify Apache Kafka cluster",
		MetadataModifyCluster{},
	)

	operations.Register(
		OperationTypeMetadataUpdate,
		"Update Apache Kafka cluster metadata",
		MetadataModifyCluster{},
	)

	operations.Register(
		OperationTypeClusterDelete,
		"Delete Apache Kafka cluster",
		MetadataDeleteCluster{},
	)

	operations.Register(
		OperationTypeClusterStart,
		"Start Apache Kafka cluster",
		MetadataStartCluster{},
	)

	operations.Register(
		OperationTypeClusterStop,
		"Stop Apache Kafka cluster",
		MetadataStopCluster{},
	)

	operations.Register(
		OperationTypeClusterMove,
		"Move Apache Kafka cluster",
		MetadataMoveCluster{},
	)

	operations.Register(
		OperationTypeClusterUpgrade,
		"Upgrade Apache Kafka cluster",
		MetadataModifyCluster{},
	)

	operations.Register(
		OperationTypeUserCreate,
		"Create user in Apache Kafka cluster",
		MetadataCreateUser{},
	)

	operations.Register(
		OperationTypeUserModify,
		"Modify user in Apache Kafka cluster",
		MetadataModifyUser{},
	)

	operations.Register(
		OperationTypeUserGrantPermission,
		"Grant permission to user in Apache Kafka cluster",
		MetadataModifyUser{},
	)

	operations.Register(
		OperationTypeUserRevokePermission,
		"Revoke permission from user in Apache Kafka cluster",
		MetadataModifyUser{},
	)

	operations.Register(
		OperationTypeUserDelete,
		"Delete user from Apache Kafka cluster",
		MetadataDeleteUser{},
	)

	operations.Register(
		OperationTypeTopicAdd,
		"Add topic to Apache Kafka cluster",
		MetadataCreateTopic{},
	)

	operations.Register(
		OperationTypeTopicModify,
		"Modify topic in Apache Kafka cluster",
		MetadataModifyTopic{},
	)

	operations.Register(
		OperationTypeTopicDelete,
		"Delete topic from Apache Kafka cluster",
		MetadataDeleteTopic{},
	)

	operations.Register(
		OperationTypeConnectorCreate,
		"Create managed connector on Apache Kafka cluster",
		MetadataCreateConnector{},
	)

	operations.Register(
		OperationTypeConnectorUpdate,
		"Update managed connector on Apache Kafka cluster",
		MetadataUpdateConnector{},
	)

	operations.Register(
		OperationTypeConnectorDelete,
		"Delete managed connector on Apache Kafka cluster",
		MetadataDeleteConnector{},
	)

	operations.Register(
		OperationTypeConnectorPause,
		"Pause managed connector on Apache Kafka cluster",
		MetadataPauseConnector{},
	)

	operations.Register(
		OperationTypeConnectorResume,
		"Resume managed connector on Apache Kafka cluster",
		MetadataResumeConnector{},
	)

	operations.Register(
		OperationTypeHostDelete,
		"Delete Kafka host from Apache Kafka cluster",
		MetadataDeleteHost{},
	)

	operations.Register(
		OperationTypeMaintenanceReschedule,
		"Reschedule maintenance in Kafka cluster",
		MetadataRescheduleMaintenance{},
	)

	operations.Register(
		OperationTypeOnlineResetup,
		"Resetup host in Kafka cluster",
		MetadataResetupCluster{},
	)
}

type MetadataCreateCluster struct{}

func (md MetadataCreateCluster) Build(op operations.Operation) proto.Message {
	return &kfv1.CreateClusterMetadata{
		ClusterId: op.TargetID,
	}
}

type MetadataModifyCluster struct{}

func (md MetadataModifyCluster) Build(op operations.Operation) proto.Message {
	return &kfv1.UpdateClusterMetadata{
		ClusterId: op.TargetID,
	}
}

type MetadataDeleteCluster struct{}

func (md MetadataDeleteCluster) Build(op operations.Operation) proto.Message {
	return &kfv1.DeleteClusterMetadata{
		ClusterId: op.TargetID,
	}
}

type MetadataStartCluster struct{}

func (md MetadataStartCluster) Build(op operations.Operation) proto.Message {
	return &kfv1.StartClusterMetadata{
		ClusterId: op.TargetID,
	}
}

type MetadataStopCluster struct{}

func (md MetadataStopCluster) Build(op operations.Operation) proto.Message {
	return &kfv1.StopClusterMetadata{
		ClusterId: op.TargetID,
	}
}

type MetadataMoveCluster struct {
	SourceFolderID      string `json:"source_folder_id"`
	DestinationFolderID string `json:"destination_folder_id"`
}

func (md MetadataMoveCluster) Build(op operations.Operation) proto.Message {
	return &kfv1.MoveClusterMetadata{
		ClusterId:           op.TargetID,
		SourceFolderId:      md.SourceFolderID,
		DestinationFolderId: md.DestinationFolderID,
	}
}

type TopicMetadata interface {
	GetTopicName() string
}

type MetadataCreateTopic struct {
	TopicName string `json:"topic_name"`
}

func (md MetadataCreateTopic) Build(op operations.Operation) proto.Message {
	return &kfv1.CreateTopicMetadata{
		ClusterId: op.TargetID,
		TopicName: md.TopicName,
	}
}

func (md MetadataCreateTopic) GetTopicName() string {
	return md.TopicName
}

var _ TopicMetadata = MetadataCreateTopic{}

type MetadataModifyTopic struct {
	TopicName string `json:"topic_name"`
}

func (md MetadataModifyTopic) Build(op operations.Operation) proto.Message {
	return &kfv1.UpdateTopicMetadata{
		ClusterId: op.TargetID,
		TopicName: md.TopicName,
	}
}

func (md MetadataModifyTopic) GetTopicName() string {
	return md.TopicName
}

var _ TopicMetadata = MetadataModifyTopic{}

type MetadataDeleteTopic struct {
	TopicName string `json:"topic_name"`
}

func (md MetadataDeleteTopic) Build(op operations.Operation) proto.Message {
	return &kfv1.DeleteTopicMetadata{
		ClusterId: op.TargetID,
		TopicName: md.TopicName,
	}
}

func (md MetadataDeleteTopic) GetTopicName() string {
	return md.TopicName
}

var _ TopicMetadata = MetadataDeleteTopic{}

type UserMetadata interface {
	GetUserName() string
}

type MetadataCreateUser struct {
	UserName string `json:"user_name"`
}

func (md MetadataCreateUser) Build(op operations.Operation) proto.Message {
	return &kfv1.CreateUserMetadata{
		ClusterId: op.TargetID,
		UserName:  md.UserName,
	}
}

func (md MetadataCreateUser) GetUserName() string {
	return md.UserName
}

var _ UserMetadata = MetadataCreateUser{}

type MetadataModifyUser struct {
	UserName string `json:"user_name"`
}

func (md MetadataModifyUser) Build(op operations.Operation) proto.Message {
	return &kfv1.UpdateUserMetadata{
		ClusterId: op.TargetID,
		UserName:  md.UserName,
	}
}

func (md MetadataModifyUser) GetUserName() string {
	return md.UserName
}

var _ UserMetadata = MetadataModifyUser{}

type MetadataDeleteUser struct {
	UserName string `json:"user_name"`
}

func (md MetadataDeleteUser) Build(op operations.Operation) proto.Message {
	return &kfv1.DeleteUserMetadata{
		ClusterId: op.TargetID,
		UserName:  md.UserName,
	}
}

func (md MetadataDeleteUser) GetUserName() string {
	return md.UserName
}

var _ UserMetadata = MetadataDeleteUser{}

type MetadataCreateConnector struct {
	ConnectorName string `json:"connector_name"`
}

type ConnectorMetadata interface {
	GetConnectorName() string
}

func (md MetadataCreateConnector) Build(op operations.Operation) proto.Message {
	return &kfv1.CreateConnectorMetadata{
		ClusterId:     op.TargetID,
		ConnectorName: md.ConnectorName,
	}
}

func (md MetadataCreateConnector) GetConnectorName() string {
	return md.ConnectorName
}

var _ ConnectorMetadata = MetadataCreateConnector{}

type MetadataUpdateConnector struct {
	ConnectorName string `json:"connector_name"`
}

func (md MetadataUpdateConnector) Build(op operations.Operation) proto.Message {
	return &kfv1.UpdateConnectorMetadata{
		ClusterId:     op.TargetID,
		ConnectorName: md.ConnectorName,
	}
}

func (md MetadataUpdateConnector) GetConnectorName() string {
	return md.ConnectorName
}

var _ ConnectorMetadata = MetadataUpdateConnector{}

type MetadataDeleteConnector struct {
	ConnectorName string `json:"connector_name"`
}

func (md MetadataDeleteConnector) Build(op operations.Operation) proto.Message {
	return &kfv1.DeleteConnectorMetadata{
		ClusterId:     op.TargetID,
		ConnectorName: md.ConnectorName,
	}
}

func (md MetadataDeleteConnector) GetConnectorName() string {
	return md.ConnectorName
}

var _ ConnectorMetadata = MetadataDeleteConnector{}

type MetadataResumeConnector struct {
	ConnectorName string `json:"connector_name"`
}

func (md MetadataResumeConnector) Build(op operations.Operation) proto.Message {
	return &kfv1.ResumeConnectorMetadata{
		ClusterId:     op.TargetID,
		ConnectorName: md.ConnectorName,
	}
}

func (md MetadataResumeConnector) GetConnectorName() string {
	return md.ConnectorName
}

var _ ConnectorMetadata = MetadataResumeConnector{}

type MetadataPauseConnector struct {
	ConnectorName string `json:"connector_name"`
}

func (md MetadataPauseConnector) Build(op operations.Operation) proto.Message {
	return &kfv1.PauseConnectorMetadata{
		ClusterId:     op.TargetID,
		ConnectorName: md.ConnectorName,
	}
}

func (md MetadataPauseConnector) GetConnectorName() string {
	return md.ConnectorName
}

var _ ConnectorMetadata = MetadataPauseConnector{}

type MetadataDeleteHost struct{}

func (md MetadataDeleteHost) Build(op operations.Operation) proto.Message {
	return &kfv1.UpdateClusterMetadata{
		ClusterId: op.TargetID,
	}
}

type MetadataRescheduleMaintenance struct {
	DelayedUntil time.Time `json:"delayed_until"`
}

func (md MetadataRescheduleMaintenance) Build(op operations.Operation) proto.Message {
	return &kfv1.RescheduleMaintenanceMetadata{
		ClusterId:    op.TargetID,
		DelayedUntil: timestamppb.New(md.DelayedUntil),
	}
}

type MetadataResetupCluster struct{}

func (md MetadataResetupCluster) Build(op operations.Operation) proto.Message {
	return &kfv1.UpdateClusterMetadata{
		ClusterId: op.TargetID,
	}
}
