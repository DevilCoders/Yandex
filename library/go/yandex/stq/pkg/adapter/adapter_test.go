package adapter

import (
	"context"
	"os"
	"sync"
	"testing"
	"time"

	"github.com/stretchr/testify/require"
	"go.uber.org/atomic"

	"a.yandex-team.ru/library/go/core/log/nop"
	"a.yandex-team.ru/library/go/yandex/stq/pkg/worker"
)

type MockConnector struct {
	t *testing.T

	limit    int32
	curTasks atomic.Int32

	mockedRequest *worker.Request
}

func (m *MockConnector) GetRequest() (worker.Request, error) {
	m.curTasks.Inc()

	if m.curTasks.Load() > m.limit && m.limit > 0 {
		return worker.Request{
			Cmd: cmdStop,
		}, nil
	}

	if m.mockedRequest != nil {
		return *m.mockedRequest, nil
	}
	return worker.Request{}, os.ErrDeadlineExceeded
}

func (m *MockConnector) SendResponse(response worker.Response) error {
	if m.curTasks.Load() == 0 {
		require.EqualValues(m.t, worker.Response{
			Cmd: cmdReady,
		}, response)
	} else {
		response.ExecTime = 0 // cant check real value

		require.EqualValues(m.t, worker.Response{
			Cmd:     cmdMark,
			TaskID:  m.mockedRequest.TaskID,
			Success: true,
		}, response)
	}
	return nil
}

type MockWorker struct {
	t *testing.T

	expectedRequest *worker.Request
}

func (m MockWorker) ProcessRequest(request worker.Request) error {
	require.NotNil(m.t, m.expectedRequest, "method must not be called with nil expected")
	require.EqualValues(m.t, *m.expectedRequest, request)
	return nil
}

func TestHappyPathAdapter(t *testing.T) {
	someReq := worker.Request{
		Cmd:               cmdExec,
		TaskID:            "some_task_id",
		Args:              []interface{}{"some_arg", 2},
		Kwargs:            map[string]interface{}{"some_key": "some_value"},
		ExecTries:         4,
		RescheduleCounter: 3,
		Eta:               time.Time{},
	}

	adapter, err := newStqRunnerAdapter(&MockConnector{t: t, limit: 10, mockedRequest: &someReq}, &nop.Logger{})
	require.NoError(t, err)

	goodWorkerQueue := "good_worker_queue"
	adapter.RegisterQueueWorker(goodWorkerQueue, MockWorker{t, &someReq})
	require.NoError(t, adapter.Run(context.Background(), goodWorkerQueue))
}

func TestBadQueueAdapter(t *testing.T) {
	adapter, err := newStqRunnerAdapter(&MockConnector{t: t, limit: 10, mockedRequest: nil}, &nop.Logger{})
	require.NoError(t, err)

	badWorkerQueue := "bad_worker_queue"
	adapter.RegisterQueueWorker(badWorkerQueue, MockWorker{t, nil})
	require.NoError(t, adapter.Run(context.Background(), badWorkerQueue))
}

func TestCancelWorker(t *testing.T) {
	adapter, err := newStqRunnerAdapter(&MockConnector{t: t, limit: -1, mockedRequest: nil}, &nop.Logger{})
	require.NoError(t, err)

	contextCancellingWorkerQueue := "context_cancelling_worker_queue"
	adapter.RegisterQueueWorker(contextCancellingWorkerQueue, MockWorker{t, nil})

	wg := sync.WaitGroup{}
	wg.Add(1)
	ctx, cancelFunc := context.WithCancel(context.Background())

	go func() {
		defer wg.Done()
		require.NoError(t, adapter.Run(ctx, contextCancellingWorkerQueue))
	}()

	cancelFunc() // checking that infinite read loop can be closed by context
	wg.Wait()
}
