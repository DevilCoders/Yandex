package api

import (
	"context"
	"encoding/json"
	"errors"
	"io"
	"net/http"
	"strconv"

	schemaregistry "a.yandex-team.ru/cloud/dataplatform/api/schemaregistry"
	"a.yandex-team.ru/cloud/dataplatform/schemaregistry/internal/server/domain"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type schemaAPI struct {
	api *API
}

func (a *schemaAPI) Create(ctx context.Context, in *schemaregistry.CreateSchemaRequest) (*schemaregistry.CreateSchemaResponse, error) {
	if len(in.Data) == 0 {
		return nil, xerrors.Errorf("empty schema data, schema is required")
	}
	if err := a.api.auth.CheckNamespace(ctx, in.NamespaceId, SchemaCreatePermission); err != nil {
		return nil, xerrors.Errorf("unable to authorize:%s on %s:%w", SchemaCreatePermission, in.NamespaceId, err)
	}
	metadata := &domain.Metadata{Format: in.GetFormat().String(), Compatibility: in.GetCompatibility().String()}
	sc, err := a.api.schema.Create(ctx, in.NamespaceId, in.SchemaId, metadata, in.GetData())
	return &schemaregistry.CreateSchemaResponse{
		Version:  sc.Version,
		Id:       sc.ID,
		Location: sc.Location,
	}, err
}
func (a *schemaAPI) HTTPUpload(w http.ResponseWriter, req *http.Request, pathParams map[string]string) error {
	data, err := io.ReadAll(req.Body)
	if err != nil {
		return err
	}
	format := req.Header.Get("X-Format")
	compatibility := req.Header.Get("X-Compatibility")
	metadata := &domain.Metadata{Format: format, Compatibility: compatibility}
	namespaceID := pathParams["namespace"]
	schemaName := pathParams["name"]
	sc, err := a.api.schema.Create(req.Context(), namespaceID, schemaName, metadata, data)
	if err != nil {
		return err
	}
	respData, _ := json.Marshal(sc)
	w.Header().Set("Content-Type", "application/json")
	w.WriteHeader(http.StatusCreated)
	_, _ = w.Write(respData)
	return nil
}

func (a *schemaAPI) CheckCompatibility(ctx context.Context, req *schemaregistry.CheckCompatibilityRequest) (*schemaregistry.CheckCompatibilityResponse, error) {
	if err := a.api.auth.CheckNamespace(ctx, req.NamespaceId, SchemaGetPermission); err != nil {
		return nil, xerrors.Errorf("unable to authorize:%s on %s:%w", SchemaGetPermission, req.NamespaceId, err)
	}
	resp := &schemaregistry.CheckCompatibilityResponse{}
	err := a.api.schema.CheckCompatibility(ctx, req.GetNamespaceId(), req.GetSchemaId(), req.GetCompatibility().String(), req.GetData())
	return resp, err
}

func (a *schemaAPI) HTTPCheckCompatibility(w http.ResponseWriter, req *http.Request, pathParams map[string]string) error {
	data, err := io.ReadAll(req.Body)
	if err != nil {
		return err
	}
	compatibility := req.Header.Get("X-Compatibility")
	namespaceID := pathParams["namespace"]
	schemaName := pathParams["name"]
	return a.api.schema.CheckCompatibility(req.Context(), namespaceID, schemaName, compatibility, data)
}

func (a *schemaAPI) Diff(ctx context.Context, req *schemaregistry.DiffRequest) (*schemaregistry.DiffResponse, error) {
	if err := a.api.auth.CheckNamespace(ctx, req.NamespaceId, SchemaGetPermission); err != nil {
		return nil, xerrors.Errorf("unable to authorize:%s on %s:%w", SchemaGetPermission, req.NamespaceId, err)
	}
	resp := &schemaregistry.DiffResponse{}
	diffs, err := a.api.schema.Diff(ctx, req.GetNamespaceId(), req.GetSchemaId(), req.GetData())
	if err != nil {
		return nil, xerrors.Errorf("unable to get diff: %w", err)
	}
	resp.Diffs = diffs
	return resp, err
}

func (a *schemaAPI) List(ctx context.Context, in *schemaregistry.ListSchemasRequest) (*schemaregistry.ListSchemasResponse, error) {
	if err := a.api.auth.CheckNamespace(ctx, in.Id, SchemaGetPermission); err != nil {
		return nil, xerrors.Errorf("unable to authorize:%s on %s:%w", SchemaGetPermission, in.Id, err)
	}
	schemas, err := a.api.schema.List(ctx, in.Id)
	return &schemaregistry.ListSchemasResponse{Schemas: schemas}, err
}

func (a *schemaAPI) GetLatest(ctx context.Context, in *schemaregistry.GetLatestSchemaRequest) (*schemaregistry.GetLatestSchemaResponse, error) {
	if err := a.api.auth.CheckNamespace(ctx, in.NamespaceId, SchemaGetPermission); err != nil {
		return nil, xerrors.Errorf("unable to authorize:%s on %s:%w", SchemaGetPermission, in.NamespaceId, err)
	}
	_, data, err := a.api.schema.GetLatest(ctx, in.NamespaceId, in.SchemaId)
	return &schemaregistry.GetLatestSchemaResponse{
		Data: data,
	}, err
}

func (a *schemaAPI) HTTPLatestSchema(w http.ResponseWriter, req *http.Request, pathParams map[string]string) (*domain.Metadata, []byte, error) {
	namespaceID := pathParams["namespace"]
	schemaName := pathParams["name"]
	return a.api.schema.GetLatest(req.Context(), namespaceID, schemaName)
}

func (a *schemaAPI) Get(ctx context.Context, in *schemaregistry.GetSchemaRequest) (*schemaregistry.GetSchemaResponse, error) {
	if err := a.api.auth.CheckNamespace(ctx, in.NamespaceId, SchemaGetPermission); err != nil {
		return nil, xerrors.Errorf("unable to authorize:%s on %s:%w", SchemaGetPermission, in.NamespaceId, err)
	}
	_, data, err := a.api.schema.Get(ctx, in.NamespaceId, in.SchemaId, in.GetVersionId())
	return &schemaregistry.GetSchemaResponse{
		Data: data,
	}, err
}

func (a *schemaAPI) HTTPGetSchema(w http.ResponseWriter, req *http.Request, pathParams map[string]string) (*domain.Metadata, []byte, error) {
	namespaceID := pathParams["namespace"]
	schemaName := pathParams["name"]
	versionString := pathParams["version"]
	v, err := strconv.ParseInt(versionString, 10, 32)
	if err != nil {
		return nil, nil, errors.New("invalid version number")
	}
	return a.api.schema.Get(req.Context(), namespaceID, schemaName, int32(v))
}

func (a *schemaAPI) GetMetadata(ctx context.Context, in *schemaregistry.GetSchemaMetadataRequest) (*schemaregistry.GetSchemaMetadataResponse, error) {
	if err := a.api.auth.CheckNamespace(ctx, in.NamespaceId, SchemaGetPermission); err != nil {
		return nil, xerrors.Errorf("unable to authorize:%s on %s:%w", SchemaGetPermission, in.NamespaceId, err)
	}
	meta, err := a.api.schema.GetMetadata(ctx, in.NamespaceId, in.SchemaId)
	return &schemaregistry.GetSchemaMetadataResponse{
		Format:        schemaregistry.Schema_Format(schemaregistry.Schema_Format_value[meta.Format]),
		Compatibility: schemaregistry.Schema_Compatibility(schemaregistry.Schema_Compatibility_value[meta.Compatibility]),
		Authority:     meta.Authority,
	}, err
}

func (a *schemaAPI) UpdateMetadata(ctx context.Context, in *schemaregistry.UpdateSchemaMetadataRequest) (*schemaregistry.UpdateSchemaMetadataResponse, error) {
	if err := a.api.auth.CheckNamespace(ctx, in.NamespaceId, SchemaUpdatePermission); err != nil {
		return nil, xerrors.Errorf("unable to authorize:%s on %s:%w", SchemaUpdatePermission, in.NamespaceId, err)
	}
	meta, err := a.api.schema.UpdateMetadata(ctx, in.NamespaceId, in.SchemaId, &domain.Metadata{
		Compatibility: in.Compatibility.String(),
	})
	return &schemaregistry.UpdateSchemaMetadataResponse{
		Format:        schemaregistry.Schema_Format(schemaregistry.Schema_Format_value[meta.Format]),
		Compatibility: schemaregistry.Schema_Compatibility(schemaregistry.Schema_Compatibility_value[meta.Compatibility]),
		Authority:     meta.Authority,
	}, err
}

func (a *schemaAPI) Delete(ctx context.Context, in *schemaregistry.DeleteSchemaRequest) (*schemaregistry.DeleteSchemaResponse, error) {
	if err := a.api.auth.CheckNamespace(ctx, in.NamespaceId, SchemaDeletePermission); err != nil {
		return nil, xerrors.Errorf("unable to authorize:%s on %s:%w", SchemaDeletePermission, in.NamespaceId, err)
	}
	err := a.api.schema.Delete(ctx, in.NamespaceId, in.SchemaId)
	message := "success"
	if err != nil {
		message = "failed"
	}
	return &schemaregistry.DeleteSchemaResponse{
		Message: message,
	}, err
}

func NewSchemaAPI(api *API) *schemaAPI {
	return &schemaAPI{api: api}
}
