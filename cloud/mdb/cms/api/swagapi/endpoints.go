package swagapi

import (
	"net/http"
	"reflect"
	"runtime"

	"github.com/go-openapi/runtime/middleware"

	swagmodels "a.yandex-team.ru/cloud/mdb/cms/api/generated/swagger/models"
	"a.yandex-team.ru/cloud/mdb/cms/api/generated/swagger/restapi/operations/monrun"
	"a.yandex-team.ru/cloud/mdb/cms/api/generated/swagger/restapi/operations/stability"
	"a.yandex-team.ru/cloud/mdb/cms/api/generated/swagger/restapi/operations/tasks"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/webservice"
	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/models"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
)

type API struct {
	logger log.Logger
	srv    *webservice.Service
}

func New(srv *webservice.Service, logger log.Logger) *API {
	return &API{
		logger: logger.WithName("mdb-cms-api"),
		srv:    srv,
	}
}

func GetFunctionName(i interface{}) string {
	return runtime.FuncForPC(reflect.ValueOf(i).Pointer()).Name()
}

func getTicket(r *http.Request) string {
	return r.Header.Get("X-Ya-Service-Ticket")
}

func (api *API) ListUnhandledManagementRequests(params tasks.ListUnhandledManagementRequestsParams) middleware.Responder {
	ctx := ctxlog.WithFields(
		params.HTTPRequest.Context(),
		getLogFields(GetFunctionName(api.ListUnhandledManagementRequests), params)...)
	reqs, err := api.srv.GetRequests(ctx, getTicket(params.HTTPRequest))
	if err != nil {
		switch {
		case semerr.IsAuthorization(err) || semerr.IsAuthentication(err):
			ctxlog.Error(
				ctxlog.WithFields(ctx, log.Error(err)),
				api.logger,
				"Access denied")
			return tasks.NewListUnhandledManagementRequestsDefault(http.StatusUnauthorized).
				WithPayload(&swagmodels.Error{Message: err.Error()})
		default:
			ctxlog.Error(
				ctxlog.WithFields(ctx, log.Error(err)),
				api.logger,
				"Could not handle request")
			return tasks.NewListUnhandledManagementRequestsDefault(http.StatusInternalServerError).
				WithPayload(&swagmodels.Error{})
		}
	}
	pl, err := respListReqStatus(reqs)
	if err != nil {
		ctxlog.Error(
			ctxlog.WithFields(ctx, log.Error(err)),
			api.logger,
			"Could not convert to json")
		return tasks.NewListUnhandledManagementRequestsDefault(http.StatusInternalServerError).
			WithPayload(&swagmodels.Error{})
	}
	return tasks.NewListUnhandledManagementRequestsOK().WithPayload(pl)
}

func (api *API) RegisterRequestToManageNodes(params tasks.RegisterRequestToManageNodesParams) middleware.Responder {
	ctx := ctxlog.WithFields(
		params.HTTPRequest.Context(),
		getLogFields(GetFunctionName(api.RegisterRequestToManageNodes), params)...)

	isDryRun := false
	if params.DryRun != nil {
		isDryRun = *params.DryRun
	}
	status, err := api.srv.RegisterRequestToManageNodes(
		ctx,
		getTicket(params.HTTPRequest),
		*params.Body.Action,
		string(*params.Body.ID),
		params.Body.Comment,
		*params.Body.Issuer,
		*params.Body.Type,
		FQDNsToStrings(params.Body.Hosts),
		params.Body.Extra,
		params.Body.FailureType,
		models.ScenarioInfoFromAPI(params.Body.ScenarioInfo),
		isDryRun,
	)
	if err != nil {
		switch {
		case semerr.IsAuthorization(err):
			ctxlog.Error(
				ctxlog.WithFields(ctx, log.Error(err)),
				api.logger,
				"Could not authorize")
			return tasks.NewRegisterRequestToManageNodesDefault(http.StatusUnauthorized).
				WithPayload(&swagmodels.Error{Message: err.Error()})
		default:
			ctxlog.Error(
				ctxlog.WithFields(ctx, log.Error(err)),
				api.logger,
				"Could not handle request")
			return tasks.NewRegisterRequestToManageNodesDefault(http.StatusInternalServerError).
				WithPayload(&swagmodels.Error{})
		}
	}
	pl, err := respReqStatus(models.ManagementRequest{
		ExtID:   string(*params.Body.ID),
		Status:  status,
		Comment: params.Body.Comment,
		Fqnds:   FQDNsToStrings(params.Body.Hosts),
	})
	if err != nil {
		ctxlog.Error(
			ctxlog.WithFields(ctx, log.Error(err)),
			api.logger,
			"Could not convert to json")
		return tasks.NewRegisterRequestToManageNodesDefault(http.StatusInternalServerError).
			WithPayload(&swagmodels.Error{})
	}
	return tasks.NewRegisterRequestToManageNodesOK().WithPayload(pl)
}

