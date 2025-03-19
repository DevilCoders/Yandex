package common

import (
	"context"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/search"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/sessions"
	taskslogic "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/tasks"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/operations"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/tasks"
)

type ClusterChanges struct {
	NeedUpgrade bool
	// hs required
	HasChanges bool
	// metadata operation required
	HasMetadataChanges bool
	// only changes in metadb
	HasMetaDBChanges bool
	TaskArgs         map[string]interface{}
	Timeout          time.Duration
}

type TaskCreationInfo struct {
	ClusterUpgradeTask     tasks.Type
	ClusterModifyTask      tasks.Type
	ClusterModifyOperation operations.Type

	MetadataUpdateTask      tasks.Type
	MetadataUpdateOperation operations.Type

	SearchService string
}

var runServiceOnMetaChanges = map[clusters.Type]interface{}{
	clusters.TypeGreenplumCluster: nil,
	clusters.TypeRedis:            nil,
}

func (changes *ClusterChanges) HasOnlyMetadbChanges() bool {
	return changes.HasMetaDBChanges && !changes.HasMetadataChanges && !changes.HasChanges
}

func GetClusterChanges() ClusterChanges {
	return ClusterChanges{Timeout: time.Hour, TaskArgs: map[string]interface{}{}}
}

func CreateClusterModifyOperation(
	ts taskslogic.Tasks,
	ctx context.Context,
	session sessions.Session,
	cluster clusters.Cluster,
	searchAttributesExtractor search.AttributesExtractor,
	clusterChanges ClusterChanges,
	securityGroupIDs optional.Strings,
	taskInfo TaskCreationInfo,
) (operations.Operation, error) {
	taskType := taskInfo.ClusterModifyTask
	opType := taskInfo.ClusterModifyOperation
	modifyCluster := false

	if clusterChanges.HasChanges {
		modifyCluster = true
		if clusterChanges.NeedUpgrade {
			if len(taskInfo.ClusterUpgradeTask) == 0 {
				return operations.Operation{}, semerr.NotImplemented("cluster upgrade is not implemented")
			}
			taskType = taskInfo.ClusterUpgradeTask
		}
	} else if clusterChanges.HasMetadataChanges {
		modifyCluster = true
		// some clusters need to run service operation on metadata changes
		if _, ok := runServiceOnMetaChanges[cluster.Type]; !ok {
			taskType = taskInfo.MetadataUpdateTask
			opType = taskInfo.MetadataUpdateOperation
		}
	}

	if modifyCluster {
		op, err := ts.ModifyCluster(
			ctx,
			session,
			cluster.ClusterID,
			cluster.Revision,
			taskType,
			opType,
			securityGroupIDs,
			taskInfo.SearchService,
			searchAttributesExtractor,
			taskslogic.ModifyTimeout(clusterChanges.Timeout),
			taskslogic.ModifyTaskArgs(clusterChanges.TaskArgs),
		)
		if err != nil {
			return operations.Operation{}, err
		}
		return op, nil
	}

	if clusterChanges.HasMetaDBChanges {
		op, err := ts.CreateFinishedTask(
			ctx,
			session,
			cluster.ClusterID,
			cluster.Revision,
			taskInfo.ClusterModifyOperation,
			nil,
			false,
		)
		if err != nil {
			return operations.Operation{}, err
		}

		return op, nil
	} else {
		return operations.Operation{}, semerr.FailedPrecondition("no changes detected")
	}
}
