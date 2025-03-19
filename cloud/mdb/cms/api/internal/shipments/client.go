package shipments

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/deploy/api/pkg/models"
)

type ShipmentClient interface {
	EnsureNoPrimary(ctx context.Context, fqdn string) (models.ShipmentID, error)
	Shipment(ctx context.Context, id models.ShipmentID) (models.Shipment, error)
}
