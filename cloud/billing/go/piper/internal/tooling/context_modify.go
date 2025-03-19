package tooling

import (
	"context"
	"strings"

	"github.com/opentracing/opentracing-go"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling/tracetag"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling/tracing"
)

func InitContext(ctx context.Context, service string) (context.Context, ContextModifier) {
	ctx, store := embedStore(ctx)
	store.service = service
	return ctx, ContextModifier{store}
}

func ModifyContext(ctx context.Context) ContextModifier {
	store := getStoreFromCtx(ctx)
	return ContextModifier{store}
}

type ContextModifier struct {
	*ctxStore
}

func (cm ContextModifier) InitTracing(operation string) ContextModifier {
	if cm.ctxStore != nil {
		cm.requestID = GenerateRequestID()
		cm.span = tracing.RootSpan(operation, cm.requestID)
		tracing.TagSpan(cm.span, tracetag.Service(cm.service), tracetag.Source(cm.sourceFull), tracetag.Handler(cm.handler))
	}
	return cm
}

func (cm ContextModifier) FinishTracing() ContextModifier {
	if cm.ctxStore != nil {
		tracing.FinishSpan(cm.span)
		cm.span = nil
	}
	return cm
}

func (cm ContextModifier) SetSource(source string) ContextModifier {
	if cm.ctxStore != nil {
		cm.sourceFull = source

		parts := strings.SplitN(source, ":", 2)
		cm.sourceShort, cm.sourcePartition = parts[0], ""
		if len(parts) > 1 {
			cm.sourcePartition = parts[1]
		}
	}
	return cm
}

func (cm ContextModifier) SetHandler(handler string) ContextModifier {
	if cm.ctxStore != nil {
		cm.handler = handler
	}
	return cm
}

func (cm ContextModifier) SetRequestMeta(operation string, requestID string, span opentracing.Span) ContextModifier {
	if cm.ctxStore != nil && cm.requestID == "" {
		cm.requestID = requestID
		cm.span = span
	}
	return cm
}
