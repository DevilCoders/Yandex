package provider

import (
	"context"

	authcontext "a.yandex-team.ru/cloud/mdb/internal/auth"
	as "a.yandex-team.ru/cloud/mdb/internal/compute/accessservice"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/cloud/mdb/mdb-pillar-secrets/internal/auth"
	"a.yandex-team.ru/cloud/mdb/mdb-pillar-secrets/internal/metadb"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
)

const (
	permissionReadSecrets = "mdb.clusters.getSecret"
)

type AuthProvider struct {
	metaDB        metadb.MetaDB
	accessService as.AccessService
	l             log.Logger
}

func NewAuthProvider(metaDB metadb.MetaDB, accessService as.AccessService, l log.Logger) auth.Authenticator {
	return &AuthProvider{
		metaDB:        metaDB,
		accessService: accessService,
		l:             l,
	}
}

func (a AuthProvider) ReadOnCluster(ctx context.Context, clusterID string, do auth.ReadOnClusterFunc) error {
	if clusterID == "" {
		return semerr.InvalidInput("cluster id must be specified")
	}

	ctx, resources, err := a.prepareAuthByClusterID(ctx, clusterID)
	if err != nil {
		return err
	}
	defer a.rollback(ctx)

	token, ok := authcontext.TokenFromContext(ctx)
	if !ok {
		return semerr.Authentication("missing auth token")
	}

	_, err = a.accessService.Auth(ctx, token, permissionReadSecrets, resources...)
	if err != nil {
		ctxlog.Warn(ctx, a.l, "auth request failed", log.Error(err))
		err = semerr.WhitelistErrors(err, semerr.SemanticUnavailable, semerr.SemanticAuthentication, semerr.SemanticAuthorization)
		return err
	}

	return do(ctx, a.metaDB)
}

// TODO choose not stale replica after MDB-12866
func (a *AuthProvider) prepareAuthByClusterID(ctx context.Context, clusterID string) (context.Context, []as.Resource, error) {
	ctxTx, errDefault := a.metaDB.Begin(ctx, sqlutil.Primary)
	if errDefault != nil {
		return nil, nil, errDefault
	}

	folderExtID, _, err := a.metaDB.FolderCoordsByClusterID(ctxTx, clusterID)
	if err != nil {
		a.rollback(ctxTx)
	}

	return ctxTx, []as.Resource{as.ResourceFolder(folderExtID)}, nil
}

func (a *AuthProvider) rollback(ctx context.Context) {
	_ = a.metaDB.Rollback(ctx)
}
