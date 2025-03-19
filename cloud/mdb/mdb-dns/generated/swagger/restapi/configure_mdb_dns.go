// This file is safe to edit. Once it exists it will not be overwritten

package restapi

import (
	"crypto/tls"
	"net/http"

	"github.com/go-openapi/runtime"
	"github.com/go-openapi/runtime/middleware"

	"a.yandex-team.ru/cloud/mdb/mdb-dns/generated/swagger/restapi/operations"
	"a.yandex-team.ru/cloud/mdb/mdb-dns/generated/swagger/restapi/operations/dns"
	"a.yandex-team.ru/cloud/mdb/mdb-dns/internal"
	"a.yandex-team.ru/cloud/mdb/mdb-dns/internal/api/swagger"
)

////go:generate swagger generate server --target ../generated/swagger --name mdb-dns --spec ../api/swagger.yaml --skip-models

var app *internal.App

func configureFlags(api *operations.MdbDNSAPI) {
	// api.CommandLineOptionsGroups = []swag.CommandLineOptionsGroup{ ... }
}

func configureAPI(api *operations.MdbDNSAPI) http.Handler {
	app = internal.NewApp(api.Context())
	swapi := swagger.New(app.L(), app.DNSManager())
	// configure the api here
	api.ServeError = app.ServeError

	api.Logger = app.LogCallback

	api.JSONConsumer = runtime.JSONConsumer()

	api.JSONProducer = runtime.JSONProducer()

	api.DNSPingHandler = dns.PingHandlerFunc(func(params dns.PingParams) middleware.Responder {
		return swapi.GetPingHandler(params)
	})
	api.DNSStatsHandler = dns.StatsHandlerFunc(func(params dns.StatsParams) middleware.Responder {
		return swapi.GetStatsHandler(params)
	})
	api.DNSLiveHandler = dns.LiveHandlerFunc(func(params dns.LiveParams) middleware.Responder {
		return swapi.GetLiveHandler(params)
	})
	api.DNSLiveByClusterHandler = dns.LiveByClusterHandlerFunc(func(params dns.LiveByClusterParams) middleware.Responder {
		return swapi.GetLiveByClusterHandler(params)
	})
	api.DNSUpdatePrimaryDNSHandler = dns.UpdatePrimaryDNSHandlerFunc(func(params dns.UpdatePrimaryDNSParams) middleware.Responder {
		return swapi.UpdatePrimaryDNSHandler(params)
	})

	api.ServerShutdown = app.Shutdown

	return setupGlobalMiddleware(api.Serve(setupMiddlewares))
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
	return app.SetupGlobalMiddleware(handler)
}
