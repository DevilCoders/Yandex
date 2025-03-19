package redis

import (
	"context"
	"encoding/json"
	"strings"
	"time"

	goredis "github.com/go-redis/redis/v8"
	"github.com/opentracing/opentracing-go"

	"a.yandex-team.ru/cloud/mdb/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/dbsupport"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/healthstore"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (b *backend) formatStoreHostHealth(ctx context.Context, pl goredis.Pipeliner, hh types.HostHealth, timeout time.Duration) error {
	// Prepare data before transaction
	mapOfHostServiceHealth := make(map[string]interface{}, len(hh.Services()))
	for _, sh := range hh.Services() {
		ssh := storeServiceHealth{
			Timestamp:       sh.Timestamp().Unix(),
			Status:          sh.Status(),
			Role:            sh.Role(),
			ReplicaType:     sh.ReplicaType(),
			ReplicaUpstream: sh.ReplicaUpstream(),
			ReplicaLag:      sh.ReplicaLag(),
			Metrics:         sh.Metrics(),
		}
		serviceHealthPackedContent, err := json.Marshal(ssh)
		if err != nil {
			return xerrors.Errorf("marshal storeServiceHealth: %w", err)
		}

		mapOfHostServiceHealth[sh.Name()] = serviceHealthPackedContent
	}
	ctxlog.Debugf(ctx, b.logger, "store host health information %+v, store health update map %s", hh, mapOfHostServiceHealth)

	mapOfSystemMetrics := make(map[string]interface{}, numOfSystemMetrics)
	if hh.System() != nil {
		var err error
		if hh.System().CPU != nil {
			cpu := storeCPUMetrics{
				Timestamp: hh.System().CPU.Timestamp,
				Used:      hh.System().CPU.Used,
			}
			mapOfSystemMetrics["cpu"], err = json.Marshal(cpu)
			if err != nil {
				return xerrors.Errorf("marshal cpu metrics: %w", err)
			}
		}
		if hh.System().Memory != nil {
			memory := storeMemoryMetrics{
				Timestamp: hh.System().Memory.Timestamp,
				Used:      hh.System().Memory.Used,
				Total:     hh.System().Memory.Total,
			}
			mapOfSystemMetrics["memory"], err = json.Marshal(memory)
			if err != nil {
				return xerrors.Errorf("marshal memory metrics: %w", err)
			}
		}
		if hh.System().Disk != nil {
			disk := storeDiskMetrics{
				Timestamp: hh.System().Disk.Timestamp,
				Used:      hh.System().Disk.Used,
				Total:     hh.System().Disk.Total,
			}
			mapOfSystemMetrics["disk"], err = json.Marshal(disk)
			if err != nil {
				return xerrors.Errorf("marshal disk metrics: %w", err)
			}
		}
	}

	var modePackedContent []byte
	if mode := hh.Mode(); mode != nil {
		sm := storeMode{
			Timestamp:       mode.Timestamp.Unix(),
			Read:            mode.Read,
			Write:           mode.Write,
			UserFaultBroken: mode.UserFaultBroken,
		}
		var err error
		modePackedContent, err = json.Marshal(sm)
		if err != nil {
			return xerrors.Errorf("marshal storeMode: %w", err)
		}
		ctxlog.Debugf(ctx, b.logger, "mode marshaled for store %s", modePackedContent)
	}

	for {
		select {
		case <-ctx.Done():
			return context.Canceled
		default:
			b.updateHostServicesHealth(ctx, pl, hh.FQDN(), hh.ClusterID(), mapOfHostServiceHealth, mapOfSystemMetrics, modePackedContent, timeout)
			return nil
		}
	}
}

