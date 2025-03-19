package provider

import (
	"testing"

	"github.com/stretchr/testify/require"

	consolemodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/console"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts"
)

func TestConsole_getHostTypes(t *testing.T) {
	const (
		rangeMin = 1
		rangeMax = 10
	)
	var (
		sizes   = []int64{1, 2, 3}
		presets []consolemodels.ResourcePreset
	)

	for _, role := range []hosts.Role{hosts.RoleZooKeeper, hosts.RoleClickHouse} {
		for _, extID := range []string{"preset2", "preset1"} {
			for _, zone := range []string{"zone2", "zone1"} {
				for _, diskType := range []string{"disk2", "disk1"} {
					presets = append(presets, consolemodels.ResourcePreset{
						Role:          role,
						ExtID:         extID,
						Zone:          zone,
						DiskTypeExtID: diskType,
						DiskSizeRange: consolemodels.Int64Range{
							Lower: rangeMin,
							Upper: rangeMax,
						},
					})
					presets = append(presets, consolemodels.ResourcePreset{
						Role:          role,
						ExtID:         extID,
						Zone:          zone,
						DiskTypeExtID: diskType,
						DiskSizes:     sizes,
					})
				}
			}
		}
	}

	var expected []consolemodels.HostType
	for _, role := range []hosts.Role{hosts.RoleClickHouse, hosts.RoleZooKeeper} {
		var resourcePresets []consolemodels.HostTypeResourcePreset

		for _, extID := range []string{"preset1", "preset2"} {
			var zones []consolemodels.HostTypeZone

			for _, zone := range []string{"zone1", "zone2"} {
				var diskTypes []consolemodels.HostTypeDiskType

				for _, diskType := range []string{"disk1", "disk2"} {
					diskTypes = append(diskTypes, consolemodels.HostTypeDiskType{
						ID: diskType,
						SizeRange: &consolemodels.Int64Range{
							Lower: rangeMin,
							Upper: rangeMax,
						},
						Sizes: nil,
					})
					diskTypes = append(diskTypes, consolemodels.HostTypeDiskType{
						ID:        diskType,
						SizeRange: nil,
						Sizes:     sizes,
					})
				}

				zones = append(zones, consolemodels.HostTypeZone{
					ID:        zone,
					DiskTypes: diskTypes,
				})
			}

			resourcePresets = append(resourcePresets, consolemodels.HostTypeResourcePreset{
				ID:    extID,
				Zones: zones,
			})
		}

		expected = append(expected, consolemodels.HostType{
			Type:            role,
			ResourcePresets: resourcePresets,
		})
	}

	actual := getHostTypes(presets)

	require.Equal(t, expected, actual)
}
