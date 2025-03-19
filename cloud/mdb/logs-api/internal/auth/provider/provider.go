package provider

import (
	"context"

	authcontext "a.yandex-team.ru/cloud/mdb/internal/auth"
	as "a.yandex-team.ru/cloud/mdb/internal/compute/accessservice"
	"a.yandex-team.ru/cloud/mdb/internal/datatransfer"
	"a.yandex-team.ru/cloud/mdb/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/cloud/mdb/logs-api/internal/auth"
	"a.yandex-team.ru/cloud/mdb/logs-api/internal/models"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
)

const (
	permissionReadLogs = "mdb.all.read"
)

type AuthProvider struct {
	metaDB              metadb.MetaDB
	accessService       as.AccessService
	datatransferService datatransfer.DataTransferService
	l                   log.Logger
}

var _ auth.Authenticator = &AuthProvider{}

func NewAuthProvider(metaDB metadb.MetaDB, accessService as.AccessService, datatransfer datatransfer.DataTransferService, l log.Logger) *AuthProvider {
	return &AuthProvider{
		metaDB:              metaDB,
		accessService:       accessService,
		datatransferService: datatransfer,
		l:                   l,
	}
}

func (a *AuthProvider) Authorize(ctx context.Context, resources []models.LogSource) error {
	var clusterIDs []models.LogSource
	var transferIDs []string

	for _, res := range resources {
		switch res.Type {
		case models.LogSourceTypeClickhouse, models.LogSourceTypeKafka:
			clusterIDs = append(clusterIDs, res)
		case models.LogSourceTypeTransfer:
			transferIDs = append(transferIDs, res.ID)
		}
	}

	clustersFolders, err := a.resolveClustersFolders(ctx, clusterIDs)
	if err != nil {
		return err
	}
	transfersFolders, err := a.resolveTransfersFolders(ctx, transferIDs)
	if err != nil {
		return err
	}

	return a.authorize(ctx, append(clustersFolders, transfersFolders...))
}

func (a *AuthProvider) resolveClustersFolders(ctx context.Context, clusterIDs []models.LogSource) ([]as.Resource, error) {
	if len(clusterIDs) == 0 {
		return nil, nil
	}
	ctxTx, err := a.metaDB.Begin(ctx, sqlutil.Primary)
	if err != nil {
		return nil, err
	}
	defer func() {
		_ = a.metaDB.Rollback(ctx)
	}()

	folders := map[string]struct{}{}
	for _, c := range clusterIDs {
		folderExtID, err := a.metaDB.FolderExtIDByClusterID(ctxTx, c.ID, c.Type.ClusterType())
		if err != nil {
			return nil, err
		}
		folders[folderExtID] = struct{}{}
	}

	result := make([]as.Resource, 0, len(folders))
	for f := range folders {
		result = append(result, as.ResourceFolder(f))
	}

	return result, nil
}

func (a *AuthProvider) resolveTransfersFolders(ctx context.Context, transfersIDs []string) ([]as.Resource, error) {
	if len(transfersIDs) == 0 {
		return nil, nil
	}
	folders, err := a.datatransferService.ResolveFolderID(ctx, transfersIDs)
	if err != nil {
		return nil, err
	}
	var res []as.Resource
	for _, fodlerID := range folders {
		res = append(res, as.Resource{
			Type: as.ResourceTypeFolder,
			ID:   fodlerID,
		})
	}
	return res, nil
}

func (a *AuthProvider) authorize(ctx context.Context, resources []as.Resource) error {
	token, ok := authcontext.TokenFromContext(ctx)
	if !ok {
		return semerr.Authentication("missing auth token")
	}

	_, err := a.accessService.Auth(ctx, token, permissionReadLogs, resources...)
	a.l.Warn("authorize resources: %")
	if err != nil {
		ctxlog.Warn(ctx, a.l, "auth request failed", log.Error(err))
		err = semerr.WhitelistErrors(err, semerr.SemanticUnavailable, semerr.SemanticAuthentication, semerr.SemanticAuthorization)
		return err
	}

	return nil
}
