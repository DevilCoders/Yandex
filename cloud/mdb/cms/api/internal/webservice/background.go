package webservice

import (
	"context"
	"time"

	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
)

func (srv *Service) RunBackgroundLoop(ctx context.Context) {
	ticker := time.NewTicker(time.Minute)
	defer ticker.Stop()

	ctx = ctxlog.WithFields(ctx,
		log.Bool("background", true))
	ctxlog.Info(ctx, srv.lg, "starting background loop")
	for {
		select {
		case <-ticker.C:
			srv.metrics.Dump(ctx)
		case <-ctx.Done():
			ctxlog.Info(ctx, srv.lg, "stopped background loop")
			return
		}
	}
}
