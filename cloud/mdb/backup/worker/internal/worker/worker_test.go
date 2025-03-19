package worker

import (
	"context"
	"fmt"
	"testing"
	"time"

	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/backup/worker/internal/models"
	queuehandlerMock "a.yandex-team.ru/cloud/mdb/backup/worker/internal/queuehandler/mocks"
	queueproducerMock "a.yandex-team.ru/cloud/mdb/backup/worker/internal/queueproducer/mocks"
	"a.yandex-team.ru/cloud/mdb/internal/encodingutil"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
)

func newQueueProducerOkMock(ctrl *gomock.Controller, jobsCount int) *queueproducerMock.MockQueueProducer {
	queueproducer := queueproducerMock.NewMockQueueProducer(ctrl)
	queueproducer.EXPECT().ProduceQueue(gomock.Any(), gomock.Any()).DoAndReturn(
		func(produceCtx, jobCtx context.Context) (chan models.BackupJob, chan error) {
			jobsch := make(chan models.BackupJob)
			errch := make(chan error)
			go func() {
				defer func() {
					close(jobsch)
					close(errch)
				}()
				for i := 0; i < jobsCount; i++ {
					j := models.BackupJob{
						FetchedTS: time.Unix(int64(i), 0),
						Ctx:       jobCtx,
					}
					select {
					case jobsch <- j:
					case <-produceCtx.Done():
					}
				}
				<-produceCtx.Done()
			}()
			return jobsch, errch
		})
	return queueproducer
}

func newQueueProducerErrMock(ctrl *gomock.Controller, err error) *queueproducerMock.MockQueueProducer {
	queueproducer := queueproducerMock.NewMockQueueProducer(ctrl)
	queueproducer.EXPECT().ProduceQueue(gomock.Any(), gomock.Any()).DoAndReturn(
		func(produceCtx, jobCtx context.Context) (chan models.BackupJob, chan error) {
			jobsch := make(chan models.BackupJob, 1)
			errch := make(chan error)
			go func() {
				defer func() {
					close(jobsch)
					close(errch)
				}()
				errch <- err
			}()
			return jobsch, errch
		})
	return queueproducer
}

func newQueueHandlerOkMock(ctrl *gomock.Controller) *queuehandlerMock.MockQueueHandler {
	queuehandler := queuehandlerMock.NewMockQueueHandler(ctrl)
	queuehandler.EXPECT().HandleQueue(gomock.Any()).DoAndReturn(
		func(ch chan models.BackupJob) chan error {
			errch := make(chan error)
			go func() {
				defer close(errch)
				for range ch {
				}
			}()
			return errch
		})
	return queuehandler
}

func newQueueHandlerErrMock(ctrl *gomock.Controller, err error) *queuehandlerMock.MockQueueHandler {
	queuehandler := queuehandlerMock.NewMockQueueHandler(ctrl)
	queuehandler.EXPECT().HandleQueue(gomock.Any()).DoAndReturn(
		func(ch chan models.BackupJob) chan error {
			errch := make(chan error)
			go func() {
				defer close(errch)
				errch <- err
			}()
			return errch
		})
	return queuehandler
}

func newQueueHandlerHangMock(ctrl *gomock.Controller) *queuehandlerMock.MockQueueHandler {
	queuehandler := queuehandlerMock.NewMockQueueHandler(ctrl)
	queuehandler.EXPECT().HandleQueue(gomock.Any()).DoAndReturn(
		func(ch chan models.BackupJob) chan error {
			errch := make(chan error)
			go func() {
				defer close(errch)
				for j := range ch {
					<-j.Ctx.Done()
				}
			}()
			return errch
		})
	return queuehandler
}

func newLogger(t *testing.T) log.Logger {
	l, err := zap.New(zap.KVConfig(log.DebugLevel))
	require.NoError(t, err)
	require.NotNil(t, l)
	return l
}

