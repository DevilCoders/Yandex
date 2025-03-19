package swagapi

import (
	"fmt"
	"net/http"
	"strconv"
	"strings"
	"time"

	"github.com/go-openapi/runtime/middleware"
	"github.com/prometheus/client_golang/prometheus"

	swagmodels "a.yandex-team.ru/cloud/mdb/deploy/api/generated/swagger/models"
	"a.yandex-team.ru/cloud/mdb/deploy/api/generated/swagger/restapi/operations/commands"
	"a.yandex-team.ru/cloud/mdb/deploy/api/generated/swagger/restapi/operations/common"
	"a.yandex-team.ru/cloud/mdb/deploy/api/generated/swagger/restapi/operations/groups"
	"a.yandex-team.ru/cloud/mdb/deploy/api/generated/swagger/restapi/operations/masters"
	"a.yandex-team.ru/cloud/mdb/deploy/api/generated/swagger/restapi/operations/minions"
	"a.yandex-team.ru/cloud/mdb/deploy/api/internal/core"
	"a.yandex-team.ru/cloud/mdb/deploy/api/internal/deploydb"
	"a.yandex-team.ru/cloud/mdb/deploy/api/pkg/models"
	"a.yandex-team.ru/cloud/mdb/internal/encodingutil"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	uprometheus "a.yandex-team.ru/cloud/mdb/internal/prometheus"
	"a.yandex-team.ru/cloud/mdb/internal/reflectutil"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

// API swagger interface to service
type API struct {
	logger log.Logger
	srv    *core.Service
}

// New constructor for API
func New(srv *core.Service, logger log.Logger) *API {
	return &API{
		logger: logger,
		srv:    srv,
	}
}

// GetPingHandler GET /v1/ping
func (api *API) GetPingHandler(params common.PingParams) middleware.Responder {
	if err := api.srv.IsReady(params.HTTPRequest.Context()); err != nil {
		return common.NewPingDefault(http.StatusServiceUnavailable).WithPayload(&swagmodels.Error{Message: err.Error()})
	}

	return common.NewPingOK()
}

// GetStatsHandler GET /v1/stat
func (api *API) GetStatsHandler(params common.StatsParams) middleware.Responder {
	mfs, err := prometheus.DefaultGatherer.Gather()
	if err != nil {
		return common.NewStatsDefault(http.StatusInternalServerError).WithPayload(&swagmodels.Error{Message: err.Error()})
	}

	stats, err := uprometheus.FormatYasmStats(mfs)
	if err != nil {
		return common.NewStatsDefault(http.StatusInternalServerError).WithPayload(&swagmodels.Error{Message: err.Error()})
	}

	return common.NewStatsOK().WithPayload(stats)
}

// CreateGroupHandler POST /v1/groups
func (api *API) CreateGroupHandler(params groups.CreateGroupParams) middleware.Responder {
	if err := api.auth(params.HTTPRequest); err != nil {
		return newAuthError(params.HTTPRequest.URL, err, api.logger)
	}

	group, err := api.srv.CreateGroup(params.HTTPRequest.Context(), params.Body.Name)
	if err != nil {
		return groups.NewCreateGroupDefault(http.StatusInternalServerError).WithPayload(&swagmodels.Error{Message: err.Error()})
	}
	return groups.NewCreateGroupOK().WithPayload(groupToREST(group))
}

// GetGroupHandler GET /v1/groups/<name>
func (api *API) GetGroupHandler(params groups.GetGroupParams) middleware.Responder {
	if err := api.auth(params.HTTPRequest); err != nil {
		return newAuthError(params.HTTPRequest.URL, err, api.logger)
	}

	group, err := api.srv.Group(params.HTTPRequest.Context(), params.Groupname)
	if err != nil {
		return newLogicError(&groups.GetGroupDefault{}, err)
	}

	return groups.NewGetGroupOK().WithPayload(groupToREST(group))
}

// GetGroupsListHandler GET /v1/groups
func (api *API) GetGroupsListHandler(params groups.GetGroupsListParams) middleware.Responder {
	if err := api.auth(params.HTTPRequest); err != nil {
		return newAuthError(params.HTTPRequest.URL, err, api.logger)
	}

	limit, last, err := parsePaging(params.PageSize, params.PageToken)
	if err != nil {
		return groups.NewGetGroupsListDefault(http.StatusBadRequest).WithPayload(&swagmodels.Error{Message: err.Error()})
	}

	sortOrder := models.SortOrderUnknown
	if params.SortOrder != nil {
		sortOrder = SortOrderFromREST(*params.SortOrder)
	}

	grs, err := api.srv.Groups(params.HTTPRequest.Context(), sortOrder, limit, last)
	if err != nil {
		return newLogicError(&groups.GetGroupsListDefault{}, err)
	}

	var token models.GroupID
	var resp swagmodels.GroupsListResp
	for _, group := range grs {
		token = groupPagingToken(group, token)
		resp.Groups = append(resp.Groups, groupToREST(group))
	}
	resp.Paging.Token = token.String()

	return groups.NewGetGroupsListOK().WithPayload(&resp)
}

// GetMastersListHandler GET /v1/masters
func (api *API) GetMastersListHandler(params masters.GetMastersListParams) middleware.Responder {
	if err := api.auth(params.HTTPRequest); err != nil {
		return newAuthError(params.HTTPRequest.URL, err, api.logger)
	}

	limit, last, err := parsePaging(params.PageSize, params.PageToken)
	if err != nil {
		return masters.NewGetMastersListDefault(http.StatusBadRequest).WithPayload(&swagmodels.Error{Message: err.Error()})
	}

	ms, err := api.srv.Masters(params.HTTPRequest.Context(), limit, last)
	if err != nil {
		return newLogicError(&masters.GetMastersListDefault{}, err)
	}

	var token models.MasterID
	var resp swagmodels.MastersListResp
	for _, master := range ms {
		token = masterPagingToken(master, token)
		resp.Masters = append(resp.Masters, masterToREST(master))
	}
	resp.Paging.Token = token.String()

	return masters.NewGetMastersListOK().WithPayload(&resp)
}

// CreateMasterHandler POST /v1/masters
func (api *API) CreateMasterHandler(params masters.CreateMasterParams) middleware.Responder {
	if err := api.auth(params.HTTPRequest); err != nil {
		return newAuthError(params.HTTPRequest.URL, err, api.logger)
	}

	if params.Body.Group == nil {
		return masters.NewCreateMasterDefault(http.StatusBadRequest).WithPayload(&swagmodels.Error{Message: "group required"})
	}

	isOpen := false
	if params.Body.IsOpen != nil {
		isOpen = *params.Body.IsOpen
	}
	desc := ""
	if params.Body.Description != nil {
		desc = *params.Body.Description
	}

	master, err := api.srv.CreateMaster(params.HTTPRequest.Context(), params.Body.Fqdn, *params.Body.Group, isOpen, desc)
	if err != nil {
		return newLogicError(&masters.CreateMasterDefault{}, err)
	}

	return masters.NewCreateMasterOK().WithPayload(masterToREST(master))
}

// UpsertMasterHandler PUT /v1/masters/<fqdn>
func (api *API) UpsertMasterHandler(params masters.UpsertMasterParams) middleware.Responder {
	if err := api.auth(params.HTTPRequest); err != nil {
		return newAuthError(params.HTTPRequest.URL, err, api.logger)
	}

	args := deploydb.UpsertMasterAttrs{}
	if params.Body.Group != nil {
		args.Group.Set(*params.Body.Group)
	}
	if params.Body.IsOpen != nil {
		args.IsOpen.Set(*params.Body.IsOpen)
	}
	if params.Body.Description != nil {
		args.Description.Set(*params.Body.Description)
	}

	master, err := api.srv.UpsertMaster(params.HTTPRequest.Context(), params.Body.Fqdn, args)
	if err != nil {
		return newLogicError(&masters.UpsertMasterDefault{}, err)
	}

	return masters.NewUpsertMasterOK().WithPayload(masterToREST(master))
}

// GetMasterHandler GET /v1/masters/<fqdn>
func (api *API) GetMasterHandler(params masters.GetMasterParams) middleware.Responder {
	if err := api.auth(params.HTTPRequest); err != nil {
		return newAuthError(params.HTTPRequest.URL, err, api.logger)
	}

	master, err := api.srv.Master(params.HTTPRequest.Context(), params.Fqdn)
	if err != nil {
		return newLogicError(&masters.GetMasterDefault{}, err)
	}

	return masters.NewGetMasterOK().WithPayload(masterToREST(master))
}

func groupPagingToken(group models.Group, token models.GroupID) models.GroupID {
	if group.ID > token {
		token = group.ID
	}

	return token
}

func masterPagingToken(master models.Master, token models.MasterID) models.MasterID {
	if master.ID > token {
		token = master.ID
	}

	return token
}

func minionPagingToken(minion models.Minion, token models.MinionID) models.MinionID {
	if minion.ID > token {
		token = minion.ID
	}

	return token
}

func shipmentPagingToken(shipment models.Shipment, token models.ShipmentID) models.ShipmentID {
	if shipment.ID > token {
		token = shipment.ID
	}

	return token
}

func commandPagingToken(c models.Command, token models.CommandID) models.CommandID {
	if c.ID > token {
		token = c.ID
	}

	return token
}

func jobPagingToken(j models.Job, token models.JobID) models.JobID {
	if j.ID > token {
		token = j.ID
	}

	return token
}

func jobResultPagingToken(jr models.JobResult, token models.JobResultID) models.JobResultID {
	if jr.JobResultID > token {
		token = jr.JobResultID
	}

	return token
}

// GetMasterMinionsHandler GET /v1/masters/<fqdn>/minions
func (api *API) GetMasterMinionsHandler(params masters.GetMasterMinionsParams) middleware.Responder {
	if err := api.auth(params.HTTPRequest); err != nil {
		return newAuthError(params.HTTPRequest.URL, err, api.logger)
	}

	limit, last, err := parsePaging(params.PageSize, params.PageToken)
	if err != nil {
		return masters.NewGetMasterMinionsDefault(http.StatusBadRequest).WithPayload(&swagmodels.Error{Message: err.Error()})
	}

	ms, err := api.srv.MinionsByMaster(params.HTTPRequest.Context(), params.Fqdn, limit, last)
	if err != nil {
		return newLogicError(&masters.GetMasterMinionsDefault{}, err)
	}

	var token models.MinionID
	var resp swagmodels.MinionsListResp
	for _, minion := range ms {
		token = minionPagingToken(minion, token)
		resp.Minions = append(resp.Minions, minionToREST(minion))
	}
	resp.Paging.Token = token.String()

	return masters.NewGetMasterMinionsOK().WithPayload(&resp)
}

// GetMinionsListHandler GET /v1/minions
func (api *API) GetMinionsListHandler(params minions.GetMinionsListParams) middleware.Responder {
	if err := api.auth(params.HTTPRequest); err != nil {
		return newAuthError(params.HTTPRequest.URL, err, api.logger)
	}

	limit, last, err := parsePaging(params.PageSize, params.PageToken)
	if err != nil {
		return minions.NewGetMinionsListDefault(http.StatusBadRequest).WithPayload(&swagmodels.Error{Message: err.Error()})
	}

	ms, err := api.srv.Minions(params.HTTPRequest.Context(), limit, last)
	if err != nil {
		return newLogicError(&minions.GetMinionsListDefault{}, err)
	}

	var token models.MinionID
	var resp swagmodels.MinionsListResp
	for _, minion := range ms {
		token = minionPagingToken(minion, token)
		resp.Minions = append(resp.Minions, minionToREST(minion))
	}
	resp.Paging.Token = token.String()

	return minions.NewGetMinionsListOK().WithPayload(&resp)
}

// CreateMinionHandler POST /v1/minions
func (api *API) CreateMinionHandler(params minions.CreateMinionParams) middleware.Responder {
	if err := api.auth(params.HTTPRequest); err != nil {
		return newAuthError(params.HTTPRequest.URL, err, api.logger)
	}

	if params.Body.Group == nil {
		return minions.NewCreateMinionDefault(http.StatusBadRequest).WithPayload(&swagmodels.Error{Message: "group required"})
	}

	autoReassign := false
	if params.Body.AutoReassign != nil {
		autoReassign = *params.Body.AutoReassign
	}

	minion, err := api.srv.CreateMinion(params.HTTPRequest.Context(), params.Body.Fqdn, *params.Body.Group, autoReassign)
	if err != nil {
		return newLogicError(&minions.CreateMinionDefault{}, err)
	}

	return minions.NewCreateMinionOK().WithPayload(minionToREST(minion))
}

// UpsertMinionHandler PUT /v1/minions/<fqdn>
func (api *API) UpsertMinionHandler(params minions.UpsertMinionParams) middleware.Responder {
	if err := api.auth(params.HTTPRequest); err != nil {
		return newAuthError(params.HTTPRequest.URL, err, api.logger)
	}

	args := deploydb.UpsertMinionAttrs{}
	if params.Body.Group != nil {
		args.Group.Set(*params.Body.Group)
	}
	if params.Body.AutoReassign != nil {
		args.AutoReassign.Set(*params.Body.AutoReassign)
	}
	if params.Body.Master != nil {
		args.Master.Set(*params.Body.Master)
	}

	minion, err := api.srv.UpsertMinion(params.HTTPRequest.Context(), params.Body.Fqdn, args)
	if err != nil {
		return newLogicError(&minions.UpsertMinionDefault{}, err)
	}

	return minions.NewUpsertMinionOK().WithPayload(minionToREST(minion))
}

// GetMinionHandler GET /v1/minions/<fqdn>
func (api *API) GetMinionHandler(params minions.GetMinionParams) middleware.Responder {
	if err := api.auth(params.HTTPRequest); err != nil {
		return newAuthError(params.HTTPRequest.URL, err, api.logger)
	}

	minion, err := api.srv.Minion(params.HTTPRequest.Context(), params.Fqdn)
	if err != nil {
		return newLogicError(&minions.GetMinionDefault{}, err)
	}

	return minions.NewGetMinionOK().WithPayload(minionToREST(minion))
}

// GetMinionMasterHandler GET /v1/minions/<fqdn>/master
func (api *API) GetMinionMasterHandler(params minions.GetMinionMasterParams) middleware.Responder {
	mm, err := api.srv.Minion(params.HTTPRequest.Context(), params.Fqdn)
	if err != nil {
		return newLogicError(&minions.GetMinionMasterDefault{}, err)
	}

	master, err := api.srv.Master(params.HTTPRequest.Context(), mm.MasterFQDN)
	if err != nil {
		return newLogicError(&masters.GetMasterDefault{}, err)
	}

	return minions.NewGetMinionMasterOK().WithPayload(&swagmodels.MinionMaster{
		MinionPublicKey: swagmodels.MinionPublicKey{
			PublicKey: mm.PublicKey,
		},
		Master:          mm.MasterFQDN,
		MasterPublicKey: master.PublicKey,
	})
}

// RegisterMinionHandler POST /v1/minions/<fqdn>/register
func (api *API) RegisterMinionHandler(params minions.RegisterMinionParams) middleware.Responder {
	if err := api.auth(params.HTTPRequest); err != nil {
		return newAuthError(params.HTTPRequest.URL, err, api.logger)
	}

	minion, err := api.srv.RegisterMinion(params.HTTPRequest.Context(), params.Fqdn, params.Body.PublicKey)
	if err != nil {
		return newLogicError(&minions.RegisterMinionDefault{}, err)
	}

	return minions.NewRegisterMinionOK().WithPayload(minionToREST(minion))
}

// UnregisterMinionHandler POST /v1/minions/<fqdn>/unregister
func (api *API) UnregisterMinionHandler(params minions.UnregisterMinionParams) middleware.Responder {
	if err := api.auth(params.HTTPRequest); err != nil {
		return newAuthError(params.HTTPRequest.URL, err, api.logger)
	}

	minion, err := api.srv.UnregisterMinion(params.HTTPRequest.Context(), params.Fqdn)
	if err != nil {
		return newLogicError(&minions.UnregisterMinionDefault{}, err)
	}

	return minions.NewUnregisterMinionOK().WithPayload(minionToREST(minion))
}

// DeleteMinionHandler DELETE /v1/minions/<fqdn>
func (api *API) DeleteMinionHandler(params minions.DeleteMinionParams) middleware.Responder {
	if err := api.auth(params.HTTPRequest); err != nil {
		return newAuthError(params.HTTPRequest.URL, err, api.logger)
	}

	if err := api.srv.DeleteMinion(params.HTTPRequest.Context(), params.Fqdn); err != nil {
		return newLogicError(&minions.DeleteMinionDefault{}, err)
	}

	return minions.NewDeleteMinionOK()
}

// CreateShipmentHandler POST /v1/shipments
func (api *API) CreateShipmentHandler(params commands.CreateShipmentParams) middleware.Responder {
	if err := api.auth(params.HTTPRequest); err != nil {
		return newAuthError(params.HTTPRequest.URL, err, api.logger)
	}

	// TODO: remove later when batch size is gone from dbaas-worker
	parallel := params.Body.Parallel
	if parallel == 0 {
		parallel = params.Body.BatchSize
	}

	commandDefs := commandsDefFromREST(params.Body.Commands)
	shipment, err := api.srv.CreateShipment(
		params.HTTPRequest.Context(),
		params.Body.Fqdns,
		commandDefs,
		parallel,
		params.Body.StopOnErrorCount,
		time.Second*time.Duration(params.Body.Timeout),
	)
	if err != nil {
		return newLogicError(&commands.CreateShipmentDefault{}, err)
	}

	return commands.NewCreateShipmentOK().WithPayload(shipmentToREST(shipment))
}

// GetShipmentHandler GET /v1/shipments/<id>
func (api *API) GetShipmentHandler(params commands.GetShipmentParams) middleware.Responder {
	if err := api.auth(params.HTTPRequest); err != nil {
		return newAuthError(params.HTTPRequest.URL, err, api.logger)
	}

	id, err := models.ParseShipmentID(params.ShipmentID)
	if err != nil {
		return commands.NewGetShipmentDefault(http.StatusBadRequest).WithPayload(&swagmodels.Error{Message: err.Error()})
	}

	shipment, err := api.srv.Shipment(params.HTTPRequest.Context(), id)
	if err != nil {
		return newLogicError(&commands.GetShipmentDefault{}, err)
	}

	return commands.NewGetShipmentOK().WithPayload(shipmentToREST(shipment))
}

// GetShipmentsListHandler GET /v1/shipments
func (api *API) GetShipmentsListHandler(params commands.GetShipmentsListParams) middleware.Responder {
	if err := api.auth(params.HTTPRequest); err != nil {
		return newAuthError(params.HTTPRequest.URL, err, api.logger)
	}

	limit, last, err := parsePaging(params.PageSize, params.PageToken)
	if err != nil {
		return commands.NewGetShipmentsListDefault(http.StatusBadRequest).WithPayload(&swagmodels.Error{Message: err.Error()})
	}

	attrs := deploydb.SelectShipmentsAttrs{}
	if params.Fqdn != nil {
		attrs.FQDN.Set(*params.Fqdn)
	}
	if params.SortOrder != nil {
		attrs.SortOrder = SortOrderFromREST(*params.SortOrder)
	}
	if params.ShipmentStatus != nil {
		status := ShipmentStatusFromREST(swagmodels.ShipmentStatus(strings.ToLower(*params.ShipmentStatus)))
		if status == models.ShipmentStatusUnknown {
			return commands.NewGetShipmentsListDefault(http.StatusBadRequest).WithPayload(
				&swagmodels.Error{
					Message: fmt.Sprintf("invalid shipment status: %s", *params.ShipmentStatus),
				},
			)
		}

		attrs.Status.Set(string(status))
	}

	ss, err := api.srv.Shipments(params.HTTPRequest.Context(), attrs, limit, last)
	if err != nil {
		return newLogicError(&commands.GetShipmentsListDefault{}, err)
	}

	var token models.ShipmentID
	var resp swagmodels.ShipmentsListResp
	for _, shipment := range ss {
		token = shipmentPagingToken(shipment, token)
		resp.Shipments = append(resp.Shipments, shipmentToREST(shipment))
	}
	resp.Paging.Token = token.String()

	return commands.NewGetShipmentsListOK().WithPayload(&resp)
}

// GetCommandHandler GET /v1/commands/<id>
func (api *API) GetCommandHandler(params commands.GetCommandParams) middleware.Responder {
	if err := api.auth(params.HTTPRequest); err != nil {
		return newAuthError(params.HTTPRequest.URL, err, api.logger)
	}

	id, err := models.ParseCommandID(params.CommandID)
	if err != nil {
		return commands.NewGetCommandDefault(http.StatusBadRequest).WithPayload(&swagmodels.Error{Message: err.Error()})
	}

	cmd, err := api.srv.Command(params.HTTPRequest.Context(), id)
	if err != nil {
		return newLogicError(&commands.GetCommandDefault{}, err)
	}

	return commands.NewGetCommandOK().WithPayload(commandToREST(cmd))
}

// GetCommandsListHandler GET /v1/commands
func (api *API) GetCommandsListHandler(params commands.GetCommandsListParams) middleware.Responder {
	if err := api.auth(params.HTTPRequest); err != nil {
		return newAuthError(params.HTTPRequest.URL, err, api.logger)
	}

	limit, last, err := parsePaging(params.PageSize, params.PageToken)
	if err != nil {
		return commands.NewGetJobsListDefault(http.StatusBadRequest).WithPayload(&swagmodels.Error{Message: err.Error()})
	}

	attrs := deploydb.SelectCommandsAttrs{}
	if params.ShipmentID != nil {
		var shipmentID models.ShipmentID
		shipmentID, err = models.ParseShipmentID(*params.ShipmentID)
		if err != nil {
			return commands.NewGetJobsListDefault(http.StatusBadRequest).WithPayload(&swagmodels.Error{Message: err.Error()})
		}
		attrs.ShipmentID.Set(int64(shipmentID))
	}
	if params.Fqdn != nil {
		attrs.FQDN.Set(*params.Fqdn)
	}
	if params.SortOrder != nil {
		attrs.SortOrder = SortOrderFromREST(*params.SortOrder)
	}
	if params.CommandStatus != nil {
		status := CommandStatusFromREST(swagmodels.CommandStatus(strings.ToLower(*params.CommandStatus)))
		if status == models.CommandStatusUnknown {
			return commands.NewGetCommandsListDefault(http.StatusBadRequest).WithPayload(
				&swagmodels.Error{
					Message: fmt.Sprintf("invalid command status: %s", *params.CommandStatus),
				},
			)
		}

		attrs.Status.Set(string(status))
	}

	cmds, err := api.srv.Commands(params.HTTPRequest.Context(), attrs, limit, last)
	if err != nil {
		return newLogicError(&commands.GetCommandsListDefault{}, err)
	}

	var token models.CommandID
	var resp swagmodels.CommandsListResp
	for _, cmd := range cmds {
		token = commandPagingToken(cmd, token)
		resp.Commands = append(resp.Commands, commandToREST(cmd))
	}
	resp.Paging.Token = token.String()

	return commands.NewGetCommandsListOK().WithPayload(&resp)
}

// GetJobHandler GET /v1/jobs/<id>
func (api *API) GetJobHandler(params commands.GetJobParams) middleware.Responder {
	if err := api.auth(params.HTTPRequest); err != nil {
		return newAuthError(params.HTTPRequest.URL, err, api.logger)
	}

	id, err := models.ParseJobID(params.JobID)
	if err != nil {
		return commands.NewGetJobDefault(http.StatusBadRequest).WithPayload(&swagmodels.Error{Message: err.Error()})
	}

	job, err := api.srv.Job(params.HTTPRequest.Context(), id)
	if err != nil {
		return newLogicError(&commands.GetJobDefault{}, err)
	}

	return commands.NewGetJobOK().WithPayload(jobToREST(job))
}

// GetJobsListHandler GET /v1/jobs
func (api *API) GetJobsListHandler(params commands.GetJobsListParams) middleware.Responder {
	if err := api.auth(params.HTTPRequest); err != nil {
		return newAuthError(params.HTTPRequest.URL, err, api.logger)
	}

	limit, last, err := parsePaging(params.PageSize, params.PageToken)
	if err != nil {
		return commands.NewGetJobsListDefault(http.StatusBadRequest).WithPayload(&swagmodels.Error{Message: err.Error()})
	}

	attrs := deploydb.SelectJobsAttrs{}
	if params.ShipmentID != nil {
		var shipmentID models.ShipmentID
		shipmentID, err = models.ParseShipmentID(*params.ShipmentID)
		if err != nil {
			return commands.NewGetJobsListDefault(http.StatusBadRequest).WithPayload(&swagmodels.Error{Message: err.Error()})
		}
		attrs.ShipmentID.Set(int64(shipmentID))
	}
	if params.Fqdn != nil {
		attrs.FQDN.Set(*params.Fqdn)
	}
	if params.ExtJobID != nil {
		attrs.ExtJobID.Set(*params.ExtJobID)
	}
	if params.SortOrder != nil {
		attrs.SortOrder = SortOrderFromREST(*params.SortOrder)
	}
	if params.JobStatus != nil {
		status := JobStatusFromREST(swagmodels.JobStatus(strings.ToLower(*params.JobStatus)))
		if status == models.JobStatusUnknown {
			return commands.NewGetJobsListDefault(http.StatusBadRequest).WithPayload(
				&swagmodels.Error{
					Message: fmt.Sprintf("invalid job status: %s", *params.JobStatus),
				},
			)
		}

		attrs.Status.Set(string(status))
	}

	jobs, err := api.srv.Jobs(params.HTTPRequest.Context(), attrs, limit, last)
	if err != nil {
		return newLogicError(&commands.GetJobsListDefault{}, err)
	}

	var token models.JobID
	var resp swagmodels.JobsListResp
	for _, jr := range jobs {
		token = jobPagingToken(jr, token)
		resp.Jobs = append(resp.Jobs, jobToREST(jr))
	}
	resp.Paging.Token = token.String()

	return commands.NewGetJobsListOK().WithPayload(&resp)
}

// CreateJobResultHandler POST /v1/minions/<fqdn>/jobs/<id>/results
func (api *API) CreateJobResultHandler(params commands.CreateJobResultParams) middleware.Responder {
	jr, err := api.srv.CreateJobResult(params.HTTPRequest.Context(), params.JobID, params.Fqdn, params.Body.Result)
	if err != nil {
		return newLogicError(&commands.CreateJobResultDefault{}, err)
	}

	return commands.NewCreateJobResultOK().WithPayload(jobResultToREST(jr))
}

// GetJobResultHandler GET /v1/jobresults/<id>
func (api *API) GetJobResultHandler(params commands.GetJobResultParams) middleware.Responder {
	if err := api.auth(params.HTTPRequest); err != nil {
		return newAuthError(params.HTTPRequest.URL, err, api.logger)
	}

	id, err := models.ParseJobResultID(params.JobResultID)
	if err != nil {
		return commands.NewGetJobResultDefault(http.StatusBadRequest).WithPayload(&swagmodels.Error{Message: err.Error()})
	}

	job, err := api.srv.JobResult(params.HTTPRequest.Context(), id)
	if err != nil {
		return newLogicError(&commands.GetJobResultDefault{}, err)
	}

	return commands.NewGetJobResultOK().WithPayload(jobResultToREST(job))
}

// GetJobResultsListHandler GET /v1/jobresults
func (api *API) GetJobResultsListHandler(params commands.GetJobResultsListParams) middleware.Responder {
	if err := api.auth(params.HTTPRequest); err != nil {
		return newAuthError(params.HTTPRequest.URL, err, api.logger)
	}

	limit, last, err := parsePaging(params.PageSize, params.PageToken)
	if err != nil {
		return commands.NewGetJobResultsListDefault(http.StatusBadRequest).WithPayload(&swagmodels.Error{Message: err.Error()})
	}

	attrs := deploydb.SelectJobResultsAttrs{}
	if params.JobID != nil {
		// FIXME: FIX ALL USAGES OF THIS PARAMETER
		// FIXME: THEN FIX THIS ASSIGNMENT
		// FIXME: THEN FIX swagger.yaml
		attrs.ExtJobID.Set(*params.JobID)
	}
	if params.ExtJobID != nil {
		attrs.ExtJobID.Set(*params.ExtJobID)
	}
	if params.Fqdn != nil {
		attrs.FQDN.Set(*params.Fqdn)
	}
	if params.SortOrder != nil {
		attrs.SortOrder = SortOrderFromREST(*params.SortOrder)
	}
	if params.JobResultStatus != nil {
		jrs := JobResultStatusFromREST(swagmodels.JobResultStatus(strings.ToLower(*params.JobResultStatus)))
		if jrs == models.JobResultStatusUnknown {
			return commands.NewGetJobResultsListDefault(http.StatusBadRequest).WithPayload(
				&swagmodels.Error{
					Message: fmt.Sprintf("invalid job result status: %s", *params.JobResultStatus),
				},
			)
		}

		attrs.Status.Set(string(jrs))
	}

	jrs, err := api.srv.JobResults(params.HTTPRequest.Context(), attrs, limit, last)
	if err != nil {
		return newLogicError(&commands.GetJobResultsListDefault{}, err)
	}

	var token models.JobResultID
	var resp swagmodels.JobResultsListResp
	for _, jr := range jrs {
		token = jobResultPagingToken(jr, token)
		resp.JobResults = append(resp.JobResults, jobResultToREST(jr))
	}
	resp.Paging.Token = token.String()

	return commands.NewGetJobResultsListOK().WithPayload(&resp)
}

func parsePaging(pageSize *int64, pageToken *string) (int64, optional.Int64, error) {
	var last optional.Int64
	if pageToken != nil {
		var err error
		value, err := strconv.ParseInt(*pageToken, 10, 64)
		if err != nil {
			return 0, optional.Int64{}, xerrors.Errorf("failed to parse page token: %w", err)
		}
		last.Set(value)
	}

	return *pageSize, last, nil
}

func groupToREST(g models.Group) *swagmodels.GroupResp {
	return &swagmodels.GroupResp{
		Group: swagmodels.Group{
			ID:   int64(g.ID),
			Name: g.Name,
		},
	}
}

func masterToREST(m models.Master) *swagmodels.MasterResp {
	return &swagmodels.MasterResp{
		Master: swagmodels.Master{
			Fqdn:        m.FQDN,
			Aliases:     m.Aliases,
			Group:       &m.Group,
			IsOpen:      &m.IsOpen,
			Description: &m.Description,
			PublicKey:   &m.PublicKey,
		},
		AliveCheckAt: m.AliveCheckAt.Unix(),
		IsAlive:      m.Alive,
		CreatedAt:    m.CreatedAt.Unix(),
	}
}

func minionToREST(m models.Minion) *swagmodels.MinionResp {
	return &swagmodels.MinionResp{
		Minion: swagmodels.Minion{
			Fqdn:         m.FQDN,
			Group:        &m.Group,
			AutoReassign: &m.AutoReassign,
			Master:       &m.MasterFQDN,
		},
		MinionPublicKey: swagmodels.MinionPublicKey{
			PublicKey: m.PublicKey,
		},
		CreatedAt:     m.CreatedAt.Unix(),
		UpdatedAt:     m.UpdatedAt.Unix(),
		RegisterUntil: m.RegisterUntil.Unix(),
		Registered:    m.Registered,
		Deleted:       m.Deleted,
	}
}

func commandsDefFromREST(cdl []*swagmodels.CommandDef) []models.CommandDef {
	out := make([]models.CommandDef, 0, len(cdl))
	for _, c := range cdl {
		out = append(out, models.CommandDef{
			Type:    c.Type,
			Args:    c.Arguments,
			Timeout: encodingutil.FromDuration(time.Second * time.Duration(c.Timeout)),
		})
	}
	return out
}

func commandDefsToREST(cdl []models.CommandDef) []*swagmodels.CommandDef {
	out := make([]*swagmodels.CommandDef, 0, len(cdl))
	for _, c := range cdl {
		out = append(out, commandDefToREST(c))
	}
	return out
}

func commandDefToREST(c models.CommandDef) *swagmodels.CommandDef {
	return &swagmodels.CommandDef{
		Type:      c.Type,
		Arguments: c.Args,
		Timeout:   int64(c.Timeout.Seconds()),
	}
}

func shipmentToREST(s models.Shipment) *swagmodels.ShipmentResp {
	return &swagmodels.ShipmentResp{
		Shipment: swagmodels.Shipment{
			Fqdns:            s.FQDNs,
			Commands:         commandDefsToREST(s.Commands),
			Parallel:         s.Parallel,
			StopOnErrorCount: s.StopOnErrorCount,
			Timeout:          int64(s.Timeout.Seconds()),
		},
		ID:          s.ID.String(),
		Status:      ShipmentStatusToREST(s.Status),
		OtherCount:  s.OtherCount,
		DoneCount:   s.DoneCount,
		ErrorsCount: s.ErrorsCount,
		TotalCount:  s.TotalCount,
		CreatedAt:   s.CreatedAt.Unix(),
		UpdatedAt:   s.UpdatedAt.Unix(),
	}
}

var shipmentStatusToREST = map[models.ShipmentStatus]swagmodels.ShipmentStatus{
	models.ShipmentStatusUnknown:    swagmodels.ShipmentStatusUnknown,
	models.ShipmentStatusInProgress: swagmodels.ShipmentStatusInprogress,
	models.ShipmentStatusDone:       swagmodels.ShipmentStatusDone,
	models.ShipmentStatusError:      swagmodels.ShipmentStatusError,
	models.ShipmentStatusTimeout:    swagmodels.ShipmentStatusTimeout,
}
var shipmentStatusFromREST = reflectutil.ReverseMap(shipmentStatusToREST).(map[swagmodels.ShipmentStatus]models.ShipmentStatus)

// ShipmentStatusToREST ...
func ShipmentStatusToREST(ss models.ShipmentStatus) swagmodels.ShipmentStatus {
	v, ok := shipmentStatusToREST[ss]
	if !ok {
		return swagmodels.ShipmentStatusUnknown
	}

	return v
}

// ShipmentStatusFromREST ...
func ShipmentStatusFromREST(ss swagmodels.ShipmentStatus) models.ShipmentStatus {
	v, ok := shipmentStatusFromREST[ss]
	if !ok {
		return models.ShipmentStatusUnknown
	}

	return v
}

func commandToREST(c models.Command) *swagmodels.CommandResp {
	return &swagmodels.CommandResp{
		CommandDef: *commandDefToREST(c.CommandDef),
		ID:         c.ID.String(),
		ShipmentID: c.ShipmentID.String(),
		Fqdn:       c.FQDN,
		Status:     CommandStatusToREST(c.Status),
		CreatedAt:  c.CreatedAt.Unix(),
		UpdatedAt:  c.UpdatedAt.Unix(),
	}
}

var commandStatusToREST = map[models.CommandStatus]swagmodels.CommandStatus{
	models.CommandStatusUnknown:   swagmodels.CommandStatusUnknown,
	models.CommandStatusAvailable: swagmodels.CommandStatusAvailable,
	models.CommandStatusRunning:   swagmodels.CommandStatusRunning,
	models.CommandStatusDone:      swagmodels.CommandStatusDone,
	models.CommandStatusError:     swagmodels.CommandStatusError,
	models.CommandStatusCanceled:  swagmodels.CommandStatusCanceled,
	models.CommandStatusTimeout:   swagmodels.CommandStatusTimeout,
}
var commandStatusFromREST = reflectutil.ReverseMap(commandStatusToREST).(map[swagmodels.CommandStatus]models.CommandStatus)

// CommandStatusToREST ...
func CommandStatusToREST(cs models.CommandStatus) swagmodels.CommandStatus {
	v, ok := commandStatusToREST[cs]
	if !ok {
		return swagmodels.CommandStatusUnknown
	}

	return v
}

// CommandStatusFromREST ...
func CommandStatusFromREST(cs swagmodels.CommandStatus) models.CommandStatus {
	v, ok := commandStatusFromREST[cs]
	if !ok {
		return models.CommandStatusUnknown
	}

	return v
}

func jobToREST(j models.Job) *swagmodels.JobResp {
	return &swagmodels.JobResp{
		ID:        j.ID.String(),
		ExtID:     j.ExtID,
		CommandID: j.CommandID.String(),
		Status:    JobStatusToREST(j.Status),
		CreatedAt: j.CreatedAt.Unix(),
		UpdatedAt: j.UpdatedAt.Unix(),
	}
}

var jobStatusToREST = map[models.JobStatus]swagmodels.JobStatus{
	models.JobStatusUnknown: swagmodels.JobStatusUnknown,
	models.JobStatusRunning: swagmodels.JobStatusRunning,
	models.JobStatusDone:    swagmodels.JobStatusDone,
	models.JobStatusError:   swagmodels.JobStatusError,
	models.JobStatusTimeout: swagmodels.JobStatusTimeout,
}
var jobStatusFromREST = reflectutil.ReverseMap(jobStatusToREST).(map[swagmodels.JobStatus]models.JobStatus)

// JobStatusToREST ...
func JobStatusToREST(js models.JobStatus) swagmodels.JobStatus {
	v, ok := jobStatusToREST[js]
	if !ok {
		return swagmodels.JobStatusUnknown
	}

	return v
}

// JobStatusFromREST ...
func JobStatusFromREST(js swagmodels.JobStatus) models.JobStatus {
	v, ok := jobStatusFromREST[js]
	if !ok {
		return models.JobStatusUnknown
	}

	return v
}

func jobResultToREST(jr models.JobResult) *swagmodels.JobResultResp {
	return &swagmodels.JobResultResp{
		JobResult: swagmodels.JobResult{
			Result: []byte(jr.Result),
		},
		ID:         int64(jr.JobResultID),
		ExtID:      jr.ExtID,
		Fqdn:       jr.FQDN,
		Order:      int32(jr.Order),
		Status:     JobResultStatusToREST(jr.Status),
		RecordedAt: jr.RecordedAt.Unix(),
	}
}

var jobResultStatusToREST = map[models.JobResultStatus]swagmodels.JobResultStatus{
	models.JobResultStatusUnknown:    swagmodels.JobResultStatusUnknown,
	models.JobResultStatusSuccess:    swagmodels.JobResultStatusSuccess,
	models.JobResultStatusFailure:    swagmodels.JobResultStatusFailure,
	models.JobResultStatusTimeout:    swagmodels.JobResultStatusTimeout,
	models.JobResultStatusNotRunning: swagmodels.JobResultStatusNotrunning,
}
var jobResultStatusFromREST = reflectutil.ReverseMap(jobResultStatusToREST).(map[swagmodels.JobResultStatus]models.JobResultStatus)

// JobResultStatusToREST ...
func JobResultStatusToREST(jrs models.JobResultStatus) swagmodels.JobResultStatus {
	v, ok := jobResultStatusToREST[jrs]
	if !ok {
		return swagmodels.JobResultStatusUnknown
	}

	return v
}

// JobResultStatusFromREST ...
func JobResultStatusFromREST(jrs swagmodels.JobResultStatus) models.JobResultStatus {
	v, ok := jobResultStatusFromREST[jrs]
	if !ok {
		return models.JobResultStatusUnknown
	}

	return v
}

func (api *API) auth(r *http.Request) error {
	return api.srv.Auth(r.Context(), r)
}

var sortOrderToREST = map[models.SortOrder]string{
	models.SortOrderAsc:  "asc",
	models.SortOrderDesc: "desc",
}
var sortOrderFromREST = reflectutil.ReverseMap(sortOrderToREST).(map[string]models.SortOrder)

// SortOrderToREST ...
func SortOrderToREST(so models.SortOrder) (string, error) {
	v, ok := sortOrderToREST[so]
	if !ok {
		return "", xerrors.Errorf("unknown sort order: %s", so)
	}

	return v, nil
}

// SortOrderFromREST ...
func SortOrderFromREST(so string) models.SortOrder {
	v, ok := sortOrderFromREST[so]
	if !ok {
		return models.SortOrderUnknown
	}

	return v
}
