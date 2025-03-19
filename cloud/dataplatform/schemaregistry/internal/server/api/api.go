package api

import (
	"fmt"
	"net/http"
	"strconv"

	"github.com/grpc-ecosystem/grpc-gateway/protoc-gen-grpc-gateway/httprule"
	"github.com/grpc-ecosystem/grpc-gateway/runtime"
	"google.golang.org/grpc/health/grpc_health_v1"

	"a.yandex-team.ru/cloud/dataplatform/internal/logger"
	"a.yandex-team.ru/cloud/dataplatform/schemaregistry/internal/server/domain"
	"a.yandex-team.ru/transfer_manager/go/pkg/auth"
)

type getSchemaData func(http.ResponseWriter, *http.Request, map[string]string) (*domain.Metadata, []byte, error)
type errHandleFunc func(http.ResponseWriter, *http.Request, map[string]string) error

//API root handlers and dependencies
type API struct {
	grpc_health_v1.UnimplementedHealthServer
	namespace domain.NamespaceService
	schema    domain.SchemaService
	search    domain.SearchService
	auth      *authorizer
}

func NewAPI(namespace domain.NamespaceService, schema domain.SchemaService, search domain.SearchService, auth auth.Provider) *API {
	return &API{
		namespace: namespace,
		schema:    schema,
		search:    search,
		auth:      NewAuthorizer(auth, namespace),
	}
}

// RegisterSchemaHandlers registers HTTP handlers for schema download
func (a *API) RegisterSchemaHandlers(mux *runtime.ServeMux) {
	checkErr(HandlePath(mux, "GET", "/ping", func(w http.ResponseWriter, r *http.Request, pathParams map[string]string) {
		_, _ = fmt.Fprint(w, "pong")
	}))
	checkErr(HandlePath(mux, "GET", "/v1/namespaces/{namespace}/schemas/{name}/versions/{version}", handleSchemaResponse(mux, NewSchemaAPI(a).HTTPGetSchema)))
	checkErr(HandlePath(mux, "GET", "/v1/namespaces/{namespace}/schemas/{name}", handleSchemaResponse(mux, NewSchemaAPI(a).HTTPLatestSchema)))
	checkErr(HandlePath(mux, "POST", "/v1/namespaces/{namespace}/schemas/{name}", wrapErrHandler(mux, NewSchemaAPI(a).HTTPUpload)))
	checkErr(HandlePath(mux, "POST", "/v1/namespaces/{namespace}/schemas/{name}/check", wrapErrHandler(mux, NewSchemaAPI(a).HTTPCheckCompatibility)))
}

func checkErr(err error) {
	if err != nil {
		logger.Log.Fatalf("err: %v", err)
	}
}

func HandlePath(s *runtime.ServeMux, meth string, pathPattern string, h runtime.HandlerFunc) error {
	compiler, err := httprule.Parse(pathPattern)
	if err != nil {
		return fmt.Errorf("parsing path pattern: %w", err)
	}
	tp := compiler.Compile()
	pattern, err := runtime.NewPattern(tp.Version, tp.OpCodes, tp.Pool, tp.Verb)
	if err != nil {
		return fmt.Errorf("creating new pattern: %w", err)
	}
	s.Handle(meth, pattern, h)
	return nil
}

func handleSchemaResponse(mux *runtime.ServeMux, getSchemaFn getSchemaData) runtime.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request, pathParams map[string]string) {
		meta, data, err := getSchemaFn(w, r, pathParams)
		if err != nil {
			_, outbound := runtime.MarshalerForRequest(mux, r)
			runtime.HTTPError(r.Context(), mux, outbound, w, r, err)
			return
		}
		contentType := "application/json"
		if meta.Format == "FORMAT_PROTOBUF" {
			contentType = "application/octet-stream"
		}
		w.Header().Set("Content-Type", contentType)
		w.Header().Set("Content-Length", strconv.Itoa(len(data)))
		w.WriteHeader(http.StatusOK)
		_, _ = w.Write(data)
	}
}

func wrapErrHandler(mux *runtime.ServeMux, handler errHandleFunc) runtime.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request, pathParams map[string]string) {
		err := handler(w, r, pathParams)
		if err != nil {
			_, outbound := runtime.MarshalerForRequest(mux, r)
			runtime.GlobalHTTPErrorHandler(r.Context(), mux, outbound, w, r, err)
			return
		}
	}
}