func (b *backend) updateHostServicesHealth(
	ctx context.Context,
	pl goredis.Pipeliner,
	fqdn, _ string,
	mapOfHostServiceHealth map[string]interface{},
	mapOfSystemMetrics map[string]interface{},
	modePackedContent []byte,
	timeout time.Duration,
) {
	key := marshalHostHealthKey(fqdn)
	args := make(map[string]interface{})
	for serviceHealthName, serviceHealthPackedContent := range mapOfHostServiceHealth {
		args[marshalServiceField(serviceHealthName)] = serviceHealthPackedContent
	}
	for systemMetricName, systemMetricContent := range mapOfSystemMetrics {
		args[marshalSystemMetricField(systemMetricName)] = systemMetricContent
	}
	if len(modePackedContent) > 0 {
		args[marshalModeField(modeReadWrite)] = modePackedContent
	}
	pl.HSet(ctx, key, args)
	pl.Expire(ctx, key, timeout)
}

// StoreHostHealth stores host health to Redis
func (b *backend) StoreHostHealth(ctx context.Context, hh types.HostHealth, timeout time.Duration) error {
	span, ctx := opentracing.StartSpanFromContext(ctx, "StoreHostHealth")
	defer span.Finish()

	pl := b.client.Pipeline()
	defer func() { _ = pl.Close() }()

	if err := b.formatStoreHostHealth(ctx, pl, hh, timeout); err != nil {
		return xerrors.Errorf("formatStoreHostHealth: %w", err)
	}

	if _, err := pl.Exec(ctx); err != nil {
		return semerr.WrapWithUnavailable(err, redisUnavailableErrText)
	}
	return nil
}

func (b *backend) StoreHostsHealth(ctx context.Context, hostsHealth []healthstore.HostHealthToStore) error {
	span, ctx := opentracing.StartSpanFromContext(ctx, "StoreHostsHealth")
	defer span.Finish()

	pl := b.client.Pipeline()
	defer func() { _ = pl.Close() }()

	for _, hh := range hostsHealth {
		if err := b.formatStoreHostHealth(ctx, pl, hh.Health, hh.TTL); err != nil {
			ctxlog.Warn(ctx, b.logger, "cat not format host health", log.String("fqdn", hh.Health.FQDN()), log.Error(err))
		}
	}

	if _, err := pl.Exec(ctx); err != nil {
		return semerr.WrapWithUnavailable(err, redisUnavailableErrText)
	}
	return nil
}

