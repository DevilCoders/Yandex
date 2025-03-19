package applicables

import "context"

type Applier interface {
	ApplyUnits(context.Context, []Unit) error
	ApplyServices(context.Context, []Service) error
	ApplySchemas(context.Context, []Schema) error
	ApplySkus(context.Context, []Sku) error
}
