package core

import (
	"context"
	"time"

	"github.com/opentracing/opentracing-go"

	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
)

func (gw *GeneralWard) processLeadLoop(ctx context.Context, service string, processFunc func(context.Context, time.Time)) {
	timer := time.NewTicker(gw.cfg.LeadTimeout)
	defer timer.Stop()
	ctxlog.Infof(ctx, gw.logger, "init processLeadLoop activity for %s for %s", service, gw.cfg.LeadTimeout)

	for {
		select {
		case <-timer.C:
			processFunc(ctx, time.Now())
		case <-ctx.Done():
			ctxlog.Info(ctx, gw.logger, "stop service loop")
			return
		}
	}
}

func (gw *GeneralWard) serviceCycle(ctx context.Context, _ time.Time) {
	ctxlog.Debug(ctx, gw.logger, "serviceCycle call")
	if err := gw.ds.IsReady(ctx); err != nil {
		ctxlog.Warn(ctx, gw.logger, "service cycle skip, datastore not ready", log.Error(err))
		return
	}

	span, ctx := opentracing.StartSpanFromContext(ctx, "Service Cycle")
	defer span.Finish()

	if err := gw.updateSLI(ctx); err != nil {
		ctxlog.Warnf(ctx, gw.logger, "failed to update SLI metrics: %s", err)
	}

	now := time.Now()
	if !gw.leaderElector.IsLeader(ctx) {
		gw.logger.Info("service cycle skip, no leader flag")
		return
	}

	if now.Sub(gw.topologyUpd) < gw.cfg.UpdTopologyTimeout {
		return
	}

	if err := gw.mdb.IsReady(ctx); err != nil {
		ctxlog.Infof(ctx, gw.logger, "update topology skip, metadb not ready: %s", err)
		return
	}

	if err := gw.updateTopology(ctx, now); err != nil {
		ctxlog.Infof(ctx, gw.logger, "service cycle failed update topology: %+v", err)
		return
	}
}

func (gw *GeneralWard) collectDSStat(ctx context.Context, _ time.Time) {
	ctxlog.Debug(ctx, gw.logger, "collectDSStat call")
	gw.ds.WriteStats()
}
