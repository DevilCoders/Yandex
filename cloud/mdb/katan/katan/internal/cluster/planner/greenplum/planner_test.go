package greenplum_test

import (
	"testing"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/katan/internal/tags"
	"a.yandex-team.ru/cloud/mdb/katan/katan/internal/cluster/planner"
	"a.yandex-team.ru/cloud/mdb/katan/katan/internal/cluster/planner/greenplum"
)

func TestGroupBySubCluster(t *testing.T) {
	t.Run("group by success when master service specified", func(t *testing.T) {
		ret, err := greenplum.GroupBySubCluster(
			planner.Cluster{
				Hosts: map[string]planner.Host{
					"master-host.db.yandex.net": {
						Services: map[string]planner.Service{
							"greenplum_master": {
								Role:        "Master",
								ReplicaType: "Unknown",
								Status:      "Alive",
							},
						},
						Tags: tags.HostTags{
							Meta: tags.HostMeta{
								Roles: []string{"greenplum_cluster.master_subcluster"},
							},
						},
					},
					"replica-host.db.yandex.net": {
						Services: map[string]planner.Service{
							"greenplum_master": {
								Role:        "Replica",
								ReplicaType: "Unknown",
								Status:      "Alive",
							},
						},
						Tags: tags.HostTags{
							Meta: tags.HostMeta{
								Roles: []string{"greenplum_cluster.master_subcluster"},
							},
						},
					},
					"segment-primary-host.db.yandex.net": {
						Services: map[string]planner.Service{
							"greenplum_segments": {},
						},
						Tags: tags.HostTags{
							Meta: tags.HostMeta{
								Roles: []string{"greenplum_cluster.segment_subcluster"},
							},
						},
					},
					"segment-mirror-host.db.yandex.net": {
						Services: map[string]planner.Service{
							"greenplum_segments": {},
						},
						Tags: tags.HostTags{
							Meta: tags.HostMeta{
								Roles: []string{"greenplum_cluster.segment_subcluster"},
							},
						},
					},
				},
			})
		require.NoError(t, err)
		require.Equal(t, map[string][]string{
			greenplum.MasterSubCluster: {
				"replica-host.db.yandex.net",
				"master-host.db.yandex.net",
			},
			greenplum.SegmentsSubCluster: {
				"segment-mirror-host.db.yandex.net",
				"segment-primary-host.db.yandex.net",
			},
		}, ret)
	})
	t.Run("group by failed when no master service specivied", func(t *testing.T) {
		ret, err := greenplum.GroupBySubCluster(
			planner.Cluster{
				Hosts: map[string]planner.Host{
					"master-host.db.yandex.net":  {},
					"replica-host.db.yandex.net": {},
				},
			})
		require.Error(t, err)
		require.Nil(t, ret)
	})
}

func TestPlanner(t *testing.T) {
	t.Run("empty cluster is not ok", func(t *testing.T) {
		ret, err := greenplum.Planner(planner.Cluster{})
		require.Nil(t, ret)
		require.Error(t, err)
	})
	t.Run("service without greenplum_master is not ok", func(t *testing.T) {
		ret, err := greenplum.Planner(
			planner.Cluster{
				Hosts: map[string]planner.Host{
					"p1": {
						Services: map[string]planner.Service{
							"bouncer": {},
						},
					},
				},
			},
		)
		require.Error(t, err)
		require.Nil(t, ret)
	})
	t.Run("service without greenplum_segment is not ok", func(t *testing.T) {
		ret, err := greenplum.Planner(
			planner.Cluster{
				Hosts: map[string]planner.Host{
					"p1": {
						Services: map[string]planner.Service{
							"bouncer": {},
						},
					},
				},
			},
		)
		require.Error(t, err)
		require.Nil(t, ret)
	})
	t.Run("get plan on alive cluster", func(t *testing.T) {
		ret, err := greenplum.Planner(
			planner.Cluster{
				Hosts: map[string]planner.Host{
					"master-host.db.yandex.net": {
						Services: map[string]planner.Service{
							"greenplum_master": {
								Role:        "Master",
								ReplicaType: "Unknown",
								Status:      "Alive",
							},
						},
						Tags: tags.HostTags{
							Meta: tags.HostMeta{
								Roles: []string{"greenplum_cluster.master_subcluster"},
							},
						},
					},
					"replica-host.db.yandex.net": {
						Services: map[string]planner.Service{
							"greenplum_master": {
								Role:        "Replica",
								ReplicaType: "Unknown",
								Status:      "Alive",
							},
						},
						Tags: tags.HostTags{
							Meta: tags.HostMeta{
								Roles: []string{"greenplum_cluster.master_subcluster"},
							},
						},
					},
					"segment-primary-host.db.yandex.net": {
						Services: map[string]planner.Service{
							"greenplum_segments": {},
						},
						Tags: tags.HostTags{
							Meta: tags.HostMeta{
								Roles: []string{"greenplum_cluster.segment_subcluster"},
							},
						},
					},
					"segment-mirror-host.db.yandex.net": {
						Services: map[string]planner.Service{
							"greenplum_segments": {},
						},
						Tags: tags.HostTags{
							Meta: tags.HostMeta{
								Roles: []string{"greenplum_cluster.segment_subcluster"},
							},
						},
					},
				},
			},
		)
		require.NoError(t, err)
		require.Equal(t, [][]string{{"segment-mirror-host.db.yandex.net"}, {"segment-primary-host.db.yandex.net"}, {"replica-host.db.yandex.net"}, {"master-host.db.yandex.net"}}, ret)

	})
}
