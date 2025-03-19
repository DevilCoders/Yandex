package tracing

import (
	"context"

	goredis "github.com/go-redis/redis/v8"
	"github.com/opentracing/opentracing-go"
	"github.com/opentracing/opentracing-go/log"

	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/tracing/tags"
	"a.yandex-team.ru/cloud/mdb/internal/tracing"
)

type Tracer struct{}

var _ goredis.Hook = &Tracer{}

func OnConnect(ctx context.Context, cn *goredis.Conn) error {
	span := opentracing.SpanFromContext(ctx)
	if span == nil {
		span, _ = opentracing.StartSpanFromContext(ctx, "OnConnect")
		defer span.Finish()
	}
	span.SetTag(string(tags.RedisConnInfo), cn.String())
	span.LogFields(
		log.Event("connect"),
	)
	return nil
}

func (t *Tracer) BeforeProcess(ctx context.Context, cmd goredis.Cmder) (context.Context, error) {
	_, ctx = opentracing.StartSpanFromContext(
		ctx,
		tags.OperationDBQuery,
		tags.DBType.Tag("redis"),
		tags.RedisCommand.Tag(cmd.Name()),
	)
	return ctx, nil
}

func (t *Tracer) AfterProcess(ctx context.Context, cmd goredis.Cmder) error {
	span := opentracing.SpanFromContext(ctx)
	if span == nil {
		return nil
	}
	defer span.Finish()

	if err := cmd.Err(); err != nil {
		tracing.SetErrorOnSpan(span, err)
	}

	return nil
}

func (t *Tracer) BeforeProcessPipeline(ctx context.Context, cmds []goredis.Cmder) (context.Context, error) {
	cmdsTag := make([]string, 0, len(cmds))
	for _, cmd := range cmds {
		cmdsTag = append(cmdsTag, cmd.Name())
	}

	_, ctx = opentracing.StartSpanFromContext(
		ctx,
		tags.OperationDBQuery,
		tags.DBType.Tag("redis"),
		tags.RedisCommands.Tag(cmdsTag),
		tags.RedisCommandsCount.Tag(len(cmdsTag)),
	)
	return ctx, nil
}

func (t *Tracer) AfterProcessPipeline(ctx context.Context, cmds []goredis.Cmder) error {
	span := opentracing.SpanFromContext(ctx)
	if span == nil {
		return nil
	}
	defer span.Finish()

	for _, cmd := range cmds {
		if err := cmd.Err(); err != nil {
			tracing.SetErrorOnSpan(span, err)
			break
		}
	}

	return nil
}