func TestWorker_Run(t *testing.T) {
	cfg := Config{GracefulShutdownTimeout: encodingutil.FromDuration(1 * time.Second)}
	t.Run("context_stop,no_error_exit", func(t *testing.T) {
		ctrl := gomock.NewController(t)
		pipelines := []Pipeline{
			{
				"test_pipeline1",
				newQueueProducerOkMock(ctrl, 2),
				newQueueHandlerOkMock(ctrl),
			},
			{
				"test_pipeline2",
				newQueueProducerOkMock(ctrl, 2),
				newQueueHandlerOkMock(ctrl),
			},
		}
		ctx, cancel := context.WithCancel(context.Background())
		defer cancel()
		w := &Worker{
			lg:        newLogger(t),
			pipelines: pipelines,
			cfg:       cfg,
		}
		cancel()
		err := w.Run(ctx)
		require.NoError(t, err)
		ctrl.Finish()
	})

	t.Run("producer_error", func(t *testing.T) {
		ctrl := gomock.NewController(t)
		pipelines := []Pipeline{
			{
				"test_pipeline1",
				newQueueProducerErrMock(ctrl, fmt.Errorf("pipeline1 producer error")),
				newQueueHandlerOkMock(ctrl),
			},
			{
				"test_pipeline2",
				newQueueProducerOkMock(ctrl, 0),
				newQueueHandlerOkMock(ctrl),
			},
		}
		w := &Worker{
			lg:        newLogger(t),
			pipelines: pipelines,
			cfg:       cfg,
		}
		err := w.Run(context.Background())
		require.EqualError(t, err, "pipeline1 producer error")
		ctrl.Finish()
	})

	t.Run("handler_error", func(t *testing.T) {
		ctrl := gomock.NewController(t)
		pipelines := []Pipeline{
			{
				"test_pipeline1",
				newQueueProducerOkMock(ctrl, 0),
				newQueueHandlerOkMock(ctrl),
			},
			{
				"test_pipeline2",
				newQueueProducerOkMock(ctrl, 0),
				newQueueHandlerErrMock(ctrl, fmt.Errorf("pipeline2 handler error")),
			},
		}
		w := &Worker{
			lg:        newLogger(t),
			pipelines: pipelines,
			cfg:       cfg,
		}
		err := w.Run(context.Background())
		require.EqualError(t, err, "pipeline2 handler error")
		ctrl.Finish()
	})

	t.Run("multiple_errors", func(t *testing.T) {
		ctrl := gomock.NewController(t)
		pipelines := []Pipeline{
			{
				"test_pipeline1",
				newQueueProducerErrMock(ctrl, fmt.Errorf("pipeline1 producer error")),
				newQueueHandlerOkMock(ctrl),
			},
			{
				"test_pipeline2",
				newQueueProducerOkMock(ctrl, 0),
				newQueueHandlerErrMock(ctrl, fmt.Errorf("pipeline2 handler error")),
			},
		}
		w := &Worker{
			lg:        newLogger(t),
			pipelines: pipelines,
			cfg:       cfg,
		}
		err := w.Run(context.Background())
		require.Error(t, err)
		ctrl.Finish()
	})

	t.Run("handler_hangs,clean_shutdown", func(t *testing.T) {
		ctrl := gomock.NewController(t)
		pipelines := []Pipeline{
			{
				"test_pipeline1",
				newQueueProducerOkMock(ctrl, 3),
				newQueueHandlerHangMock(ctrl),
			},
			{
				"test_pipeline2",
				newQueueProducerOkMock(ctrl, 2),
				newQueueHandlerHangMock(ctrl),
			},
			{
				"test_pipeline3",
				newQueueProducerOkMock(ctrl, 2),
				newQueueHandlerOkMock(ctrl),
			},
		}
		w := &Worker{
			lg:        newLogger(t),
			pipelines: pipelines,
			cfg:       cfg,
		}
		ctx, cancel := context.WithCancel(context.Background())
		cancel()
		err := w.Run(ctx)
		require.NoError(t, err)
		ctrl.Finish()
	})

	t.Run("handler_hangs,error_shutdown", func(t *testing.T) {
		ctrl := gomock.NewController(t)
		pipelines := []Pipeline{
			{
				"test_pipeline1",
				newQueueProducerOkMock(ctrl, 3),
				newQueueHandlerHangMock(ctrl),
			},
			{
				"test_pipeline2",
				newQueueProducerOkMock(ctrl, 1),
				newQueueHandlerHangMock(ctrl),
			},
			{
				"test_pipeline3",
				newQueueProducerOkMock(ctrl, 1),
				newQueueHandlerErrMock(ctrl, fmt.Errorf("pipeline3 handler error")),
			},
		}
		w := &Worker{
			lg:        newLogger(t),
			pipelines: pipelines,
			cfg:       cfg,
		}
		ctx, cancel := context.WithCancel(context.Background())
		cancel()
		err := w.Run(ctx)
		require.EqualError(t, err, "pipeline3 handler error")
		ctrl.Finish()
	})

}
