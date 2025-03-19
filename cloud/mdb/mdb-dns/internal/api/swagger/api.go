package swagger

import (
	"fmt"
	"time"

	"github.com/go-openapi/runtime/middleware"
	"github.com/prometheus/client_golang/prometheus"

	"a.yandex-team.ru/cloud/mdb/internal/httputil"
	"a.yandex-team.ru/cloud/mdb/internal/metadb"
	uprometheus "a.yandex-team.ru/cloud/mdb/internal/prometheus"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-dns/generated/swagger/models"
	"a.yandex-team.ru/cloud/mdb/mdb-dns/generated/swagger/restapi/operations/dns"
	"a.yandex-team.ru/cloud/mdb/mdb-dns/pkg/core"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

// API swagger interface to service
type API struct {
	logger log.Logger
	dnsm   *core.DNSManager
}

// New constructor for API
func New(logger log.Logger, dnsm *core.DNSManager) *API {
	return &API{
		logger: logger,
		dnsm:   dnsm,
	}
}

// GetPingHandler GET /v1/ping
func (api *API) GetPingHandler(params dns.PingParams) middleware.Responder {
	ctx := params.HTTPRequest.Context()
	if err := api.dnsm.IsReady(ctx); err == nil {
		return dns.NewPingOK()
	}
	return dns.NewPingServiceUnavailable()
}

// GetStatsHandler GET /v1/stats
func (api *API) GetStatsHandler(params dns.StatsParams) middleware.Responder {
	mfs, err := prometheus.DefaultGatherer.Gather()
	if err != nil {
		return dns.NewStatsInternalServerError().WithPayload(&models.Error{Message: err.Error()})
	}

	stats, err := uprometheus.FormatYasmStats(mfs)
	if err != nil {
		return dns.NewStatsInternalServerError().WithPayload(&models.Error{Message: err.Error()})
	}

	return dns.NewStatsOK().WithPayload(stats)
}

func prepareLiveDNSRoleStatus(now time.Time, ldr core.LiveDNSRole) *models.LiveDNSRoleStatus {
	var empty time.Time
	if ldr.UpdateTime == empty {
		return nil
	}

	return &models.LiveDNSRoleStatus{
		Lastfailedcycles: int64(ldr.LastFailedCycles),
		Pointlagsec:      int64(now.Sub(ldr.UpdateTime).Seconds()),
	}
}

func prepareLiveDNSStatus(now time.Time, ld core.LiveDNS) *models.LiveDNSStatus {
	ls := &models.LiveDNSStatus{
		Primary:           prepareLiveDNSRoleStatus(now, ld.Primary),
		Secondary:         prepareLiveDNSRoleStatus(now, ld.Secondary),
		Totalresolveerror: int64(ld.ResErr),
		Lastresolveerror:  int64(ld.LastResErr),
	}
	if ls.Primary == nil && ls.Secondary == nil {
		return nil
	}
	return ls
}

func dumpLiveStatus(status core.LiveStatus) *models.LiveStatus {
	now := time.Now()
	ls := &models.LiveStatus{
		Activeclients:    int64(status.ActiveClients),
		Lastfailedcycles: int64(status.LiveSlayer.Primary.LastFailedCycles) + int64(status.LiveCompute.Primary.LastFailedCycles),
		Slayerdns:        prepareLiveDNSStatus(now, status.LiveSlayer),
		Computedns:       prepareLiveDNSStatus(now, status.LiveCompute),
	}
	if status.LiveStatistic.UpdReq > 0 {
		ls.Updprimaryratio = float64(status.LiveStatistic.UpdPrim) * 100 / float64(status.LiveStatistic.UpdReq)
		ls.Updsecondaryratio = float64(status.LiveStatistic.UpdSec) * 100 / float64(status.LiveStatistic.UpdReq)
	}
	return ls
}

// GetLiveHandler GET /v1/live
func (api *API) GetLiveHandler(params dns.LiveParams) middleware.Responder {
	status := api.dnsm.GetLiveStatus()
	ls := dumpLiveStatus(status)
	return dns.NewLiveOK().WithPayload(ls)
}

// GetLiveHandler GET /v1/lives/{ctype}/{env}
func (api *API) GetLiveByClusterHandler(params dns.LiveByClusterParams) middleware.Responder {
	status := api.dnsm.GetLiveStatusByCluster(params.Ctype, params.Env)
	ls := dumpLiveStatus(status)
	return dns.NewLiveByClusterOK().WithPayload(ls)
}

// UpdatePrimaryDNSHandler PUT /v1/dns
func (api *API) UpdatePrimaryDNSHandler(params dns.UpdatePrimaryDNSParams) middleware.Responder {
	ctx := params.HTTPRequest.Context()

	bf := []log.Field{log.String("module", "SWAGGERAPI"), log.String("func", "UpdatePrimaryDNSParams")}
	body := httputil.RequestBodyFromContext(ctx)
	if body == nil {
		api.logger.Error("no request body context value found for", bf...)
		return dns.NewUpdatePrimaryDNSInternalServerError().WithPayload(
			&models.Error{Message: "failed to verify message"},
		)
	}

	cid := params.Body.Primarydns.Cid
	sid := params.Body.Primarydns.Sid
	bfa := append(bf,
		log.String("cid", cid),
		log.String("primary_fqdn", params.Body.Primarydns.Primaryfqdn),
	)
	if sid != "" {
		bfa = append(bfa, log.String("sid", sid))
	}
	pubkey, err := api.dnsm.GetPublicKey(ctx, cid)
	if err != nil {
		var msg string
		defer func() {
			api.logger.Warn(msg, append(bfa,
				log.Error(err),
			)...)
		}()
		if xerrors.Is(err, metadb.ErrNoResultRecords) {
			msg = "update failed to authenticate: no secret found"
			return dns.NewUpdatePrimaryDNSBadRequest().WithPayload(&models.Error{Message: msg})
		}

		if semerr.IsUnavailable(err) {
			msg = "metadb not available"
			return dns.NewUpdatePrimaryDNSServiceUnavailable().WithPayload(&models.Error{Message: msg})
		}

		msg = fmt.Sprintf("error while loading secret: %s", err)
		return dns.NewUpdatePrimaryDNSInternalServerError().WithPayload(&models.Error{Message: msg})
	}

	err = httputil.VerifyBodySignature(ctx, pubkey, body, params.XSignature)
	if err != nil {
		api.logger.Warn("failed to verify body signature", append(bfa,
			log.Error(err),
		)...)
		return dns.NewUpdatePrimaryDNSForbidden().WithPayload(&models.Error{Message: err.Error()})
	}

	ts := time.Unix(params.Body.Timestamp, 0)
	recs := params.Body.Primarydns
	accept, err := api.dnsm.UpdateDNS(ctx, ts, cid, sid, recs.Primaryfqdn, recs.Secondaryfqdn)
	if err != nil {
		msg := fmt.Sprintf("error while updating DNS: %s", err)
		api.logger.Warn(msg, append(bfa,
			log.Error(err),
		)...)
		if err == core.ErrResolveError {
			return dns.NewUpdatePrimaryDNSGatewayTimeout().WithPayload(&models.Error{Message: msg})
		}
		if err == core.ErrTimestampInvalid || err == core.ErrInvalidCid {
			return dns.NewUpdatePrimaryDNSBadRequest().WithPayload(&models.Error{Message: msg})
		}

		return dns.NewUpdatePrimaryDNSInternalServerError().WithPayload(&models.Error{Message: msg})
	}

	if accept {
		return dns.NewUpdatePrimaryDNSAccepted()
	}

	return dns.NewUpdatePrimaryDNSNoContent()
}
