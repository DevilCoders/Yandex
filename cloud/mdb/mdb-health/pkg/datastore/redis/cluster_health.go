package redis

import (
	"context"
	"encoding/json"
	"strconv"
	"strings"
	"time"

	"github.com/opentracing/opentracing-go"

	"a.yandex-team.ru/cloud/mdb/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/internal/sentry"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/datastore"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/dbsupport"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (b *backend) GetClusterHealth(ctx context.Context, cid string) (types.ClusterHealth, error) {
	span, _ := opentracing.StartSpanFromContext(ctx, "GetClusterHealth")
	defer span.Finish()

	chReq := b.slaveClient.HMGet(ctx, marshalClusterHealthKey(cid), "status", "statusts", "lastalivets")
	chAns, err := chReq.Result()
	if err != nil {
		return types.NewUnknownClusterHealth(cid), semerr.WrapWithInternal(err, redisRequestFailedErrText)
	}
	ch := types.ClusterHealth{
		Cid:    cid,
		Status: types.ClusterStatusUnknown,
	}
	status, ok := chAns[0].(string)
	if ok {
		ch.Status = types.ClusterStatus(status)
	}
	statustsStr, ok := chAns[1].(string)
	if ok {
		if statusts, err := strconv.ParseInt(statustsStr, 0, 0); err == nil {
			ch.StatusTS = time.Unix(statusts, 0)
		}
	}
	alivetsStr, ok := chAns[2].(string)
	if ok {
		if alivets, err := strconv.ParseInt(alivetsStr, 0, 0); err == nil {
			ch.AliveTS = time.Unix(alivets, 0)
		}
	}
	return ch, nil
}

func (b *backend) parseClusterHealth(ctx context.Context, cid string, content interface{}) (parsedClusterInfo, error) {
	res := parsedClusterInfo{ctype: metadb.ClusterType("unknown_ctype")}
	answerArray, ok := content.([]interface{})
	if !ok || len(answerArray) < clusterHealthArrayHeaderSize {
		return res, xerrors.Errorf(
			"invalid LoadClusterHealth content, expecting array (with length greater than %d): %v",
			clusterHealthArrayHeaderSize,
			content,
		)
	}
	if (len(answerArray)-clusterHealthArrayHeaderSize)&1 == 1 {
		return res, xerrors.Errorf(
			"invalid LoadClusterHealth content, expecting array with even=(length - %d) length, got %d: %v",
			clusterHealthArrayHeaderSize,
			len(answerArray),
			content,
		)
	}
	headerIndex := 0
	fqdnsStr, ok := answerArray[headerIndex].(string)
	if !ok {
		return res, xerrors.Errorf("invalid LoadClusterHealth fqdns content %v", answerArray[headerIndex])
	}

	headerIndex++
	clusterTypeStr, ok := answerArray[headerIndex].(string)
	if !ok {
		return res, xerrors.Errorf("invalid LoadClusterHealth cluster type content %v", answerArray[headerIndex])
	}

	headerIndex++
	slaCluster, ok := answerArray[headerIndex].(string)
	if !ok {
		return res, xerrors.Errorf("invalid LoadClusterHealth sla content %v", answerArray[headerIndex])
	}

	headerIndex++
	slaListShards, ok := answerArray[headerIndex].(string)
	if !ok {
		return res, xerrors.Errorf("invalid LoadClusterHealth slashards %v", answerArray[headerIndex])
	}

	headerIndex++
	var nonaggregatable bool
	// we don't store nonaggregatable for aggregatable clusters,
	// so it is okay if we got nil for it
	if answerArray[headerIndex] != nil {
		isNA, ok := answerArray[headerIndex].(string)
		if !ok {
			return res, xerrors.Errorf("invalid LoadClusterHealth nonaggregatable. Should be a nil or a bool-string %v", answerArray[headerIndex])
		}
		nonaggregatable = isNA == "1"
	}

	res = parsedClusterInfo{
		ctype:           metadb.ClusterType(clusterTypeStr),
		fqdns:           strings.Split(fqdnsStr, " "),
		sla:             slaCluster == "1",
		slaShards:       strings.Split(slaListShards, " "),
		nonaggregatable: nonaggregatable,
		roleHosts:       make(map[string][]string),
		shardHosts:      make(map[string][]string),
		geoHosts:        make(map[string][]string),
		hostsHealth:     make(map[string][]types.ServiceHealth),
		hostsMode:       make(map[string]types.Mode),
	}
	for i := headerIndex + 1; i < len(answerArray); i += 2 {
		field, ok := answerArray[i].(string)
		if !ok {
			ctxlog.Warnf(ctx, b.logger, "for cid %s invalid LoadClusterHealth %d content key '%v'", cid, i, answerArray[i])
			continue
		}
		if b.parseMatchedField(ctx, cid, field, answerArray[i+1], rolePrefix, res.roleHosts) {
			continue
		}
		if b.parseMatchedField(ctx, cid, field, answerArray[i+1], shardidPrefix, res.shardHosts) {
			continue
		}
		if b.parseMatchedField(ctx, cid, field, answerArray[i+1], geoPrefix, res.geoHosts) {
			continue
		}
		fqdn, healthField, err := unmarshalFqdn(field)
		if err != nil {
			ctxlog.Warnf(ctx, b.logger, "failed to unmarshal fqdn health field %s: %s", field, err)
			continue
		}
		data := answerArray[i+1].(string)
		if strings.HasPrefix(healthField, servicePrefix) {
			service := strings.TrimPrefix(healthField, servicePrefix)
			sh, err := unmarshalHostService(fqdn, service, data)
			res.hostsHealth[fqdn] = append(res.hostsHealth[fqdn], sh)
			if err != nil {
				ctxlog.Warnf(ctx, b.logger, "for cid %s fqdn %s invalid LoadClusterHealth %d record: %s", cid, fqdn, i, err)
				continue
			}
		}
		if strings.HasPrefix(healthField, modePrefix) {
			if strings.TrimPrefix(healthField, modePrefix) == modeReadWrite {
				var mode storeMode
				err = json.Unmarshal([]byte(data), &mode)
				if err != nil {
					ctxlog.Warnf(ctx, b.logger, "failed to unmarshal mode for cluster %s, fqdn %s, data %s: %s", cid, fqdn, data, err)
					continue
				}
				res.hostsMode[fqdn] = types.Mode{
					Timestamp:       time.Unix(mode.Timestamp, 0),
					Read:            mode.Read,
					Write:           mode.Write,
					UserFaultBroken: mode.UserFaultBroken,
				}
			}
		}
	}
	ctxlog.Debugf(ctx, b.logger, " * parseClusterHealth info %v", res)
	return res, nil
}

func (b *backend) LoadFewClustersHealth(ctx context.Context, clusterType metadb.ClusterType, cursor string) (datastore.FewClusterHealthInfo, error) {
	reportToSentry := func(errToSentry error) error {
		sentry.GlobalClient().CaptureError(ctx, errToSentry, map[string]string{
			"cluster_type": string(clusterType),
			"method":       "LoadFewClustersHealth",
		})
		return errToSentry
	}
	span, _ := opentracing.StartSpanFromContext(ctx, "LoadFewClustersHealth")
	defer span.Finish()
	span.SetTag("cluster_type", clusterType)
	span.SetTag("cursor", cursor)

	switch cursor {
	case datastore.EndCursor:
		return datastore.FewClusterHealthInfo{}, xerrors.Errorf("call with end cursor")
	case "":
		cursor = datastore.EndCursor
	}

	loadRes := b.client.Eval(ctx, luaScriptLoadFewClustersHealth, []string{string(clusterType), cursor, strconv.Itoa(b.limRec)})
	res, err := loadRes.Result()
	if err != nil {
		return datastore.FewClusterHealthInfo{}, semerr.WrapWithInternal(err, redisRequestFailedErrText)
	}
	answerArray, ok := res.([]interface{})
	if !ok || len(answerArray) < 1 || len(answerArray)%4 != 1 {
		return datastore.FewClusterHealthInfo{}, reportToSentry(
			xerrors.Errorf("unexpected result len %d from luaScriptLoadFewClustersHealth", len(answerArray)),
		)
	}
	nextCursor, ok := answerArray[0].(string)
	if !ok {
		return datastore.FewClusterHealthInfo{}, reportToSentry(
			xerrors.Errorf("failed extract next cursor value '%v' from luaScriptLoadFewClustersHealth", answerArray[0]),
		)
	}
	recsPerCluster := 4
	resHealth := make([]types.ClusterHealth, len(answerArray)/recsPerCluster)
	resHostsMode := make(map[string]types.DBRWInfo)
	resNoSLAShards := make(map[string]types.DBRWInfo)
	resSLAShards := make(map[string]types.DBRWInfo)
	resShardEnv := make(map[string]string)
	resHostInfo := make(map[string]datastore.HostInfo)
	resGeoInfo := make(map[string][]string)
	//resGeoInfo := make(map[string]string)
	for i := 1; i < len(answerArray); i += recsPerCluster {
		cid, ok := answerArray[i].(string)
		if !ok {
			return datastore.FewClusterHealthInfo{}, reportToSentry(
				xerrors.Errorf("failed extract cid value '%v' from luaScriptLoadFewClustersHealth", answerArray[i]),
			)
		}
		env, ok := answerArray[i+1].(string)
		if !ok {
			return datastore.FewClusterHealthInfo{}, reportToSentry(
				xerrors.Errorf("failed extract env value '%v' from luaScriptLoadFewClustersHealth", answerArray[i+1]),
			)
		}

		rawClusterHealth := answerArray[i+2]
		info, err := b.parseClusterHealth(ctx, cid, rawClusterHealth)
		retClusterHealth := func(ch types.ClusterHealth) {
			ch.Env = env
			ch.SLA = info.sla
			ch.Nonaggregatable = info.nonaggregatable
			resHealth[i/recsPerCluster] = ch
			ctxlog.Debugf(ctx, b.logger, "aggregated cluster health %+v by info %+v", ch, info)
		}

		if err != nil {
			errWithContext := xerrors.Errorf(
				"parse cluster %s health '%v' from luaScriptLoadFewClustersHealth: %w",
				cid,
				rawClusterHealth,
				err,
			)
			sentry.GlobalClient().CaptureError(
				ctx,
				errWithContext,
				map[string]string{
					"cluster_id":   cid,
					"cluster_type": string(clusterType),
					"method":       "LoadFewClustersHealth",
				},
			)
			ctxlog.Errorf(ctx, b.logger, "failed: %s", errWithContext)

			ch := types.NewUnknownClusterHealth(cid)
			retClusterHealth(ch)
			continue
		}

		clusterTypeExtractor, ok := dbsupport.DBspec[clusterType]
		if !ok {
			ctxlog.Warnf(ctx, b.logger, "no cluster type extractor for cluster type %s", clusterType)
			ch := types.NewUnknownClusterHealth(cid)
			retClusterHealth(ch)
			continue
		}

		ch, err := clusterTypeExtractor.EvaluateClusterHealth(cid, info.fqdns, info.roleHosts, info.hostsHealth)
		if err != nil {
			ctxlog.Warnf(ctx, b.logger, "was mistake on evaluate cluster %s health: %s", cid, err)
		}
		retClusterHealth(ch)

		clusterStatus, ok := answerArray[i+3].(string)
		if !ok {
			return datastore.FewClusterHealthInfo{}, reportToSentry(
				xerrors.Errorf("failed extract cluster status value '%v' from luaScriptLoadFewClustersHealth", answerArray[i+3]),
			)
		}

		dbInfo, err := clusterTypeExtractor.EvaluateDBInfo(cid, clusterStatus, info.roleHosts, info.shardHosts, info.hostsMode, info.hostsHealth)
		if err != nil {
			ctxlog.Warnf(ctx, b.logger, "was mistake on evaluate cluster %s rw info: %s", cid, err)
		}
		if dbInfo.HostsTotal > 0 {
			resHostsMode[cid] = dbInfo
		}

		if info.nonaggregatable {
			// Do not calculate that cluster in SLA, shards, host, geo projections,
			// cause that cluster marked as nonaggregatable
			continue
		}
		// calculate information by shards
		for _, sid := range info.slaShards {
			oneShard := make(map[string][]string)
			oneShard[sid] = info.shardHosts[sid]
			dbInfo, err := clusterTypeExtractor.EvaluateDBInfo(cid, clusterStatus, info.roleHosts, oneShard, info.hostsMode, info.hostsHealth)
			if err != nil {
				ctxlog.Warnf(ctx, b.logger, "was mistake on evaluate shard %s rw info: %s", sid, err)
			}
			resSLAShards[sid] = dbInfo
		}

		for sid, shosts := range info.shardHosts {
			_, sla := resSLAShards[sid]
			resShardEnv[sid] = env
			if sla {
				continue
			}
			oneShard := make(map[string][]string)
			oneShard[sid] = shosts
			dbInfo, err := clusterTypeExtractor.EvaluateDBInfo(cid, clusterStatus, info.roleHosts, oneShard, info.hostsMode, info.hostsHealth)
			if err != nil {
				ctxlog.Warnf(ctx, b.logger, "was mistake on evaluate shard %s rw info: %s", sid, err)
			}
			resNoSLAShards[sid] = dbInfo
		}

		for geo, geoHosts := range info.geoHosts {
			for _, fqdn := range geoHosts {
				hs := types.HostStatusUnknown
				var ts time.Time
				serviceHealth, ok := info.hostsHealth[fqdn]
				if ok {
					hs, ts = clusterTypeExtractor.EvaluateHostHealth(serviceHealth)
				}

				userfaultBroken := false
				if mode, ok := info.hostsMode[fqdn]; ok && mode.UserFaultBroken {
					userfaultBroken = true
				}

				resHostInfo[fqdn] = datastore.HostInfo{
					Geo:             geo,
					TS:              ts,
					Status:          hs,
					Env:             env,
					SLA:             info.sla, // host belongs to SLA cluster, but not shard
					UserFaultBroken: userfaultBroken,
				}
			}
		}

		resGeoInfo[cid] = clusterTypeExtractor.EvaluateGeoAtWarning(info.roleHosts, info.shardHosts, info.geoHosts, info.hostsMode)
	}
	return datastore.FewClusterHealthInfo{
		Clusters:    resHealth,
		ClusterInfo: resHostsMode,
		NextCursor:  nextCursor,
		NoSLAShards: resNoSLAShards,
		SLAShards:   resSLAShards,
		ShardEnv:    resShardEnv,
		HostInfo:    resHostInfo,
		GeoInfo:     resGeoInfo,
	}, nil
}

func (b *backend) SaveClustersHealth(ctx context.Context, health []types.ClusterHealth, timeout time.Duration) error {
	span, _ := opentracing.StartSpanFromContext(ctx, "SaveClustersHealth")
	defer span.Finish()

	pl := b.client.Pipeline()
	defer func() { _ = pl.Close() }()
	for _, h := range health {
		key := marshalClusterHealthKey(h.Cid)
		m := make(map[string]interface{})
		m["status"] = string(h.Status)
		m["statusts"] = h.StatusTS.Unix()
		if h.Status == types.ClusterStatusAlive {
			m["lastalivets"] = h.StatusTS.Unix()
		}
		pl.HMSet(ctx, key, m)
		pl.Expire(ctx, key, timeout)
	}
	_, err := pl.Exec(ctx)
	if err != nil {
		return xerrors.Errorf("failed save cluster health for %d clusters: %s", len(health), err)
	}
	return nil
}
