package applier

import (
	"context"

	"gopkg.in/yaml.v2"

	"a.yandex-team.ru/cloud/billing/go/pkg/selfsku/applicables"
	"a.yandex-team.ru/cloud/billing/go/pkg/selfsku/tools"
	"a.yandex-team.ru/library/go/core/resource"
)

func Resources(ctx context.Context, applier applicables.Applier) error {
	var (
		units    []applicables.Unit
		services []applicables.Service
		schemas  []applicables.Schema
		skus     []applicables.Sku
	)
	errs := tools.ValidErrors{}

	errs.CollectWrapped("units load: %w", yaml.Unmarshal(resource.Get("units"), &units))
	errs.CollectWrapped("services load: %w", yaml.Unmarshal(resource.Get("services"), &services))
	errs.CollectWrapped("schemas load: %w", yaml.Unmarshal(resource.Get("schemas"), &schemas))
	errs.CollectWrapped("skus load: %w", yaml.Unmarshal(resource.Get("skus"), &skus))

	if err := errs.Expose(); err != nil {
		return err
	}

	if err := applier.ApplyUnits(ctx, units); err != nil {
		return err
	}
	if err := applier.ApplyServices(ctx, services); err != nil {
		return err
	}
	if err := applier.ApplySchemas(ctx, schemas); err != nil {
		return err
	}
	if err := applier.ApplySkus(ctx, skus); err != nil {
		return err
	}
	return nil
}
