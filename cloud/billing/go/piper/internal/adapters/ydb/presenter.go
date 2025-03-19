package ydb

import (
	"context"
	"time"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/core/entities"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/db/ydb/qtool"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/db/ydb/reports"
)

type PresenterSession struct {
	commonSession

	adapter *DataAdapter
	rows    []reports.ResourceReportsRow
}

func (a *DataAdapter) PresenterSession() *PresenterSession {
	if !a.EnablePresenter {
		panic("misconfiguration: presenter sessions disabled in this ydb adapter")
	}
	return &PresenterSession{
		adapter: a,
	}
}

func (s *PresenterSession) PushPresenterMetric(ctx context.Context, scope entities.ProcessingScope, m entities.PresenterMetric) error {
	row := reports.ResourceReportsRow{
		BillingAccountID:      m.BillingAccountID,
		Date:                  qtool.DateString(m.Usage.Finish),
		CloudID:               m.CloudID,
		SkuID:                 m.SkuID,
		FolderID:              m.FolderID,
		ResourceID:            m.ResourceID,
		LabelsHash:            m.LabelsHash,
		PricingQuantity:       qtool.DefaultDecimal(m.PricingQuantity),
		Cost:                  qtool.DefaultDecimal(m.Cost),
		Credit:                qtool.DefaultDecimal(m.Credit),
		CudCredit:             qtool.DefaultDecimal(m.CudCredit),
		MonetaryGrantCredit:   qtool.DefaultDecimal(m.MonetaryGrantCredit),
		VolumeIncentiveCredit: qtool.DefaultDecimal(m.VolumeIncentiveCredit),
		// TODO: ? FreeCredit:            qtool.DefaultDecimal(m.FreeCredit),
	}
	s.rows = append(s.rows, row)
	return nil
}

func (s *PresenterSession) FlushPresenterMetric(ctx context.Context) (resErr error) {
	tx, err := s.adapter.db.BeginTxx(ctx, serializable())
	if err != nil {
		return err
	}
	defer func() {
		autoTx(tx, resErr)
	}()

	queries := reports.NewQueries(tx, s.adapter.queryParams)
	updatedAt := qtool.UInt64Ts(time.Now())

	err = queries.PushResourceReports(ctx, updatedAt, s.rows...)
	if err != nil {
		return err
	}
	s.rows = s.rows[:0]
	return nil
}
