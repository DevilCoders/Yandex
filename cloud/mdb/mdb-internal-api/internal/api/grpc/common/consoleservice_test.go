package common

import (
	"testing"

	"github.com/stretchr/testify/require"

	consolemodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/console"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts"
)

func TestAggregateResourcePresets(t *testing.T) {
	data := []struct {
		inputPresets []consolemodels.ResourcePreset
		expected     map[hosts.Role]map[string]map[string][]consolemodels.ResourcePreset
	}{
		{
			inputPresets: []consolemodels.ResourcePreset{},
			expected:     map[hosts.Role]map[string]map[string][]consolemodels.ResourcePreset{},
		},
		{
			inputPresets: []consolemodels.ResourcePreset{
				{
					ExtID: "preset1",
					Role:  hosts.RolePostgreSQL,
					Zone:  "sas",
				},
			},
			expected: map[hosts.Role]map[string]map[string][]consolemodels.ResourcePreset{
				hosts.RolePostgreSQL: {
					"preset1": {
						"sas": {
							{
								ExtID: "preset1",
								Role:  hosts.RolePostgreSQL,
								Zone:  "sas",
							},
						},
					},
				},
			},
		},
		{
			inputPresets: []consolemodels.ResourcePreset{
				{
					ExtID:         "preset1",
					Role:          hosts.RolePostgreSQL,
					Zone:          "sas",
					DiskTypeExtID: "disk_type1",
				},
				{
					ExtID:         "preset1",
					Role:          hosts.RolePostgreSQL,
					Zone:          "sas",
					DiskTypeExtID: "disk_type2",
				},
				{
					ExtID:         "preset1",
					Role:          hosts.RolePostgreSQL,
					Zone:          "sas",
					DiskTypeExtID: "disk_type3",
				},
			},
			expected: map[hosts.Role]map[string]map[string][]consolemodels.ResourcePreset{
				hosts.RolePostgreSQL: {
					"preset1": {
						"sas": {
							{
								ExtID:         "preset1",
								Role:          hosts.RolePostgreSQL,
								Zone:          "sas",
								DiskTypeExtID: "disk_type1",
							},
							{
								ExtID:         "preset1",
								Role:          hosts.RolePostgreSQL,
								Zone:          "sas",
								DiskTypeExtID: "disk_type2",
							},
							{
								ExtID:         "preset1",
								Role:          hosts.RolePostgreSQL,
								Zone:          "sas",
								DiskTypeExtID: "disk_type3",
							},
						},
					},
				},
			},
		},
		{
			inputPresets: []consolemodels.ResourcePreset{
				{
					ExtID:         "preset1",
					Role:          hosts.RolePostgreSQL,
					Zone:          "sas",
					DiskTypeExtID: "disk_type1",
				},
				{
					ExtID:         "preset1",
					Role:          hosts.RolePostgreSQL,
					Zone:          "sas",
					DiskTypeExtID: "disk_type2",
				},
				{
					ExtID:         "preset2",
					Role:          hosts.RolePostgreSQL,
					Zone:          "sas",
					DiskTypeExtID: "disk_type1",
				},
				{
					ExtID:         "preset1",
					Role:          hosts.RolePostgreSQL,
					Zone:          "vla",
					DiskTypeExtID: "disk_type1",
				},
			},
			expected: map[hosts.Role]map[string]map[string][]consolemodels.ResourcePreset{
				hosts.RolePostgreSQL: {
					"preset1": {
						"sas": {
							{
								ExtID:         "preset1",
								Role:          hosts.RolePostgreSQL,
								Zone:          "sas",
								DiskTypeExtID: "disk_type1",
							},
							{
								ExtID:         "preset1",
								Role:          hosts.RolePostgreSQL,
								Zone:          "sas",
								DiskTypeExtID: "disk_type2",
							},
						},
						"vla": {
							{
								ExtID:         "preset1",
								Role:          hosts.RolePostgreSQL,
								Zone:          "vla",
								DiskTypeExtID: "disk_type1",
							},
						},
					},
					"preset2": {
						"sas": {
							{
								ExtID:         "preset2",
								Role:          hosts.RolePostgreSQL,
								Zone:          "sas",
								DiskTypeExtID: "disk_type1",
							},
						},
					},
				},
			},
		},
		{
			inputPresets: []consolemodels.ResourcePreset{
				{
					ExtID:         "preset1",
					Role:          hosts.RolePostgreSQL,
					Zone:          "sas",
					DiskTypeExtID: "disk_type1",
				},
				{
					ExtID:         "preset1",
					Role:          hosts.RoleClickHouse,
					Zone:          "sas",
					DiskTypeExtID: "disk_type1",
				},
				{
					ExtID:         "preset1",
					Role:          hosts.RoleClickHouse,
					Zone:          "sas",
					DiskTypeExtID: "disk_type2",
				},
				{
					ExtID:         "preset2",
					Role:          hosts.RolePostgreSQL,
					Zone:          "vla",
					DiskTypeExtID: "disk_type1",
				},
			},
			expected: map[hosts.Role]map[string]map[string][]consolemodels.ResourcePreset{
				hosts.RolePostgreSQL: {
					"preset1": {
						"sas": {
							{
								ExtID:         "preset1",
								Role:          hosts.RolePostgreSQL,
								Zone:          "sas",
								DiskTypeExtID: "disk_type1",
							},
						},
					},
					"preset2": {
						"vla": {
							{
								ExtID:         "preset2",
								Role:          hosts.RolePostgreSQL,
								Zone:          "vla",
								DiskTypeExtID: "disk_type1",
							},
						},
					},
				},
				hosts.RoleClickHouse: {
					"preset1": {
						"sas": {
							{
								ExtID:         "preset1",
								Role:          hosts.RoleClickHouse,
								Zone:          "sas",
								DiskTypeExtID: "disk_type1",
							},
							{
								ExtID:         "preset1",
								Role:          hosts.RoleClickHouse,
								Zone:          "sas",
								DiskTypeExtID: "disk_type2",
							},
						},
					},
				},
			},
		},
	}

	for _, d := range data {
		result := AggregateResourcePresets(d.inputPresets)
		require.Equal(t, d.expected, result)
	}
}
