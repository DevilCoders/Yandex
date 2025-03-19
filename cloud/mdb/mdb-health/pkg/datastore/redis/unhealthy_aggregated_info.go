package redis

import (
	"context"
	"encoding/json"
	"time"

	goredis "github.com/go-redis/redis/v8"
	"github.com/opentracing/opentracing-go"

	"a.yandex-team.ru/cloud/mdb/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-health/internal/unhealthy"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/datastore"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (b *backend) SetUnhealthyAggregatedInfo(ctx context.Context, ctype metadb.ClusterType, uai unhealthy.UAInfo, ttl time.Duration) error {
	span, _ := opentracing.StartSpanFromContext(ctx, "SetUnhealthyAggregatedInfo")
	defer span.Finish()
	span.SetTag("cluster_type", ctype)

	pl := b.client.Pipeline()
	defer func() { _ = pl.Close() }()

	statusKey := marshalUnhealthyStatusAggregateKey(ctype)
	pl.Del(ctx, statusKey)
	for key, rec := range uai.StatusRecs {
		suai := storeUnhealthyAggregatedInfo{
			Count: rec.Count,
		}
		if len(rec.Examples) > maxExampleCount {
			suai.Examples = rec.Examples[:maxExampleCount]
		} else {
			suai.Examples = rec.Examples
		}
		content, err := json.Marshal(suai)
		if err != nil {
			return xerrors.Errorf("failed to marshal storeUnhealthyStatusAggregateInfo: %s", err)
		}

		pl.HSet(ctx, statusKey, marshalUnhealthyStatusAggregateField(key.Env, key.SLA, key.Status), content)
	}
	pl.Expire(ctx, statusKey, ttl)

	warninGeoKey := marshalUnhealthyWarningGeoAggregateKey(ctype)
	pl.Del(ctx, warninGeoKey)
	for key, rec := range uai.WarningGeoRecs {
		suai := storeUnhealthyAggregatedInfo{
			Count: rec.Count,
		}
		if len(rec.Examples) > maxExampleCount {
			suai.Examples = rec.Examples[:maxExampleCount]
		} else {
			suai.Examples = rec.Examples
		}
		content, err := json.Marshal(suai)
		if err != nil {
			return xerrors.Errorf("failed to marshal storeUnhealthyWarningGeoAggregateInfo: %s", err)
		}

		pl.HSet(ctx, warninGeoKey, marshalUnhealthyWarningGeoAggregateField(key.Env, key.SLA, key.Geo), content)
	}
	pl.Expire(ctx, warninGeoKey, ttl)

	rwClustersKey := marshalUnhealthyRWAggregateKey(ctype, types.AggClusters)
	rwShardsKey := marshalUnhealthyRWAggregateKey(ctype, types.AggShards)
	pl.Del(ctx, rwShardsKey)
	pl.Del(ctx, rwClustersKey)

	for key, rec := range uai.RWRecs {
		suai := storeUnhealthyRWAggregatedInfo{
			Count:        rec.Count,
			NoReadCount:  rec.NoReadCount,
			NoWriteCount: rec.NoWriteCount,
		}
		if len(rec.Examples) > maxExampleCount {
			suai.Examples = rec.Examples[:maxExampleCount]
		} else {
			suai.Examples = rec.Examples
		}
		content, err := json.Marshal(suai)
		if err != nil {
			return xerrors.Errorf("failed to marshal storeUnhealthyRWAggregateInfo: %s", err)
		}
		if key.AggType == types.AggClusters {
			pl.HSet(ctx, rwClustersKey, marshalUnhealthyRWAggregateField(key.Env, key.SLA, key.Readable, key.Writeable, key.UserfaultBroken), content)
		} else {
			pl.HSet(ctx, rwShardsKey, marshalUnhealthyRWAggregateField(key.Env, key.SLA, key.Readable, key.Writeable, key.UserfaultBroken), content)
		}
	}
	pl.Expire(ctx, rwClustersKey, ttl)
	pl.Expire(ctx, rwShardsKey, ttl)
	_, err := pl.Exec(ctx)
	if err != nil {
		return semerr.WrapWithInternal(err, datastore.NotAllUpdatedErrText)
	}
	return nil
}

