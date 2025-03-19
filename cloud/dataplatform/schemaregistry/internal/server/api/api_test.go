package api_test

import (
	"github.com/grpc-ecosystem/grpc-gateway/runtime"

	"a.yandex-team.ru/cloud/dataplatform/schemaregistry/internal/mocks"
	"a.yandex-team.ru/cloud/dataplatform/schemaregistry/internal/server/api"
	"a.yandex-team.ru/transfer_manager/go/pkg/auth"
)

func setup() (*mocks.NamespaceService, *mocks.SchemaService, *mocks.SearchService, *runtime.ServeMux, *api.API) {
	nsService := &mocks.NamespaceService{}
	schemaService := &mocks.SchemaService{}
	searchService := &mocks.SearchService{}
	mux := runtime.NewServeMux()
	v1 := api.NewAPI(nsService, schemaService, searchService, auth.AnonymousFakeProvider())
	v1.RegisterSchemaHandlers(mux)
	return nsService, schemaService, searchService, mux, v1
}
