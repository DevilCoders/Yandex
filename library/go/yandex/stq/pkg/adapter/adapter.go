package adapter

import (
	"context"
	"errors"
	"fmt"
	"os"
	"sync"
	"time"

	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/yandex/stq/internal"
	"a.yandex-team.ru/library/go/yandex/stq/pkg/worker"
)

const (
	cmdExec  = "exec"
	cmdReady = "ready"
	cmdStop  = "stop"
	cmdMark  = "mark"
	cmdStats = "stats"
)

type StqRunnerAdapter struct {
	connector        internal.StqRunnerConnector
	registeredQueues map[string]worker.Worker
	sendMsgMutex     sync.Mutex
	logger           log.Logger
}

func NewStqRunnerAdapterWithSocketConnector(socketFd int, procNumber int, workerConfig string, logger log.Logger) (*StqRunnerAdapter, error) {
	connector, err := internal.NewSocketStqRunnerConnector(socketFd)
	if err != nil {
		return nil, err
	}

	// TODO: proc number flag usage
	// TODO: worker config flag usage

	return newStqRunnerAdapter(connector, logger)
}

func newStqRunnerAdapter(connector internal.StqRunnerConnector, logger log.Logger) (*StqRunnerAdapter, error) {
	return &StqRunnerAdapter{
		connector:        connector,
		registeredQueues: map[string]worker.Worker{},
		sendMsgMutex:     sync.Mutex{},
		logger:           logger,
	}, nil
}

func (a *StqRunnerAdapter) RegisterQueueWorker(q string, w worker.Worker) {
	a.registeredQueues[q] = w
}

func (a *StqRunnerAdapter) Run(ctx context.Context, queue string) error {
	queueWorker, ok := a.registeredQueues[queue]
	if !ok {
		keys := []string{}
		for k := range a.registeredQueues {
			keys = append(keys, k)
		}
		return fmt.Errorf("unregistered queue %s, registered queues: %v", queue, keys)
	}

	err := a.connector.SendResponse(worker.Response{Cmd: cmdReady})
	if err != nil {
		return fmt.Errorf("cant send ready command: %w", err)
	}

	wg := sync.WaitGroup{}
	defer wg.Wait()

	for {
		var req worker.Request

		select {
		case <-ctx.Done():
			a.logger.Infof("adapter context done")
			return nil
		default:
			req, err = a.connector.GetRequest()
			if errors.Is(err, os.ErrDeadlineExceeded) {
				continue
			}
			if err != nil {
				a.logger.Errorf("adapter error on get request: %s", err)
				continue
			}
		}

		if req.Cmd == cmdStop {
			a.logger.Infof("adapter command stop")
			break
		}

		wg.Add(1)
		go func(request worker.Request) {
			defer wg.Done()

			success := true

			start := time.Now()
			workerErr := queueWorker.ProcessRequest(request)
			elapsed := time.Since(start)

			if workerErr != nil {
				a.logger.Errorf("[taskID=%s] worker failed on process request: %v", request.TaskID, workerErr)
				success = false
			}

			response := worker.Response{
				Cmd:      cmdMark,
				TaskID:   request.TaskID,
				Success:  success,
				ExecTime: elapsed.Milliseconds(),
			}

			a.sendMsgMutex.Lock()
			defer a.sendMsgMutex.Unlock()

			err := a.connector.SendResponse(response)
			if err != nil {
				a.logger.Errorf("[taskID=%s] worker failed on send response: %v", request.TaskID, err)
				return
			}
		}(req)
	}

	return nil
}
