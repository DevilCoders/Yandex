package namespace

import (
	"context"

	"a.yandex-team.ru/cloud/dataplatform/schemaregistry/internal/server/domain"
)

type Service struct {
	Repo domain.NamespaceRepository
}

func (s Service) Create(ctx context.Context, ns domain.Namespace) (domain.Namespace, error) {
	return s.Repo.CreateNamespace(ctx, ns)
}

func (s Service) Update(ctx context.Context, ns domain.Namespace) (domain.Namespace, error) {
	return s.Repo.UpdateNamespace(ctx, ns)
}

func (s Service) List(ctx context.Context, folderID string) ([]string, error) {
	return s.Repo.ListNamespaces(ctx, folderID)
}

func (s Service) Get(ctx context.Context, name string) (domain.Namespace, error) {
	return s.Repo.GetNamespace(ctx, name)
}

func (s Service) Delete(ctx context.Context, name string) error {
	return s.Repo.DeleteNamespace(ctx, name)
}
