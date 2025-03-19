package provider

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/search"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/sessions"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/tasks"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/operations"
	tasksmodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/tasks"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/slices"
)

func (t *Tasks) CreateCluster(ctx context.Context, session sessions.Session, cid string, revision int64, taskType tasksmodels.Type, opType operations.Type, metadata operations.Metadata, s3bucket optional.String, securityGroupIDs []string, service string, attributesExtractor search.AttributesExtractor, opts ...tasks.CreateClusterOption) (operations.Operation, error) {
	options := &tasks.CreateClusterOptions{}
	for _, o := range opts {
		o(options)
	}

	taskArgs := make(map[string]interface{})
	if s3bucket.Valid {
		taskArgs["s3_buckets"] = map[string]string{"backup": s3bucket.String}
	}
	if securityGroupIDs != nil {
		taskArgs["security_group_ids"] = slices.DedupStrings(securityGroupIDs)
	}
	if options.TaskArgs != nil {
		for k, v := range options.TaskArgs {
			taskArgs[k] = v
		}
	}

	op, err := t.CreateTask(
		ctx,
		session,
		tasksmodels.CreateTaskArgs{
			ClusterID:     cid,
			FolderID:      session.FolderCoords.FolderID,
			Auth:          session.Subject,
			TaskType:      taskType,
			TaskArgs:      taskArgs,
			OperationType: opType,
			Metadata:      metadata,
			Timeout:       options.Timeout,
			Revision:      revision,
		})
	if err != nil {
		return operations.Operation{}, xerrors.Errorf("cluster create task %q for operation %q not created: %w", taskType, opType, err)
	}

	if err = t.search.StoreDoc(ctx, service, session.FolderCoords.FolderExtID, session.FolderCoords.CloudExtID, op, attributesExtractor); err != nil {
		return operations.Operation{}, xerrors.Errorf("cluster create task %q for operation %q not sent to search: %w", taskType, opType, err)
	}

	return op, nil
}

func (t *Tasks) ModifyCluster(
	ctx context.Context, session sessions.Session, cid string, revision int64,
	taskType tasksmodels.Type, opType operations.Type,
	securityGroupIDs optional.Strings,
	service string, attributesExtractor search.AttributesExtractor,
	opts ...tasks.ModifyClusterOption) (operations.Operation, error) {

	options := &tasks.ModifyClusterOptions{}
	for _, o := range opts {
		o(options)
	}
	taskArgs := make(map[string]interface{})
	if securityGroupIDs.Valid {
		taskArgs["security_group_ids"] = slices.DedupStrings(securityGroupIDs.Strings)
	}
	if options.TaskArgs != nil {
		for k, v := range options.TaskArgs {
			taskArgs[k] = v
		}
	}

	op, err := t.CreateTask(
		ctx,
		session,
		tasksmodels.CreateTaskArgs{
			ClusterID:     cid,
			FolderID:      session.FolderCoords.FolderID,
			Auth:          session.Subject,
			TaskType:      taskType,
			TaskArgs:      taskArgs,
			OperationType: opType,
			Timeout:       options.Timeout,
			Revision:      revision,
		})
	if err != nil {
		return operations.Operation{}, xerrors.Errorf("cluster modify task %q for operation %q not created: %w", taskType, opType, err)
	}

	if err = t.search.StoreDoc(ctx, service, session.FolderCoords.FolderExtID, session.FolderCoords.CloudExtID, op, attributesExtractor); err != nil {
		return operations.Operation{}, xerrors.Errorf("cluster modify task %q for operation %q not sent to search: %w", taskType, opType, err)
	}

	return op, nil
}

