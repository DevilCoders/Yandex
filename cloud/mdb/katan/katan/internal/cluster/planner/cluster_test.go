package planner_test

import (
	"testing"
	"time"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/katan/internal/tags"
	"a.yandex-team.ru/cloud/mdb/katan/katan/internal/cluster/planner"
	"a.yandex-team.ru/cloud/mdb/katan/katan/internal/models"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types"
)

func TestComposeCluster(t *testing.T) {
	t.Run("empty is ok", func(t *testing.T) {
		ret, err := planner.ComposeCluster(models.Cluster{}, nil)
		require.NoError(t, err)
		require.Empty(t, ret.Hosts)
	})
	t.Run("cluster without services is ok", func(t *testing.T) {
		ret, err := planner.ComposeCluster(
			models.Cluster{
				Hosts: map[string]tags.HostTags{"c1": {}}},
			nil,
		)
		require.NoError(t, err)
		require.Equal(
			t,
			planner.Cluster{Hosts: map[string]planner.Host{"c1": {}}},
			ret,
		)
	})
	t.Run("one host", func(t *testing.T) {
		ret, err := planner.ComposeCluster(
			models.Cluster{
				ID:   "cid1",
				Tags: tags.ClusterTags{},
				Hosts: map[string]tags.HostTags{
					"fqdn": {Geo: "vla"},
				},
			},
			[]types.HostHealth{
				types.NewHostHealth(
					"cid1",
					"fqdn",
					[]types.ServiceHealth{
						types.NewServiceHealth(
							"pg_service",
							time.Time{},
							types.ServiceStatusAlive,
							types.ServiceRoleReplica,
							types.ServiceReplicaTypeAsync,
							"",
							0,
							map[string]string{"rpc": "100"},
						),
					},
				),
			},
		)
		require.NoError(t, err)
		require.Equal(t,
			planner.Cluster{
				ID: "cid1",
				Hosts: map[string]planner.Host{
					"fqdn": {
						Services: map[string]planner.Service{
							"pg_service": {
								Role:        types.ServiceRoleReplica,
								ReplicaType: types.ServiceReplicaTypeAsync,
								Status:      types.ServiceStatusAlive,
							},
						},
						Tags: tags.HostTags{Geo: "vla"},
					},
				},
			},
			ret,
		)
	})

	t.Run("duplicate services is not ok", func(t *testing.T) {
		ret, err := planner.ComposeCluster(
			models.Cluster{},
			[]types.HostHealth{
				types.NewHostHealth(
					"cid1",
					"fqdn",
					[]types.ServiceHealth{
						types.NewServiceHealth(
							"pg_service",
							time.Time{},
							types.ServiceStatusAlive,
							types.ServiceRoleReplica,
							types.ServiceReplicaTypeAsync,
							"",
							0,
							map[string]string{"rpc": "100"},
						),
						types.NewServiceHealth(
							"pg_service",
							time.Time{},
							types.ServiceStatusAlive,
							types.ServiceRoleReplica,
							types.ServiceReplicaTypeAsync,
							"",
							0,
							map[string]string{"rpc": "100"},
						),
					},
				),
			},
		)
		require.Error(t, err)
		require.Empty(t, ret)
	})
}
