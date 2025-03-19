package opmetas

import deploymodels "a.yandex-team.ru/cloud/mdb/deploy/api/pkg/models"

type DeployShipmentMeta interface {
	GetShipments() ShipmentsInfo
	SetShipment(fqdn string, ID deploymodels.ShipmentID)
}

type ShipmentsInfo map[string][]deploymodels.ShipmentID
