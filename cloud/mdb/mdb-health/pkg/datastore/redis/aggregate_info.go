package redis

import (
	"context"
	"encoding/json"
	"time"

	goredis "github.com/go-redis/redis/v8"
	"github.com/opentracing/opentracing-go"

	"a.yandex-team.ru/cloud/mdb/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/datastore"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (b *backend) SetAggregateInfo(ctx context.Context, ai datastore.AggregatedInfo, ttl time.Duration) error {
	span, _ := opentracing.StartSpanFromContext(ctx, "SetAggregateInfo")
	defer span.Finish()

	sai := storeAggregateInfo{
		Timestamp:         ai.Timestamp.Unix(),
		Total:             ai.Total,
		Alive:             ai.Alive,
		Degraded:          ai.Degraded,
		Unknown:           ai.Unknown,
		Dead:              ai.Dead,
		HostsTotal:        ai.RWInfo.HostsTotal,
		HostsRead:         ai.RWInfo.HostsRead,
		HostsWrite:        ai.RWInfo.HostsWrite,
		HostsBrokenByUser: ai.RWInfo.HostsBrokenByUser,
		DBTotal:           ai.RWInfo.DBTotal,
		DBRead:            ai.RWInfo.DBRead,
		DBWrite:           ai.RWInfo.DBWrite,
		DBBroken:          ai.RWInfo.DBBroken,
	}
	content, err := json.Marshal(sai)
	if err != nil {
		return xerrors.Errorf("failed to marshal storeAggregateInfo: %s", err)
	}
	pl := b.client.Pipeline()
	defer func() { _ = pl.Close() }()
	key := marshalAggregateKey(ai.AggType, ai.SLA, ai.UserFaultBroken)
	pl.HSet(ctx, key, marshalAggregateField(ai.CType, ai.Env), content)
	pl.Expire(ctx, key, ttl)
	_, err = pl.Exec(ctx)
	if err != nil {
		return semerr.WrapWithInternal(err, datastore.NotAllUpdatedErrText)
	}
	return nil
}

func (b *backend) LoadAggregateInfo(ctx context.Context) ([]datastore.AggregatedInfo, error) {
	span, _ := opentracing.StartSpanFromContext(ctx, "LoadAggregateInfo")
	defer span.Finish()

	aggMapCmd := make(map[string]*goredis.StringStringMapCmd, 4)

	pl := b.slaveClient.Pipeline()
	defer func() { _ = pl.Close() }()
	for _, sla := range []bool{false, true} {
		for _, userfaultBroken := range []bool{false, true} {
			for _, at := range []types.AggType{types.AggClusters, types.AggShards, types.AggGeoHosts} {
				key := marshalAggregateKey(at, sla, userfaultBroken)
				aggMapCmd[key] = b.slaveClient.HGetAll(ctx, key)
			}
		}
	}
	_, err := pl.Exec(ctx)
	if err != nil {
		return nil, semerr.WrapWithUnavailable(err, redisUnavailableErrText)
	}

	var result []datastore.AggregatedInfo
	for _, sla := range []bool{false, true} {
		for _, userfaultBroken := range []bool{false, true} {
			for _, at := range []types.AggType{types.AggClusters, types.AggShards, types.AggGeoHosts} {
				key := marshalAggregateKey(at, sla, userfaultBroken)
				aggMap, err := aggMapCmd[key].Result()
				if err != nil {
					continue
				}
				for f, v := range aggMap {
					ctype, env, err := unmarshalAggregateField(f)
					if err != nil {
						return nil, err
					}
					var sai storeAggregateInfo
					err = json.Unmarshal([]byte(v), &sai)
					if err != nil {
						return nil, err
					}
					result = append(result, datastore.AggregatedInfo{
						CType:           metadb.ClusterType(ctype),
						AggType:         at,
						SLA:             sla,
						Env:             env,
						UserFaultBroken: userfaultBroken,
						Timestamp:       time.Unix(sai.Timestamp, 0),
						Total:           sai.Total,
						Alive:           sai.Alive,
						Degraded:        sai.Degraded,
						Unknown:         sai.Unknown,
						Dead:            sai.Dead,
						RWInfo: types.DBRWInfo{
							HostsTotal:        sai.HostsTotal,
							HostsRead:         sai.HostsRead,
							HostsWrite:        sai.HostsWrite,
							HostsBrokenByUser: sai.HostsBrokenByUser,
							DBTotal:           sai.DBTotal,
							DBRead:            sai.DBRead,
							DBWrite:           sai.DBWrite,
							DBBroken:          sai.DBBroken,
						},
					})
				}
			}
		}
	}
	return result, nil
}
