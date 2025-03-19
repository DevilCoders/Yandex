package main

import (
	"context"
	"fmt"
	"strings"
	"sync"
	"time"

	"github.com/spf13/cobra"

	"a.yandex-team.ru/cloud/mdb/internal/cli"
	computedns "a.yandex-team.ru/cloud/mdb/internal/compute/dns"
	"a.yandex-team.ru/cloud/mdb/internal/metadb"
	dnsconf "a.yandex-team.ru/cloud/mdb/mdb-dns/pkg/config"
	dnscore "a.yandex-team.ru/cloud/mdb/mdb-dns/pkg/core"
	"a.yandex-team.ru/cloud/mdb/mdb-dns/pkg/dnsapi"
	"a.yandex-team.ru/library/go/core/log"
)

var (
	cmdCleanRecords = initCleanRecords()

	flagMultiClean     bool
	flagRWRecords      bool
	flagShowAllStat    bool
	flagComputeNetwork bool
	flagClusterType    string
	flagEnv            string
	flagLimit          int
	flagOffset         int
)

const (
	wipDuration = 2 * time.Second

	flagNameMultiClean     = "multi-clean"
	flagNameRWRecords      = "rw-clean"
	flagNameShowAllStat    = "show-all-stat"
	flagNameComputeNetwork = "compute-network"
	flagNameClusterType    = "ctype"
	flagNameEnv            = "env"
)

func doClusterTypeAll() bool {
	return flagClusterType == "all"
}

func doClusterType() metadb.ClusterType {
	ctypeMap := map[string]string{
		"pg": "postgresql_cluster",
		"my": "mysql_cluster",
		"mg": "mongodb_cluster",
		"ch": "clickhouse_cluster",
		"rd": "redis_cluster",
		"es": "elasticsearch_cluster",
		"ms": "sqlserver_cluster",
	}
	return metadb.ClusterType(ctypeMap[flagClusterType])
}

func initCleanRecords() *cli.Command {
	cmd := &cobra.Command{
		Use:   "clean all CNAME records for qa envs",
		Short: "clean all CNAME records for qa envs, can work only for RO records on both with RW",
		Long:  "clean all CNAME records for qa envs, can work only for RO records on both with RW, also can calculate statistic of cluster",
	}

	cmd.Flags().BoolVar(
		&flagMultiClean,
		flagNameMultiClean,
		false,
		"clean records in concurent",
	)

	cmd.Flags().BoolVar(
		&flagRWRecords,
		flagNameRWRecords,
		false,
		"clean RW records too, it's more priority on update",
	)

	cmd.Flags().BoolVar(
		&flagShowAllStat,
		flagNameShowAllStat,
		false,
		"show current stat by all cluster types",
	)

	cmd.Flags().BoolVar(
		&flagComputeNetwork,
		flagNameComputeNetwork,
		false,
		"clean names from compute networks",
	)

	cmd.Flags().StringVar(
		&flagClusterType,
		flagNameClusterType,
		"all",
		"use only cluster type",
	)

	cmd.Flags().StringVar(
		&flagEnv,
		flagNameEnv,
		"qa",
		"use only cluster environment, use 'all' for any env",
	)

	cmd.Flags().IntVarP(
		&flagLimit,
		"limit",
		"L",
		100,
		"limited input clusters",
	)

	cmd.Flags().IntVarP(
		&flagOffset,
		"offset",
		"O",
		0,
		"use offset from input clusters",
	)

	return &cli.Command{Cmd: cmd, Run: cleanRecords}
}

type internal struct {
	ctx     context.Context
	log     log.Logger
	dac     *dnscore.DAClient
	mdb     metadb.MetaDB
	compute computedns.Client
	config  dnscore.DMConfig
}

func (ii *internal) requestCleanRecords(updList []dnscore.ToUpd, dryrun bool, wg *sync.WaitGroup) {
	if wg != nil {
		defer wg.Done()
	}
	doUpdate := func(req *dnsapi.RequestUpdate) {
		st := time.Now()
		if dryrun {
			ii.log.Infof("because of dry run do not clean %d records from network '%s' into DNS API by following request %s", len(req.Records), req.GroupID, req)
		} else {
			partial, err := ii.dac.DA.UpdateRecords(ii.ctx, req)
			if err != nil {
				ii.log.Warnf("update failed and splited by %d parts, network_id '%s', records %s, error: %v", len(partial), req.GroupID, req.Records, err)
				return
			}
			ii.log.Infof("update %d records from network '%s' into DNS API, duration=%s partialrecs=%d records=%s", len(req.Records), req.GroupID, time.Since(st), len(partial), req.Records)
		}
	}
	for len(updList) > 0 {
		if err := ii.ctx.Err(); err != nil {
			ii.log.Warnf("context error: %s", err)
			return
		}
		req := dnscore.PrepReqPart(ii.log, ii.dac, updList)
		if len(req.Records) == 0 {
			break
		}
		doUpdate(req)
		updList = updList[len(req.Records):]
	}
}

