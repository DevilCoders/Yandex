package sender

import (
	"context"
	"time"

	"a.yandex-team.ru/cloud/mdb/billing/internal/billingdb"
	"a.yandex-team.ru/cloud/mdb/billing/sender/internal/writer"
	"a.yandex-team.ru/cloud/mdb/internal/encodingutil"
	logbrokerWriter "a.yandex-team.ru/cloud/mdb/internal/logbroker/writer"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

// Config ...
type Config struct {
	PendingInterval encodingutil.Duration `json:"pending_interval" yaml:"pending_interval"`
	MaxTasks        int32                 `json:"max_tasks" yaml:"max_tasks"`
	SendTimeout     encodingutil.Duration `json:"send_timeout" yaml:"send_timeout"`
}

func DefaultConfig() Config {
	return Config{
		PendingInterval: encodingutil.FromDuration(10 * time.Second),
		SendTimeout:     encodingutil.FromDuration(10 * time.Second),
	}
}

// Sender ...
type Sender struct {
	bdb        billingdb.BillingDB
	lg         log.Logger
	cfg        Config
	btype      billingdb.BillType
	handleFunc FetchAndSendBatchFunc
	ws         writer.Writer
}

// New create new Sender.
func New(
	btype billingdb.BillType,
	bdb billingdb.BillingDB,
	fetchAndSendBatchFunc FetchAndSendBatchFunc,
	writer writer.Writer,
	lg log.Logger,
	cfg Config,
) *Sender {
	return &Sender{
		btype:      btype,
		bdb:        bdb,
		ws:         writer,
		lg:         lg,
		cfg:        cfg,
		handleFunc: fetchAndSendBatchFunc,
	}
}

func (s *Sender) IsReady(ctx context.Context) error {
	if err := s.bdb.IsReady(ctx); err != nil {
		return semerr.WrapWithUnavailable(err, "billingdb is not ready")
	}
	return nil
}

func (s *Sender) Serve(ctx context.Context) error {
	var selected int32

	timer := time.NewTimer(0)
	defer func() {
		if !timer.Stop() {
			select {
			case <-timer.C:
			default:
			}
		}
		s.lg.Debug("finished")
	}()
	for {
		select {
		case <-ctx.Done():
			s.lg.Info("context closed, exit now")
			return nil
		case <-timer.C:
			s.lg.Info("wakeup by timer")
			for {
				if err := s.handleFunc(ctx, s.btype, s.ws, s.bdb, s.lg, s.cfg.SendTimeout.Duration); err != nil {
					if xerrors.Is(err, billingdb.ErrDataNotFound) {
						break
					}
					s.lg.Error("fetch and send batch failed", log.Error(err))
				}
				selected++
				if s.cfg.MaxTasks > 0 && selected >= s.cfg.MaxTasks {
					s.lg.Infof("max_tasks to select has reached: %d", s.cfg.MaxTasks)
					return nil
				}
			}
			timer.Reset(s.cfg.PendingInterval.Duration)
		}
	}
}

type FetchAndSendBatchFunc func(ctx context.Context, btype billingdb.BillType, ws writer.Writer, bdb billingdb.BillingDB, lg log.Logger, timeout time.Duration) error

var _ FetchAndSendBatchFunc = FetchAndSendBatch

func FetchAndSendBatch(ctx context.Context, btype billingdb.BillType, ws writer.Writer, bdb billingdb.BillingDB, lg log.Logger, timeout time.Duration) (err error) {
	ctx, err = bdb.Begin(ctx, sqlutil.Primary)
	if err != nil {
		return xerrors.Errorf("begin transaction: %w", err)
	}
	// This is a must, otherwise we can leak transaction
	defer func() {
		if r := recover(); r != nil {
			_ = bdb.Rollback(ctx)
			panic(r)
		}
		if err != nil {
			_ = bdb.Rollback(ctx)
		}
	}()

	startedTS := time.Now()
	batch, err := bdb.DequeueBatch(ctx, btype)
	if err != nil {
		return err
	}
	ctx = ctxlog.WithFields(ctx, log.String("batch_id", batch.ID), log.Int64("seq_no", batch.SeqNo))
	ctxlog.Debug(ctx, lg, "Fetched metrics batch")
	docs := []logbrokerWriter.Doc{{ID: batch.SeqNo, Data: batch.Data, CreatedAt: time.Now()}}

	if err := ws.Write(docs, timeout); err != nil {
		ctxlog.Errorf(ctx, lg, "Failed to synchronously write metrics: %s", err.Error())
		if err := postponeBatch(ctx, batch, bdb); err != nil {
			return err
		}
		return wrapErrorForBatch("synchronous batch write", batch, err)
	}

	ctxlog.Debug(ctx, lg, "Successfully sent metrics batch")
	if err := bdb.CompleteBatch(ctx, batch.ID, startedTS, time.Now()); err != nil {
		return wrapErrorForBatch("complete batch", batch, err)
	}

	if err := bdb.Commit(ctx); err != nil {
		return wrapErrorForBatch("commit batch completion", batch, err)
	}

	return nil
}

// FetchCommitAndSendBatch should be used for writers that not support exactly once semantics
func FetchCommitAndSendBatch(ctx context.Context, btype billingdb.BillType, ws writer.Writer, bdb billingdb.BillingDB, lg log.Logger, timeout time.Duration) (err error) {
	ctx, err = bdb.Begin(ctx, sqlutil.Primary)
	if err != nil {
		return xerrors.Errorf("begin transaction: %w", err)
	}
	// This is a must, otherwise we can leak transaction
	defer func() {
		if r := recover(); r != nil {
			_ = bdb.Rollback(ctx)
			panic(r)
		}
		if err != nil {
			_ = bdb.Rollback(ctx)
		}
	}()

	startedTS := time.Now()
	batch, err := bdb.DequeueBatch(ctx, btype)
	if err != nil {
		return err
	}
	ctx = ctxlog.WithFields(ctx, log.String("batch_id", batch.ID), log.Int64("seq_no", batch.SeqNo))
	ctxlog.Debug(ctx, lg, "Fetched metrics batch")

	// Commit batch before send to prevent double billing

	if err := bdb.CompleteBatch(ctx, batch.ID, startedTS, time.Now()); err != nil {
		return wrapErrorForBatch("complete batch", batch, err)
	}

	if err := bdb.Commit(ctx); err != nil {
		return wrapErrorForBatch("commit batch completion", batch, err)
	}

	docs := []logbrokerWriter.Doc{{ID: batch.SeqNo, Data: batch.Data, CreatedAt: time.Now()}}

	if err := ws.Write(docs, timeout); err != nil {
		ctxlog.Errorf(ctx, lg, "Failed to synchronously write metrics: %s", err.Error())
		if err := rollbackBatch(ctx, batch, bdb); err != nil {
			return err
		}
		return wrapErrorForBatch("synchronous batch write", batch, err)
	}

	ctxlog.Debug(ctx, lg, "Successfully sent metrics batch")
	return nil
}

func postponeBatch(ctx context.Context, batch billingdb.Batch, bdb billingdb.BillingDB) error {
	if err := bdb.PostponeBatch(ctx, batch.ID); err != nil {
		return wrapErrorForBatch("postpone batch", batch, err)
	}
	if err := bdb.Commit(ctx); err != nil {
		return wrapErrorForBatch("commit batch failure", batch, err)
	}
	return nil
}

func wrapErrorForBatch(msg string, batch billingdb.Batch, err error) error {
	if err != nil {
		return xerrors.Errorf("%s (id=%s, seq_num=%d): %w", msg, batch.ID, batch.SeqNo, err)
	}
	return nil
}

func rollbackBatch(ctx context.Context, batch billingdb.Batch, bdb billingdb.BillingDB) error {
	ctx, err := bdb.Begin(ctx, sqlutil.Primary)
	if err != nil {
		return xerrors.Errorf("begin transaction: %w", err)
	}
	// This is a must, otherwise we can leak transaction
	defer func() {
		if r := recover(); r != nil {
			_ = bdb.Rollback(ctx)
			panic(r)
		}
		if err != nil {
			_ = bdb.Rollback(ctx)
		}
	}()

	return postponeBatch(ctx, batch, bdb)
}
