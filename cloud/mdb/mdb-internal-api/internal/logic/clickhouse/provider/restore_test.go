package provider

import (
	"testing"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/clickhouse/provider/internal/chpillars"
	bmodel "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/backups"
)

func TestClickHouse_OverlappedShardGroupsByBackups(t *testing.T) {
	shardGroupName1, shardGroupName2 := "sg1", "sg2"
	shardName1, shardName2, shardName3, shardName4 := "s1", "s2", "s3", "s4"

	shardGroupsCommon := map[string]chpillars.ShardGroup{
		shardGroupName1: {
			ShardNames: []string{shardName1, shardName2},
		},
		shardGroupName2: {
			ShardNames: []string{shardName3, shardName4},
		},
	}

	backupsCommon := []bmodel.Backup{
		{SourceShardNames: []string{shardName1}},
		{SourceShardNames: []string{shardName2}},
		{SourceShardNames: []string{shardName3}},
		{SourceShardNames: []string{shardName4}},
	}

	t.Run("EmptyContainersOverlapInVoid", func(t *testing.T) {
		shardGroups := getOverlappedShardGroupsByBackups(map[string]chpillars.ShardGroup{}, []bmodel.Backup{})
		require.Equal(t, map[string]chpillars.ShardGroup{}, shardGroups)
	})

	t.Run("NoShardGroups", func(t *testing.T) {
		shardGroups := getOverlappedShardGroupsByBackups(map[string]chpillars.ShardGroup{}, backupsCommon)
		require.Equal(t, map[string]chpillars.ShardGroup{}, shardGroups)
	})

	t.Run("NoBackups", func(t *testing.T) {
		shardGroups := getOverlappedShardGroupsByBackups(shardGroupsCommon, []bmodel.Backup{})
		require.Equal(t, map[string]chpillars.ShardGroup{}, shardGroups)
	})

	t.Run("FullOverlap", func(t *testing.T) {
		shardGroups := getOverlappedShardGroupsByBackups(shardGroupsCommon, backupsCommon)
		require.Equal(t, shardGroupsCommon, shardGroups)
	})

	t.Run("PartialOverlapWithFirstShardGroup", func(t *testing.T) {
		shardGroups := getOverlappedShardGroupsByBackups(
			shardGroupsCommon,
			[]bmodel.Backup{
				backupsCommon[0],
				backupsCommon[1],
			},
		)
		require.Equal(
			t,
			map[string]chpillars.ShardGroup{
				shardGroupName1: shardGroupsCommon[shardGroupName1],
			},
			shardGroups,
		)
	})

	t.Run("PartialOverlapWithSecondShardGroup", func(t *testing.T) {
		shardGroups := getOverlappedShardGroupsByBackups(
			shardGroupsCommon,
			[]bmodel.Backup{
				backupsCommon[2],
				backupsCommon[3],
			},
		)
		require.Equal(
			t,
			map[string]chpillars.ShardGroup{
				shardGroupName2: shardGroupsCommon[shardGroupName2],
			},
			shardGroups,
		)
	})

	t.Run("PartialOverlapWithBothShardGroups", func(t *testing.T) {
		shardGroups := getOverlappedShardGroupsByBackups(
			shardGroupsCommon,
			[]bmodel.Backup{
				backupsCommon[0],
				backupsCommon[2],
			},
		)
		require.Equal(
			t,
			map[string]chpillars.ShardGroup{
				shardGroupName1: {
					ShardNames: []string{shardName1},
				},
				shardGroupName2: {
					ShardNames: []string{shardName3},
				},
			},
			shardGroups,
		)
	})
}
