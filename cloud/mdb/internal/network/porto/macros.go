package porto

import (
	"sort"
	"strings"

	"a.yandex-team.ru/cloud/mdb/internal/racktables"
)

func GetNetworksWithMatchedOwners(macrosWithOwners racktables.MacrosWithOwners, abcSlug string) []string {
	networkKeys := make(map[string]bool)

	for macro, owners := range macrosWithOwners {
		for _, owner := range owners {
			switch owner.Type {
			case racktables.OwnerTypeService:
				ownerAbcSlug := strings.TrimPrefix(owner.Name, "svc_")
				if ownerAbcSlug == abcSlug {
					networkKeys[macro] = true
				}
			case racktables.OwnerTypeServiceRole:
				ownerAbcSlugAndRole := strings.Split(owner.Name, "_")
				if len(ownerAbcSlugAndRole) >= 3 && ownerAbcSlugAndRole[1] == abcSlug {
					networkKeys[macro] = true
				}
			}
		}
	}

	var networks []string

	for network := range networkKeys {
		networks = append(networks, network)
	}

	sort.Strings(networks)

	return networks
}
