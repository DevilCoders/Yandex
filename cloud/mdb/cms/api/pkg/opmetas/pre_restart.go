package opmetas

import (
	deploymodels "a.yandex-team.ru/cloud/mdb/deploy/api/pkg/models"
)

var _ OpMeta = &PreRestartStepMeta{}
var _ DeployShipmentMeta = &PreRestartStepMeta{}

type PreRestartStepMeta struct {
	Shipments ShipmentsInfo `json:"shipments"`
}

func (sm *PreRestartStepMeta) SetShipment(fqdn string, ID deploymodels.ShipmentID) {
	history, ok := sm.Shipments[fqdn]
	if !ok {
		history = []deploymodels.ShipmentID{}
	}
	history = append(history, ID)
	sm.Shipments[fqdn] = history
}

func (sm *PreRestartStepMeta) GetShipments() ShipmentsInfo {
	return sm.Shipments
}

func NewPreRestartMeta() DeployShipmentMeta {
	return &PreRestartStepMeta{
		Shipments: ShipmentsInfo{},
	}
}
