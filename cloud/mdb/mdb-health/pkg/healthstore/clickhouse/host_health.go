package clickhouse

import (
	"context"

	"github.com/opentracing/opentracing-go"
	"github.com/opentracing/opentracing-go/ext"
	tracelog "github.com/opentracing/opentracing-go/log"

	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/healthstore"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/healthstore/clickhouse/internal/models"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (b *backend) StoreHostsHealth(ctx context.Context, hostsHealth []healthstore.HostHealthToStore) error {
	span, ctx := opentracing.StartSpanFromContext(ctx, "StoreHostsHealth")
	defer span.Finish()

	tx, err := b.node.DBx().BeginTx(ctx, nil)
	if err != nil {
		ext.LogError(span, err)
		return xerrors.Errorf("begin tx: %w", err)
	}
	span.LogFields(tracelog.String("event", "got tx"))

	stm, err := tx.PrepareContext(ctx, queryStoreHostsHealth.Query)
	if err != nil {
		ext.LogError(span, err)
		return xerrors.Errorf("prepare statement: %w", err)
	}
	span.LogFields(tracelog.String("event", "prepared statement"))

	for _, hostHealth := range hostsHealth {
		ctxl := ctxlog.WithFields(ctx, log.String("fqdn", hostHealth.Health.FQDN()))
		intModel, err := models.HostHealthToInternal(hostHealth)
		if err != nil {
			ext.LogError(span, err, tracelog.String("fqdn", hostHealth.Health.FQDN()))
			ctxlog.Warn(ctxl, b.l, "can not prepare host health", log.Error(err))
			continue
		}
		args := intModel.ToInsert()
		ctxlog.Trace(ctxl, b.l, "write host health", log.Array("args", args))
		if _, err = stm.Exec(args...); err != nil {
			ext.LogError(span, err, tracelog.String("fqdn", hostHealth.Health.FQDN()))
			ctxlog.Warn(ctxl, b.l, "can not add host health", log.Error(err))
			continue
		}
	}
	span.LogFields(tracelog.String("event", "prepare data block"))

	if err = tx.Commit(); err != nil {
		ext.LogError(span, err)
		return xerrors.Errorf("execute query: %w", err)
	}
	span.LogFields(tracelog.String("event", "committed"))
	return nil
}
