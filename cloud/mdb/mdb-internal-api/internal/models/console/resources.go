package console

import (
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/environment"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts"
	"a.yandex-team.ru/library/go/slices"
)

type Int64Range struct {
	Lower int64 `json:"lower" yaml:"lower"`
	Upper int64 `json:"upper" yaml:"upper"`
}

type ResourcePreset struct {
	ExtID           string         `json:"preset_id" yaml:"preset_id"`
	Role            hosts.Role     `json:"role" yaml:"role"`
	Zone            string         `json:"zone" yaml:"zone"`
	CPULimit        int64          `json:"cpu_limit" yaml:"cpu_limit"`
	CPUFraction     int64          `json:"cpu_fraction" yaml:"cpu_fraction"`
	MemoryLimit     int64          `json:"memory_limit" yaml:"memory_limit"`
	DiskTypeExtID   string         `json:"disk_type_id" yaml:"disk_type_id"`
	DiskSizes       []int64        `json:"disk_sizes" yaml:"disk_sizes"`
	DiskSizeRange   Int64Range     `json:"disk_size_range" yaml:"disk_size_range"`
	DefaultDiskSize optional.Int64 `json:"default_disk_size" yaml:"default_disk_size"`
	MinHosts        int64          `json:"min_hosts" yaml:"min_hosts"`
	MaxHosts        int64          `json:"max_hosts" yaml:"max_hosts"`
	FlavorType      string         `json:"flavor_type" yaml:"flavor_type"`
	Generation      int64          `json:"generation" yaml:"generation"`
	GenerationName  string         `json:"generation_name" yaml:"generation_name"`
	Decommissioned  bool           `json:"decommissioning" yaml:"decommissioning"`
}

type ResourcePresets struct {
	Presets []ResourcePreset
}

func MinMaxHosts(presets []ResourcePreset, diskType optional.String) (int64, int64) {
	var (
		minHosts = presets[0].MinHosts
		maxHosts int64
	)
	for _, rp := range presets {
		if diskType.Valid {
			if rp.DiskTypeExtID != diskType.Must() {
				continue
			}
		}

		if rp.MinHosts < minHosts {
			minHosts = rp.MinHosts
		}
		if rp.MaxHosts > maxHosts {
			maxHosts = rp.MaxHosts
		}
	}

	return minHosts, maxHosts
}

func DiskTypes(presets []ResourcePreset) (diskTypes []string) {
	for _, rp := range presets {
		diskTypes = append(diskTypes, rp.DiskTypeExtID)
	}
	diskTypes = slices.DedupStrings(diskTypes)

	return diskTypes
}

func GroupResourcePresetsByRole(presets []ResourcePreset) map[hosts.Role][]ResourcePreset {
	res := map[hosts.Role][]ResourcePreset{}

	for _, rp := range presets {
		res[rp.Role] = make([]ResourcePreset, 0)
	}

	for _, rp := range presets {
		res[rp.Role] = append(res[rp.Role], rp)
	}

	return res
}

func GroupResourcePresetsByZone(presets []ResourcePreset) map[string][]ResourcePreset {
	res := map[string][]ResourcePreset{}

	for _, rp := range presets {
		res[rp.Zone] = make([]ResourcePreset, 0)
	}

	for _, rp := range presets {
		res[rp.Zone] = append(res[rp.Zone], rp)
	}

	return res
}

func GroupResourcePresetsByExtID(presets []ResourcePreset) map[string][]ResourcePreset {
	res := map[string][]ResourcePreset{}

	for _, rp := range presets {
		res[rp.ExtID] = make([]ResourcePreset, 0)
	}

	for _, rp := range presets {
		res[rp.ExtID] = append(res[rp.ExtID], rp)
	}

	return res
}

func GroupResourcePresetsByDiskType(presets []ResourcePreset) map[string][]ResourcePreset {
	res := map[string][]ResourcePreset{}

	for _, rp := range presets {
		res[rp.DiskTypeExtID] = make([]ResourcePreset, 0)
	}

	for _, rp := range presets {
		res[rp.DiskTypeExtID] = append(res[rp.DiskTypeExtID], rp)
	}

	return res
}

type DefaultResources struct {
	ResourcePresetExtID string `json:"resource_preset_id" yaml:"resource_preset_id"`
	DiskTypeExtID       string `json:"disk_type_id" yaml:"disk_type_id"`
	DiskSize            int64  `json:"disk_size" yaml:"disk_size"`
	Generation          int64  `json:"generation" yaml:"generation"`
	GenerationName      string `json:"generation_name" yaml:"generation_name"`
}

type Cloud struct {
	Cloud   environment.CloudType
	Regions []string
}
