package tags

import (
	"context"
	"encoding/hex"

	"github.com/opentracing/opentracing-go"
	"github.com/spf13/cast"

	"a.yandex-team.ru/cloud/mdb/internal/idempotence"
	"a.yandex-team.ru/cloud/mdb/internal/requestid"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
)

func WellKnownTags(ctx context.Context) map[string]string {
	tags := make(map[string]string)

	// Store specific context tags

	if id, ok := requestid.CheckedFromContext(ctx); ok {
		tags["ctx.request.id"] = id
	}

	if id, ok := idempotence.IncomingFromContext(ctx); ok {
		tags["ctx.idempotence.incoming.id"] = id.ID
		dst := make([]byte, hex.EncodedLen(len(id.Hash)))
		hex.Encode(dst, id.Hash)
		tags["ctx.idempotence.incoming.hash"] = string(dst)
	}

	if id, ok := idempotence.OutgoingFromContext(ctx); ok {
		tags["ctx.idempotence.outgoing.id"] = id
	}

	// Store all logging tags
	fields := ctxlog.ContextFields(ctx)
	for _, field := range fields {
		tags["log."+field.Key()] = cast.ToString(field.Any())
	}

	// Store all tracing tags
	if span := opentracing.SpanFromContext(ctx); span != nil {
		carrier := opentracing.TextMapCarrier{}
		if opentracing.GlobalTracer().Inject(span.Context(), opentracing.TextMap, carrier) == nil {
			for k, v := range carrier {
				tags["ot."+k] = v
			}
		}
	}

	return tags
}

func MergeTags(t1, t2 map[string]string) map[string]string {
	res := make(map[string]string, len(t1)+len(t2))
	for k, v := range t1 {
		res[k] = v
	}
	for k, v := range t2 {
		res[k] = v
	}

	return res
}
