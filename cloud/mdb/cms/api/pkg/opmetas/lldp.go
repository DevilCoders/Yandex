package opmetas

import "fmt"

type LLDP struct {
	LLDP struct {
		Interface map[string]struct {
			Chassis map[string]interface{} `json:"chassis"`
			Port    struct {
				ID struct {
					Value string `json:"value"`
				} `json:"id"`
			} `json:"port"`
		} `json:"interface"`
	} `json:"lldp"`
}

type SwitchPort struct {
	Switch string `json:"switch"`
	Port   string `json:"port"`
}

func (s *SwitchPort) String() string {
	return fmt.Sprintf("switch %q and port %q", s.Switch, s.Port)
}

func (l *LLDP) SwitchPorts() (res []SwitchPort) {
	for _, iface := range l.LLDP.Interface {
		for sw := range iface.Chassis {
			res = append(res, SwitchPort{Switch: sw, Port: iface.Port.ID.Value})
		}
	}
	return res
}
