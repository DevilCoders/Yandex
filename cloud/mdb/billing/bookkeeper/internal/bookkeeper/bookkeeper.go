package bookkeeper

import (
	"context"
	"time"

	"a.yandex-team.ru/cloud/mdb/billing/bookkeeper/internal/exporter"
	"a.yandex-team.ru/cloud/mdb/billing/bookkeeper/internal/invoicer"
	"a.yandex-team.ru/cloud/mdb/billing/internal/billingdb"
	"a.yandex-team.ru/cloud/mdb/billing/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

// Config ...
type Config struct {
	MaxInvoicesForBatch int `json:"max_invoices_for_batch" yaml:"max_invoices_for_batch"`
}

func DefaultConfig() Config {
	return Config{
		MaxInvoicesForBatch: 50,
	}
}

// Bookkeeper ...
type Bookkeeper struct {
	calc  map[metadb.ClusterType]invoicer.Invoicer
	mdb   metadb.MetaDB
	bdb   billingdb.BillingDB
	lg    log.Logger
	cfg   Config
	btype billingdb.BillType
	mb    exporter.MetricsBatcher
	exp   exporter.BillingExporter
}

// New create new bookkeeper.
func New(
	btype billingdb.BillType,
	calc map[metadb.ClusterType]invoicer.Invoicer,
	exp exporter.BillingExporter,
	mdb metadb.MetaDB,
	bdb billingdb.BillingDB,
	mb exporter.MetricsBatcher,
	lg log.Logger,
	cfg Config,
) *Bookkeeper {
	return &Bookkeeper{btype: btype, calc: calc, mdb: mdb, bdb: bdb, lg: lg, cfg: cfg, mb: mb, exp: exp}
}

func (b *Bookkeeper) IsReady(ctx context.Context) error {
	if err := b.mdb.IsReady(ctx); err != nil {
		return semerr.WrapWithUnavailable(err, "metadb is not ready")
	}

	if err := b.bdb.IsReady(ctx); err != nil {
		return semerr.WrapWithUnavailable(err, "billingdb is not ready")
	}
	return nil
}

func (b *Bookkeeper) Bill(ctx context.Context) error {
	ctx, err := b.bdb.Begin(ctx, sqlutil.Primary)
	if err != nil {
		return xerrors.Errorf("begin transaction: %w", err)
	}
	// This is a must, otherwise we can leak transaction
	defer func() {
		if r := recover(); r != nil {
			_ = b.bdb.Rollback(ctx)
			panic(r)
		}
		_ = b.bdb.Rollback(ctx)
	}()

	if err = b.bdb.TryGetLock(ctx, b.btype); err != nil {
		if xerrors.Is(err, billingdb.ErrLockNotTaken) {
			b.lg.Warn("Lock is already taken by another worker. It's unexpected behavior. Expecting that only one sender is running")
		}
		b.lg.Error("Can not get lock", log.Error(err))
		return err
	}
	tracks, err := b.bdb.Tracks(ctx, b.btype)
	if err == billingdb.ErrDataNotFound {
		b.lg.Warnf("no clusters for billing type %q found", b.btype)
		return nil
	}
	if err != nil {
		return xerrors.Errorf("fetch clusters tracks: %w", err)
	}

	invoices, err := b.issueInvoices(ctx, tracks)
	if err != nil {
		return xerrors.Errorf("issue invoices: %w", err)
	}
	b.lg.Info("Invoices issued", log.String("bill_type", string(b.btype)), log.Int("count", len(invoices)))

	for _, invoicesChunk := range invoicer.ChunkInvoices(invoices, b.cfg.MaxInvoicesForBatch) {
		if err := b.processInvoices(ctx, invoicesChunk, b.btype); err != nil {
			return xerrors.Errorf("process invoices: %w", err)
		}
	}
	return b.bdb.Commit(ctx)
}

func (b *Bookkeeper) issueInvoices(ctx context.Context, tracks []billingdb.Track) ([]invoicer.Invoice, error) {
	var invoices []invoicer.Invoice
	endTS := time.Now().Round(time.Second)
	for _, track := range tracks {
		b.lg.Debugf("Fetched track: %+v", track)
		beginTS := track.UntilTS.Time
		if !track.UntilTS.Valid {
			b.lg.Info(
				"Detected first run for cluster. Starting bill point is 'from_ts'",
				log.String("cluster_id", track.ClusterID),
				log.Time("from_ts", track.FromTS))
			beginTS = track.FromTS
		}

		calc, ok := b.calc[track.ClusterType]
		if !ok {
			return nil, xerrors.Errorf("invoicer not found for cluster type: %s", track.ClusterType)
		}
		inv, err := calc.Invoice(ctx, track.ClusterID, beginTS, endTS)
		if err == invoicer.ErrNoMetrics {
			b.lg.Warnf("No metrics produced for cluster %q", track.ClusterID)
			continue
		}
		if err != nil {
			return nil, err // TODO: skip errors for individual clusters?
		}
		b.lg.Debug("Calculated invoice", log.String("cluster_id", inv.Resource.ClusterID), log.Any("invoice", inv))
		invoices = append(invoices, inv)
	}
	return invoices, nil
}

func (b *Bookkeeper) processInvoices(ctx context.Context, invoices []invoicer.Invoice, billType billingdb.BillType) error {
	if err := b.mb.Reset(); err != nil {
		return err
	}

	for _, inv := range invoices {
		metrics, err := b.exp.Export(inv)
		if err != nil {
			return err
		}
		for i := range metrics {
			b.mb.Add(metrics[i])
		}

		if len(metrics) > 0 {
			if err := b.bdb.UpdateClusterTrack(ctx, inv.Resource.ClusterID, inv.UntilTS, billType); err != nil {
				return xerrors.Errorf("update cluster track %q: %s", inv.Resource.ClusterID, err)
			}
		}
	}

	if !b.mb.Empty() {
		if err := b.bdb.EnqueueMetrics(ctx, b.mb, billType); err != nil {
			return xerrors.Errorf("enqueue metrics: %s", err)
		}
	}

	return nil
}
