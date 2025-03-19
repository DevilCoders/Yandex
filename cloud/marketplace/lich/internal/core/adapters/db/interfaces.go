package db

import (
	"context"

	"a.yandex-team.ru/cloud/marketplace/lich/internal/core/model"
)

type ProductVersionsAdapter interface {
	GetProductVersions(ctx context.Context, ids ...string) ([]model.ProductVersion, error)
}
