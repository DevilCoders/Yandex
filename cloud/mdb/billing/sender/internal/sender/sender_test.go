package sender

import (
	"context"
	"testing"
	"time"

	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/billing/internal/billingdb"
	bdbMock "a.yandex-team.ru/cloud/mdb/billing/internal/billingdb/mocks"
	"a.yandex-team.ru/cloud/mdb/billing/sender/internal/writer"
	writerMock "a.yandex-team.ru/cloud/mdb/billing/sender/internal/writer/mocks"
	logbrokerWriter "a.yandex-team.ru/cloud/mdb/internal/logbroker/writer"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/log/zap"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type ctxKey struct{}

func getWriteAndWaitFuncSucceed(t *testing.T, ctl *gomock.Controller, seqNo int64, data []byte) writer.Writer {
	result := writerMock.NewMockWriter(ctl)
	result.EXPECT().Write(gomock.Any(), gomock.Any()).DoAndReturn(func(docs []logbrokerWriter.Doc, timeout time.Duration) error {
		require.Len(t, docs, 1)
		require.Equal(t, seqNo, docs[0].ID)
		require.Equal(t, data, docs[0].Data)
		return nil
	}).AnyTimes()

	return result
}

func getWriteAndWaitFuncFailed(t *testing.T, ctl *gomock.Controller, seqNo int64, data []byte, err error) writer.Writer {
	result := writerMock.NewMockWriter(ctl)
	result.EXPECT().Write(gomock.Any(), gomock.Any()).DoAndReturn(func(docs []logbrokerWriter.Doc, timeout time.Duration) error {
		require.Len(t, docs, 1)
		require.Equal(t, seqNo, docs[0].ID)
		require.Equal(t, data, docs[0].Data)
		return err
	}).AnyTimes()

	return result
}

func TestFetchAndSendBatch(t *testing.T) {
	lg, err := zap.New(zap.KVConfig(log.DebugLevel))
	require.NoError(t, err)

	t.Run("success", func(t *testing.T) {
		batchID := "11"
		seqNo := int64(42)
		data := []byte("data")
		ctx := context.Background()
		txnCtx := context.WithValue(ctx, ctxKey{}, 42)

		ctl := gomock.NewController(t)
		mockBdb := bdbMock.NewMockBillingDB(ctl)
		mockBdb.EXPECT().Begin(ctx, sqlutil.Primary).Times(1).Return(txnCtx, nil)
		mockBdb.EXPECT().DequeueBatch(txnCtx, billingdb.BillTypeBackup).Times(1).Return(
			billingdb.Batch{ID: batchID, SeqNo: seqNo, Restarts: 0, Data: data}, nil)
		txnCtxWithFields := ctxlog.WithFields(txnCtx, log.String("batch_id", batchID), log.Int64("seq_no", seqNo))
		mockBdb.EXPECT().CompleteBatch(txnCtxWithFields, batchID, gomock.Any(), gomock.Any()).Times(1).Return(nil)
		mockBdb.EXPECT().Commit(txnCtxWithFields).Times(1).Return(nil)

		err := FetchAndSendBatch(
			context.Background(),
			billingdb.BillTypeBackup,
			getWriteAndWaitFuncSucceed(t, ctl, seqNo, data),
			mockBdb,
			lg,
			time.Duration(0),
		)
		require.NoError(t, err)
	})

	t.Run("rollback_after_failure", func(t *testing.T) {
		ctx := context.Background()
		txnCtx := context.WithValue(ctx, ctxKey{}, "ok")

		ctl := gomock.NewController(t)
		mockBdb := bdbMock.NewMockBillingDB(ctl)
		dequeueErr := xerrors.Errorf("dequeue batch")
		mockBdb.EXPECT().Begin(ctx, gomock.Any()).Times(1).Return(txnCtx, nil)
		mockBdb.EXPECT().DequeueBatch(txnCtx, billingdb.BillTypeBackup).Times(1).Return(billingdb.Batch{}, dequeueErr)
		mockBdb.EXPECT().Rollback(txnCtx).Times(1).Return(nil)

		gotErr := FetchAndSendBatch(
			ctx,
			billingdb.BillTypeBackup,
			nil,
			mockBdb,
			lg,
			time.Duration(0),
		)
		require.ErrorIs(t, gotErr, dequeueErr)
	})

	t.Run("write_failure,success_postpone", func(t *testing.T) {
		batchID := "11"
		seqNo := int64(42)
		data := []byte("data")

		ctx := context.Background()
		txnCtx := context.WithValue(ctx, ctxKey{}, "ok")

		ctl := gomock.NewController(t)
		mockBdb := bdbMock.NewMockBillingDB(ctl)
		mockBdb.EXPECT().Begin(ctx, gomock.Any()).Times(1).Return(txnCtx, nil)
		mockBdb.EXPECT().DequeueBatch(txnCtx, billingdb.BillTypeBackup).Times(1).Return(
			billingdb.Batch{ID: batchID, SeqNo: seqNo, Restarts: 0, Data: data}, nil)
		txnCtxWithFields := ctxlog.WithFields(txnCtx, log.String("batch_id", batchID), log.Int64("seq_no", seqNo))
		mockBdb.EXPECT().PostponeBatch(txnCtxWithFields, batchID).Times(1).Return(nil)
		mockBdb.EXPECT().Commit(txnCtxWithFields).Times(1).Return(nil)
		mockBdb.EXPECT().Rollback(txnCtxWithFields).Times(1).Return(nil)

		writeErr := xerrors.Errorf("write error")
		gotErr := FetchAndSendBatch(
			ctx,
			billingdb.BillTypeBackup,
			getWriteAndWaitFuncFailed(t, ctl, seqNo, data, writeErr),
			mockBdb,
			lg,
			time.Duration(0),
		)
		require.ErrorIs(t, gotErr, writeErr)
	})

	t.Run("write_failure,failed_postpone", func(t *testing.T) {
		batchID := "11"
		seqNo := int64(42)
		data := []byte("data")

		ctx := context.Background()
		txnCtx := context.WithValue(ctx, ctxKey{}, "ok")

		ctl := gomock.NewController(t)
		mockBdb := bdbMock.NewMockBillingDB(ctl)
		mockBdb.EXPECT().Begin(ctx, gomock.Any()).Times(1).Return(txnCtx, nil)
		mockBdb.EXPECT().DequeueBatch(txnCtx, billingdb.BillTypeBackup).Times(1).Return(
			billingdb.Batch{ID: batchID, SeqNo: seqNo, Restarts: 0, Data: data}, nil)
		txnCtxWithFields := ctxlog.WithFields(txnCtx, log.String("batch_id", batchID), log.Int64("seq_no", seqNo))

		postponeErr := xerrors.Errorf("postpone error")
		mockBdb.EXPECT().PostponeBatch(txnCtxWithFields, batchID).Times(1).Return(postponeErr)
		mockBdb.EXPECT().Rollback(txnCtxWithFields).Times(1).Return(nil)

		writeErr := xerrors.Errorf("write error")
		gotErr := FetchAndSendBatch(
			ctx,
			billingdb.BillTypeBackup,
			getWriteAndWaitFuncFailed(t, ctl, seqNo, data, writeErr),
			mockBdb,
			lg,
			time.Duration(0),
		)
		require.ErrorIs(t, gotErr, postponeErr)
	})
}

func TestSender_Serve(t *testing.T) {
	lg, err := zap.New(zap.KVConfig(log.DebugLevel))
	require.NoError(t, err)

	t.Run("work_until_ctx_cancel", func(t *testing.T) {
		ctx, cancel := context.WithCancel(context.Background())
		jobNum := 0
		jobLimit := 3
		handleFunc := func(_ context.Context, _ billingdb.BillType, _ writer.Writer, _ billingdb.BillingDB, _ log.Logger, _ time.Duration) error {
			if jobNum < jobLimit {
				jobNum++
				return nil
			}
			cancel()
			return billingdb.ErrDataNotFound
		}

		s := &Sender{
			lg:         lg,
			cfg:        DefaultConfig(),
			handleFunc: handleFunc,
		}

		require.NoError(t, s.Serve(ctx))
		require.Equal(t, jobLimit, jobNum)
	})

	t.Run("work_until_max_tasks_reached", func(t *testing.T) {
		jobNum := 0
		jobLimit := 3
		handleFunc := func(_ context.Context, _ billingdb.BillType, _ writer.Writer, _ billingdb.BillingDB, _ log.Logger, duration time.Duration) error {
			jobNum++
			return nil
		}

		s := &Sender{
			lg:         lg,
			cfg:        Config{MaxTasks: int32(jobLimit)},
			handleFunc: handleFunc,
		}

		require.NoError(t, s.Serve(context.Background()))
		require.Equal(t, jobLimit, jobNum)
	})

}

func TestFetchCommitAndSendBatch(t *testing.T) {
	lg, err := zap.New(zap.KVConfig(log.DebugLevel))
	require.NoError(t, err)

	t.Run("success", func(t *testing.T) {
		batchID := "11"
		seqNo := int64(42)
		data := []byte("data")
		ctx := context.Background()
		txnCtx := context.WithValue(ctx, ctxKey{}, 42)

		ctl := gomock.NewController(t)
		mockBdb := bdbMock.NewMockBillingDB(ctl)
		mockBdb.EXPECT().Begin(ctx, sqlutil.Primary).Times(1).Return(txnCtx, nil)
		mockBdb.EXPECT().DequeueBatch(txnCtx, billingdb.BillTypeBackup).Times(1).Return(
			billingdb.Batch{ID: batchID, SeqNo: seqNo, Restarts: 0, Data: data}, nil)
		txnCtxWithFields := ctxlog.WithFields(txnCtx, log.String("batch_id", batchID), log.Int64("seq_no", seqNo))
		mockBdb.EXPECT().CompleteBatch(txnCtxWithFields, batchID, gomock.Any(), gomock.Any()).Times(1).Return(nil)
		mockBdb.EXPECT().Commit(txnCtxWithFields).Times(1).Return(nil)

		err := FetchCommitAndSendBatch(
			context.Background(),
			billingdb.BillTypeBackup,
			getWriteAndWaitFuncSucceed(t, ctl, seqNo, data),
			mockBdb,
			lg,
			time.Duration(0),
		)
		require.NoError(t, err)
	})

	t.Run("rollback_after_write_failure", func(t *testing.T) {
		batchID := "11"
		seqNo := int64(42)
		data := []byte("data")
		ctx := context.Background()
		txnCtx := context.WithValue(ctx, ctxKey{}, 42)
		txnCtxWithFields := ctxlog.WithFields(txnCtx, log.String("batch_id", batchID), log.Int64("seq_no", seqNo))

		ctl := gomock.NewController(t)
		mockBdb := bdbMock.NewMockBillingDB(ctl)
		mockBdb.EXPECT().Begin(ctx, sqlutil.Primary).Times(1).Return(txnCtx, nil)
		mockBdb.EXPECT().DequeueBatch(txnCtx, billingdb.BillTypeBackup).Times(1).Return(
			billingdb.Batch{ID: batchID, SeqNo: seqNo, Restarts: 0, Data: data}, nil)
		mockBdb.EXPECT().CompleteBatch(txnCtxWithFields, batchID, gomock.Any(), gomock.Any()).Times(1).Return(nil)
		mockBdb.EXPECT().Commit(txnCtxWithFields).Times(1).Return(nil)
		mockBdb.EXPECT().Begin(txnCtxWithFields, sqlutil.Primary).Times(1).Return(txnCtx, nil)
		mockBdb.EXPECT().PostponeBatch(txnCtx, batchID).Times(1).Return(nil)
		mockBdb.EXPECT().Commit(txnCtx).Times(1).Return(nil)
		mockBdb.EXPECT().Rollback(txnCtxWithFields).Times(1).Return(nil)

		err := FetchCommitAndSendBatch(
			context.Background(),
			billingdb.BillTypeBackup,
			getWriteAndWaitFuncFailed(t, ctl, seqNo, data, xerrors.Errorf("write failed")),
			mockBdb,
			lg,
			time.Duration(0),
		)
		require.ErrorContains(t, err, "write failed")
	})
}
