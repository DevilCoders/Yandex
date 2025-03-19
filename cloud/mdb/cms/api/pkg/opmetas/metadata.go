package opmetas

import deploymodels "a.yandex-team.ru/cloud/mdb/deploy/api/pkg/models"

var _ OpMeta = &BringMetadataMeta{}
var _ DeployShipmentMeta = &BringMetadataMeta{}

type BringMetadataMeta struct {
	Shipments ShipmentsInfo `json:"shipments"`
}

func (sm *BringMetadataMeta) SetShipment(fqdn string, ID deploymodels.ShipmentID) {
	history, ok := sm.Shipments[fqdn]
	if !ok {
		history = []deploymodels.ShipmentID{}
	}
	history = append(history, ID)
	sm.Shipments[fqdn] = history
}

func (sm *BringMetadataMeta) GetShipments() ShipmentsInfo {
	return sm.Shipments
}

func NewBringMetadataMeta() *BringMetadataMeta {
	return &BringMetadataMeta{
		Shipments: ShipmentsInfo{},
	}
}
