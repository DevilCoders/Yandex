package ydb

import (
	"context"
	"errors"
	"fmt"
	"time"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/core/entities"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/db/ydb/qtool"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/db/ydb/usage"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/pkg/errtool"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/pkg/timetool"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling"
	"a.yandex-team.ru/cloud/billing/go/pkg/decimal"
)

func (s *MetaSession) CalculateCumulativeUsage(
	ctx context.Context, scope entities.ProcessingScope, period entities.UsagePeriod, src []entities.CumulativeSource,
) (diffCount int, log []entities.CumulativeUsageLog, err error) {
	if period.PeriodType != entities.MonthlyUsage {
		err = ErrCumulativePeriodType.Wrap(errors.New(period.PeriodType.String()))
		return
	}
	month := timetool.MonthStart(period.Period)
	if err = s.initCumulative(ctx, month.Location()); err != nil {
		return
	}
	if diffCount, err = s.calculateCumulative(ctx, scope, month, src); err != nil {
		return
	}
	log, err = s.fetchLogs(ctx, scope, month)
	return
}

func (s *MetaSession) initCumulative(ctx context.Context, loc *time.Location) error {
	// NOTE: This action performed in python implementation but not necessary so skip here
	// if err := sch.CreateCumulativeTracking(ctx); err != nil {
	// 	return ErrCumulativeInit.Wrap(err)
	// }

	curMonth := timetool.MonthStart(time.Now().In(loc))
	months := []time.Time{
		curMonth.AddDate(0, -1, 0),
		curMonth,
		curMonth.AddDate(0, 1, 0),
	}

	for _, mnth := range months {
		if err := s.adapter.ensureCumulative(mnth, func() error { return s.createCumulativeLog(ctx, mnth) }); err != nil {
			return err
		}
	}
	return nil
}

func (s *MetaSession) createCumulativeLog(ctx context.Context, month time.Time) error {
	sess, err := s.adapter.schemeDB.Get(ctx)
	if err != nil {
		return errtool.WrapNotCtxErr(ctx, ErrSchemeSession, err)
	}
	defer s.adapter.schemeDB.Put(ctx, sess)

	sch := usage.NewScheme(sess, s.adapter.queryParams)

	if err := sch.CreateCumulativeLog(ctx, month); err != nil {
		return ErrCumulativeInit.Wrap(err)
	}
	return nil
}

func (s *MetaSession) calculateCumulative(
	ctx context.Context, scope entities.ProcessingScope, month time.Time, src []entities.CumulativeSource,
) (count int, err error) {
	records := make([]usage.CumulativeCalculateRecord, 0, len(src))
	for _, s := range src {
		records = append(records, usage.CumulativeCalculateRecord{
			ResourceID: s.ResourceID,
			SequenceID: s.MetricOffset,
			SkuID:      s.SkuID,
			Quantity:   qtool.DefaultDecimal(s.PricingQuantity),
		})
	}
	queries := usage.New(s.adapter.db, s.adapter.queryParams)
	sourceID := fmt.Sprintf("%s:%s", scope.SourceType, scope.SourceID)
	count, err = queries.CalculateCumulative(ctx, month, sourceID, records...)
	return
}

func (s *MetaSession) fetchLogs(
	ctx context.Context, scope entities.ProcessingScope, month time.Time,
) (log []entities.CumulativeUsageLog, err error) {
	monthStr := month.Format("2006-01-02")
	limit := uint64(s.paramsBatchSize())

	cursor := usage.CumulativeLogCursor{}
	for {
		var dbResult []usage.CumulativeLogRow
		retryCtx := tooling.StartRetry(ctx)
		err = s.retryRead(retryCtx, func() (resErr error) {
			tooling.RetryIteration(retryCtx)

			tx, err := s.adapter.db.BeginTxx(retryCtx, readCommitted())
			if err != nil {
				return err
			}
			defer func() {
				autoTx(tx, resErr)
			}()

			queries := usage.New(tx, s.adapter.queryParams)
			sourceID := fmt.Sprintf("%s:%s", scope.SourceType, scope.SourceID)
			dbResult, resErr = queries.GetCumulativeLogByOffset(
				retryCtx, month, sourceID, scope.MinMessageOffset, scope.MaxMessageOffset, limit, cursor,
			)
			return
		})
		if err != nil {
			return
		}
		for _, r := range dbResult {
			log = append(log, entities.CumulativeUsageLog{
				FirstPeriod:  r.FirstUseMonth == monthStr,
				ResourceID:   r.ResourceID,
				SkuID:        r.SkuID,
				MetricOffset: r.SequenceID,
				Delta:        decimal.Decimal128(r.Delta),
			})
		}
		if len(dbResult) < int(limit) {
			break
		}
		lastRec := dbResult[len(dbResult)-1]
		cursor.ResourceID = lastRec.ResourceID
		cursor.SequenceID = lastRec.SequenceID
		cursor.SkuID = lastRec.SkuID
	}
	return
}

func (a *MetaAdapter) ensureCumulative(month time.Time, f func() error) error {
	a.muCumulative.Lock()
	defer a.muCumulative.Unlock()

	if a.ensuredCumulativeLogs[month] {
		return nil
	}

	err := f()
	if err == nil {
		a.ensuredCumulativeLogs[month] = true
	}
	return err
}