func (b *backend) LoadUnhealthyAggregatedInfo(ctx context.Context, ctype metadb.ClusterType, agg types.AggType) (unhealthy.UAInfo, error) {
	span, _ := opentracing.StartSpanFromContext(ctx, "LoadUnhealthyAggregatedInfo")
	defer span.Finish()
	span.SetTag("cluster_type", ctype)
	span.SetTag("agg_type", agg)

	pl := b.slaveClient.Pipeline()
	defer func() { _ = pl.Close() }()
	var clustersStatusMapCmd, clustersWarningGeoMapCmd, rwMapCmd *goredis.StringStringMapCmd
	if agg == types.AggClusters {
		clustersStatusMapCmd = b.slaveClient.HGetAll(ctx, marshalUnhealthyStatusAggregateKey(ctype))
		clustersWarningGeoMapCmd = b.slaveClient.HGetAll(ctx, marshalUnhealthyWarningGeoAggregateKey(ctype))
	}
	rwMapCmd = b.slaveClient.HGetAll(ctx, marshalUnhealthyRWAggregateKey(ctype, agg))
	_, err := pl.Exec(ctx)
	if err != nil {
		return unhealthy.UAInfo{}, semerr.WrapWithUnavailable(err, redisUnavailableErrText)
	}
	uai := unhealthy.UAInfo{
		StatusRecs:     make(map[unhealthy.StatusKey]*unhealthy.UARecord),
		RWRecs:         make(map[unhealthy.RWKey]*unhealthy.UARWRecord),
		WarningGeoRecs: make(map[unhealthy.GeoKey]*unhealthy.UARecord),
	}

	if clustersStatusMapCmd != nil {
		clustersStatusAggMap, err := clustersStatusMapCmd.Result()
		if err == nil {
			for f, v := range clustersStatusAggMap {
				env, sla, status, err := unmarshalUnhealthyStatusAggregateField(f)
				if err != nil {
					return unhealthy.UAInfo{}, err
				}
				var suai storeUnhealthyAggregatedInfo
				err = json.Unmarshal([]byte(v), &suai)
				if err != nil {
					return unhealthy.UAInfo{}, err
				}
				sk := unhealthy.StatusKey{
					SLA:    sla,
					Env:    env,
					Status: status,
				}
				uai.StatusRecs[sk] = &unhealthy.UARecord{
					Count:    suai.Count,
					Examples: suai.Examples,
				}
			}
		} else {
			ctxlog.Warnf(ctx, b.logger, "read unhealthy status aggregate: %s", err)
		}
	}

	if clustersWarningGeoMapCmd != nil {
		clustersWarningGeoAggMap, err := clustersWarningGeoMapCmd.Result()
		if err == nil {
			for f, v := range clustersWarningGeoAggMap {
				env, sla, geo, err := unmarshalUnhealthyWarningGeoAggregateField(f)
				if err != nil {
					return unhealthy.UAInfo{}, err
				}
				var suai storeUnhealthyAggregatedInfo
				err = json.Unmarshal([]byte(v), &suai)
				if err != nil {
					return unhealthy.UAInfo{}, err
				}
				gk := unhealthy.GeoKey{
					SLA: sla,
					Env: env,
					Geo: geo,
				}
				uai.WarningGeoRecs[gk] = &unhealthy.UARecord{
					Count:    suai.Count,
					Examples: suai.Examples,
				}
			}
		} else {
			ctxlog.Warnf(ctx, b.logger, "read unhealthy warning geo aggregate: %s", err)
		}
	}

	rwAggMap, err := rwMapCmd.Result()
	if err == nil {
		for f, v := range rwAggMap {
			env, sla, readable, writable, userfaultBroken := unmarshalUnhealthyRWAggregateField(f)
			var suai storeUnhealthyRWAggregatedInfo
			err = json.Unmarshal([]byte(v), &suai)
			if err != nil {
				return unhealthy.UAInfo{}, err
			}
			rw := unhealthy.RWKey{
				AggType:         agg,
				SLA:             sla,
				Env:             env,
				Readable:        readable,
				Writeable:       writable,
				UserfaultBroken: userfaultBroken,
			}
			uai.RWRecs[rw] = &unhealthy.UARWRecord{
				Count:        suai.Count,
				Examples:     suai.Examples,
				NoReadCount:  suai.NoReadCount,
				NoWriteCount: suai.NoWriteCount,
			}
		}
	} else {
		ctxlog.Warnf(ctx, b.logger, "read unhealthy RW aggregate: %s", err)
	}

	return uai, nil
}
