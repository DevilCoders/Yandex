package common

import (
	"sort"

	consolemodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/console"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts"
)

// AggregateResourcePresets aggregates resource presets by Role, PresetID and Zone
func AggregateResourcePresets(presets []consolemodels.ResourcePreset) map[hosts.Role]map[string]map[string][]consolemodels.ResourcePreset {
	aggregated := make(map[hosts.Role]map[string]map[string][]consolemodels.ResourcePreset)
	for _, preset := range presets {
		// Group by role
		roleAgg, ok := aggregated[preset.Role]
		if !ok {
			roleAgg = make(map[string]map[string][]consolemodels.ResourcePreset)
			aggregated[preset.Role] = roleAgg
		}
		// Group by preset id
		presetAgg, ok := roleAgg[preset.ExtID]
		if !ok {
			presetAgg = make(map[string][]consolemodels.ResourcePreset)
			roleAgg[preset.ExtID] = presetAgg
		}
		// Group by zone
		zoneAgg, ok := presetAgg[preset.Zone]
		if !ok {
			zoneAgg = make([]consolemodels.ResourcePreset, 0)
			presetAgg[preset.Zone] = zoneAgg
		}
		zoneAgg = append(zoneAgg, preset)
		presetAgg[preset.Zone] = zoneAgg
	}
	return aggregated
}

func CollectAllVersionNames(clusterVersions []consolemodels.DefaultVersion) []string {
	var versions []string
	for _, ver := range clusterVersions {
		if ver.IsDeprecated {
			continue
		}
		versions = append(versions, ver.Name)
	}

	sort.Strings(versions)
	return versions
}

func CollectDefaultVersion(clusterVersions []consolemodels.DefaultVersion) string {
	for _, ver := range clusterVersions {
		if ver.IsDeprecated {
			continue
		}
		if ver.IsDefault {
			return ver.MajorVersion
		}
	}
	return ""
}
