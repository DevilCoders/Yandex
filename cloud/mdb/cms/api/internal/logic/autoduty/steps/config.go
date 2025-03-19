package steps

import "a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/helpers/shipments"

type StepsCfg struct {
	RegisterMinion RegisterMinionStepConfig      `json:"register_minion" yaml:"register_minion"`
	NewHosts       NewHostsConfig                `json:"new_hosts" yaml:"new_hosts"`
	Shipments      shipments.AwaitShipmentConfig `json:"shipments" yaml:"shipments"`
	SelfDNS        SelfDNSConfig                 `json:"selfdns" yaml:"selfdns"`
	Drills         DrillsConfig                  `json:"drills" yaml:"drills"`
}