func (ii *internal) requestMultiCleanRecords(updList []dnscore.ToUpd, dryrun bool) {
	if len(updList) == 0 {
		return
	}
	numThreads := int(ii.dac.UpdThr)
	if numThreads > len(updList) {
		numThreads = len(updList)
	}
	maxRecs := (len(updList) + numThreads - 1) / numThreads
	updListParted := make([][]dnscore.ToUpd, numThreads)
	for i := 0; i < numThreads; i++ {
		updListParted[i] = make([]dnscore.ToUpd, 0, maxRecs)
	}
	for i, val := range updList {
		updListParted[i%numThreads] = append(updListParted[i%numThreads], val)
	}
	var wg sync.WaitGroup
	for i := 0; i < numThreads; i++ {
		wg.Add(1)
		ii.requestCleanRecords(updListParted[i], dryrun, &wg)
	}
	wg.Wait()
}

func (ii *internal) collectAllByNetworkList(clustersRev []metadb.ClusterRev) ([]dnscore.ToUpd, map[metadb.ClusterType]int) {
	updList := make([]dnscore.ToUpd, 0)
	stat := make(map[metadb.ClusterType]int)
	type cidInfo map[string]struct{}        // cid -> {}
	networksMap := make(map[string]cidInfo) // net_id -> cidInfo

	logTime := time.Now()
	cleanClusters := 0
	for i, cr := range clustersRev {
		if err := ii.ctx.Err(); err != nil {
			ii.log.Warnf("context error: %s", err)
			break
		}
		if time.Since(logTime) > wipDuration {
			ii.log.Infof("collecting info by cids [%d/%d]... networks: %d", i, len(clustersRev), len(networksMap))
			logTime = time.Now()
		}
		cid := cr.ClusterID
		ci, err := ii.mdb.ClusterInfo(ii.ctx, cid)
		if err != nil {
			ii.log.Warnf("unable get cluster info for %s cid: %s", cid, err)
			continue
		}
		net, ok := networksMap[ci.NetID]
		if !ok {
			net = make(cidInfo)
			networksMap[ci.NetID] = net
		}
		net[cid] = struct{}{}
	}

	netcount := 0
	for netid, clusters := range networksMap {
		if err := ii.ctx.Err(); err != nil {
			ii.log.Warnf("context error: %s", err)
			break
		}
		netcount++
		if time.Since(logTime) > wipDuration {
			ii.log.Infof("get records for proccess for network [%d/%d], records for update: %d", netcount, len(networksMap), len(updList))
			logTime = time.Now()
		}
		contentMap, err := ii.compute.ListRecords(ii.ctx, netid)
		if err != nil {
			ii.log.Warnf("failed to list network %s: %s", netid, err)
			continue
		}
		for cname, target := range contentMap {
			names := strings.Split(strings.TrimSuffix(cname, ii.dac.Suffix), ".")
			if len(names) < 3 {
				ii.log.Warnf("failed extract cid for cname %s", cname)
				continue
			}
			recCid := names[len(names)-3]
			recCid = strings.TrimPrefix(recCid, "c-")
			_, has := clusters[recCid]
			ii.log.Debugf("for cname %s extract cid '%s' and network has cid: %t", cname, recCid, has)
			if has {
				continue
			}
			updList = append(updList, dnscore.ToUpd{
				Fqdn:  cname,
				Netid: netid,
				Info: dnsapi.Update{
					CNAMEOld: target.Value,
				},
			})
		}
	}

	ii.log.Infof("collected for clean records %d and total clean clusters %d", uint(len(updList)), cleanClusters)
	return updList, stat
}

