package resolving

import (
	"a.yandex-team.ru/cloud/billing/go/pkg/decimal"
	"a.yandex-team.ru/cloud/billing/go/pkg/selfsku/tools"
)

type TestMetric struct {
	Schema  string                 `yaml:"schema" json:"schema"`
	Version string                 `yaml:"version" json:"version"`
	Usage   MetricUsage            `yaml:"usage" json:"usage"`
	Tags    map[string]interface{} `yaml:"tags" json:"tags"`
}

type MetricUsage struct {
	Quantity decimal.Decimal128 `yaml:"quantity" json:"quantity"`
	Start    int64              `yaml:"start" json:"start"`
	Finish   int64              `yaml:"finish" json:"finish"`
	Unit     string             `yaml:"unit" json:"unit"`
	Type     string             `yaml:"type" json:"type"`
}

type ResultQuantity struct {
	Quantity decimal.Decimal128 `yaml:"quantity"`
	Unit     string             `yaml:"unit"`
}

type SkuResult struct {
	Usage   ResultQuantity `yaml:"usage"`
	Pricing ResultQuantity `yaml:"pricing"`
}

type TestCase struct {
	tools.SourcePath `yaml:"-"`
	Metric           TestMetric           `yaml:"metric"`
	Skus             map[string]SkuResult `yaml:"skus"`
}
