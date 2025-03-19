package provider

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/deploy/api/pkg/deployapi"
	deployModels "a.yandex-team.ru/cloud/mdb/deploy/api/pkg/models"
	"a.yandex-team.ru/cloud/mdb/internal/metadb"
)

type ShipmentProvider struct {
	d   deployapi.Client
	mDB metadb.MetaDB
}

func (p *ShipmentProvider) Shipment(ctx context.Context, id deployModels.ShipmentID) (deployModels.Shipment, error) {
	return p.d.GetShipment(ctx, id)
}

func NewShipmentProvider(d deployapi.Client, mDB metadb.MetaDB) *ShipmentProvider {
	return &ShipmentProvider{d: d, mDB: mDB}
}