func (t *Tasks) DeleteCluster(ctx context.Context, session sessions.Session, cid string, revision int64, taskTypes tasks.DeleteClusterTaskTypes, opType operations.Type, s3Buckets tasks.DeleteClusterS3Buckets, opts ...tasks.DeleteClusterOption) (operations.Operation, error) {
	if err := taskTypes.Validate(); err != nil {
		return operations.Operation{}, err
	}

	options := &tasks.DeleteClusterOptions{}
	for _, o := range opts {
		o(options)
	}
	taskArgs := make(map[string]interface{})
	if len(s3Buckets) > 0 {
		taskArgs["s3_buckets"] = s3Buckets
	}
	if options.TaskArgs != nil {
		for k, v := range options.TaskArgs {
			taskArgs[k] = v
		}
	}

	deleteOp, err := t.CreateTask(
		ctx,
		session,
		tasksmodels.CreateTaskArgs{
			ClusterID:     cid,
			FolderID:      session.FolderCoords.FolderID,
			Auth:          session.Subject,
			TaskType:      taskTypes.Delete,
			TaskArgs:      taskArgs,
			OperationType: opType,
			Revision:      revision,
		},
	)
	if err != nil {
		return operations.Operation{}, xerrors.Errorf("creating cluster deletion task %q for operation %q. %w", taskTypes.Delete, opType, err)
	}

	deleteMetadataOp, err := t.CreateTask(
		ctx,
		session,
		tasksmodels.CreateTaskArgs{
			ClusterID:           cid,
			FolderID:            session.FolderCoords.FolderID,
			Auth:                session.Subject,
			TaskType:            taskTypes.Metadata,
			TaskArgs:            taskArgs,
			OperationType:       opType,
			Revision:            revision,
			RequiredOperationID: optional.NewString(deleteOp.OperationID),
			Hidden:              true,
			SkipIdempotence:     true,
		},
	)
	if err != nil {
		return operations.Operation{}, xerrors.Errorf("creating cluster metadata deletion task %q for operation %q. %w", taskTypes.Metadata, opType, err)
	}

	// TODO: this 'if' must be removed when all databases use purge tasks
	if taskTypes.Purge != "" {
		_, err = t.CreateTask(
			ctx,
			session,
			tasksmodels.CreateTaskArgs{
				ClusterID:           cid,
				FolderID:            session.FolderCoords.FolderID,
				Auth:                session.Subject,
				TaskType:            taskTypes.Purge,
				TaskArgs:            taskArgs,
				OperationType:       opType,
				Revision:            revision,
				RequiredOperationID: optional.NewString(deleteMetadataOp.OperationID),
				DelayBy:             optional.NewDuration(t.cfg.BackupPurgeDelay),
				Hidden:              true,
				SkipIdempotence:     true,
			},
		)
		if err != nil {
			return operations.Operation{}, xerrors.Errorf("creating cluster purge task %q for operation %q. %w", taskTypes.Purge, opType, err)
		}
	}

	return deleteOp, nil
}

func (t *Tasks) StartCluster(ctx context.Context, session sessions.Session, cid string, revision int64, taskType tasksmodels.Type, opType operations.Type, metadata operations.Metadata, opts ...tasks.StartClusterOption) (operations.Operation, error) {
	options := &tasks.StartClusterOptions{}
	for _, o := range opts {
		o(options)
	}
	taskArgs := make(map[string]interface{})
	if options.TaskArgs != nil {
		for k, v := range options.TaskArgs {
			taskArgs[k] = v
		}
	}

	op, err := t.CreateTask(
		ctx,
		session,
		tasksmodels.CreateTaskArgs{
			ClusterID:     cid,
			FolderID:      session.FolderCoords.FolderID,
			Auth:          session.Subject,
			TaskType:      taskType,
			TaskArgs:      taskArgs,
			OperationType: opType,
			Metadata:      metadata,
			Revision:      revision,
		})
	if err != nil {
		return operations.Operation{}, xerrors.Errorf("cluster start task %q for operation %q not created: %w", taskType, opType, err)
	}

	return op, nil
}

func (t *Tasks) StopCluster(ctx context.Context, session sessions.Session, cid string, revision int64, taskType tasksmodels.Type, opType operations.Type, metadata operations.Metadata) (operations.Operation, error) {
	op, err := t.CreateTask(
		ctx,
		session,
		tasksmodels.CreateTaskArgs{
			ClusterID:     cid,
			FolderID:      session.FolderCoords.FolderID,
			Auth:          session.Subject,
			TaskType:      taskType,
			OperationType: opType,
			Metadata:      metadata,
			Revision:      revision,
		})
	if err != nil {
		return operations.Operation{}, xerrors.Errorf("cluster stop task %q for operation %q not created: %w", taskType, opType, err)
	}

	return op, nil
}

