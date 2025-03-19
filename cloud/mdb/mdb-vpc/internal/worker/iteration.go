package worker

import (
	"context"
	"sync"

	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
)

func (w *Worker) Iteration(ctx context.Context) {
	ctxlog.Info(ctx, w.log, "Start new iteration")
	defer ctxlog.Info(ctx, w.log, "Finish iteration")

	var wg sync.WaitGroup
	for i := 0; i < w.maxConcurrentTasks; i++ {
		n := i
		wCtx := ctxlog.WithFields(ctx, log.Int("workerNumber", n))
		wg.Add(1)
		go w.RunOperation(wCtx, &wg)
	}
	wg.Wait()
}
