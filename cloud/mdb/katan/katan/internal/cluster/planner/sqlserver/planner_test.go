package sqlserver

import (
	"testing"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/katan/internal/tags"
	"a.yandex-team.ru/cloud/mdb/katan/katan/internal/cluster/planner"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types"
)

func TestPlanner(t *testing.T) {
	t.Run("empty cluster is not ok", func(t *testing.T) {
		ret, err := Planner(planner.Cluster{})
		require.Error(t, err)
		require.Nil(t, ret)
	})

	t.Run("service without sqlserver is not ok", func(t *testing.T) {
		ret, err := Planner(
			planner.Cluster{
				Hosts: map[string]planner.Host{
					"p1": {
						Services: map[string]planner.Service{
							"irrelevant_service": {},
						},
					},
				},
			},
		)
		require.Error(t, err)
		require.Nil(t, ret)
	})

	t.Run("cluster with one host", func(t *testing.T) {
		ret, err := Planner(
			planner.Cluster{
				Hosts: map[string]planner.Host{
					"p1": {
						Services: map[string]planner.Service{
							ServiceName: {
								Role: types.ServiceRoleMaster,
							},
						},
						Tags: tags.HostTags{
							Meta: tags.HostMeta{
								Roles: []string{string(hostRoleSQLServer)},
							},
						},
					},
				},
			},
		)

		require.NoError(t, err)
		require.Equal(t, [][]string{{"p1"}}, ret)
	})

	t.Run("cluster with unknown master role", func(t *testing.T) {
		ret, err := Planner(
			planner.Cluster{
				Hosts: map[string]planner.Host{
					"p1": {
						Services: map[string]planner.Service{
							ServiceName: {
								Role: types.ServiceRoleUnknown,
							},
						},
						Tags: tags.HostTags{
							Meta: tags.HostMeta{
								Roles: []string{string(hostRoleSQLServer)},
							},
						},
					},
				},
			},
		)
		require.Error(t, err)
		require.Nil(t, ret)
	})

	t.Run("cluster without master", func(t *testing.T) {
		ret, err := Planner(
			planner.Cluster{
				Hosts: map[string]planner.Host{
					"p1": {
						Services: map[string]planner.Service{
							ServiceName: {
								Role: types.ServiceRoleReplica,
							},
						},
						Tags: tags.HostTags{
							Meta: tags.HostMeta{
								Roles: []string{string(hostRoleSQLServer)},
							},
						},
					},
				},
			},
		)

		require.Error(t, err)
		require.Nil(t, ret)
	})

	t.Run("cluster with 2 masters", func(t *testing.T) {
		ret, err := Planner(
			planner.Cluster{
				Hosts: map[string]planner.Host{
					"p1": {
						Services: map[string]planner.Service{
							ServiceName: {
								Role: types.ServiceRoleMaster,
							},
						},
						Tags: tags.HostTags{
							Meta: tags.HostMeta{
								Roles: []string{string(hostRoleSQLServer)},
							},
						},
					},
					"p2": {
						Services: map[string]planner.Service{
							ServiceName: {
								Role: types.ServiceRoleMaster,
							},
						},
						Tags: tags.HostTags{
							Meta: tags.HostMeta{
								Roles: []string{string(hostRoleSQLServer)},
							},
						},
					},
				},
			},
		)

		require.Error(t, err)
		require.Nil(t, ret)
	})

	t.Run("2 node cluster with witness", func(t *testing.T) {
		ret, err := Planner(
			planner.Cluster{
				Hosts: map[string]planner.Host{
					"master": {
						Services: map[string]planner.Service{
							ServiceName: {
								Role: types.ServiceRoleMaster,
							},
						},
						Tags: tags.HostTags{
							Meta: tags.HostMeta{
								Roles: []string{string(hostRoleSQLServer)},
							},
						},
					},
					"replica": {
						Services: map[string]planner.Service{
							ServiceName: {
								Role:        types.ServiceRoleReplica,
								ReplicaType: types.ServiceReplicaTypeSync,
							},
						},
						Tags: tags.HostTags{
							Meta: tags.HostMeta{
								Roles: []string{string(hostRoleSQLServer)},
							},
						},
					},
					"witness": {
						Services: map[string]planner.Service{
							ServiceName: {
								Role:        types.ServiceRoleUnknown,
								ReplicaType: types.ServiceReplicaTypeUnknown,
							},
						},
						Tags: tags.HostTags{
							Meta: tags.HostMeta{
								Roles: []string{string(hostRoleWitness)},
							},
						},
					},
				},
			},
		)

		require.NoError(t, err)
		require.Equal(t, [][]string{{"witness"}, {"replica"}, {"master"}}, ret)
	})

	t.Run("3 node cluster", func(t *testing.T) {
		ret, err := Planner(
			planner.Cluster{
				Hosts: map[string]planner.Host{
					"master": {
						Services: map[string]planner.Service{
							ServiceName: {
								Role: types.ServiceRoleMaster,
							},
						},
						Tags: tags.HostTags{
							Meta: tags.HostMeta{
								Roles: []string{string(hostRoleSQLServer)},
							},
						},
					},
					"r1": {
						Services: map[string]planner.Service{
							ServiceName: {
								Role:        types.ServiceRoleReplica,
								ReplicaType: types.ServiceReplicaTypeSync,
							},
						},
						Tags: tags.HostTags{
							Meta: tags.HostMeta{
								Roles: []string{string(hostRoleSQLServer)},
							},
						},
					},
					"r2": {
						Services: map[string]planner.Service{
							ServiceName: {
								Role:        types.ServiceRoleReplica,
								ReplicaType: types.ServiceReplicaTypeSync,
							},
						},
						Tags: tags.HostTags{
							Meta: tags.HostMeta{
								Roles: []string{string(hostRoleSQLServer)},
							},
						},
					},
				},
			},
		)

		require.NoError(t, err)
		// r1, r2 may appear in random order, that's okay
		require.ElementsMatch(t, [][]string{{"r1"}, {"r2"}, {"master"}}, ret)
		require.Equal(t, "master", ret[2][0])
	})
}