// LoadHostsHealth loads health of multiple hosts from Redis
func (b *backend) LoadHostsHealth(ctx context.Context, fqdns []string) ([]types.HostHealth, error) {
	span, _ := opentracing.StartSpanFromContext(ctx, "LoadHostsHealth")
	defer span.Finish()

	hhs := make([]types.HostHealth, len(fqdns))
	hostReqList := make([]*goredis.Cmd, len(fqdns))

	pl := b.slaveClient.Pipeline()
	defer func() { _ = pl.Close() }()
	for i, fqdn := range fqdns {
		hostReqList[i] = pl.Eval(ctx, luaScriptLoadHostHealth, []string{fqdn})
	}
	_, err := pl.Exec(ctx)
	if err != nil {
		return nil, semerr.WrapWithUnavailable(err, redisUnavailableErrText)
	}

	for i, r := range hostReqList {
		hiRes, err := r.Result()
		if err != nil {
			ctxlog.Infof(ctx, b.logger, "unknown health for host %s: %+v", fqdns[i], err)
			hhs[i] = types.NewUnknownHostHealth(fqdns[i])
			continue
		}
		resLst := hiRes.([]interface{})
		if len(resLst) < 2 {
			ctxlog.Warnf(ctx, b.logger, "broken health content for host %s: %+v", fqdns[i], err)
			hhs[i] = types.NewUnknownHostHealth(fqdns[i])
			continue
		}
		cid := resLst[0].(string)
		ctype := metadb.ClusterType(resLst[1].(string))
		clusterTypeExtractor, okExtractor := dbsupport.DBspec[ctype]
		serviceCount := (len(resLst) - 2) / 2
		shs := make([]types.ServiceHealth, 0, serviceCount)
		var hostMode *types.Mode
		systemMetrics := &types.SystemMetrics{
			CPU:    nil,
			Memory: nil,
			Disk:   nil,
		}
		hasMetrics := false
		for is := 0; is < serviceCount; is++ {
			fqdnHealthField := resLst[2+2*is].(string)
			data := resLst[3+2*is].(string)
			fqdn, healthField, err := unmarshalFqdn(fqdnHealthField)
			if err != nil {
				ctxlog.Warnf(ctx, b.logger, "failed to unmarshal fqdn health field %s for fqdn %s: %s", fqdnHealthField, fqdns[i], err)
				continue
			}
			if fqdn != fqdns[i] {
				ctxlog.Warnf(ctx, b.logger, "fqdn %s and fqdn from health field %s is not same", fqdns[i], fqdn)
				continue
			}
			if strings.HasPrefix(healthField, servicePrefix) {
				service := strings.TrimPrefix(healthField, servicePrefix)
				sh, err := unmarshalHostService(fqdns[i], service, data)
				shs = append(shs, sh)
				if err != nil {
					ctxlog.Warnf(ctx, b.logger, "invalid service health for %s fqdn: %s", fqdns[i], err)
					continue
				}
			} else if strings.HasPrefix(healthField, systemMetricsPrefix) {
				metricName := strings.TrimPrefix(healthField, systemMetricsPrefix)
				switch metricName {
				case "cpu":
					var cpuMetric storeCPUMetrics
					err = json.Unmarshal([]byte(data), &cpuMetric)
					if err != nil {
						ctxlog.Warnf(ctx, b.logger, "failed to unmarshal cpu metrics for host fqdn %s, data %s: %s", fqdns[i], data, err)
						continue
					}
					systemMetrics.CPU = &types.CPUMetric{
						BaseMetric: types.BaseMetric{Timestamp: cpuMetric.Timestamp},
						Used:       cpuMetric.Used,
					}
					hasMetrics = true
				case "memory":
					var memoryMetric storeMemoryMetrics
					err = json.Unmarshal([]byte(data), &memoryMetric)
					if err != nil {
						ctxlog.Warnf(ctx, b.logger, "failed to unmarshal memory metrics for host fqdn %s, data %s: %s", fqdns[i], data, err)
						continue
					}
					systemMetrics.Memory = &types.MemoryMetric{
						BaseMetric: types.BaseMetric{Timestamp: memoryMetric.Timestamp},
						Used:       memoryMetric.Used,
						Total:      memoryMetric.Total,
					}
					hasMetrics = true
				case "disk":
					var diskMetric storeDiskMetrics
					err = json.Unmarshal([]byte(data), &diskMetric)
					if err != nil {
						ctxlog.Warnf(ctx, b.logger, "failed to unmarshal disk metrics for host fqdn %s, data %s: %s", fqdns[i], data, err)
						continue
					}
					systemMetrics.Disk = &types.DiskMetric{
						BaseMetric: types.BaseMetric{Timestamp: diskMetric.Timestamp},
						Used:       diskMetric.Used,
						Total:      diskMetric.Total,
					}
					hasMetrics = true
				default:
					ctxlog.Warnf(ctx, b.logger, "unknown system metric %s for fqdn %s", metricName, fqdns[i])
				}
			} else if strings.HasPrefix(healthField, modePrefix) {
				if strings.TrimPrefix(healthField, modePrefix) == modeReadWrite {
					var mode storeMode
					err = json.Unmarshal([]byte(data), &mode)
					if err != nil {
						ctxlog.Warnf(ctx, b.logger, "failed to unmarshal mode for host fqdn %s, data %s: %s", fqdns[i], data, err)
						continue
					}
					hostMode = &types.Mode{
						Timestamp:       time.Unix(mode.Timestamp, 0),
						Read:            mode.Read,
						Write:           mode.Write,
						UserFaultBroken: mode.UserFaultBroken,
					}
				}
			} else {
				ctxlog.Warnf(ctx, b.logger, "unknown field %s for fqdn %s", healthField, fqdn)
			}
		}

		if !hasMetrics {
			systemMetrics = nil
		}

		hostStatus := types.HostStatusUnknown
		if okExtractor {
			hostStatus, _ = clusterTypeExtractor.EvaluateHostHealth(shs)
		}
		hhs[i] = types.NewHostHealthWithStatusSystemAndMode(cid, fqdns[i], shs, systemMetrics, hostMode, hostStatus)
	}

	return hhs, nil
}
