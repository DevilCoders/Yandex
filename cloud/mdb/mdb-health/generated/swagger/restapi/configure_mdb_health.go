// This file is safe to edit. Once it exists it will not be overwritten

package restapi

import (
	"crypto/tls"
	"net/http"

	"github.com/go-openapi/runtime"
	"github.com/go-openapi/runtime/middleware"

	"a.yandex-team.ru/cloud/mdb/mdb-health/generated/swagger/restapi/operations"
	"a.yandex-team.ru/cloud/mdb/mdb-health/generated/swagger/restapi/operations/clusterhealth"
	"a.yandex-team.ru/cloud/mdb/mdb-health/generated/swagger/restapi/operations/health"
	"a.yandex-team.ru/cloud/mdb/mdb-health/generated/swagger/restapi/operations/hostneighbours"
	"a.yandex-team.ru/cloud/mdb/mdb-health/generated/swagger/restapi/operations/hostshealth"
	"a.yandex-team.ru/cloud/mdb/mdb-health/generated/swagger/restapi/operations/listhostshealth"
	"a.yandex-team.ru/cloud/mdb/mdb-health/generated/swagger/restapi/operations/unhealthyaggregatedinfo"
	"a.yandex-team.ru/cloud/mdb/mdb-health/internal"
	"a.yandex-team.ru/cloud/mdb/mdb-health/internal/api/swagger"
)

////go:generate swagger generate server --target ../generated/swagger --name mdbhealth --spec ../api/swagger.yaml --skip-models

var app *internal.App

func configureFlags(api *operations.MdbHealthAPI) {
	// api.CommandLineOptionsGroups = []swag.CommandLineOptionsGroup{ ... }
}

func configureAPI(api *operations.MdbHealthAPI) http.Handler {
	app = internal.NewApp(api.Context())
	return ConfigureAPIWithApp(api, app)
}

func ConfigureAPIWithApp(api *operations.MdbHealthAPI, app *internal.App) http.Handler {
	swapi := swagger.New(app.GeneralWard(), app.L(), app.ReadyCheckAggregator())
	// configure the api here
	api.ServeError = app.ServeError

	// Set your custom logger if needed. Default one is log.Printf
	// Expected interface func(string, ...interface{})
	//
	// Example:
	// api.Logger = log.Printf
	api.Logger = app.LogCallback

	api.JSONConsumer = runtime.JSONConsumer()

	api.JSONProducer = runtime.JSONProducer()

	api.HealthPingHandler = health.PingHandlerFunc(func(params health.PingParams) middleware.Responder {
		return swapi.GetPingHandler(params)
	})
	api.HealthStatsHandler = health.StatsHandlerFunc(func(params health.StatsParams) middleware.Responder {
		return swapi.GetStatsHandler(params)
	})
	api.ListhostshealthListHostsHealthHandler = listhostshealth.ListHostsHealthHandlerFunc(func(params listhostshealth.ListHostsHealthParams) middleware.Responder {
		return swapi.PostListHostsHealthHandler(params)
	})
	api.HostshealthGetHostsHealthHandler = hostshealth.GetHostsHealthHandlerFunc(func(params hostshealth.GetHostsHealthParams) middleware.Responder {
		return swapi.GetHostsHealthHandler(params)
	})
	api.HostshealthUpdateHostHealthHandler = hostshealth.UpdateHostHealthHandlerFunc(func(params hostshealth.UpdateHostHealthParams) middleware.Responder {
		return swapi.UpdateHostHealthHandler(params)
	})
	api.ClusterhealthGetClusterHealthHandler = clusterhealth.GetClusterHealthHandlerFunc(func(params clusterhealth.GetClusterHealthParams) middleware.Responder {
		return swapi.GetClusterHealthHandler(params)
	})
	api.UnhealthyaggregatedinfoGetUnhealthyAggregatedInfoHandler = unhealthyaggregatedinfo.GetUnhealthyAggregatedInfoHandlerFunc(func(params unhealthyaggregatedinfo.GetUnhealthyAggregatedInfoParams) middleware.Responder {
		return swapi.GetUnhealthyAggregatedInfoHandler(params)
	})
	api.HostneighboursGetHostNeighboursHandler = hostneighbours.GetHostNeighboursHandlerFunc(func(params hostneighbours.GetHostNeighboursParams) middleware.Responder {
		return swapi.GetHostNeighboursHandler(params)
	})

	api.ServerShutdown = app.Shutdown
	handler := app.SetupGlobalMiddleware(api.Serve(setupMiddlewares))
	return setupGlobalMiddleware(handler)
}

// The TLS configuration before HTTPS server starts.
func configureTLS(tlsConfig *tls.Config) {
	// Make all necessary changes to the TLS configuration here.
}

// As soon as server is initialized but not run yet, this function will be called.
// If you need to modify a config, store server instance to stop it individually later, this is the place.
// This function can be called multiple times, depending on the number of serving schemes.
// scheme value will be set accordingly: "http", "https" or "unix"
func configureServer(s *http.Server, scheme, addr string) {
}

// The middleware configuration is for the handler executors. These do not apply to the swagger.json document.
// The middleware executes after routing but before authentication, binding and validation
func setupMiddlewares(handler http.Handler) http.Handler {
	return handler
}

// The middleware configuration happens before anything, this middleware also applies to serving the swagger.json document.
// So this is a good place to plug in a panic handling middleware, logging and metrics
func setupGlobalMiddleware(handler http.Handler) http.Handler {
	return handler
}
