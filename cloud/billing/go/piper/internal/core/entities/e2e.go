package entities

import (
	"time"

	"a.yandex-team.ru/cloud/billing/go/pkg/decimal"
)

type E2EMetric struct {
	SkuInfo
	Label     string
	Value     decimal.Decimal128
	UsageTime time.Time
}
