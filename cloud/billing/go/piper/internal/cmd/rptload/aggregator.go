package main

import (
	"context"
	"time"

	"golang.org/x/sync/errgroup"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/db/ydb/qtool"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/db/ydb/reports"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/types"
	"a.yandex-team.ru/cloud/billing/go/pkg/decimal"
)

type aggregator struct {
	logger printer
}

func NewAggregator(logger printer) *aggregator {
	return &aggregator{
		logger: logger,
	}
}

func (a *aggregator) Run(ctx context.Context, workers int, input <-chan types.EnrichedQueueMetric, batchSizeLimit int) (<-chan []reports.ResourceReportsRow, func() error) {
	outputs := make(chan []reports.ResourceReportsRow, 1)
	eg, ctx := errgroup.WithContext(ctx)
	for i := 0; i < workers; i++ {
		i := i
		eg.Go(func() error {
			return a.aggr(ctx, i, input, outputs, batchSizeLimit)
		})
	}
	return outputs, func() error {
		defer close(outputs)
		err := eg.Wait()
		return err
	}
}

func (a *aggregator) aggr(ctx context.Context, nWorker int, input <-chan types.EnrichedQueueMetric, outputs chan<- []reports.ResourceReportsRow, batchSizeLimit int) error {
	defer func() {
		a.logger.Printf("[a%d] aggr done.\n", nWorker)
	}()
	a.logger.Printf("[a%d] aggr started.\n", nWorker)
	return a.aggregateResourceReports(ctx, input, outputs, batchSizeLimit)
}

func (a *aggregator) aggregateResourceReports(ctx context.Context, input <-chan types.EnrichedQueueMetric, output chan<- []reports.ResourceReportsRow, batchSizeLimit int) error {
	report := map[reports.ResourceReportsRowKey]reports.ResourceReportsRow{}
LOOP:
	for {
		var m types.EnrichedQueueMetric
		var ok bool
		select {
		case m, ok = <-input:
			if !ok {
				break LOOP
			}
		case <-ctx.Done():
			return ctx.Err()
		}

		key := reports.ResourceReportsRowKey{
			BillingAccountID: m.BillingAccountID,
			Date:             qtool.DateString(m.Usage.Finish),
			CloudID:          m.CloudID,
			SkuID:            m.SkuID,
			FolderID:         m.FolderID,
			ResourceID:       m.ResourceID,
			LabelsHash:       m.LabelsHash,
		}
		r, ok := report[key]
		if !ok {
			r = reports.ResourceReportsRow{
				BillingAccountID: m.BillingAccountID,
				Date:             qtool.DateString(m.Usage.Finish),
				CloudID:          m.CloudID,
				SkuID:            m.SkuID,
				FolderID:         m.FolderID,
				ResourceID:       m.ResourceID,
				LabelsHash:       m.LabelsHash}
			report[key] = r
		}
		r.PricingQuantity = a.decAdd(r.PricingQuantity, m.PricingQuantity)
		r.Cost = a.decAdd(r.Cost, m.Cost)
		r.Credit = a.decAdd(r.Credit, m.Credit)
		r.CudCredit = a.decAdd(r.CudCredit, m.CudCredit)
		r.MonetaryGrantCredit = a.decAdd(r.MonetaryGrantCredit, m.MonetaryGrantCredit)
		r.VolumeIncentiveCredit = a.decAdd(r.VolumeIncentiveCredit, m.VolumeIncentiveCredit)
		r.UpdatedAt = qtool.UInt64Ts(time.Now())
		report[key] = r

		if len(report) >= batchSizeLimit {
			err := a.pushReport(ctx, report, output)
			if err != nil {
				return err
			}
			report = map[reports.ResourceReportsRowKey]reports.ResourceReportsRow{}
		}
	}
	return a.pushReport(ctx, report, output)
}

func (a *aggregator) decAdd(d qtool.DefaultDecimal, v types.JSONDecimal) qtool.DefaultDecimal {
	return qtool.DefaultDecimal(decimal.Decimal128(d).Add(decimal.Decimal128(v)))
}

func (a *aggregator) pushReport(ctx context.Context, report map[reports.ResourceReportsRowKey]reports.ResourceReportsRow, output chan<- []reports.ResourceReportsRow) error {
	reportRows := make([]reports.ResourceReportsRow, len(report))
	i := 0
	for _, v := range report {
		reportRows[i] = v
		i++
	}

	select {
	case output <- reportRows:
	case <-ctx.Done():
		return ctx.Err()
	}
	return nil
}
