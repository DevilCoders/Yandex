// This file is safe to edit. Once it exists it will not be overwritten

package restapi

import (
	"crypto/tls"
	"net/http"

	"github.com/go-openapi/runtime"
	"github.com/go-openapi/runtime/middleware"

	"a.yandex-team.ru/cloud/mdb/mdb-secrets/generated/swagger/models"
	"a.yandex-team.ru/cloud/mdb/mdb-secrets/generated/swagger/restapi/operations"
	"a.yandex-team.ru/cloud/mdb/mdb-secrets/generated/swagger/restapi/operations/certs"
	"a.yandex-team.ru/cloud/mdb/mdb-secrets/generated/swagger/restapi/operations/common"
	"a.yandex-team.ru/cloud/mdb/mdb-secrets/generated/swagger/restapi/operations/gpg"
	"a.yandex-team.ru/cloud/mdb/mdb-secrets/internal"
	"a.yandex-team.ru/library/go/core/log"
)

////go:generate swagger generate server --target ../../swagger --name MdbSecrets --spec ../../../api/swagger.yaml --skip-models

var app *internal.App

func configureFlags(api *operations.MdbSecretsAPI) {
	// api.CommandLineOptionsGroups = []swag.CommandLineOptionsGroup{ ... }
}

func configureAPI(api *operations.MdbSecretsAPI) http.Handler {
	// configure the api here
	app = internal.NewApp(api.Context())

	api.ServeError = app.ServeError

	api.Logger = app.LogCallback

	api.JSONConsumer = runtime.JSONConsumer()

	api.JSONProducer = runtime.JSONProducer()

	api.GpgDeleteHandler = gpg.DeleteHandlerFunc(func(params gpg.DeleteParams) middleware.Responder {
		return app.GpgAPI.DeleteGpgHandler(params.HTTPRequest.Context(), params)
	})
	api.GpgGetHandler = gpg.GetHandlerFunc(func(params gpg.GetParams) middleware.Responder {
		return app.GpgAPI.PutGpgHandler(params.HTTPRequest.Context(), params)
	})

	api.CertsPutHandler = certs.PutHandlerFunc(func(params certs.PutParams) middleware.Responder {
		return app.CertAPI.PutCertHandler(params.HTTPRequest.Context(), params)
	})

	api.CertsGetCertificateHandler = certs.GetCertificateHandlerFunc(func(params certs.GetCertificateParams) middleware.Responder {
		return app.CertAPI.GetCertHandler(params.HTTPRequest.Context(), params)
	})

	api.CertsRevokeCertificateHandler = certs.RevokeCertificateHandlerFunc(func(params certs.RevokeCertificateParams) middleware.Responder {
		return app.CertAPI.DeleteCertificate(params.HTTPRequest.Context(), params)
	})

	api.CommonPingHandler = common.PingHandlerFunc(func(params common.PingParams) middleware.Responder {
		if err := app.ReadyProvider.IsReady(params.HTTPRequest.Context()); err != nil {
			app.L().Error("Ping failed", log.Error(err))
			return common.NewPingServiceUnavailable().WithPayload(&models.Error{
				Message: err.Error(),
			})
		}
		return common.NewPingOK()
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
