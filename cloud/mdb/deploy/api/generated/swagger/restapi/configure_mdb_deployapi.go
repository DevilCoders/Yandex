// This file is safe to edit. Once it exists it will not be overwritten

package restapi

import (
	"crypto/tls"
	"net/http"

	"github.com/go-openapi/runtime"
	"github.com/go-openapi/runtime/middleware"

	"a.yandex-team.ru/cloud/mdb/deploy/api/generated/swagger/restapi/operations"
	"a.yandex-team.ru/cloud/mdb/deploy/api/generated/swagger/restapi/operations/commands"
	"a.yandex-team.ru/cloud/mdb/deploy/api/generated/swagger/restapi/operations/common"
	"a.yandex-team.ru/cloud/mdb/deploy/api/generated/swagger/restapi/operations/groups"
	"a.yandex-team.ru/cloud/mdb/deploy/api/generated/swagger/restapi/operations/masters"
	"a.yandex-team.ru/cloud/mdb/deploy/api/generated/swagger/restapi/operations/minions"
	"a.yandex-team.ru/cloud/mdb/deploy/api/internal"
	"a.yandex-team.ru/cloud/mdb/deploy/api/internal/api/swagapi"
)

////go:generate swagger generate server --target ../generated/swagger --name mdbdeployapi --spec ../api/swagger.yaml --skip-models

var app *internal.App

func configureFlags(api *operations.MdbDeployapiAPI) {
	// api.CommandLineOptionsGroups = []swag.CommandLineOptionsGroup{ ... }
}