func (ii *internal) collectByResolve(clustersRev []metadb.ClusterRev) ([]dnscore.ToUpd, map[metadb.ClusterType]int) {
	updList := make([]dnscore.ToUpd, 0)
	stat := make(map[metadb.ClusterType]int)

	logTime := time.Now()
	cleanClusters := 0
	for i, cr := range clustersRev {
		if err := ii.ctx.Err(); err != nil {
			ii.log.Warnf("context error: %s", err)
			break
		}
		if time.Since(logTime) > wipDuration {
			ii.log.Infof("collecting records for clean [%d/%d]...", i, len(clustersRev))
			logTime = time.Now()
		}
		cid := cr.ClusterID
		ci, err := ii.mdb.ClusterAtRev(ii.ctx, cid, cr.Rev)
		if err != nil {
			ii.log.Warnf("unable get cluster info for %s cid %d rev: %s", cid, cr.Rev, err)
			continue
		}
		if !ci.Visible {
			continue
		}
		if flagEnv != "all" && ci.Environment != flagEnv {
			continue
		}
		netid := ""

		if !flagShowAllStat && !doClusterTypeAll() && doClusterType() != ci.Type {
			continue
		}

		var clusterUpdList []dnscore.ToUpd
		if flagRWRecords {
			clusterUpdList = dnscore.AddCleanRecords(ii.ctx, ii.dac, clusterUpdList, netid, dnscore.PrimaryFQDNByCid(ii.dac, ii.config, cid))
		}
		clusterUpdList = dnscore.AddCleanRecords(ii.ctx, ii.dac, clusterUpdList, netid, dnscore.SecondaryFQDNByCid(ii.dac, ii.config, cid))
		if ci.Type == metadb.ClickhouseCluster {
			clusterUpdList, err = dnscore.AddCleanShardsRecords(ii.ctx, ii.dac, ii.mdb, clusterUpdList, cid, netid, flagRWRecords, ii.config)
			if err != nil {
				ii.log.Warnf("failed AddCleanShardsRecords: %s", err)
			}
		}
		if len(clusterUpdList) == 0 {
			continue
		}

		stat[ci.Type] += len(clusterUpdList)
		if doClusterTypeAll() || doClusterType() == ci.Type {
			updList = append(updList, clusterUpdList...)
			cleanClusters++
		}
	}
	ii.log.Infof("collected for clean records %d and total clean clusters %d", uint(len(updList)), cleanClusters)
	return updList, stat
}

func cleanRecords(ctx context.Context, env *cli.Env, cmd *cobra.Command, args []string) {
	log := env.Logger

	cfgMdbdns := dnsconf.DefaultConfig()

	ii := &internal{
		ctx: ctx,
		log: log,
	}
	ii.ctx, ii.dac, ii.compute = createCommon(ii.ctx, log, cfgMdbdns, env, flagComputeNetwork)
	ii.mdb = createAndWaitMetadb(ctx, log)

	clustersRev, err := ii.mdb.ClustersRevs(ctx)
	if err != nil {
		log.Fatalf("failed to get clusters revs: %s", err)
	}

	log.Warnf("get %d clusters revs from metadb, input offset %d and limit %d", len(clustersRev), flagOffset, flagLimit)
	if flagOffset > len(clustersRev) {
		flagOffset = len(clustersRev)
	}
	if flagOffset+flagLimit > len(clustersRev) {
		flagLimit = len(clustersRev) - flagOffset
	}
	if flagLimit == 0 {
		log.Warnf("get %d clusters revs from metadb, used offset %d and limit %d out of scope", len(clustersRev), flagOffset, flagLimit)
		return
	}
	log.Infof("get %d clusters revs from metadb, used offset %d and limit %d", len(clustersRev), flagOffset, flagLimit)
	clustersRev = clustersRev[flagOffset : flagOffset+flagLimit]

	var updList []dnscore.ToUpd
	log.Infof("collecting records for clean...")
	var stat map[metadb.ClusterType]int
	if flagComputeNetwork {
		updList, stat = ii.collectAllByNetworkList(clustersRev)
	} else {
		updList, stat = ii.collectByResolve(clustersRev)
	}

	statFormat := ""
	for ctype, recCount := range stat {
		statFormat = statFormat + fmt.Sprintf("\t%s: %d\n", ctype, recCount)
	}

	log.Infof("collected stats by cluster type:\n%s\n", statFormat)
	if err := ctx.Err(); err != nil {
		log.Warnf("context error: %s", err)
		return
	}

	ts := time.Now()
	if flagMultiClean {
		ii.requestMultiCleanRecords(updList, env.IsDryRunMode())
	} else {
		ii.requestCleanRecords(updList, env.IsDryRunMode(), nil)
	}
	log.Infof("total time of update %s", time.Since(ts))
}
