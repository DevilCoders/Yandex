package actions

import (
	"context"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/core/entities"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling"
)

type PresenterPusher interface {
	PushPresenterMetric(context.Context, entities.ProcessingScope, entities.PresenterMetric) error
	FlushPresenterMetric(context.Context) error
}

func PushPresenterMetrics(ctx context.Context, scope entities.ProcessingScope, pusher PresenterPusher, metrics []entities.PresenterMetric) (err error) {
	ctx = tooling.ActionStarted(ctx)
	defer func() { tooling.ActionDone(ctx, err) }()

	for _, m := range metrics {
		if err = pusher.PushPresenterMetric(ctx, scope, m); err != nil {
			return
		}
	}

	return pusher.FlushPresenterMetric(ctx)
}
