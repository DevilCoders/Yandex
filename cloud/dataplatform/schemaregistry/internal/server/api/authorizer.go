package api

import (
	"context"

	"a.yandex-team.ru/cloud/dataplatform/schemaregistry/internal/server/domain"
	"a.yandex-team.ru/cloud/iam/accessservice/client/go/cloudauth"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/transfer_manager/go/pkg/auth"
)

const (
	NamespaceGetPermission    = auth.Permission("schemaregistry.namespace.get")
	NamespaceUpdatePermission = auth.Permission("schemaregistry.namespace.update")
	NamespaceCreatePermission = auth.Permission("schemaregistry.namespace.create")
	NamespaceDeletePermission = auth.Permission("schemaregistry.namespace.delete")

	SchemaGetPermission    = auth.Permission("schemaregistry.schema.get")
	SchemaCreatePermission = auth.Permission("schemaregistry.schema.create")
	SchemaUpdatePermission = auth.Permission("schemaregistry.schema.update")
	SchemaDeletePermission = auth.Permission("schemaregistry.schema.delete")
)

type authorizer struct {
	auth      auth.Provider
	namespace domain.NamespaceService
}

func ResourceFolder(id string) cloudauth.Resource {
	if id == "" {
		id = "mdb-junk"
	}
	return cloudauth.ResourceFolder(id)
}

func NewAuthorizer(auth auth.Provider, namespace domain.NamespaceService) *authorizer {
	return &authorizer{
		auth:      auth,
		namespace: namespace,
	}
}

func (a *authorizer) CheckNamespace(ctx context.Context, namespaceID string, permission auth.Permission) error {
	ns, err := a.namespace.Get(ctx, namespaceID)
	if err != nil {
		return xerrors.Errorf("unable to get namespace:%s:%w", namespaceID, err)
	}
	return a.auth.Authorize(ctx, permission, ResourceFolder(ns.FolderID)).Err()
}

func (a *authorizer) CheckFolder(ctx context.Context, folderID string, permission auth.Permission) error {
	return a.auth.Authorize(ctx, permission, ResourceFolder(folderID)).Err()
}
