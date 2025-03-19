package provider

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/common"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/sessions"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/operations"
)

type Operations struct {
	sessions sessions.Sessions
	metaDB   metadb.Backend
}

var _ common.Operations = &Operations{}

func NewOperations(sessions sessions.Sessions, metaDB metadb.Backend) *Operations {
	return &Operations{sessions: sessions, metaDB: metaDB}
}

func (o *Operations) Operation(ctx context.Context, opID string) (operations.Operation, error) {
	// Use primary - we need guaranteed consistency
	ctx, _, err := o.sessions.Begin(ctx, sessions.ResolveByOperation(opID), sessions.WithPrimary())
	if err != nil {
		return operations.Operation{}, err
	}
	defer o.sessions.Rollback(ctx)

	return o.metaDB.OperationByID(ctx, opID)
}

func (o *Operations) OperationWithFolderCoords(ctx context.Context, opID string) (operations.Operation, metadb.FolderCoords, error) {
	// Use primary - we need guaranteed consistency
	ctx, _, err := o.sessions.Begin(ctx, sessions.ResolveByOperation(opID), sessions.WithPrimary())
	if err != nil {
		return operations.Operation{}, metadb.FolderCoords{}, err
	}
	defer o.sessions.Rollback(ctx)

	op, err := o.metaDB.OperationByID(ctx, opID)
	if err != nil {
		return operations.Operation{}, metadb.FolderCoords{}, err
	}

	folder, err := o.metaDB.FolderCoordsByOperationID(ctx, opID)
	if err != nil {
		return operations.Operation{}, metadb.FolderCoords{}, err
	}

	return op, folder, nil
}

func (o *Operations) OperationsByFolderID(ctx context.Context, folderExtID string, args models.ListOperationsArgs) ([]operations.Operation, error) {
	// Use primary - we need guaranteed consistency
	ctx, sess, err := o.sessions.Begin(ctx, sessions.ResolveByFolder(folderExtID, models.PermMDBAllRead), sessions.WithPrimary())
	if err != nil {
		return nil, err
	}
	defer o.sessions.Rollback(ctx)

	return o.metaDB.OperationsByFolderID(ctx, sess.FolderCoords.FolderID, args)
}

func (o *Operations) OperationsByClusterID(ctx context.Context, cid string, limit, offset int64) ([]operations.Operation, error) {
	// Use primary - we need guaranteed consistency
	ctx, sess, err := o.sessions.Begin(ctx, sessions.ResolveByCluster(cid, models.PermMDBAllRead), sessions.WithPrimary())
	if err != nil {
		return nil, err
	}
	defer o.sessions.Rollback(ctx)

	if limit <= 0 {
		limit = 100
	}
	if offset < 0 {
		offset = 0
	}

	return o.metaDB.OperationsByClusterID(ctx, cid, sess.FolderCoords.FolderID, offset, limit)
}
