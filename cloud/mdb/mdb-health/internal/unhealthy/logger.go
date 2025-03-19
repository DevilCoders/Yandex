package unhealthy

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
)

type Logger interface {
	Log(ctx context.Context, info UAInfo)
}

type logger struct {
	l     log.Logger
	ctype string
}

func (l *logger) Log(ctx context.Context, info UAInfo) {
	l.logRW(ctx, info.RWRecs)
	l.logStatus(ctx, info.StatusRecs)
	l.logWarningGeo(ctx, info.WarningGeoRecs)
}

func (l *logger) logStatus(ctx context.Context, statusRecs map[StatusKey]*UARecord) {
	for agg, rec := range statusRecs {
		if rec.Count == 0 || agg.Status == string(types.ClusterStatusAlive) {
			continue
		}
		nctx := ctxlog.WithFields(ctx,
			log.Bool("under_sla", agg.SLA),
			log.String("c_type", string(l.ctype)),
			log.String("env", agg.Env),
			log.String("status", agg.Status),
			log.Int("count", rec.Count),
			log.Strings("examples", rec.Examples),
		)
		ctxlog.Infof(nctx, l.l, "cluster status")
	}
}

func (l *logger) logWarningGeo(ctx context.Context, warningGeoRecs map[GeoKey]*UARecord) {
	for agg, rec := range warningGeoRecs {
		if rec.Count == 0 {
			continue
		}
		nctx := ctxlog.WithFields(ctx,
			log.Bool("under_sla", agg.SLA),
			log.String("c_type", l.ctype),
			log.String("env", agg.Env),
			log.String("geo", agg.Geo),
			log.Int("count", rec.Count),
			log.Strings("examples", rec.Examples),
		)
		ctxlog.Infof(nctx, l.l, "warning cluster geo")
	}
}

func (l *logger) logRW(ctx context.Context, rwRecs map[RWKey]*UARWRecord) {
	for agg, rec := range rwRecs {
		if rec.Count == 0 || (agg.Readable && agg.Writeable) {
			continue
		}
		nctx := ctxlog.WithFields(ctx,
			log.Bool("under_sla", agg.SLA),
			log.String("agg_type", string(agg.AggType)),
			log.String("c_type", l.ctype),
			log.String("env", agg.Env),
			log.Int("count", rec.Count),
			log.Int("noread_cnt", rec.NoReadCount),
			log.Int("nowrite_cnt", rec.NoWriteCount),
			log.Strings("examples", rec.Examples),
		)
		entity := string(agg.AggType)
		read := ""
		if !agg.Readable {
			read = "no "
		}
		write := ""
		if !agg.Writeable {
			write = "no "
		}
		broken := ""
		if agg.UserfaultBroken {
			broken = " and broken by usr"
		}
		ctxlog.Infof(nctx, l.l,
			"%s status %sread+%swrite%s",
			entity, read, write, broken)
	}
}

func NewLogger(base log.Logger, ctype metadb.ClusterType) Logger {
	return &logger{
		l:     base,
		ctype: string(ctype),
	}
}
