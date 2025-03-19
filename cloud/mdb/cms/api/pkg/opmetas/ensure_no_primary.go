package opmetas

import (
	deploymodels "a.yandex-team.ru/cloud/mdb/deploy/api/pkg/models"
)

var _ OpMeta = &EnsureNoPrimaryMeta{}
var _ DeployShipmentMeta = &EnsureNoPrimaryMeta{}

type EnsureNoPrimaryMeta struct {
	Shipments ShipmentsInfo `json:"shipments"`
}

func (sm *EnsureNoPrimaryMeta) SetShipment(fqdn string, ID deploymodels.ShipmentID) {
	history, ok := sm.Shipments[fqdn]
	if !ok {
		history = []deploymodels.ShipmentID{}
	}
	history = append(history, ID)
	sm.Shipments[fqdn] = history
}

func (sm *EnsureNoPrimaryMeta) GetShipments() ShipmentsInfo {
	return sm.Shipments
}

func NewEnsureNoPrimaryMeta() *EnsureNoPrimaryMeta {
	return &EnsureNoPrimaryMeta{
		Shipments: ShipmentsInfo{},
	}
}
