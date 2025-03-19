package opmetas

import (
	deploymodels "a.yandex-team.ru/cloud/mdb/deploy/api/pkg/models"
)

var _ OpMeta = &PostRestartMeta{}
var _ DeployShipmentMeta = &PostRestartMeta{}

type PostRestartMeta struct {
	Shipments ShipmentsInfo `json:"shipments"`
}

func (sm *PostRestartMeta) SetShipment(fqdn string, ID deploymodels.ShipmentID) {
	history, ok := sm.Shipments[fqdn]
	if !ok {
		history = []deploymodels.ShipmentID{}
	}
	history = append(history, ID)
	sm.Shipments[fqdn] = history
}

func (sm *PostRestartMeta) GetShipments() ShipmentsInfo {
	return sm.Shipments
}

func NewPostRestartMeta() DeployShipmentMeta {
	return &PostRestartMeta{
		Shipments: ShipmentsInfo{},
	}
}
