package applier

import (
	"context"

	"a.yandex-team.ru/cloud/billing/go/pkg/selfsku/applicables"
)

var _ applicables.Applier = &StatsApplier{}

type StatsApplier struct {
	Units    int
	Services int
	Schemas  int
	Skus     int
}

func NewStatsApplier() *StatsApplier {
	return &StatsApplier{}
}

func (a *StatsApplier) ApplyUnits(_ context.Context, input []applicables.Unit) error {
	a.Units = len(input)
	return nil
}

func (a *StatsApplier) ApplyServices(_ context.Context, input []applicables.Service) error {
	a.Services = len(input)
	return nil
}

func (a *StatsApplier) ApplySchemas(_ context.Context, input []applicables.Schema) error {
	a.Schemas = len(input)
	return nil
}

func (a *StatsApplier) ApplySkus(_ context.Context, input []applicables.Sku) error {
	a.Skus = len(input)
	return nil
}
