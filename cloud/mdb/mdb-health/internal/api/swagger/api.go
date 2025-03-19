package swagger

import (
	"context"
	"encoding/base64"
	"net/http"
	"os"
	"time"

	"github.com/go-openapi/runtime/middleware"
	"github.com/prometheus/client_golang/prometheus"

	"a.yandex-team.ru/cloud/mdb/internal/httputil"
	"a.yandex-team.ru/cloud/mdb/internal/metadb"
	uprometheus "a.yandex-team.ru/cloud/mdb/internal/prometheus"
	"a.yandex-team.ru/cloud/mdb/internal/ready"
	"a.yandex-team.ru/cloud/mdb/internal/requestid"
	"a.yandex-team.ru/cloud/mdb/internal/tracing/tags"
	"a.yandex-team.ru/cloud/mdb/mdb-health/generated/swagger/models"
	"a.yandex-team.ru/cloud/mdb/mdb-health/generated/swagger/restapi/operations/clusterhealth"
	"a.yandex-team.ru/cloud/mdb/mdb-health/generated/swagger/restapi/operations/health"
	"a.yandex-team.ru/cloud/mdb/mdb-health/generated/swagger/restapi/operations/hostneighbours"
	"a.yandex-team.ru/cloud/mdb/mdb-health/generated/swagger/restapi/operations/hostshealth"
	"a.yandex-team.ru/cloud/mdb/mdb-health/generated/swagger/restapi/operations/listhostshealth"
	"a.yandex-team.ru/cloud/mdb/mdb-health/generated/swagger/restapi/operations/unhealthyaggregatedinfo"
	"a.yandex-team.ru/cloud/mdb/mdb-health/internal/unhealthy"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

// API swagger interface to service
type API struct {
	logger  log.Logger
	gw      *core.GeneralWard
	checker ready.Checker
}

const (
	closeHealthFilePath = "/var/tmp/mdb_health.close"
)

// New constructor for API
func New(gw *core.GeneralWard, logger log.Logger, checker ready.Checker) *API {
	return &API{
		logger:  logger,
		gw:      gw,
		checker: checker,
	}
}

// GetPingHandler GET /v1/ping
func (api *API) GetPingHandler(params health.PingParams) middleware.Responder {
	ctx := contextWithRequestID(params.HTTPRequest.Context(), params.XRequestID)
	if _, err := os.Stat(closeHealthFilePath); err == nil {
		api.logger.Warn("close file exist", log.String("filePath", closeHealthFilePath))
		return health.NewPingDefault(503)
	} else if !os.IsNotExist(err) {
		return api.handleError(ctx, api.GetPingHandler, &health.PingDefault{}, err)
	}

	if err := api.checker.IsReady(ctx); err != nil {
		return api.handleError(ctx, api.GetPingHandler, &health.PingDefault{}, err)
	}

	return health.NewPingOK()
}

// GetStatsHandler GET /v1/stats
func (api *API) GetStatsHandler(params health.StatsParams) middleware.Responder {
	ctx := contextWithRequestID(params.HTTPRequest.Context(), nil)
	mfs, err := prometheus.DefaultGatherer.Gather()
	if err != nil {
		return api.handleError(ctx, api.GetStatsHandler, &health.StatsDefault{}, err)
	}

	stats, err := uprometheus.FormatYasmStats(mfs)
	if err != nil {
		return api.handleError(ctx, api.GetStatsHandler, &health.StatsDefault{}, err)
	}

	return health.NewStatsOK().WithPayload(stats)
}

// PostListHostsHealthHandler POST /v1/listhostshealth
func (api *API) PostListHostsHealthHandler(params listhostshealth.ListHostsHealthParams) middleware.Responder {
	ctx := contextWithRequestID(params.HTTPRequest.Context(), params.XRequestID)
	hhs, err := api.gw.LoadHostsHealth(ctx, params.Body.Hosts)
	if err != nil {
		return api.handleError(ctx, api.PostListHostsHealthHandler, &listhostshealth.ListHostsHealthDefault{}, err)
	}

	hhModels := make([]*models.HostHealth, 0, len(hhs))
	for _, hh := range hhs {
		hhModels = append(hhModels, hostHealthToModel(hh))
	}

	return listhostshealth.NewListHostsHealthOK().WithPayload(&models.HostsHealthResp{Hosts: hhModels})
}

// GetHostsHealthHandler GET /v1/hostshealth
func (api *API) GetHostsHealthHandler(params hostshealth.GetHostsHealthParams) middleware.Responder {
	ctx := contextWithRequestID(params.HTTPRequest.Context(), params.XRequestID)
	hhs, err := api.gw.LoadHostsHealth(ctx, params.Fqdns)
	if err != nil {
		return api.handleError(ctx, api.GetHostsHealthHandler, &hostshealth.GetHostsHealthDefault{}, err)
	}

	hhModels := make([]*models.HostHealth, 0, len(hhs))
	for _, hh := range hhs {
		hhModels = append(hhModels, hostHealthToModel(hh))
	}

	return hostshealth.NewGetHostsHealthOK().WithPayload(&models.HostsHealthResp{Hosts: hhModels})
}

// UpdateHostHealthHandler PUT /v1/hostshealth
func (api *API) UpdateHostHealthHandler(params hostshealth.UpdateHostHealthParams) middleware.Responder {
	ctx := contextWithRequestID(params.HTTPRequest.Context(), params.XRequestID)
	body := httputil.RequestBodyFromContext(ctx)
	if body == nil {
		ctxlog.Error(ctx, api.logger, "no request body context value found for signature verification")
		return hostshealth.NewUpdateHostHealthDefault(http.StatusBadRequest).WithPayload(&models.Error{Message: "no request body found"})
	}

	signature, err := base64.StdEncoding.DecodeString(params.XSignature)
	if err != nil {
		return api.handleError(ctx, api.UpdateHostHealthHandler, &hostshealth.UpdateHostHealthDefault{}, err)
	}

	// Verify signature
	if err := api.gw.VerifyClusterSignature(
		ctx,
		params.Body.Hosthealth.Cid,
		body,
		signature,
	); err != nil {
		return api.handleError(ctx, api.UpdateHostHealthHandler, &hostshealth.UpdateHostHealthDefault{}, err)
	}

	hh, errors := hostHealthFromModel(params.Body.Hosthealth)
	if errors != nil {
		ctxlog.Warnf(ctx, api.logger, "error with processing system metrics from %s: %s", params.Body.Hosthealth.Fqdn, errors)
	}
	// Store update
	if err := api.gw.StoreHostHealth(
		ctx,
		hh,
		time.Duration(params.Body.TTL)*time.Second,
	); err != nil {
		return api.handleError(ctx, api.UpdateHostHealthHandler, &hostshealth.UpdateHostHealthDefault{}, err)
	}

	return hostshealth.NewUpdateHostHealthOK()
}

// GetClusterHealthHandler GET /v1/clusterhealth
func (api *API) GetClusterHealthHandler(params clusterhealth.GetClusterHealthParams) middleware.Responder {
	ctx := contextWithRequestID(params.HTTPRequest.Context(), params.XRequestID)
	ch, err := api.gw.LoadClusterHealth(ctx, params.Cid)
	if err != nil {
		return api.handleError(ctx, api.GetClusterHealthHandler, &clusterhealth.GetClusterHealthDefault{}, err)
	}

	return clusterhealth.NewGetClusterHealthOK().WithPayload(clusterHealthToModel(ch))
}

func contextWithRequestID(ctx context.Context, requestID *string) context.Context {
	rid := requestid.New()
	if requestID != nil {
		rid = *requestID
	}

	ctx = requestid.WithRequestID(ctx, rid)
	ctx = requestid.WithLogField(ctx, rid)
	tags.RequestID.SetContext(ctx, rid)

	return ctx
}

func hostNeighboursToModel(fqdn string, mi types.HostNeighboursInfo) *models.HostNeighboursInfo {
	return &models.HostNeighboursInfo{
		Fqdn:               fqdn,
		Cid:                mi.Cid,
		Sid:                mi.Sid,
		Env:                mi.Env,
		Hacluster:          mi.HACluster,
		Hashard:            mi.HAShard,
		Roles:              mi.Roles,
		Samerolestotal:     int64(mi.SameRolesTotal),
		Samerolesalive:     int64(mi.SameRolesAlive),
		Samerolestimestamp: mi.SameRolesTS.Unix(),
	}
}

// GetHostNeighboursHandler GET /v1/hostneighbours
func (api *API) GetHostNeighboursHandler(params hostneighbours.GetHostNeighboursParams) middleware.Responder {
	ctx := contextWithRequestID(params.HTTPRequest.Context(), params.XRequestID)
	hn, err := api.gw.GetHostNeighbours(ctx, params.Fqdns)
	if err != nil {
		return api.handleError(ctx, api.GetHostNeighboursHandler, &hostneighbours.GetHostNeighboursDefault{}, err)
	}

	hhModels := make([]*models.HostNeighboursInfo, 0, len(hn))
	for fqdn, mi := range hn {
		hhModels = append(hhModels, hostNeighboursToModel(fqdn, mi))
	}

	return hostneighbours.NewGetHostNeighboursOK().WithPayload(&models.HostNeighboursResp{Hosts: hhModels})
}

// GetClusterHealthHandler GET /v1/unhealthyaggregatedinfo
func (api *API) GetUnhealthyAggregatedInfoHandler(params unhealthyaggregatedinfo.GetUnhealthyAggregatedInfoParams) middleware.Responder {
	ctx := contextWithRequestID(params.HTTPRequest.Context(), params.XRequestID)
	uai, err := api.gw.LoadUnhealthyAggregatedInfo(ctx, metadb.ClusterType(params.CType), types.AggType(params.AggType), params.Env)
	if err != nil {
		return api.handleError(ctx, api.GetUnhealthyAggregatedInfoHandler, &unhealthyaggregatedinfo.GetUnhealthyAggregatedInfoDefault{}, err)
	}

	return unhealthyaggregatedinfo.NewGetUnhealthyAggregatedInfoOK().WithPayload(unhealthyAggregatedInfoToModel(uai))
}

func hostHealthFromModel(hh *models.HostHealth) (types.HostHealth, []error) {
	shs := make([]types.ServiceHealth, 0, len(hh.Services))
	for _, sh := range hh.Services {
		shs = append(
			shs,
			types.NewServiceHealth(
				sh.Name,
				time.Unix(sh.Timestamp, 0),
				types.ServiceStatus(*sh.Status),
				types.ServiceRole(sh.Role),
				types.ServiceReplicaType(sh.Replicatype),
				sh.ReplicaUpstream,
				sh.ReplicaLag,
				sh.Metrics,
			),
		)
	}

	sm, errors := systemMetricsFromModel(hh.System)
	if hh.Mode != nil {
		mode := &types.Mode{
			Timestamp:       time.Unix(hh.Mode.Timestamp, 0),
			Read:            hh.Mode.Read,
			Write:           hh.Mode.Write,
			UserFaultBroken: hh.Mode.InstanceUserfaultBroken,
		}
		return types.NewHostHealthWithSystemAndMode(hh.Cid, hh.Fqdn, shs, sm, mode), errors
	}
	return types.NewHostHealthWithSystem(hh.Cid, hh.Fqdn, shs, sm), errors
}

func hostHealthToModel(hh types.HostHealth) *models.HostHealth {
	shs := make([]*models.ServiceHealth, 0, len(hh.Services()))
	for _, sh := range hh.Services() {
		status := models.ServiceStatus(sh.Status())
		shs = append(
			shs,
			&models.ServiceHealth{
				Name:            sh.Name(),
				Timestamp:       sh.Timestamp().Unix(),
				Status:          &status,
				Role:            models.ServiceRole(sh.Role()),
				Replicatype:     models.ServiceReplicaType(sh.ReplicaType()),
				ReplicaUpstream: sh.ReplicaUpstream(),
				ReplicaLag:      sh.ReplicaLag(),
				Metrics:         sh.Metrics(),
			},
		)
	}

	return &models.HostHealth{
		Cid:      hh.ClusterID(),
		Fqdn:     hh.FQDN(),
		Services: shs,
		System:   systemMetricsToModel(hh.System()),
		Status:   models.HostStatus(hh.Status()),
	}
}

func cpuMetricToModel(cpu *types.CPUMetric) *models.CPUMetrics {
	if cpu == nil {
		return nil
	}

	return &models.CPUMetrics{
		Timestamp: cpu.Timestamp,
		Used:      cpu.Used,
	}
}

func memoryMetricToModel(memory *types.MemoryMetric) *models.MemoryMetrics {
	if memory == nil {
		return nil
	}

	return &models.MemoryMetrics{
		Timestamp: memory.Timestamp,
		Total:     memory.Total,
		Used:      memory.Used,
	}
}

func diskMetricToModel(disk *types.DiskMetric) *models.DiskMetrics {
	if disk == nil {
		return nil
	}

	return &models.DiskMetrics{
		Timestamp: disk.Timestamp,
		Total:     disk.Total,
		Used:      disk.Used,
	}
}

func systemMetricsToModel(sm *types.SystemMetrics) *models.HostSystemMetrics {
	if sm == nil {
		return nil
	}

	return &models.HostSystemMetrics{
		CPU:  cpuMetricToModel(sm.CPU),
		Mem:  memoryMetricToModel(sm.Memory),
		Disk: diskMetricToModel(sm.Disk),
	}
}

func isValidCPUMetric(cpu *models.CPUMetrics) bool {
	if cpu == nil {
		return true
	}

	return cpu.Used > 0 && cpu.Timestamp > 0
}

func cpuMetricFromModel(cpu *models.CPUMetrics) (*types.CPUMetric, error) {
	if cpu == nil {
		return nil, nil
	}

	if !isValidCPUMetric(cpu) {
		return nil, xerrors.Errorf("invalid cpu metric: %v", *cpu)
	}

	return &types.CPUMetric{
		BaseMetric: types.BaseMetric{Timestamp: cpu.Timestamp},
		Used:       cpu.Used,
	}, nil
}

func isValidMemoryMetric(memory *models.MemoryMetrics) bool {
	if memory == nil {
		return true
	}

	return memory.Used > 0 && memory.Total > 0 && memory.Timestamp > 0
}

func memoryMetricFromModel(memory *models.MemoryMetrics) (*types.MemoryMetric, error) {
	if memory == nil {
		return nil, nil
	}
	if !isValidMemoryMetric(memory) {
		return nil, xerrors.Errorf("invalid memory metric: %v", *memory)
	}

	return &types.MemoryMetric{
		BaseMetric: types.BaseMetric{Timestamp: memory.Timestamp},
		Used:       memory.Used,
		Total:      memory.Total,
	}, nil
}

func isValidDiskMetric(disk *models.DiskMetrics) bool {
	if disk == nil {
		return true
	}

	return disk.Used > 0 && disk.Total > 0 && disk.Timestamp > 0
}

func diskMetricFromModel(disk *models.DiskMetrics) (*types.DiskMetric, error) {
	if disk == nil {
		return nil, nil
	}

	if !isValidDiskMetric(disk) {
		return nil, xerrors.Errorf("invalid disk metric: %v", *disk)
	}

	return &types.DiskMetric{
		BaseMetric: types.BaseMetric{Timestamp: disk.Timestamp},
		Used:       disk.Used,
		Total:      disk.Total,
	}, nil
}

func appendToErrors(errors *[]error, err error) {
	if *errors == nil {
		*errors = make([]error, 0)
	}

	*errors = append(*errors, err)
}

func systemMetricsFromModel(sm *models.HostSystemMetrics) (*types.SystemMetrics, []error) {
	if sm == nil {
		return nil, nil
	}

	var errors []error = nil

	cpu, err := cpuMetricFromModel(sm.CPU)
	if err != nil {
		appendToErrors(&errors, err)
	}

	memory, err := memoryMetricFromModel(sm.Mem)
	if err != nil {
		appendToErrors(&errors, err)
	}

	disk, err := diskMetricFromModel(sm.Disk)
	if err != nil {
		appendToErrors(&errors, err)
	}

	return &types.SystemMetrics{
		CPU:    cpu,
		Memory: memory,
		Disk:   disk,
	}, errors
}

func clusterHealthToModel(ch types.ClusterHealth) *models.ClusterHealth {
	status := models.ClusterStatus(ch.Status)
	return &models.ClusterHealth{
		Status:             &status,
		Timestamp:          ch.StatusTS.Unix(),
		Lastalivetimestamp: ch.AliveTS.Unix(),
	}
}

func unhealthyAggregatedInfoToModel(uai unhealthy.UAInfo) *models.UAInfo {
	res := &models.UAInfo{
		SLA:   &models.UAInfoSLA{},
		NoSLA: &models.UAInfoNoSLA{},
	}
	for k, v := range uai.StatusRecs {
		item := &models.UAHealthItems0{
			Count:    int64(v.Count),
			Status:   models.ClusterStatus(k.Status),
			Examples: v.Examples,
		}
		// It's for better client experience only.
		// If client will use raw parsing and processing it's useful to aggregate empty array instead of null.
		if item.Examples == nil {
			item.Examples = []string{}
		}
		if k.SLA {
			res.SLA.ByHealth = append(res.SLA.ByHealth, item)
		} else {
			res.NoSLA.ByHealth = append(res.NoSLA.ByHealth, item)
		}
	}
	for k, v := range uai.RWRecs {
		item := &models.UAAvailabilityItems0{
			Count:           int64(v.Count),
			Examples:        v.Examples,
			Readable:        k.Readable,
			Writable:        k.Writeable,
			UserfaultBroken: k.UserfaultBroken,
			NoReadCount:     int64(v.NoReadCount),
			NoWriteCount:    int64(v.NoWriteCount),
		}
		// It's for better client experience only.
		// If client will use raw parsing and processing it's useful to aggregate empty array instead of null.
		if item.Examples == nil {
			item.Examples = []string{}
		}
		if k.SLA {
			res.SLA.ByAvailability = append(res.SLA.ByAvailability, item)
		} else {
			res.NoSLA.ByAvailability = append(res.NoSLA.ByAvailability, item)
		}
	}
	for k, v := range uai.WarningGeoRecs {
		item := &models.UAWarningGeoItems0{
			Count:    int64(v.Count),
			Geo:      k.Geo,
			Examples: v.Examples,
		}
		// It's for better client experience only.
		// If client will use raw parsing and processing it's useful to aggregate empty array instead of null.
		if item.Examples == nil {
			item.Examples = []string{}
		}
		if k.SLA {
			res.SLA.ByWarningGeo = append(res.SLA.ByWarningGeo, item)
		} else {
			res.NoSLA.ByWarningGeo = append(res.NoSLA.ByWarningGeo, item)
		}
	}

	return res
}
