package postgresql_test

import (
	"testing"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/katan/internal/tags"
	"a.yandex-team.ru/cloud/mdb/katan/katan/internal/cluster/planner"
	"a.yandex-team.ru/cloud/mdb/katan/katan/internal/cluster/planner/postgresql"
	"a.yandex-team.ru/cloud/mdb/katan/katan/internal/models"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types"
)

func C(fqdns ...string) models.Cluster {
	ret := models.Cluster{
		Hosts: make(map[string]tags.HostTags, len(fqdns)),
	}
	for _, f := range fqdns {
		ret.Hosts[f] = tags.HostTags{}
	}
	return ret
}

func TestPlanner(t *testing.T) {
	t.Run("empty cluster is not ok", func(t *testing.T) {
		ret, err := postgresql.Planner(planner.Cluster{})
		require.Error(t, err)
		require.Nil(t, ret)
	})

	t.Run("service without pg_replication is not ok", func(t *testing.T) {
		ret, err := postgresql.Planner(
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

	t.Run("cluster with one host", func(t *testing.T) {
		ret, err := postgresql.Planner(
			planner.Cluster{
				Hosts: map[string]planner.Host{
					"p1": {
						Services: map[string]planner.Service{
							postgresql.ReplicationService: {
								Role: types.ServiceRoleMaster,
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
		ret, err := postgresql.Planner(
			planner.Cluster{
				Hosts: map[string]planner.Host{
					"p1": {
						Services: map[string]planner.Service{
							postgresql.ReplicationService: {
								Role: types.ServiceRoleUnknown,
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
		ret, err := postgresql.Planner(
			planner.Cluster{
				Hosts: map[string]planner.Host{
					"p1": {
						Services: map[string]planner.Service{
							postgresql.ReplicationService: {
								Role: types.ServiceRoleReplica,
							},
						},
					},
				},
			},
		)

		require.Error(t, err)
		require.Nil(t, ret)
	})

	t.Run("cluster without 2 masters", func(t *testing.T) {
		ret, err := postgresql.Planner(
			planner.Cluster{
				Hosts: map[string]planner.Host{
					"p1": {
						Services: map[string]planner.Service{
							postgresql.ReplicationService: {
								Role: types.ServiceRoleMaster,
							},
						},
					},
					"p2": {
						Services: map[string]planner.Service{
							postgresql.ReplicationService: {
								Role: types.ServiceRoleMaster,
							},
						},
					},
				},
			},
		)

		require.Error(t, err)
		require.Nil(t, ret)
	})
	t.Run("3 node cluster", func(t *testing.T) {
		ret, err := postgresql.Planner(
			planner.Cluster{
				Hosts: map[string]planner.Host{
					"master": {
						Services: map[string]planner.Service{
							postgresql.ReplicationService: {
								Role: types.ServiceRoleMaster,
							},
						},
					},
					"sync": {
						Services: map[string]planner.Service{
							postgresql.ReplicationService: {
								Role:        types.ServiceRoleReplica,
								ReplicaType: types.ServiceReplicaTypeSync,
							},
						},
					},
					"async": {
						Services: map[string]planner.Service{
							postgresql.ReplicationService: {
								Role:        types.ServiceRoleReplica,
								ReplicaType: types.ServiceReplicaTypeAsync,
							},
						},
					},
				},
			},
		)

		require.NoError(t, err)
		require.Equal(t, [][]string{{"async"}, {"sync"}, {"master"}}, ret)
	})
}
