package kfmodels

import "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/tasks"

const (
	TaskTypeClusterCreate         tasks.Type = "kafka_cluster_create"
	TaskTypeClusterDelete         tasks.Type = "kafka_cluster_delete"
	TaskTypeClusterDeleteMetadata tasks.Type = "kafka_cluster_delete_metadata"
	TaskTypeClusterModify         tasks.Type = "kafka_cluster_modify"
	TaskTypeMetadataUpdate        tasks.Type = "kafka_metadata_update"
	TaskTypeClusterStart          tasks.Type = "kafka_cluster_start"
	TaskTypeClusterStop           tasks.Type = "kafka_cluster_stop"
	TaskTypeClusterMove           tasks.Type = "kafka_cluster_move"
	TaskTypeClusterMoveNoOp       tasks.Type = "noop"
	TaskTypeClusterUpgrade        tasks.Type = "kafka_cluster_upgrade"
	TaskTypeTopicCreate           tasks.Type = "kafka_topic_create"
	TaskTypeTopicModify           tasks.Type = "kafka_topic_modify"
	TaskTypeTopicDelete           tasks.Type = "kafka_topic_delete"
	TaskTypeUserCreate            tasks.Type = "kafka_user_create"
	TaskTypeUserModify            tasks.Type = "kafka_user_modify"
	TaskTypeUserDelete            tasks.Type = "kafka_user_delete"
	TaskTypeHostCreate            tasks.Type = "kafka_host_create"
	TaskTypeHostDelete            tasks.Type = "kafka_host_delete"
	TaskTypeConnectorCreate       tasks.Type = "kafka_connector_create"
	TaskTypeConnectorUpdate       tasks.Type = "kafka_connector_update"
	TaskTypeConnectorDelete       tasks.Type = "kafka_connector_delete"
	TaskTypeConnectorPause        tasks.Type = "kafka_connector_pause"
	TaskTypeConnectorResume       tasks.Type = "kafka_connector_resume"
)
