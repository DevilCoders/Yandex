package provider

import (
	"context"

	internalmetadb "a.yandex-team.ru/cloud/mdb/cms/api/internal/metadb"
	internalmetadbmodels "a.yandex-team.ru/cloud/mdb/cms/api/internal/metadb/models"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/tasksclient/models"
	as "a.yandex-team.ru/cloud/mdb/internal/compute/accessservice"
	"a.yandex-team.ru/cloud/mdb/internal/generator"
	"a.yandex-team.ru/cloud/mdb/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/library/go/core/xerrors"
)

const (
	CmsAuthServiceAccountID = "yc.mdb.cms"
)

type TasksClient struct {
	internalMeta internalmetadb.Backend
	meta         metadb.MetaDB

	generator generator.IDGenerator
}

func NewTasksClient(internalMeta internalmetadb.Backend, meta metadb.MetaDB) *TasksClient {
	return &TasksClient{
		internalMeta: internalMeta,
		generator:    generator.NewTaskIDGenerator("mdb"), // TODO
		meta:         meta,
	}
}

func (c *TasksClient) TaskStatus(ctx context.Context, operationID string) (models.TaskStatus, error) {
	txCtx, err := c.internalMeta.Begin(ctx, sqlutil.Primary)
	if err != nil {
		return "", xerrors.Errorf("begin tx: %w", err)
	}
	defer func() { _ = c.internalMeta.Rollback(txCtx) }()

	res, err := c.internalMeta.Task(txCtx, operationID)
	if err != nil {
		return "", err
	}

	err = c.internalMeta.Commit(txCtx)
	if err != nil {
		return "", xerrors.Errorf("commit tx: %w", err)
	}

	return models.TaskStatus(res.Status), nil
}

func (c *TasksClient) createDefaultCreateTaskArgs(clusterID string, folderID int64, revision int64) (internalmetadbmodels.CreateTaskArgs, error) {
	taskID, err := c.generator.Generate()
	if err != nil {
		return internalmetadbmodels.CreateTaskArgs{}, xerrors.Errorf("generate task id: %w", err)
	}

	return internalmetadbmodels.CreateTaskArgs{
		TaskID:          taskID,
		ClusterID:       clusterID,
		FolderID:        folderID,
		Auth:            as.Subject{Service: &as.ServiceAccount{ID: CmsAuthServiceAccountID}},
		Hidden:          true,
		Idempotence:     nil,
		SkipIdempotence: true,
		Revision:        revision,
	}, nil
}
