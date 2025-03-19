package opmetas

import deploymodels "a.yandex-team.ru/cloud/mdb/deploy/api/pkg/models"

var _ OpMeta = &PreRestartStepMeta{}

type Dom0StateMeta struct {
	IPs        []string      `json:"ips"`
	Shipments  ShipmentsInfo `json:"shipments"`
	SwitchPort SwitchPort    `json:"switch_port"`
}

func (sm *Dom0StateMeta) SetShipment(fqdn string, ID deploymodels.ShipmentID) {
	history, ok := sm.Shipments[fqdn]
	if !ok {
		history = []deploymodels.ShipmentID{}
	}
	history = append(history, ID)
	sm.Shipments[fqdn] = history
}

func (sm *Dom0StateMeta) GetShipments() ShipmentsInfo {
	return sm.Shipments
}

func NewDom0StateMeta() *Dom0StateMeta {
	return &Dom0StateMeta{
		Shipments: ShipmentsInfo{},
	}
}
