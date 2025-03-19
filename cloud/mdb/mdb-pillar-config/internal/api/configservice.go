package api

import (
	"context"
	"encoding/json"
	"net/http"
	"net/url"
	"strconv"
	"time"

	"github.com/go-chi/chi/v5"
	"golang.yandex/hasql"

	"a.yandex-team.ru/cloud/mdb/internal/grpcutil/grpcerr"
	"a.yandex-team.ru/cloud/mdb/internal/httputil"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-pillar-config/internal/auth"
	"a.yandex-team.ru/cloud/mdb/mdb-pillar-config/internal/metadb"
	"a.yandex-team.ru/library/go/core/log"
)

type Config struct {
	ExposeErrorDebug bool `json:"expose_error_debug" yaml:"expose_error_debug"`
}

func DefaultConfig() Config {
	return Config{ExposeErrorDebug: false}
}

type ConfigResponder struct {
	Authenticator auth.Authenticator
	MetaDB        metadb.MetaDB
	L             log.Logger
	Config        Config
}

type GenerateConfigArgs struct {
	FQDN           string
	TargetPillarID string
	Revision       optional.Int64
}

func (cr ConfigResponder) RegisterConfigHandlers(router *chi.Mux) error {
	router.Get("/v1/config/{fqdn}", func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Content-Type", "application/json")
		args, err := argsFromRequest(r)
		if err != nil {
			cr.returnError(w, err)
			return
		}

		result, err := cr.generateManagedConfig(r.Context(), args)
		if err != nil {
			cr.returnError(w, err)
			return
		}

		w.WriteHeader(http.StatusOK)
		_, _ = w.Write(result)
	})

	router.Get("/v1/config_unmanaged/{fqdn}", func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Content-Type", "application/json")
		args, err := argsFromRequest(r)
		if err != nil {
			cr.returnError(w, err)
			return
		}

		result, err := cr.generateUnmanagedConfig(r.Context(), args)
		if err != nil {
			cr.returnError(w, err)
			return
		}

		w.WriteHeader(http.StatusOK)
		_, _ = w.Write(result)
	})

	router.Get("/v1/ping", func(w http.ResponseWriter, r *http.Request) {
		ctx, cancel := context.WithTimeout(r.Context(), time.Second)
		defer cancel()

		if err := cr.MetaDB.IsReady(ctx); err != nil {
			cr.L.Warn("ping error", log.Error(err))
			w.WriteHeader(http.StatusServiceUnavailable)
			return
		}

		w.WriteHeader(http.StatusOK)
	})

	return nil
}

func (cr ConfigResponder) generateManagedConfig(ctx context.Context, args GenerateConfigArgs) (json.RawMessage, error) {
	// TODO choose nearest replica without delay MDB-12866
	ctx, err := cr.MetaDB.Begin(ctx, hasql.Alive)
	if err != nil {
		return nil, err
	}
	defer func() { _ = cr.MetaDB.Rollback(ctx) }()

	if err := cr.Authenticator.Authenticate(ctx, []string{"default"}); err != nil {
		return nil, err
	}

	return cr.MetaDB.GenerateManagedConfig(ctx, args.FQDN, args.TargetPillarID, args.Revision)
}

func (cr ConfigResponder) generateUnmanagedConfig(ctx context.Context, args GenerateConfigArgs) (json.RawMessage, error) {
	// TODO choose nearest replica without delay MDB-12866
	ctx, err := cr.MetaDB.Begin(ctx, hasql.Alive)
	if err != nil {
		return nil, err
	}
	defer func() { _ = cr.MetaDB.Rollback(ctx) }()

	if err := cr.Authenticator.Authenticate(ctx, []string{"dbaas-worker"}); err != nil {
		return nil, err
	}

	return cr.MetaDB.GenerateUnmanagedConfig(ctx, args.FQDN, args.TargetPillarID, args.Revision)
}

func argsFromRequest(r *http.Request) (GenerateConfigArgs, error) {
	query, err := url.ParseQuery(r.URL.RawQuery)
	if err != nil {
		return GenerateConfigArgs{}, err
	}

	args := GenerateConfigArgs{
		FQDN:           chi.URLParam(r, "fqdn"),
		TargetPillarID: query.Get("target-pillar-id"),
	}

	rev := query.Get("rev")
	if rev != "" {
		intRev, err := strconv.ParseInt(rev, 10, 64)
		if err != nil {
			return GenerateConfigArgs{}, semerr.InvalidInputf("invalid query parameter: %v", err)
		}

		args.Revision = optional.NewInt64(intRev)
	}

	return args, nil
}

func (cr ConfigResponder) returnError(w http.ResponseWriter, err error) {
	code, ok := httputil.CodeFromSemanticError(err)
	if !ok {
		code = http.StatusInternalServerError
	}

	w.WriteHeader(code)
	grpcError := grpcerr.ErrorToGRPC(err, cr.Config.ExposeErrorDebug, cr.L)
	data, err := json.Marshal(grpcError.Proto())
	if err != nil {
		panic(err)
	}

	_, _ = w.Write(data)
}