func configureAPI(api *operations.MdbDeployapiAPI) http.Handler {
	app = internal.NewApp(api.Context())
	swapi := swagapi.New(app.Service(), app.L())
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

	api.GroupsCreateGroupHandler = groups.CreateGroupHandlerFunc(func(params groups.CreateGroupParams) middleware.Responder {
		return swapi.CreateGroupHandler(params)
	})
	api.MastersCreateMasterHandler = masters.CreateMasterHandlerFunc(func(params masters.CreateMasterParams) middleware.Responder {
		return swapi.CreateMasterHandler(params)
	})
	api.MinionsCreateMinionHandler = minions.CreateMinionHandlerFunc(func(params minions.CreateMinionParams) middleware.Responder {
		return swapi.CreateMinionHandler(params)
	})
	api.CommandsCreateShipmentHandler = commands.CreateShipmentHandlerFunc(func(params commands.CreateShipmentParams) middleware.Responder {
		return swapi.CreateShipmentHandler(params)
	})
	api.GroupsDeleteGroupHandler = groups.DeleteGroupHandlerFunc(func(params groups.DeleteGroupParams) middleware.Responder {
		return middleware.NotImplemented("operation groups.DeleteGroup has not yet been implemented")
	})
	api.MastersDeleteMasterHandler = masters.DeleteMasterHandlerFunc(func(params masters.DeleteMasterParams) middleware.Responder {
		return middleware.NotImplemented("operation masters.DeleteMaster has not yet been implemented")
	})
	api.MinionsDeleteMinionHandler = minions.DeleteMinionHandlerFunc(func(params minions.DeleteMinionParams) middleware.Responder {
		return swapi.DeleteMinionHandler(params)
	})
	api.CommandsCreateJobResultHandler = commands.CreateJobResultHandlerFunc(func(params commands.CreateJobResultParams) middleware.Responder {
		return swapi.CreateJobResultHandler(params)
	})
	api.CommandsGetCommandHandler = commands.GetCommandHandlerFunc(func(params commands.GetCommandParams) middleware.Responder {
		return swapi.GetCommandHandler(params)
	})
	api.CommandsGetCommandsListHandler = commands.GetCommandsListHandlerFunc(func(params commands.GetCommandsListParams) middleware.Responder {
		return swapi.GetCommandsListHandler(params)
	})
	api.GroupsGetGroupHandler = groups.GetGroupHandlerFunc(func(params groups.GetGroupParams) middleware.Responder {
		return swapi.GetGroupHandler(params)
	})
	api.GroupsGetGroupsListHandler = groups.GetGroupsListHandlerFunc(func(params groups.GetGroupsListParams) middleware.Responder {
		return swapi.GetGroupsListHandler(params)
	})
	api.CommandsGetJobHandler = commands.GetJobHandlerFunc(func(params commands.GetJobParams) middleware.Responder {
		return swapi.GetJobHandler(params)
	})
	api.CommandsGetJobsListHandler = commands.GetJobsListHandlerFunc(func(params commands.GetJobsListParams) middleware.Responder {
		return swapi.GetJobsListHandler(params)
	})
	api.CommandsGetJobResultHandler = commands.GetJobResultHandlerFunc(func(params commands.GetJobResultParams) middleware.Responder {
		return swapi.GetJobResultHandler(params)
	})
	api.CommandsGetJobResultsListHandler = commands.GetJobResultsListHandlerFunc(func(params commands.GetJobResultsListParams) middleware.Responder {
		return swapi.GetJobResultsListHandler(params)
	})
	api.MastersGetMasterHandler = masters.GetMasterHandlerFunc(func(params masters.GetMasterParams) middleware.Responder {
		return swapi.GetMasterHandler(params)
	})
	api.MastersGetMasterMinionsHandler = masters.GetMasterMinionsHandlerFunc(func(params masters.GetMasterMinionsParams) middleware.Responder {
		return swapi.GetMasterMinionsHandler(params)
	})
	api.MastersGetMasterMinionsChangesHandler = masters.GetMasterMinionsChangesHandlerFunc(func(params masters.GetMasterMinionsChangesParams) middleware.Responder {
		return middleware.NotImplemented("operation masters.GetMasterMinionsChanges has not yet been implemented")
	})
	api.MastersGetMastersListHandler = masters.GetMastersListHandlerFunc(func(params masters.GetMastersListParams) middleware.Responder {
		return swapi.GetMastersListHandler(params)
	})
	api.MinionsGetMinionHandler = minions.GetMinionHandlerFunc(func(params minions.GetMinionParams) middleware.Responder {
		return swapi.GetMinionHandler(params)
	})
	api.MinionsGetMinionMasterHandler = minions.GetMinionMasterHandlerFunc(func(params minions.GetMinionMasterParams) middleware.Responder {
		return swapi.GetMinionMasterHandler(params)
	})
	api.MinionsGetMinionsListHandler = minions.GetMinionsListHandlerFunc(func(params minions.GetMinionsListParams) middleware.Responder {
		return swapi.GetMinionsListHandler(params)
	})
	api.CommandsGetShipmentHandler = commands.GetShipmentHandlerFunc(func(params commands.GetShipmentParams) middleware.Responder {
		return swapi.GetShipmentHandler(params)
	})
	api.CommandsGetShipmentsListHandler = commands.GetShipmentsListHandlerFunc(func(params commands.GetShipmentsListParams) middleware.Responder {
		return swapi.GetShipmentsListHandler(params)
	})
	api.CommonPingHandler = common.PingHandlerFunc(func(params common.PingParams) middleware.Responder {
		return swapi.GetPingHandler(params)
	})
	api.MinionsRegisterMinionHandler = minions.RegisterMinionHandlerFunc(func(params minions.RegisterMinionParams) middleware.Responder {
		return swapi.RegisterMinionHandler(params)
	})
	api.MinionsUnregisterMinionHandler = minions.UnregisterMinionHandlerFunc(func(params minions.UnregisterMinionParams) middleware.Responder {
		return swapi.UnregisterMinionHandler(params)
	})
	api.CommonStatsHandler = common.StatsHandlerFunc(func(params common.StatsParams) middleware.Responder {
		return swapi.GetStatsHandler(params)
	})
	api.MastersUpsertMasterHandler = masters.UpsertMasterHandlerFunc(func(params masters.UpsertMasterParams) middleware.Responder {
		return swapi.UpsertMasterHandler(params)
	})
	api.MinionsUpsertMinionHandler = minions.UpsertMinionHandlerFunc(func(params minions.UpsertMinionParams) middleware.Responder {
		return swapi.UpsertMinionHandler(params)
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