func (api *API) Ping(params stability.PingParams) middleware.Responder {
	ctx := ctxlog.WithFields(
		params.HTTPRequest.Context(),
		getLogFields(GetFunctionName(api.Ping), params)...)

	if err := api.srv.IsReady(ctx); err != nil {
		return stability.NewPingDefault(http.StatusServiceUnavailable).
			WithPayload(&swagmodels.Error{Message: err.Error()})
	}

	return stability.NewPingOK()
}

func (api *API) DeleteTask(params tasks.DeleteTaskParams) middleware.Responder {
	ctx := ctxlog.WithFields(
		params.HTTPRequest.Context(),
		getLogFields(GetFunctionName(api.DeleteTask), params)...)
	err := api.srv.DeleteRequest(
		ctx,
		getTicket(params.HTTPRequest),
		params.TaskID)
	if err != nil {
		switch {
		case semerr.IsAuthorization(err):
			ctxlog.Error(
				ctxlog.WithFields(ctx, log.Error(err)),
				api.logger,
				"Could not authorize")
			return tasks.NewDeleteTaskDefault(http.StatusUnauthorized).
				WithPayload(&swagmodels.Error{Message: err.Error()})
		case semerr.IsNotFound(err):
			return tasks.NewDeleteTaskDefault(http.StatusNotFound).WithPayload(&swagmodels.Error{})
		default:
			ctxlog.Error(
				ctxlog.WithFields(ctx, log.Error(err)),
				api.logger,
				"Could not handle request")
			return tasks.NewDeleteTaskDefault(http.StatusInternalServerError).WithPayload(&swagmodels.Error{Message: err.Error()})
		}
	}
	return tasks.NewDeleteTaskNoContent()
}

func (api *API) GetRequestStatus(params tasks.GetRequestStatusParams) middleware.Responder {
	ctx := ctxlog.WithFields(
		params.HTTPRequest.Context(),
		getLogFields(GetFunctionName(api.GetRequestStatus), params)...)
	req, err := api.srv.GetRequestStatus(
		ctx,
		getTicket(params.HTTPRequest),
		params.TaskID)
	if err != nil {
		switch {
		case semerr.IsAuthorization(err):
			return tasks.NewGetRequestStatusDefault(http.StatusUnauthorized).WithPayload(&swagmodels.Error{Message: err.Error()})
		case semerr.IsNotFound(err):
			return tasks.NewGetRequestStatusDefault(http.StatusNotFound).WithPayload(&swagmodels.Error{})
		default:
			ctxlog.Error(
				ctxlog.WithFields(ctx, log.Error(err)),
				api.logger,
				"Could not handle request")
			return tasks.NewGetRequestStatusDefault(http.StatusInternalServerError).WithPayload(&swagmodels.Error{})
		}
	}
	pl, err := respReqStatus(req)
	if err != nil {
		ctxlog.Error(
			ctxlog.WithFields(ctx, log.Error(err)),
			api.logger,
			"Could not convert to json")
		return tasks.NewGetRequestStatusDefault(http.StatusInternalServerError).WithPayload(&swagmodels.Error{})
	}
	return tasks.NewGetRequestStatusOK().WithPayload(pl)
}

func (api *API) MonrunSendEvents(params monrun.SendEventsParams) middleware.Responder {
	ctx := ctxlog.WithFields(
		params.HTTPRequest.Context(),
		getLogFields(GetFunctionName(api.MonrunSendEvents), params)...)

	if err := api.srv.SendEvents(ctx); err != nil {
		return monrun.NewSendEventsDefault(http.StatusServiceUnavailable).
			WithPayload(&swagmodels.Error{Message: err.Error()})
	}

	return monrun.NewSendEventsOK()
}

func getLogFields(funcName string, params interface{}) []log.Field {
	flds := []log.Field{
		log.String("output", "swagger"),
		log.String("func", funcName),
	}
	var taskID, requestID *string
	switch params := params.(type) {
	case tasks.GetRequestStatusParams:
		taskID = &params.TaskID
		requestID = params.XRequestID
	case tasks.DeleteTaskParams:
		taskID = &params.TaskID
		requestID = params.XRequestID
	case tasks.RegisterRequestToManageNodesParams:
		tmp := string(*params.Body.ID)
		taskID = &tmp
		requestID = params.XRequestID
	case tasks.ListUnhandledManagementRequestsParams:
		requestID = params.XRequestID
	}
	if taskID != nil {
		flds = append(flds, log.String("task_id", *taskID))
	}
	if requestID != nil {
		flds = append(flds, log.String("request_id", *requestID))
	}
	return flds
}
