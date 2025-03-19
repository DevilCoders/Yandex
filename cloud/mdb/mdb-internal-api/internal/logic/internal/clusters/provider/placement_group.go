package provider

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
)

func (c *Clusters) AddDiskPlacementGroup(ctx context.Context, args models.AddDiskPlacementGroupArgs) (int64, error) {
	return c.metaDB.AddDiskPlacementGroup(ctx, args)
}

func (c *Clusters) AddDisk(ctx context.Context, args models.AddDiskArgs) (int64, error) {
	return c.metaDB.AddDisk(ctx, args)
}

func (c *Clusters) AddPlacementGroup(ctx context.Context, args models.AddPlacementGroupArgs) (int64, error) {
	return c.metaDB.AddPlacementGroup(ctx, args)
}
