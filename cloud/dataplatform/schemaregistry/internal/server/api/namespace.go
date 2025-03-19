package api

import (
	"context"

	"google.golang.org/protobuf/types/known/timestamppb"

	schemaregistry "a.yandex-team.ru/cloud/dataplatform/api/schemaregistry"
	"a.yandex-team.ru/cloud/dataplatform/schemaregistry/internal/server/domain"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/transfer_manager/go/pkg/auth"
)

type namespaceAPI struct {
	api *API
}

func createNamespaceRequestToNamespace(ctx context.Context, r *schemaregistry.CreateNamespaceRequest) domain.Namespace {
	return domain.Namespace{
		ID:            r.GetId(),
		Format:        r.GetFormat().String(),
		Compatibility: r.GetCompatibility().String(),
		Description:   r.GetDescription(),
		FolderID:      r.GetFolderId(),
		Author:        auth.UserFromContext(ctx),
	}
}

func namespaceToProto(ns domain.Namespace) *schemaregistry.Namespace {
	return &schemaregistry.Namespace{
		Id:            ns.ID,
		Format:        schemaregistry.Schema_Format(schemaregistry.Schema_Format_value[ns.Format]),
		Compatibility: schemaregistry.Schema_Compatibility(schemaregistry.Schema_Compatibility_value[ns.Compatibility]),
		Description:   ns.Description,
		FolderId:      ns.FolderID,
		Author:        ns.Author,
		CreatedAt:     timestamppb.New(ns.CreatedAt),
		UpdatedAt:     timestamppb.New(ns.UpdatedAt),
	}
}

// CreateNamespace handler for creating namespace
func (a *namespaceAPI) Create(ctx context.Context, in *schemaregistry.CreateNamespaceRequest) (*schemaregistry.CreateNamespaceResponse, error) {
	if err := a.api.auth.CheckFolder(ctx, in.FolderId, NamespaceCreatePermission); err != nil {
		return nil, xerrors.Errorf("unable to authorize:%s on %s:%w", NamespaceCreatePermission, in.FolderId, err)
	}
	ns := createNamespaceRequestToNamespace(ctx, in)
	newNamespace, err := a.api.namespace.Create(ctx, ns)
	return &schemaregistry.CreateNamespaceResponse{Namespace: namespaceToProto(newNamespace)}, err
}

func (a *namespaceAPI) Update(ctx context.Context, in *schemaregistry.UpdateNamespaceRequest) (*schemaregistry.UpdateNamespaceResponse, error) {
	if err := a.api.auth.CheckNamespace(ctx, in.Id, NamespaceUpdatePermission); err != nil {
		return nil, xerrors.Errorf("unable to authorize:%s on %s:%w", NamespaceUpdatePermission, in.Id, err)
	}
	ns, err := a.api.namespace.Update(ctx, domain.Namespace{ID: in.GetId(), Format: in.GetFormat().String(), Description: in.GetDescription()})
	return &schemaregistry.UpdateNamespaceResponse{Namespace: namespaceToProto(ns)}, err
}

func (a *namespaceAPI) Get(ctx context.Context, in *schemaregistry.GetNamespaceRequest) (*schemaregistry.GetNamespaceResponse, error) {
	if err := a.api.auth.CheckNamespace(ctx, in.Id, NamespaceGetPermission); err != nil {
		return nil, xerrors.Errorf("unable to authorize:%s on %s:%w", NamespaceGetPermission, in.Id, err)
	}
	namespace, err := a.api.namespace.Get(ctx, in.GetId())
	return &schemaregistry.GetNamespaceResponse{Namespace: namespaceToProto(namespace)}, err
}

// ListNamespaces handler for returning list of available namespaces
func (a *namespaceAPI) List(ctx context.Context, in *schemaregistry.ListNamespacesRequest) (*schemaregistry.ListNamespacesResponse, error) {
	if err := a.api.auth.CheckFolder(ctx, in.FolderId, NamespaceGetPermission); err != nil {
		return nil, xerrors.Errorf("unable to authorize:%s on %s:%w", NamespaceGetPermission, in.FolderId, err)
	}
	namespaces, err := a.api.namespace.List(ctx, in.FolderId)
	return &schemaregistry.ListNamespacesResponse{Namespaces: namespaces}, err
}

func (a *namespaceAPI) Delete(ctx context.Context, in *schemaregistry.DeleteNamespaceRequest) (*schemaregistry.DeleteNamespaceResponse, error) {
	if err := a.api.auth.CheckNamespace(ctx, in.Id, NamespaceDeletePermission); err != nil {
		return nil, xerrors.Errorf("unable to authorize:%s on %s:%w", NamespaceDeletePermission, in.Id, err)
	}
	err := a.api.namespace.Delete(ctx, in.GetId())
	message := "success"
	if err != nil {
		message = "failed"
	}
	return &schemaregistry.DeleteNamespaceResponse{Message: message}, err
}

func NewNamespaceAPI(api *API) *namespaceAPI {
	return &namespaceAPI{
		api: api,
	}
}