func (t *Tasks) MoveCluster(ctx context.Context, session sessions.Session, cid string, revision int64, opType operations.Type, metadata operations.Metadata, opts ...tasks.MoveClusterOption) (operations.Operation, error) {
	options := &tasks.MoveClusterOptions{}
	for _, o := range opts {
		o(options)
	}
	taskArgs := make(map[string]interface{})
	if options.TaskArgs != nil {
		for k, v := range options.TaskArgs {
			taskArgs[k] = v
		}
	}

	op, err := t.CreateTask(
		ctx,
		session,
		tasksmodels.CreateTaskArgs{
			ClusterID:     cid,
			FolderID:      options.SrcFolderCoords.FolderID,
			Auth:          session.Subject,
			TaskType:      options.SrcFolderTaskType,
			OperationType: opType,
			Metadata:      metadata,
			Revision:      revision,
		})
	if err != nil {
		return operations.Operation{}, xerrors.Errorf("cluster move task %q for operation %q not created: %w", options.SrcFolderTaskType, opType, err)
	}

	dstFolderCoords, err := t.metaDB.FolderCoordsByFolderExtID(ctx, options.DstFolderExtID)
	if err != nil {
		return operations.Operation{}, err
	}

	_, err = t.CreateTask(
		ctx,
		session,
		tasksmodels.CreateTaskArgs{
			ClusterID:           cid,
			FolderID:            dstFolderCoords.FolderID,
			Auth:                session.Subject,
			TaskType:            options.DstFolderTaskType,
			OperationType:       opType,
			Metadata:            metadata,
			Revision:            revision,
			RequiredOperationID: optional.NewString(op.OperationID),
			SkipConflictingTask: true,
		})
	if err != nil {
		return operations.Operation{}, xerrors.Errorf("cluster move task %q for operation %q not created: %w", options.DstFolderTaskType, opType, err)
	}

	return op, nil
}

func (t *Tasks) BackupCluster(ctx context.Context, session sessions.Session, cid string, revision int64, taskType tasksmodels.Type, opType operations.Type, metadata operations.Metadata, opts ...tasks.BackupClusterOption) (operations.Operation, error) {
	options := &tasks.BackupClusterOptions{}
	for _, o := range opts {
		o(options)
	}
	taskArgs := make(map[string]interface{})
	if options.TaskArgs != nil {
		for k, v := range options.TaskArgs {
			taskArgs[k] = v
		}
	}

	op, err := t.CreateTask(
		ctx,
		session,
		tasksmodels.CreateTaskArgs{
			ClusterID:     cid,
			FolderID:      session.FolderCoords.FolderID,
			Auth:          session.Subject,
			TaskType:      taskType,
			TaskArgs:      taskArgs,
			OperationType: opType,
			Metadata:      metadata,
			Revision:      revision,
		})
	if err != nil {
		return operations.Operation{}, xerrors.Errorf("cluster backup task %q for operation %q not created: %w", taskType, opType, err)
	}

	return op, nil
}

func (t *Tasks) UpgradeCluster(ctx context.Context, session sessions.Session, cid string, revision int64, taskType tasksmodels.Type, opType operations.Type, metadata operations.Metadata, opts ...tasks.UpgradeClusterOption) (operations.Operation, error) {
	options := &tasks.UpgradeClusterOptions{}
	for _, o := range opts {
		o(options)
	}
	taskArgs := make(map[string]interface{})
	if options.TaskArgs != nil {
		for k, v := range options.TaskArgs {
			taskArgs[k] = v
		}
	}

	op, err := t.CreateTask(
		ctx,
		session,
		tasksmodels.CreateTaskArgs{
			ClusterID:     cid,
			FolderID:      session.FolderCoords.FolderID,
			Auth:          session.Subject,
			TaskType:      taskType,
			TaskArgs:      taskArgs,
			OperationType: opType,
			Metadata:      metadata,
			Revision:      revision,
		})
	if err != nil {
		return operations.Operation{}, xerrors.Errorf("cluster upgrade task %q for operation %q not created: %w", taskType, opType, err)
	}

	return op, nil
}
