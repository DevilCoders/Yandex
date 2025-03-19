package core

import (
	"context"
	"fmt"
	"math"
	"math/rand"
	"strings"
	"sync"
	"time"

	"github.com/opentracing/opentracing-go"

	"a.yandex-team.ru/cloud/mdb/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/internal/ready"
	"a.yandex-team.ru/cloud/mdb/internal/tracing"
	"a.yandex-team.ru/cloud/mdb/internal/tracing/tags"
	"a.yandex-team.ru/cloud/mdb/mdb-dns/pkg/dnsapi"
	"a.yandex-team.ru/cloud/mdb/mdb-dns/pkg/dnsq"
	l "a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

// Errors
var (
	ErrInvalidCid       = xerrors.NewSentinel("invalid cid")
	ErrRequestSidInfo   = xerrors.NewSentinel("failed to get information by sid")
	ErrTimestampInvalid = xerrors.NewSentinel("timestamp is invalid")
	ErrResolveError     = xerrors.NewSentinel("could not resolve record, perhaps lookup is out of service")
)

const (
	roUpdateSlowdownCycles = 5
)

type cidInfo struct {
	pubKey  []byte
	netid   string
	reqTime time.Time
	shards  map[string]string
	cType   metadb.ClusterType
	env     metadb.Environment
}

type cidCache struct {
	mux  sync.Mutex
	ci   map[string]*cidInfo
	ls   LiveStatus
	lsc  LiveStatistic
	lst  map[clusterKey]LiveStatus    // (cType, env) -> LiveStatus
	lsct map[clusterKey]LiveStatistic // (cType, env) -> LiveStatistic
}

// LiveDNSRole realtime live status of DNS client role
type LiveDNSRole struct {
	LastFailedCycles uint
	UpdateTime       time.Time
}

// LiveDNS realtime live status of DNS client
type LiveDNS struct {
	Primary    LiveDNSRole
	Secondary  LiveDNSRole
	LastResErr uint64
	ResErr     uint64
}

// LiveStatus realtime live status of serivce
type LiveStatus struct {
	ActiveClients uint
	LiveSlayer    LiveDNS
	LiveCompute   LiveDNS
	LiveStatistic LiveStatistic
}

// LiveStatistic update statistic info
type LiveStatistic struct {
	UpdReq  uint
	UpdPrim uint
	UpdSec  uint
}

// DNSManager handles all logic of service
type DNSManager struct {
	logger     l.Logger
	dmc        DMConfig
	dal        []*DAClient
	mdb        metadb.MetaDB
	cc         cidCache
	start      time.Time
	rand       *rand.Rand
	delTasks   []string
	delUpdTime time.Time
}

var _ ready.Checker = &DNSManager{}

// DAUse used dns api
type DAUse struct {
	Slayer  bool `json:"slayer" yaml:"slayer"`
	Compute bool `json:"compute" yaml:"compute"`
	Route53 bool `json:"route53" yaml:"route53"`
}

// DMConfig describe configuration for DNS Manager
type DMConfig struct {
	DAUse    DAUse         `json:"dnsapi" yaml:"dnsapi"`
	CertPath string        `json:"capath" yaml:"capath"`
	DNSTTL   time.Duration `json:"dnsttl" yaml:"dnsttl"`
	UpdDur   time.Duration `json:"upddur" yaml:"upddur"`
	CacheTTL time.Duration `json:"cachettl" yaml:"cachettl"`
	ClnDur   time.Duration `json:"cleandur" yaml:"cleandur"`
	ClnRng   time.Duration `json:"cleanrange" yaml:"cleanrange"`

	ClusterPrimaryFQDNTemplate   string `json:"cluster_primary_fqdn_template" yaml:"cluster_primary_fqdn_template"`
	ClusterSecondaryFQDNTemplate string `json:"cluster_secondary_fqdn_template" yaml:"cluster_secondary_fqdn_template"`
	ShardPrimaryFQDNTemplate     string `json:"shard_primary_fqdn_template" yaml:"shard_primary_fqdn_template"`
	ShardSecondaryFQDNTemplate   string `json:"shard_secondary_fqdn_template" yaml:"shard_secondary_fqdn_template"`
}

// DefaultDMConfig base DNS Manager configuration
func DefaultDMConfig() DMConfig {
	return DMConfig{
		DAUse: DAUse{
			Slayer:  true,
			Compute: false,
		},
		CertPath: "/path/to/ca.pem",
		DNSTTL:   18 * time.Second,
		UpdDur:   12 * time.Second,
		CacheTTL: 24 * time.Hour,
		ClnDur:   24 * time.Hour,
		ClnRng:   7 * 24 * time.Hour,

		ClusterPrimaryFQDNTemplate:   "c-%s.rw.%s",
		ClusterSecondaryFQDNTemplate: "c-%s.ro.%s",
		ShardPrimaryFQDNTemplate:     "%s.c-%s.rw.%s",
		ShardSecondaryFQDNTemplate:   "%s.c-%s.ro.%s",
	}
}

type clusterKey struct {
	cType metadb.ClusterType
	env   metadb.Environment
}
type fqdnBaseMap map[string]dnsapi.Update           // fqdnBase -> <old, new> target name
type groupUpdMapMap map[string]fqdnBaseMap          // network_id -> cnameMap
type clusterMapMapMap map[clusterKey]groupUpdMapMap // (cType, env) -> groupMap

// DNSClient DNS API client
type DNSClient int

// DNSClient possible DNS API clients
const (
	DNSSlayer DNSClient = iota
	DNSCompute
	DNSRoute53
)

func (dc DNSClient) String() string {
	switch dc {
	case DNSSlayer:
		return "slayer"
	case DNSCompute:
		return "compute"
	case DNSRoute53:
		return "route53"
	default:
		panic("unknown DNS client")
	}
}

// DAClient based options and some special for dns api backend
type DAClient struct {
	Client  DNSClient
	Suffix  string
	PubSuf  string
	DQ      dnsq.Client
	DA      dnsapi.Client
	MaxRec  uint
	UpdThr  uint
	primGr  clusterMapMapMap // primary and priority updates
	lazyGr  clusterMapMapMap // secondary and idle updates
	amux    sync.Mutex
	frsh    map[string]*freshnessInfo // information about last successful update
	recTTL  time.Duration
	resErr  map[clusterKey]uint64 // (cType, env) -> resErr
	lazyCnt uint
	primUpd time.Time
	updDur  time.Duration
}

type updType int

const (
	updAll updType = iota
	updPrimary
	updSecond
)

func (ut updType) String() string {
	switch ut {
	case updAll:
		return "all"
	case updPrimary:
		return "primary"
	case updSecond:
		return "secondary"
	default:
		panic("unknown update type")
	}
}

// NewDNSManager construct DNSManager struct
func NewDNSManager(
	ctx context.Context,
	logger l.Logger,
	dmc DMConfig,
	dal []*DAClient,
	mdb metadb.MetaDB,
) *DNSManager {
	now := time.Now()
	dm := &DNSManager{
		logger: logger,
		dmc:    dmc,
		dal:    dal,
		mdb:    mdb,
		cc: cidCache{
			ci:   make(map[string]*cidInfo),
			lst:  make(map[clusterKey]LiveStatus),
			lsct: make(map[clusterKey]LiveStatistic),
		},
		start: now,
		rand:  rand.New(rand.NewSource(now.UnixNano())),
	}

	for _, dac := range dm.dal {
		dac.primGr = make(clusterMapMapMap)
		dac.lazyGr = make(clusterMapMapMap)
		dac.frsh = make(map[string]*freshnessInfo)
		dac.resErr = make(map[clusterKey]uint64)
		dac.recTTL = dmc.DNSTTL
		dac.updDur = dmc.UpdDur
		go dm.cleanLoop(ctx, dmc.ClnDur, dac)
		for _, cType := range []metadb.ClusterType{
			metadb.PostgresqlCluster,
			metadb.MysqlCluster,
			metadb.MongodbCluster,
			metadb.ClickhouseCluster,
			metadb.RedisCluster,
			metadb.ElasticSearchCluster,
			metadb.SQLServerCluster,
			metadb.HadoopCluster,
			metadb.KafkaCluster,
			metadb.GreenplumCluster,
		} {
			for _, env := range []metadb.Environment{
				metadb.DevEnvironment,
				metadb.QaEnvironment,
				metadb.LoadEnvironment,
				metadb.ProdEnvironment,
				metadb.ComputeProdEnvironment,
			} {
				go dm.updateLoop(ctx, dac.updDur, dac, updPrimary, cType, env)
				go dm.updateLoop(ctx, dac.updDur, dac, updSecond, cType, env)
			}
		}
	}
	go dm.serviceLoop(ctx, 7*dmc.UpdDur)
	return dm
}

func (dnsm *DNSManager) updateLoop(ctx context.Context, dur time.Duration, dac *DAClient, ut updType, cType metadb.ClusterType, env metadb.Environment) {
	bf := []l.Field{
		l.String("module", "DNSManager"),
		l.String("func", "updateLoop"),
		l.String("api", dac.Client.String()),
		l.String("update_type", ut.String()),
		l.String("cluster_type", string(cType)),
		l.String("env", string(env)),
	}
	dnsm.logger.Info("start update loop", append(bf,
		l.Duration("process_each", dur),
	)...)
	upTick := time.NewTicker(dur)
	defer upTick.Stop()

	for {
		select {
		case <-upTick.C:
			dnsm.prepAndUpdateCycle(ctx, time.Now(), dac, ut, cType, env)
		case <-ctx.Done():
			dnsm.logger.Info("stop update loop", bf...)
			return
		}
	}
}

func (dnsm *DNSManager) serviceLoop(ctx context.Context, dur time.Duration) {
	bf := []l.Field{l.String("module", "DNSManager"), l.String("func", "serviceLoop")}
	dnsm.logger.Info("start service loop", append(bf,
		l.Duration("process_each", dur),
	)...)
	serviceTick := time.NewTicker(dur)
	defer serviceTick.Stop()

	for {
		select {
		case <-serviceTick.C:
			dnsm.serviceCycle(time.Now(), dur)
		case <-ctx.Done():
			dnsm.logger.Info("stop service loop", bf...)
			return
		}
	}
}

func (dnsm *DNSManager) calcRandDuration(dur time.Duration, errPenalty int) time.Duration {
	errFix := math.Log2(float64(1 + errPenalty))
	td := time.Second * time.Duration(dur.Seconds()*(1+dnsm.rand.Float64()*errFix))
	return td
}

func (dnsm *DNSManager) cleanLoop(ctx context.Context, dur time.Duration, dac *DAClient) {
	bf := []l.Field{
		l.String("module", "DNSManager"),
		l.String("func", "cleanLoop"),
		l.String("api", dac.Client.String()),
	}
	tid := dnsm.calcRandDuration(dur, 1)
	dnsm.logger.Info("schedule cleaning loop", append(bf,
		l.Duration("clean_after", tid),
	)...)
	timer := time.NewTimer(tid)

	var errPenalty int
	for {
		select {
		case <-timer.C:
			if !dnsm.cleanCycle(ctx, time.Now(), dac, errPenalty) {
				errPenalty++
			} else {
				errPenalty = 0
			}
			td := dnsm.calcRandDuration(dur, 1)
			dnsm.logger.Info("schedule cleaning loop", append(bf,
				l.Duration("clean_after", td),
			)...)
			timer.Reset(td)
		case <-ctx.Done():
			if !timer.Stop() {
				<-timer.C
			}
			dnsm.logger.Info("stop cleaning loop", bf...)
			return
		}
	}
}

// verifyAndUpdateTimestamp returns information from cache
func (dnsm *DNSManager) verifyAndUpdateTimestamp(cid, sid string, timestamp time.Time) (netid, sname string, err error) {
	dnsm.cc.mux.Lock()
	defer dnsm.cc.mux.Unlock()

	cache, ok := dnsm.cc.ci[cid]
	if !ok {
		return "", "", ErrInvalidCid
	}

	if cache.reqTime.After(timestamp) {
		return "", "", xerrors.Errorf("timestamp is too old in update for cid: %s, timestamp: %v, previous timestamp: %v: %w", cid, timestamp, cache.reqTime, ErrTimestampInvalid)
	}
	cache.reqTime = timestamp

	return cache.netid, cache.shards[sid], nil
}

// verifyAndUpdateShardName returns direct backend interface
func (dnsm *DNSManager) verifyAndUpdateShardName(ctx context.Context, cid, sid string) (string, error) {
	si, err := dnsm.mdb.ShardByID(ctx, sid)
	if err != nil {
		return "", ErrRequestSidInfo.Wrap(err)
	}
	dnsm.cc.mux.Lock()
	defer dnsm.cc.mux.Unlock()

	cache, ok := dnsm.cc.ci[cid]
	if !ok {
		return "", ErrInvalidCid
	}
	cache.shards[sid] = si.ShardName

	return si.ShardName, nil
}

// GetPublicKey return public key for cid and precache it
func (dnsm *DNSManager) GetPublicKey(ctx context.Context, cid string) ([]byte, error) {
	span, ctx := opentracing.StartSpanFromContext(ctx, "GetPublicKey", tags.ClusterID.Tag(cid))
	defer span.Finish()
	dnsm.cc.mux.Lock()
	defer dnsm.cc.mux.Unlock()

	pdata, exists := dnsm.cc.ci[cid]

	if exists {
		tags.PublicKeyCached.Set(span, true)
		return pdata.pubKey, nil
	}

	tags.PublicKeyCached.Set(span, false)
	pdata, err := dnsm.refreshCidCache(ctx, cid)
	if err != nil {
		tracing.SetErrorOnSpan(span, err)
		return nil, err
	}

	return pdata.pubKey, err
}

func (dnsm *DNSManager) refreshCidCache(ctx context.Context, cid string) (*cidInfo, error) {
	dnsm.logger.Debug("loading cluster info",
		l.String("module", "DNSManager"),
		l.String("func", "refreshCidCache"),
		l.String("cid", cid),
	)
	ci, err := dnsm.mdb.ClusterInfo(ctx, cid)
	if err != nil {
		return nil, err
	}
	cache := &cidInfo{
		pubKey: ci.PubKey,
		netid:  ci.NetID,
		shards: make(map[string]string),
		cType:  ci.CType,
		env:    ci.Env,
	}
	dnsm.logger.Debug("cluster info loaded",
		l.String("module", "DNSManager"),
		l.String("func", "refreshCidCache"),
		l.String("cid", cid),
		l.String("ctype", string(cache.cType)),
		l.String("env", string(cache.env)),
		l.String("netid", ci.NetID),
	)
	dnsm.cc.ci[cid] = cache
	return cache, nil
}

// GetClusterType returns cluster type and save it to cache
func (dnsm *DNSManager) GetClusterType(ctx context.Context, cid string) (metadb.ClusterType, error) {
	dnsm.cc.mux.Lock()
	defer dnsm.cc.mux.Unlock()

	cache, ok := dnsm.cc.ci[cid]
	if ok {
		return cache.cType, nil
	}
	cache, err := dnsm.refreshCidCache(ctx, cid)
	if err != nil {
		return "", err
	}
	return cache.cType, nil
}

// GetClusterType returns cluster type and save it to cache
func (dnsm *DNSManager) GetEnvironment(ctx context.Context, cid string) (metadb.Environment, error) {
	dnsm.cc.mux.Lock()
	defer dnsm.cc.mux.Unlock()

	cache, ok := dnsm.cc.ci[cid]
	if ok {
		return cache.env, nil
	}
	cache, err := dnsm.refreshCidCache(ctx, cid)
	if err != nil {
		return "", err
	}
	return cache.env, nil
}

// UpdateDNS process update DNS, return flag if update necessary
func (dnsm *DNSManager) UpdateDNS(ctx context.Context, ts time.Time, cid, sid, primary, secondary string) (upd bool, err error) {
	span, ctx := opentracing.StartSpanFromContext(
		ctx,
		"UpdateDNS",
		tags.ClusterID.Tag(cid),
		tags.ShardID.Tag(sid),
		tags.PrimaryFqdn.Tag(primary),
		tags.SecondaryFqdn.Tag(secondary))
	defer span.Finish()

	dnsm.logger.Trace("update DNS client request",
		l.String("module", "DNSManager"),
		l.String("func", "UpdateDNS"),
		l.String("cid", cid),
		l.String("primary", primary),
		l.String("secondary", secondary),
		l.Time("timestamp", ts),
	)
	netid, sname, err := dnsm.verifyAndUpdateTimestamp(cid, sid, ts)
	if err != nil {
		return false, err
	}
	if sid != "" && sname == "" {
		sname, err = dnsm.verifyAndUpdateShardName(ctx, cid, sid)
		if err != nil {
			return false, err
		}
	}

	cType, err := dnsm.GetClusterType(ctx, cid)
	if err != nil {
		tracing.SetErrorOnSpan(span, err)
		return false, err
	}
	tags.ClusterType.Set(span, string(cType))

	env, err := dnsm.GetEnvironment(ctx, cid)
	if err != nil {
		tracing.SetErrorOnSpan(span, err)
		return false, err
	}
	tags.ClusterEnv.Set(span, string(env))

	var willUpdPrim, willUpdSec, wasErrResolve bool
	for _, dac := range dnsm.dal {
		var primBase, secondBase string
		if sid == "" {
			primBase = PrimaryFQDNByCid(dac, dnsm.dmc, cid)
			secondBase = SecondaryFQDNByCid(dac, dnsm.dmc, cid)
		} else {
			primBase = PrimaryShardFQDN(dac, dnsm.dmc, cid, sname)
			secondBase = SecondaryShardFQDN(dac, dnsm.dmc, cid, sname)
		}

		primReq := ReqFQDN(primary, dac)
		secondReq := ReqFQDN(secondary, dac)

		updPrim, updSec, errRes := dnsm.updateRecsIfNecessary(ctx, ts, dac, netid, cid, primBase, primReq, secondBase, secondReq, cType, env)
		willUpdPrim = willUpdPrim || updPrim
		willUpdSec = willUpdSec || updSec
		wasErrResolve = wasErrResolve || errRes
	}

	tags.UpdatePrimaryFqdn.Set(span, willUpdPrim)
	tags.UpdateSecondaryFqdn.Set(span, willUpdSec)
	dnsm.liveWillUpd(willUpdPrim, willUpdSec, cType, env)

	if wasErrResolve {
		return willUpdPrim || willUpdSec, ErrResolveError
	}
	return willUpdPrim || willUpdSec, nil
}

func (dnsm *DNSManager) updateRecsIfNecessary(ctx context.Context, ts time.Time, dac *DAClient, netid, cid, primBase, primRec, secondBase, secondRec string, cType metadb.ClusterType, env metadb.Environment) (updPrim, updSec, errResolve bool) {
	var willUpdPrim, willUpdSec bool
	rum, ok := dnsm.getUpdateRecs(ctx, dac, netid, cid, primBase, primRec)
	wasErrResolve := !ok
	if len(rum) > 0 {
		willUpdPrim = markForUpdate(ts, dac, netid, rum, updPrimary, cType, env)
	} else if ok {
		markAsUpdated(ts, dac, updPrimary, netid, primBase)
	}

	if secondRec != "" {
		rum, ok := dnsm.getUpdateRecs(ctx, dac, netid, cid, secondBase, secondRec)
		wasErrResolve = wasErrResolve || !ok
		if len(rum) > 0 {
			willUpdSec = markForUpdate(ts, dac, netid, rum, updSecond, cType, env)
		} else if ok {
			markAsUpdated(ts, dac, updPrimary, netid, secondBase)
		}
	}

	if wasErrResolve {
		addResError(dac, cType, env)
	}
	return willUpdPrim, willUpdSec, wasErrResolve
}

func (dnsm *DNSManager) liveWillUpd(willUpdPrim, willUpdSec bool, cType metadb.ClusterType, env metadb.Environment) {
	dnsm.cc.mux.Lock()
	defer dnsm.cc.mux.Unlock()

	cKey := clusterKey{cType, env}
	lsct := dnsm.cc.lsct[cKey]

	dnsm.cc.lsc.UpdReq++
	lsct.UpdReq++
	if willUpdPrim {
		dnsm.cc.lsc.UpdPrim++
		lsct.UpdPrim++
	}
	if willUpdSec {
		dnsm.cc.lsc.UpdSec++
		lsct.UpdSec++
	}
	dnsm.cc.lsct[cKey] = lsct
}

func (dnsm *DNSManager) getUpdateRecs(ctx context.Context, dac *DAClient, netid, cid, basefqdn, newfqdn string) (rum fqdnBaseMap, ok bool) {
	curfqdn, ok := dac.DQ.LookupCNAME(ctx, basefqdn, netid)
	if !ok {
		if dac.Client != DNSSlayer {
			return rum, false
		}

		// take value from cache to aviod possible lose udp packet in slayer@
		dac.amux.Lock()
		defer dac.amux.Unlock()

		now := time.Now()
		fr := dac.getFreshness(now, basefqdn)
		// it's count as error if it repeated two time per period
		if now.Sub(fr.resErrTime) < time.Minute {
			return rum, false
		}
		dnsm.logger.Warn("ignore lookup cname error", []l.Field{
			l.String("module", "DNSManager"),
			l.String("func", "getUpdateRecs"),
			l.String("api", dac.Client.String()),
			l.String("netid", netid),
			l.String("fqdn", basefqdn),
		}...)
		fr.resErrTime = now
		curfqdn, ok = fr.expectCname, true
	}
	if curfqdn == newfqdn {
		return rum, true
	}
	dnsm.logger.Debug("prepare partial request",
		l.String("module", "DNSManager"),
		l.String("func", "getUpdateRecs"),
		l.String("api", dac.Client.String()),
		l.String("cid", cid),
		l.String("netid", netid),
		l.String("basefqdn", basefqdn),
		l.String("curfqdn", curfqdn),
		l.String("newfqdn", newfqdn),
	)

	rum = make(fqdnBaseMap)
	rum[basefqdn] = dnsapi.Update{
		CNAMEOld:    curfqdn,
		CNAMENew:    newfqdn,
		RequestSpan: opentracing.SpanFromContext(ctx),
	}
	return rum, true
}

type freshnessInfo struct {
	actRefresh     uint
	resolveBalance int
	initUpdTime    time.Time
	actUpdTime     time.Time
	resErrTime     time.Time
	expectCname    string
}

func (dac *DAClient) getFreshness(now time.Time, fqdn string) *freshnessInfo {
	fi, ok := dac.frsh[fqdn]
	if !ok {
		fi = &freshnessInfo{
			initUpdTime: now,
			actUpdTime:  now,
		}
		dac.frsh[fqdn] = fi
	}
	return fi
}

func (dac *DAClient) removeFreshnesInfo(rmList []string, cfg DMConfig) {
	dac.amux.Lock()
	defer dac.amux.Unlock()
	for _, cid := range rmList {
		delete(dac.frsh, PrimaryFQDNByCid(dac, cfg, cid))
		delete(dac.frsh, SecondaryFQDNByCid(dac, cfg, cid))
	}
}

func addResError(dac *DAClient, cType metadb.ClusterType, env metadb.Environment) {
	dac.amux.Lock()
	defer dac.amux.Unlock()
	dac.resErr[clusterKey{cType, env}]++
}

func markForUpdate(ts time.Time, dac *DAClient, netid string, rum fqdnBaseMap, ut updType, cType metadb.ClusterType, env metadb.Environment) bool {
	dac.amux.Lock()
	defer dac.amux.Unlock()

	uc := &dac.primGr
	if ut == updSecond {
		uc = &dac.lazyGr
	}
	cKey := clusterKey{cType, env}
	ug, ok := (*uc)[cKey]
	if !ok {
		ug = make(groupUpdMapMap)
		(*uc)[cKey] = ug
	}

	var updCount int
	for base, updInfo := range rum {
		fr := dac.getFreshness(ts, base)
		lostActive := ts.Sub(fr.actUpdTime) > 2*dac.recTTL
		fr.actUpdTime = ts
		if fr.expectCname != updInfo.CNAMENew || lostActive {
			fr.expectCname = updInfo.CNAMENew
			fr.actRefresh = 0
			fr.resolveBalance = 0
			fr.initUpdTime = ts
		}
		fr.resolveBalance /= 2
		fr.resolveBalance++
		if fr.actRefresh > 0 && ts.Sub(fr.initUpdTime).Seconds() <= math.Exp2(math.Min(5, float64(2+fr.actRefresh)))*dac.recTTL.Seconds() {
			continue
		}

		if fr.resolveBalance < 0 {
			continue
		}

		fr.actRefresh++
		updCount++

		updList, ok := ug[netid]
		if !ok {
			updList = make(fqdnBaseMap)
			ug[netid] = updList
		}
		updList[base] = updInfo
	}
	return updCount > 0
}

func markAsUpdated(ts time.Time, dac *DAClient, ut updType, netid, baseFQDN string) {
	// TODO: now it's time to use channel for this kind of stuff
	dac.amux.Lock()
	defer dac.amux.Unlock()

	fr := dac.getFreshness(ts, baseFQDN)
	fr.actUpdTime = ts
	fr.resolveBalance /= 2
	fr.resolveBalance--
}

// GetLiveStatus get live status of service
func (dnsm *DNSManager) GetLiveStatus() LiveStatus {
	dnsm.cc.mux.Lock()
	defer dnsm.cc.mux.Unlock()

	return dnsm.cc.ls
}

// GetLiveStatus get live status of service grouped by cluster type and environment
func (dnsm *DNSManager) GetLiveStatusByCluster(cType string, env string) LiveStatus {
	dnsm.cc.mux.Lock()
	defer dnsm.cc.mux.Unlock()

	return dnsm.cc.lst[clusterKey{metadb.ClusterType(cType), metadb.Environment(env)}]
}

// ToUpd info for update records
type ToUpd struct {
	Fqdn  string
	Netid string
	Info  dnsapi.Update
}

func PrepReqPart(log l.Logger, dac *DAClient, updList []ToUpd) *dnsapi.RequestUpdate {
	var updCount, del, add uint

	netid := updList[0].Netid
	primUp := make(dnsapi.Records)
	for _, upd := range updList {
		if upd.Netid != netid {
			break
		}
		updCount++
		if upd.Info.CNAMEOld != "" {
			del++
		}
		if upd.Info.CNAMENew != "" {
			add++
		}
		primUp[upd.Fqdn] = upd.Info
		if add+del >= dac.MaxRec {
			break
		}
	}
	log.Debug("prepare partial request", []l.Field{
		l.String("module", "DNSManager"),
		l.String("func", "PrepReqPart"),
		l.String("api", dac.Client.String()),
		l.String("netid", netid),
		l.UInt("upd", updCount),
		l.UInt("del", del),
		l.UInt("add", add),
	}...)
	return &dnsapi.RequestUpdate{
		Records: primUp,
		GroupID: netid,
	}
}

func (dnsm *DNSManager) prepareUpdList(now time.Time, dac *DAClient, ut updType, cType metadb.ClusterType, env metadb.Environment) (updList []ToUpd) {
	dac.amux.Lock()
	defer dac.amux.Unlock()

	uc := &dac.primGr
	if ut == updSecond {
		if dac.lazyCnt > 0 && now.Sub(dac.primUpd) < 3*dac.updDur/2 {
			dac.lazyCnt--
			return nil
		}
		dac.lazyCnt = roUpdateSlowdownCycles
		uc = &dac.lazyGr
	}

	cKey := clusterKey{cType, env}
	ug := (*uc)[cKey]
	for netid, recUpd := range ug {
		for fqdn, updInfo := range recUpd {
			updList = append(updList, ToUpd{
				Fqdn:  fqdn,
				Netid: netid,
				Info:  updInfo,
			})
		}
	}
	(*uc)[cKey] = make(groupUpdMapMap)

	if ut == updPrimary && len(updList) > 0 {
		dac.primUpd = now
	}

	return updList
}

func (dnsm *DNSManager) serviceCycle(now time.Time, activeDur time.Duration) {
	bf := []l.Field{l.String("module", "DNSManager"), l.String("func", "serviceCycle")}
	dnsm.cc.mux.Lock()
	defer dnsm.cc.mux.Unlock()

	var active uint
	activeByClusterType := make(map[clusterKey]uint)
	var rmList []string
	for cid, info := range dnsm.cc.ci {
		var noTime time.Time
		if info.reqTime == noTime {
			continue
		}
		lrDur := now.Sub(info.reqTime)

		if lrDur > dnsm.dmc.CacheTTL {
			rmList = append(rmList, cid)
			continue
		}

		if lrDur < activeDur {
			active++
			activeByClusterType[clusterKey{info.cType, info.env}]++
		}
	}

	dnsm.logger.Info("statistic", append(bf,
		l.UInt("total", uint(len(dnsm.cc.ci))),
		l.UInt("active", active),
		l.UInt("eliminated", uint(len(rmList))),
	)...)

	// clean active clients and old statistic
	for ck, val := range dnsm.cc.lst {
		val.ActiveClients = 0
		val.LiveStatistic = LiveStatistic{}
		dnsm.cc.lst[ck] = val
	}

	dnsm.cc.ls.ActiveClients = active
	for cType, a := range activeByClusterType {
		lst := dnsm.cc.lst[cType]
		lst.ActiveClients = a
		dnsm.cc.lst[cType] = lst
	}

	dnsm.cc.ls.LiveStatistic = dnsm.cc.lsc
	for cType, lsc := range dnsm.cc.lsct {
		lst := dnsm.cc.lst[cType]
		lst.LiveStatistic = lsc
		dnsm.cc.lst[cType] = lst
	}

	dnsm.cc.lsc = LiveStatistic{}
	dnsm.cc.lsct = make(map[clusterKey]LiveStatistic)

	for _, cid := range rmList {
		delete(dnsm.cc.ci, cid)
	}
	for _, dac := range dnsm.dal {
		dac.removeFreshnesInfo(rmList, dnsm.dmc)

		lf := &dnsm.cc.ls.LiveSlayer
		if dac.Client == DNSCompute {
			lf = &dnsm.cc.ls.LiveCompute
		}
		var resErr uint64
		for _, cResErr := range dac.resErr {
			resErr += cResErr
		}
		lf.LastResErr = resErr - lf.ResErr
		lf.ResErr = resErr
		if lf.LastResErr > 0 {
			dnsm.logger.Warn("was resolve errors by service cycle", append(bf,
				l.String("api", dac.Client.String()),
				l.UInt64("last_res_err", lf.LastResErr),
				l.UInt64("total_res_err", resErr),
			)...)
		}
	}

	for _, dac := range dnsm.dal {
		for cKey, ls := range dnsm.cc.lst {
			lf := &ls.LiveSlayer
			if dac.Client == DNSCompute {
				lf = &ls.LiveCompute
			}
			resErr := dac.resErr[cKey]
			lf.LastResErr = resErr - lf.ResErr
			lf.ResErr = resErr
			if lf.LastResErr > 0 {
				dnsm.logger.Warn("was resolve errors by service cycle by cluster type", append(bf,
					l.String("api", dac.Client.String()),
					l.UInt64("last_res_err", lf.LastResErr),
					l.UInt64("total_res_err", resErr),
					l.String("cluster_type", string(cKey.cType)),
					l.String("env", string(cKey.env)),
				)...)
			}
			dnsm.cc.lst[cKey] = ls
		}
	}
}

func (dnsm *DNSManager) updLastFailedCycles(now time.Time, wasError bool, dac *DAClient, ut updType) {
	dnsm.cc.mux.Lock()
	defer dnsm.cc.mux.Unlock()

	lf := &dnsm.cc.ls.LiveSlayer
	if dac.Client == DNSCompute {
		lf = &dnsm.cc.ls.LiveCompute
	}

	ldr := &lf.Primary
	if ut == updSecond {
		ldr = &lf.Secondary
	}

	if wasError {
		ldr.LastFailedCycles++
	} else {
		ldr.LastFailedCycles = 0
	}
	ldr.UpdateTime = now
}

func (dnsm *DNSManager) updLastFailedCyclesByCluster(now time.Time, wasError bool, dac *DAClient, ut updType, cType metadb.ClusterType, env metadb.Environment) {
	dnsm.cc.mux.Lock()
	defer dnsm.cc.mux.Unlock()

	cKey := clusterKey{cType, env}
	ls := dnsm.cc.lst[cKey]
	lf := &ls.LiveSlayer
	if dac.Client == DNSCompute {
		lf = &ls.LiveCompute
	}

	ldr := &lf.Primary
	if ut == updSecond {
		ldr = &lf.Secondary
	}

	if wasError {
		ldr.LastFailedCycles++
	} else {
		ldr.LastFailedCycles = 0
	}
	ldr.UpdateTime = now
	dnsm.cc.lst[cKey] = ls
}

func (dnsm *DNSManager) prepAndUpdateCycle(ctx context.Context, now time.Time, dac *DAClient, ut updType, cType metadb.ClusterType, env metadb.Environment) {
	if err := dnsm.IsReady(ctx); err != nil {
		dnsm.logger.Errorf("DNS Manager is not ready, err: %s", err)
		return
	}

	updList := dnsm.prepareUpdList(now, dac, ut, cType, env)

	var wasError bool
	if !dnsm.processDNS(ctx, dac, now, dnsOpUpdate, updList, ut, cType, env) {
		wasError = true
	}
	dnsm.updLastFailedCycles(now, wasError, dac, ut)
	dnsm.updLastFailedCyclesByCluster(now, wasError, dac, ut, cType, env)
}

func (dnsm *DNSManager) cleanCycle(ctx context.Context, now time.Time, dac *DAClient, errPenalty int) bool {
	if len(dnsm.delTasks) == 0 || time.Since(dnsm.delUpdTime) > 24*time.Hour {
		tasks, err := dnsm.mdb.GetTaskTypeByAction(ctx, "cluster-delete")
		if err != nil {
			dnsm.logger.Errorf("skip clean cycle, failed to get task types: %s", err)
			return false
		}
		dnsm.delUpdTime = time.Now()
		dnsm.delTasks = tasks
	}
	var wasError bool
	for _, tt := range dnsm.delTasks {
		if !dnsm.cleanGenCycle(ctx, now, dac, errPenalty, tt) {
			wasError = true
		}
	}
	return !wasError
}

func (dnsm *DNSManager) cleanGenCycle(ctx context.Context, now time.Time, dac *DAClient, errPenalty int, taskType string) bool {
	td := dnsm.calcRandDuration(dnsm.dmc.ClnRng, errPenalty)
	bf := []l.Field{
		l.String("module", "DNSManager"),
		l.String("func", "cleanGenCycle"),
		l.String("type", taskType),
		l.Duration("period", td),
		l.String("api", dac.Client.String()),
	}
	dnsm.logger.Info("load cids by period", bf...)
	lc, err := dnsm.mdb.LastOperationsByType(ctx, taskType, now.Add(-td))
	if err != nil {
		dnsm.logger.Error("failed to get list clusters", append(bf,
			l.NamedError("error", err),
		)...)
		return false
	}
	updLists := make(map[clusterKey][]ToUpd)
	for _, iter := range lc {
		cid := iter.ClusterID
		ci, err := dnsm.mdb.ClusterInfo(ctx, cid)
		if err != nil {
			dnsm.logger.Warn("unable get cluster info", append(bf,
				l.NamedError("error", err),
				l.String("cid", cid),
			)...)
			continue
		}

		cKey := clusterKey{ci.CType, ci.Env}
		updList := updLists[cKey]
		updList = AddCleanRecords(ctx, dac, updList, ci.NetID, PrimaryFQDNByCid(dac, dnsm.dmc, cid))
		updList = AddCleanRecords(ctx, dac, updList, ci.NetID, SecondaryFQDNByCid(dac, dnsm.dmc, cid))
		updList, err = AddCleanShardsRecords(ctx, dac, dnsm.mdb, updList, cid, ci.NetID, true, dnsm.dmc)
		if err != nil {
			dnsm.logger.Warn("unable AddCleanShardsRecords", append(bf,
				l.NamedError("error", err),
				l.String("cid", cid),
			)...)
		}
		updLists[cKey] = updList
	}
	var allClusterResult bool
	for cKey, updList := range updLists {
		dnsm.logger.Info("collected for clean", append(bf,
			l.String("cluster_type", string(cKey.cType)),
			l.UInt("recs_for_clean", uint(len(updList))),
			l.UInt("total_clusters", uint(len(lc))),
		)...)
		clusterResult := dnsm.processDNS(ctx, dac, now, dnsOpClean, updList, updAll, cKey.cType, cKey.env)
		allClusterResult = allClusterResult && clusterResult
	}
	return allClusterResult
}

func AddCleanShardsRecords(ctx context.Context, dac *DAClient, mdb metadb.MetaDB, updList []ToUpd, cid, netid string, addRW bool, cfg DMConfig) ([]ToUpd, error) {
	asi, err := mdb.ClusterShards(ctx, cid)
	if err != nil {
		return updList, err
	}
	for _, shard := range asi {
		if addRW {
			updList = AddCleanRecords(ctx, dac, updList, netid, PrimaryShardFQDN(dac, cfg, cid, shard.ShardName))
		}
		updList = AddCleanRecords(ctx, dac, updList, netid, SecondaryShardFQDN(dac, cfg, cid, shard.ShardName))
	}
	return updList, nil
}

func AddCleanRecords(ctx context.Context, dac *DAClient, updList []ToUpd, netid, basefqdn string) []ToUpd {
	curfqdn, ok := dac.DQ.LookupCNAME(ctx, basefqdn, netid)
	if !ok || curfqdn == "" {
		return updList
	}

	updInfo := dnsapi.Update{
		CNAMEOld: curfqdn,
		CNAMENew: "",
	}
	return append(updList, ToUpd{
		Fqdn:  basefqdn,
		Netid: netid,
		Info:  updInfo,
	})
}

type dnsOp int

const (
	dnsOpUpdate dnsOp = iota
	dnsOpClean
)

type upStat struct {
	okCnt   uint
	fCtxCnt uint
	failCnt uint
	updCnt  uint
	addCnt  uint
	delCnt  uint
}

func (dnsm *DNSManager) collectUpInfo(chCollect <-chan upStat, stat *upStat, wg *sync.WaitGroup) {
	defer wg.Done()
	for u := range chCollect {
		stat.okCnt += u.okCnt
		stat.fCtxCnt += u.fCtxCnt
		stat.failCnt += u.failCnt
		stat.updCnt += u.updCnt
		stat.addCnt += u.addCnt
		stat.delCnt += u.delCnt
	}
}

// send requestUpdate to channel and at first records from qSmall
type updateQueue struct {
	// qLarge collect request with lots of records
	qLarge []*dnsapi.RequestUpdate
	// qSmall collect request with few records and is high priority over qLarge
	qSmall []*dnsapi.RequestUpdate
	chIn   <-chan *dnsapi.RequestUpdate
	chOut  chan<- *dnsapi.RequestUpdate
}

func (uq *updateQueue) sendOrPreserve(ctx context.Context, ru *dnsapi.RequestUpdate) {
	select {
	case uq.chOut <- ru:
	default:
		if len(ru.Records) >= 10 {
			uq.qLarge = append(uq.qLarge, ru)
		} else {
			uq.qSmall = append(uq.qSmall, ru)
		}
	}
}

func (uq *updateQueue) processQueue(queue []*dnsapi.RequestUpdate) ([]*dnsapi.RequestUpdate, bool) {
	if len(queue) == 0 {
		return queue, false
	}
	id := len(queue) - 1
	select {
	case uq.chOut <- queue[id]:
		return queue[:id], true
	default:
		return queue, false
	}
}

func (uq *updateQueue) processLoop(ctx context.Context) {
	var proc bool
	td := time.Microsecond * 10
	timer := time.NewTimer(td)
	for {
		uq.qSmall, proc = uq.processQueue(uq.qSmall)
		if proc {
			continue
		}
		uq.qLarge, _ = uq.processQueue(uq.qLarge)

		timer.Reset(td)
		select {
		case msg := <-uq.chIn:
			if msg == nil {
				return
			}
			uq.sendOrPreserve(ctx, msg)
		case <-ctx.Done():
			return
		case <-timer.C:
		}
	}
}

func (dnsm *DNSManager) procUpdateLoop(ctx context.Context, dac *DAClient, chProcess <-chan *dnsapi.RequestUpdate, chCollect chan<- upStat, wg *sync.WaitGroup, chNext chan<- *dnsapi.RequestUpdate) {
	for req := range chProcess {
		dnsm.procRequest(ctx, dac, req, chCollect, wg, chNext)
	}
}

func spanForRequest(ctx context.Context, req *dnsapi.RequestUpdate) (opentracing.Span, context.Context) {
	opts := []opentracing.StartSpanOption{
		tags.NetworkID.Tag(req.GroupID),
	}
	if len(req.Records) == 1 {
		for cname, update := range req.Records {
			opts = append(
				opts,
				tags.CNAMEFqdn.Tag(cname),
				tags.NewFqdn.Tag(update.CNAMENew),
				tags.OldFqdn.Tag(update.CNAMEOld),
			)
			if update.RequestSpan != nil {
				opts = append(opts, opentracing.FollowsFrom(update.RequestSpan.Context()))
				return tracing.FollowSpanFromContext(
					ctx,
					"Process Single Record Request",
					opts...,
				)
			} else {
				return opentracing.StartSpanFromContext(
					ctx,
					"Process Record Request Without Origin",
					opts...,
				)
			}
		}
	}

	cnames := make([]string, 0, len(req.Records))
	for cname := range req.Records {
		cnames = append(cnames, cname)
	}
	opts = append(opts, tags.CNAMEFqdns.Tag(cnames))
	return opentracing.StartSpanFromContext(
		ctx,
		"Process Multiple Record Request",
		opts...,
	)
}

func (dnsm *DNSManager) procRequest(ctx context.Context, dac *DAClient, req *dnsapi.RequestUpdate, chCollect chan<- upStat, wg *sync.WaitGroup, chNext chan<- *dnsapi.RequestUpdate) {
	span, ctx := spanForRequest(ctx, req)
	defer span.Finish()

	defer wg.Done()
	bf := []l.Field{
		l.String("module", "DNSManager"),
		l.String("func", "procRequest"),
		l.String("api", dac.Client.String()),
		l.String("groupid", req.GroupID),
		l.Int("records_count", len(req.Records)),
	}
	if err := ctx.Err(); err != nil {
		chCollect <- upStat{fCtxCnt: 1}
		dnsm.logger.Error("context error", append(bf,
			l.NamedError("error", err),
		)...)
		return
	}

	dnsm.logger.Info("request for update records", append(bf,
		l.String("records", fmt.Sprintf("%v", req.Records)),
	)...)
	st := time.Now()
	parts, err := dac.DA.UpdateRecords(ctx, req)
	rt := time.Since(st)
	if err != nil {
		tracing.SetErrorOnSpan(span, err)
		if len(parts) == 0 {
			chCollect <- upStat{failCnt: 1}
			dnsm.logger.Error("process dns api update failed", append(bf,
				l.NamedError("error", err),
				l.Duration("req_duration", rt),
			)...)
			return
		}

		ps := make([]int, 0, len(parts))
		for _, p := range parts {
			ps = append(ps, len(p.Records))
		}
		dnsm.logger.Warn("has conflict on update, add parts for process", append(bf,
			l.NamedError("error", err),
			l.Int("parts_count", len(parts)),
			l.Ints("part_sizes", ps),
			l.Duration("req_duration", rt),
		)...)
		for _, p := range parts {
			wg.Add(1)
			chNext <- p
		}
		return
	}

	if rt > 3*time.Second {
		dnsm.logger.Warn("update records take a lot of time", append(bf,
			l.Duration("req_duration", rt),
		)...)
	} else {
		dnsm.logger.Debug("update records success", append(bf,
			l.Duration("req_duration", rt),
		)...)
	}

	var updCnt, addCnt, delCnt uint
	for _, r := range req.Records {
		updCnt++
		if r.CNAMEOld != "" {
			delCnt++
		}
		if r.CNAMENew != "" {
			addCnt++
		}
	}
	chCollect <- upStat{
		okCnt:  1,
		addCnt: addCnt,
		delCnt: delCnt,
		updCnt: updCnt,
	}
}

/// return is ok
func (dnsm *DNSManager) processDNS(ctx context.Context, dac *DAClient, now time.Time, op dnsOp, updList []ToUpd, ut updType, cType metadb.ClusterType, env metadb.Environment) bool {
	if len(updList) == 0 {
		return true
	}

	var opStr string
	var opMul uint
	switch op {
	case dnsOpUpdate:
		opStr = "update"
		opMul = 2
	case dnsOpClean:
		opStr = "clean"
		opMul = 1
	default:
		panic("unknown operation")
	}
	bf := []l.Field{
		l.String("module", "DNSManager"),
		l.String("func", "processDNS"),
		l.String("upd_type", ut.String()),
		l.String("operation", opStr),
		l.String("api", dac.Client.String()),
		l.String("cluster_type", string(cType)),
		l.String("env", string(env)),
	}
	ll := uint(len(updList))
	if ll > 2*dac.MaxRec {
		cycles := 1 + (opMul*ll-1)/dac.MaxRec
		dnsm.logger.Warn("a lot of records for update", append(bf,
			l.UInt("recs", uint(len(updList))),
			l.UInt("expect_calls", cycles),
			l.String("uptime", now.Sub(dnsm.start).String()),
		)...)
	}

	var callsCount uint
	var stat upStat
	before := time.Now()
	defer func() {
		if stat.okCnt+stat.failCnt+stat.fCtxCnt != 0 {
			dnsm.logger.Info("processed statistic", append(bf,
				l.UInt("success", stat.okCnt),
				l.UInt("failed", stat.failCnt),
				l.UInt("ctx_failed", stat.fCtxCnt),
				l.UInt("upd", stat.updCnt),
				l.UInt("add", stat.addCnt),
				l.UInt("del", stat.delCnt),
				l.UInt("api_calls", callsCount),
				l.String("duration", time.Since(before).String()),
			)...)
		}
	}()

	chProcess := make(chan *dnsapi.RequestUpdate, dac.UpdThr)
	chNext := make(chan *dnsapi.RequestUpdate, dac.UpdThr)
	chCollect := make(chan upStat, 1)
	uq := updateQueue{
		chIn:  chNext,
		chOut: chProcess,
	}
	var wgc sync.WaitGroup
	wgc.Add(1)
	go dnsm.collectUpInfo(chCollect, &stat, &wgc)

	go uq.processLoop(ctx)

	// parts of processed chunks
	var wgp sync.WaitGroup
	for len(updList) > 0 {
		if callsCount <= dac.UpdThr {
			go dnsm.procUpdateLoop(ctx, dac, chProcess, chCollect, &wgp, chNext)
		}
		ru := PrepReqPart(dnsm.logger, dac, updList)
		upd := len(ru.Records)
		dnsm.logger.Debug("send to processing channel", append(bf,
			l.UInt("left", uint(len(updList))),
			l.UInt("in_request", uint(upd)),
		)...)
		wgp.Add(1)
		chNext <- ru
		updList = updList[upd:]
		callsCount++
	}
	wgp.Wait()

	close(chProcess)
	close(chNext)
	close(chCollect)
	wgc.Wait()

	return stat.failCnt == 0
}

// PrimaryFQDNByCid get fqdn name by cid
func PrimaryFQDNByCid(dac *DAClient, cfg DMConfig, cid string) string {
	return fmt.Sprintf(cfg.ClusterPrimaryFQDNTemplate, cid, dac.Suffix)
}

// SecondaryFQDNByCid get fqdn name by cid
func SecondaryFQDNByCid(dac *DAClient, cfg DMConfig, cid string) string {
	return fmt.Sprintf(cfg.ClusterSecondaryFQDNTemplate, cid, dac.Suffix)
}

// PrimaryShardFQDN get fqdn name by cid and shard name
func PrimaryShardFQDN(dac *DAClient, cfg DMConfig, cid, sname string) string {
	return fmt.Sprintf(cfg.ShardPrimaryFQDNTemplate, sname, cid, dac.Suffix)
}

// SecondaryShardFQDN get fqdn name by cid and shard name
func SecondaryShardFQDN(dac *DAClient, cfg DMConfig, cid, sname string) string {
	return fmt.Sprintf(cfg.ShardSecondaryFQDNTemplate, sname, cid, dac.Suffix)
}

// ReqFQDN replaces public suffix to the private one if it is specified
func ReqFQDN(fqdn string, dac *DAClient) string {
	if dac.PubSuf == "" {
		return fqdn
	} else {
		return strings.TrimSuffix(fqdn, dac.PubSuf) + dac.Suffix
	}
}

// IsReady return error if service not ready
func (dnsm *DNSManager) IsReady(ctx context.Context) error {
	return dnsm.mdb.IsReady(ctx)
}
