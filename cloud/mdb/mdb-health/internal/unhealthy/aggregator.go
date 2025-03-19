package unhealthy

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types"
)

type Aggregator struct {
	logger Logger
	ctype  metadb.ClusterType
	info   UAInfo
}

func (ua *Aggregator) reset() {
	ua.info.RWRecs = make(map[RWKey]*UARWRecord)
	ua.info.StatusRecs = make(map[StatusKey]*UARecord)
	ua.info.WarningGeoRecs = make(map[GeoKey]*UARecord)
}

func (ua *Aggregator) AddCluster(underSLA bool, status types.ClusterStatus, env string, id string, info *types.DBRWInfo, warningGeos []string) {
	ua.addStatus(underSLA, status, env, id)
	ua.addWarningGeos(underSLA, warningGeos, env, id)
	ua.addRW(underSLA, types.AggClusters, env, id, info)
}

func (ua *Aggregator) AddShard(underSLA bool, env string, id string, info *types.DBRWInfo) {
	ua.addRW(underSLA, types.AggShards, env, id, info)
}

func (ua *Aggregator) addRW(underSLA bool, agg types.AggType, env string, id string, info *types.DBRWInfo) {
	if info == nil {
		return // we consider this OK and dont want to see that in logs
	}
	key := RWKey{
		SLA:             underSLA,
		AggType:         agg,
		Env:             env,
		Readable:        info.DBRead != 0,
		Writeable:       info.DBWrite != 0,
		UserfaultBroken: info.DBBroken != 0,
	}
	rec, ok := ua.info.RWRecs[key]
	if !ok {
		rec = &UARWRecord{}
		ua.info.RWRecs[key] = rec
	}
	rec.AddCount(info)
	if !(key.Writeable && key.Readable) {
		rec.AddExample(id)
	}
}

func (ua *Aggregator) addStatus(SLA bool, status types.ClusterStatus, env string, id string) {
	key := StatusKey{
		SLA:    SLA,
		Status: string(status),
		Env:    env,
	}
	rec, ok := ua.info.StatusRecs[key]
	if !ok {
		rec = &UARecord{}
		ua.info.StatusRecs[key] = rec
	}
	rec.AddCount()
	if status != types.ClusterStatusAlive {
		rec.AddExample(id)
	}
}

func (ua *Aggregator) addWarningGeos(SLA bool, geos []string, env string, id string) {
	for _, geo := range geos {
		key := GeoKey{
			SLA: SLA,
			Env: env,
			Geo: geo,
		}
		rec, ok := ua.info.WarningGeoRecs[key]
		if !ok {
			rec = &UARecord{}
			ua.info.WarningGeoRecs[key] = rec
		}
		rec.AddCount()
		rec.AddExample(id)
	}
}

func (ua *Aggregator) Log(ctx context.Context) {
	ua.logger.Log(ctx, ua.info)
	ua.reset()
}

func (ua *Aggregator) Info() UAInfo {
	return ua.info
}

func NewAggregator(logger Logger, ctype metadb.ClusterType) Aggregator {
	return Aggregator{
		logger: logger,
		ctype:  ctype,
		info: UAInfo{
			RWRecs:         make(map[RWKey]*UARWRecord),
			StatusRecs:     make(map[StatusKey]*UARecord),
			WarningGeoRecs: make(map[GeoKey]*UARecord),
		},
	}
}
