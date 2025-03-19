package redis

import (
	"context"
	"strings"

	goredis "github.com/go-redis/redis/v8"
	"github.com/opentracing/opentracing-go"

	"a.yandex-team.ru/cloud/mdb/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/dbspecific"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/dbsupport"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
)

func (b *backend) GetHostNeighboursInfo(ctx context.Context, fqdns []string) (map[string]types.HostNeighboursInfo, error) {
	span, _ := opentracing.StartSpanFromContext(ctx, "GetHostNeighboursInfo")
	defer span.Finish()

	hni := make(map[string]types.HostNeighboursInfo, len(fqdns))
	hostReqList := make([]*goredis.Cmd, len(fqdns))

	pl := b.slaveClient.Pipeline()
	defer func() { _ = pl.Close() }()
	for i, fqdn := range fqdns {
		hostReqList[i] = pl.Eval(ctx, luaScriptLoadDBHostsFunction, []string{fqdn})
	}
	_, err := pl.Exec(ctx)
	if err != nil {
		return nil, semerr.WrapWithUnavailable(err, redisUnavailableErrText)
	}

	for i, r := range hostReqList {
		fqdn := fqdns[i]
		hiRes, err := r.Result()
		if err != nil {
			ctxlog.Infof(ctx, b.logger, "error for load dbhost %s info: %+v", fqdn, err)
			continue
		}
		resLst := hiRes.([]interface{})
		baseFieldsCount := 7
		if len(resLst) < baseFieldsCount {
			ctxlog.Infof(ctx, b.logger, "no information about host %s unmarshaled correctly", fqdn)
			continue
		}
		ctype := metadb.ClusterType(resLst[2].(string))
		extractor, okExtractor := dbsupport.DBspec[ctype]
		if !okExtractor {
			ctxlog.Infof(ctx, b.logger, "not supported for health cluster type %s", ctype)
			continue
		}
		haCluster, _ := resLst[4].(string)
		haShard := false
		sid, _ := resLst[1].(string)
		if sid != "" {
			slaListShards, ok := resLst[5].(string)
			if ok {
				slaShards := strings.Split(slaListShards, " ")
				for _, shard := range slaShards {
					if shard == sid {
						haShard = true
						break
					}
				}
			}
		}
		info := types.HostNeighboursInfo{
			Cid:       resLst[0].(string),
			Sid:       sid,
			Env:       resLst[3].(string),
			HACluster: haCluster == "1",
			HAShard:   haShard,
		}
		localHosts := unmarshalFQDNList(resLst[6].(string))
		roleHosts := make(map[string][]string)
		hostsHealth := make(map[string][]types.ServiceHealth)
		for i := baseFieldsCount; i < len(resLst); i += 2 {
			field, ok := resLst[i].(string)
			if !ok {
				ctxlog.Warnf(ctx, b.logger, "fqdn %s invalid luaScriptLoadClusterHostsHealthFunction content in field %d (%v)", fqdn, i, resLst[i])
				continue
			}
			if b.parseMatchedField(ctx, info.Cid, field, resLst[i+1], rolePrefix, roleHosts) {
				continue
			}
			hostfqdn, healthField, err := unmarshalFqdn(field)
			if err != nil {
				ctxlog.Warnf(ctx, b.logger, "failed to unmarshal fqdn health field %s: %s", field, err)
				continue
			}
			data := resLst[i+1].(string)
			if strings.HasPrefix(healthField, servicePrefix) {
				service := strings.TrimPrefix(healthField, servicePrefix)
				sh, err := unmarshalHostService(hostfqdn, service, data)
				hostsHealth[hostfqdn] = append(hostsHealth[hostfqdn], sh)
				if err != nil {
					ctxlog.Warnf(ctx, b.logger, "for cid %s fqdn %s invalid LoadClusterHealth %d record: %s", info.Cid, hostfqdn, i, err)
					continue
				}
			}
		}

		info.Roles = dbspecific.CollectRoles(fqdn, roleHosts)
		fqdns := make(map[string]struct{})
		if info.Sid == "" {
			for _, role := range info.Roles {
				for _, h := range roleHosts[role] {
					if h == fqdn {
						continue
					}
					fqdns[h] = struct{}{}
				}
			}
		} else {
			for _, h := range localHosts {
				if h == fqdn {
					continue
				}
				fqdns[h] = struct{}{}
			}
		}
		delete(fqdns, fqdn)
		info.SameRolesTotal = len(fqdns)
		info.SameRolesAlive, info.SameRolesTS = dbspecific.CountAliveHosts(extractor, fqdns, hostsHealth)
		hni[fqdn] = info
	}

	return hni, nil
}
