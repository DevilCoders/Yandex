package api

import (
	"context"
	"fmt"

	schemaregistry "a.yandex-team.ru/cloud/dataplatform/api/schemaregistry"
	"a.yandex-team.ru/cloud/dataplatform/schemaregistry/internal/server/domain"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (a *API) Search(ctx context.Context, in *schemaregistry.SearchRequest) (*schemaregistry.SearchResponse, error) {
	if err := a.auth.CheckNamespace(ctx, in.NamespaceId, SchemaGetPermission); err != nil {
		return nil, xerrors.Errorf("unable to authorize:%s on %s:%w", SchemaGetPermission, in.NamespaceId, err)
	}
	searchReq := &domain.SearchRequest{
		NamespaceID: in.GetNamespaceId(),
		Query:       in.GetQuery(),
		SchemaID:    in.GetSchemaId(),
	}

	switch v := in.GetVersion().(type) {
	case *schemaregistry.SearchRequest_VersionId:
		searchReq.VersionID = v.VersionId
	case *schemaregistry.SearchRequest_History:
		searchReq.History = v.History
	}

	res, err := a.search.Search(ctx, searchReq)
	if err != nil {
		return nil, err
	}

	hits := make([]*schemaregistry.SearchHits, 0)
	for _, hit := range res.Hits {
		hits = append(hits, &schemaregistry.SearchHits{
			SchemaId:    hit.SchemaID,
			VersionId:   hit.VersionID,
			Fields:      hit.Fields,
			Types:       hit.Types,
			NamespaceId: hit.NamespaceID,
			Path:        fmt.Sprintf("/v1/namespaces/%s/schemas/%s/versions/%d", hit.NamespaceID, hit.SchemaID, hit.VersionID),
		})
	}
	return &schemaregistry.SearchResponse{
		Hits: hits,
		Meta: &schemaregistry.SearchMeta{
			Total: uint32(len(hits)),
		},
	}, nil
}
