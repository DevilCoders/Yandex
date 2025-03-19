package opmetas

import (
	deploymodels "a.yandex-team.ru/cloud/mdb/deploy/api/pkg/models"
)

var _ OpMeta = &StopContainersMeta{}
var _ DeployShipmentMeta = &StopContainersMeta{}

type StopContainersMeta struct {
	Shipments ShipmentsInfo `json:"shipments"`
}

func (sm *StopContainersMeta) SetShipment(fqdn string, ID deploymodels.ShipmentID) {
	history, ok := sm.Shipments[fqdn]
	if !ok {
		history = []deploymodels.ShipmentID{}
	}
	history = append(history, ID)
	sm.Shipments[fqdn] = history
}

func (sm *StopContainersMeta) GetShipments() ShipmentsInfo {
	return sm.Shipments
}

func NewStopContainersMeta() DeployShipmentMeta {
	return &StopContainersMeta{
		Shipments: ShipmentsInfo{},
	}
}
