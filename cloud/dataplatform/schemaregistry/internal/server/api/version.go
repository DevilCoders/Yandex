package api

import (
	"context"

	schemaregistry "a.yandex-team.ru/cloud/dataplatform/api/schemaregistry"
)

type versionAPI struct {
	api *API
}

func (a *versionAPI) List(ctx context.Context, in *schemaregistry.ListVersionsRequest) (*schemaregistry.ListVersionsResponse, error) {
	versions, err := a.api.schema.ListVersions(ctx, in.NamespaceId, in.SchemaId)
	return &schemaregistry.ListVersionsResponse{Versions: versions}, err
}

func (a *versionAPI) Delete(ctx context.Context, in *schemaregistry.DeleteVersionRequest) (*schemaregistry.DeleteVersionResponse, error) {
	err := a.api.schema.DeleteVersion(ctx, in.NamespaceId, in.SchemaId, in.GetVersionId())
	message := "success"
	if err != nil {
		message = "failed"
	}
	return &schemaregistry.DeleteVersionResponse{
		Message: message,
	}, err
}

func NewVersionAPI(api *API) *versionAPI {
	return &versionAPI{api: api}
}
