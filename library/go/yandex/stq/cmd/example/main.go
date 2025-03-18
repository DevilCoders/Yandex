package main

import (
	"context"
	"log"

	flag "github.com/spf13/pflag"

	"a.yandex-team.ru/library/go/core/log/zap"
	"a.yandex-team.ru/library/go/yandex/stq/cmd/example/workers"
	"a.yandex-team.ru/library/go/yandex/stq/pkg/adapter"
)

var socketFd = flag.Int("socket-fd", 0, "")
var queue = flag.String("queue", "", "")
var procNumber = flag.Int("proc-number", 0, "")
var workerConfig = flag.String("worker-config", "", "")

func main() {
	flag.Parse()

	logger, err := zap.New(zap.NewProductionDeployConfig())
	if err != nil {
		log.Fatalf("cant init logger: %s", err)
	}

	stqAdapter, err := adapter.NewStqRunnerAdapterWithSocketConnector(*socketFd, *procNumber, *workerConfig, logger)
	if err != nil {
		logger.Fatalf("cant get adapter: %s", err)
	}

	stqAdapter.RegisterQueueWorker("sample_queue_done_py3", workers.HappyPathWorker{})
	stqAdapter.RegisterQueueWorker("sample_queue_failed_py3", workers.ErrorWorker{})
	stqAdapter.RegisterQueueWorker("sample_queue_infinite_py3", workers.InfiniteWorker{})

	err = stqAdapter.Run(context.Background(), *queue)
	if err != nil {
		logger.Fatalf("adapter run error: %s", err)
	}
}
