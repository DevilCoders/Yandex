package entities

import (
	"time"

	"a.yandex-team.ru/cloud/billing/go/pkg/decimal"
)

type CumulativeSource struct {
	ResourceID      string
	SkuID           string
	PricingQuantity decimal.Decimal128
	MetricOffset    uint64
}

type UsagePeriod struct {
	Period     time.Time
	PeriodType UsagePeriodType
}

type CumulativeUsageLog struct {
	FirstPeriod  bool
	ResourceID   string
	SkuID        string
	MetricOffset uint64
	Delta        decimal.Decimal128
}
