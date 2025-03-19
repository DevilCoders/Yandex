package mwswitch

import (
	"golang.org/x/exp/slices"
)

type EnabledMWConfig struct {
	Services []string `json:"services" yaml:"services"`
	Roles    []string `json:"roles" yaml:"roles"`
}

func IsAutomaticMasterWhipEnabledMessage(serviceName string, cfg EnabledMWConfig) string {
	if slices.Contains(cfg.Services, serviceName) {
		return "Expecting leader to be reelected automatically, through maintenance."
	}
	return "MDB-16289 is not yet supported for this service, so you (the DUTY) have to notify customers by contacting support, see example ticket YCLOUD-4346."
}

func IsAutomaticMasterWhipEnabledAlert(hostRoles []string, cfg EnabledMWConfig) bool {
	for _, roleCandidate := range cfg.Roles {
		if slices.Contains(hostRoles, roleCandidate) {
			return true
		}
	}
	return false
}
