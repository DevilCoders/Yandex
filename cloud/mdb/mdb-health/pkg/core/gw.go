package core

import (
	"context"
	"time"

	"github.com/karlseguin/ccache/v2"

	"a.yandex-team.ru/cloud/mdb/internal/leaderelection"
	"a.yandex-team.ru/cloud/mdb/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/internal/ready"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/datastore"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/dbsupport"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/healthstore"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/secretsstore"
	"a.yandex-team.ru/library/go/core/log"
)

const (
	customTTLUpperLimit     = 5 * time.Minute
	redisTierCacheFile      = "/tmp/.redis_tier.cache"
	loadFewClustersMaxTries = 5
	authFailedErrText       = "authentication failed"
)

var _ ready.Checker = &GeneralWard{}

// GeneralWard handles all high-level logic
type GeneralWard struct {
	logger        log.Logger
	ds            datastore.Backend
	hs            *healthstore.Store
	ss            secretsstore.Backend
	mdb           metadb.MetaDB
	keys          *ccache.Cache
	leaderElector leaderelection.LeaderElector

	cfg         GWConfig
	keyOverride []byte
	fqdn        string
	topologyUpd time.Time
	stat        statByCtype
}

// NewGeneralWard constructs GeneralWard
func NewGeneralWard(
	ctx context.Context,
	logger log.Logger,
	gwConf GWConfig,
	ds datastore.Backend,
	hs *healthstore.Store,
	ss secretsstore.Backend,
	mdb metadb.MetaDB,
	keyOverride []byte,
	initProcessing bool,
	leaderElector leaderelection.LeaderElector,
) *GeneralWard {
	gw := &GeneralWard{
		logger:        logger,
		ds:            ds,
		hs:            hs,
		ss:            ss,
		mdb:           mdb,
		keys:          ccache.New(ccache.Configure()),
		cfg:           gwConf,
		keyOverride:   keyOverride,
		leaderElector: leaderElector,
	}
	if initProcessing {
		gw.stat = newStat(gw.ds, false)
		for _, ct := range dbsupport.DBsupp {
			go gw.processLeadLoop(ctx, string(ct), gw.getProcessSLIFunc(ct))
		}
		go gw.processLeadLoop(ctx, "service", gw.serviceCycle)
		go gw.processLeadLoop(ctx, "collectDSStats", gw.collectDSStat)
	}
	return gw
}

// SecretTimeout returns current secret timeout setting
func (gw *GeneralWard) SecretTimeout() time.Duration {
	return gw.cfg.SecretTimeout
}

// HostHealthTimeout returns current host's health timeout setting
func (gw *GeneralWard) HostHealthTimeout() time.Duration {
	return gw.cfg.HostHealthTimeout
}

// IsReady return error if service not ready
func (gw *GeneralWard) IsReady(ctx context.Context) error {
	err := gw.ds.IsReady(ctx)
	if err != nil {
		return err
	}
	return gw.ss.IsReady(ctx)
}

// RegisterReadyChecker registers general ward ready checkers
func (gw *GeneralWard) RegisterReadyChecker(agg *ready.Aggregator) {
	agg.Register(gw.ds)
	agg.Register(gw.ss)
}

func (gw *GeneralWard) ForceProcessing(ctx context.Context, ct metadb.ClusterType, now time.Time) {
	if gw.stat == nil {
		gw.stat = newStat(gw.ds, true)
	}
	gw.serviceCycle(ctx, now)
	gw.processSLI(ctx, ct, now)
}
