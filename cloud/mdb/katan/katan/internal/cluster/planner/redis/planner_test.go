package redis

import (
	"fmt"
	"testing"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/katan/internal/tags"
	"a.yandex-team.ru/cloud/mdb/katan/katan/internal/cluster/planner"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types"
)

type hostRole string

const (
	hostRoleRedis = hostRole("redis_cluster")
)

func NewRedisHost(srv types.ServiceRole, shard string, redisHealthService string) planner.Host {
	return planner.Host{
		Services: map[string]planner.Service{
			redisHealthService: {
				Role: srv,
			},
		},
		Tags: tags.HostTags{
			Meta: tags.HostMeta{
				ShardID: shard,
				Roles:   []string{string(hostRoleRedis)},
			},
		},
	}
}

func TestPlanner(t *testing.T) {
	type args struct {
		cluster planner.Cluster
	}
	tests := []struct {
		name        string
		args        args
		expected    [][]string
		expectedErr error
	}{
		{
			name: "empty_cluster_error",
			args: args{
				cluster: planner.Cluster{},
			},
			expected:    nil,
			expectedErr: fmt.Errorf("got empty cluster: {ID: Tags:{Version:0 Source: Meta:{Type: Env: Rev:0 Status:}} Hosts:map[]}"),
		},
		{
			name: "3_host_cluster_no_primary_error",
			args: args{
				cluster: planner.Cluster{
					Hosts: map[string]planner.Host{
						"redis01": NewRedisHost(types.ServiceRoleReplica, "shard1", "redis"),
						"redis02": NewRedisHost(types.ServiceRoleReplica, "shard1", "redis"),
						"redis03": NewRedisHost(types.ServiceRoleReplica, "shard1", "redis"),
					},
				},
			},
			expected:    nil,
			expectedErr: fmt.Errorf("redis \"shard1\" shard replset failure: primary not found"),
		},
		{
			name: "sharded_cluster_,one_shard_no_primary_error",
			args: args{
				cluster: planner.Cluster{
					Hosts: map[string]planner.Host{
						"redis01": NewRedisHost(types.ServiceRoleMaster, "shard1", "redis_cluster"),
						"redis02": NewRedisHost(types.ServiceRoleReplica, "shard1", "redis_cluster"),
						"redis11": NewRedisHost(types.ServiceRoleMaster, "shard2", "redis_cluster"),
						"redis12": NewRedisHost(types.ServiceRoleReplica, "shard2", "redis_cluster"),
						"redis21": NewRedisHost(types.ServiceRoleReplica, "shard3", "redis_cluster"),
						"redis22": NewRedisHost(types.ServiceRoleReplica, "shard3", "redis_cluster"),
					},
				},
			},
			expected:    nil,
			expectedErr: fmt.Errorf("redis \"shard3\" shard replset failure: primary not found"),
		},
		{
			name: "unknown_service_role_error",
			args: args{
				cluster: planner.Cluster{
					Hosts: map[string]planner.Host{
						"redis01": {
							Services: map[string]planner.Service{
								"redis": {
									Role: types.ServiceRoleUnknown,
								},
							},
							Tags: tags.HostTags{
								Meta: tags.HostMeta{
									ShardID: "shard01",
									Roles:   []string{string(hostRoleRedis)},
								},
							},
						},
					},
				},
			},
			expected:    nil,
			expectedErr: fmt.Errorf("redis \"shard01\" host failure: unsupported service role: \"Unknown\". On \"redis01\""),
		},
		{
			name: "missing_shard_id_error",
			args: args{
				cluster: planner.Cluster{
					Hosts: map[string]planner.Host{
						"redis01": {
							Services: map[string]planner.Service{
								"redis": {
									Role: types.ServiceRoleMaster,
								},
							},
							Tags: tags.HostTags{
								Meta: tags.HostMeta{
									Roles: []string{string(hostRoleRedis)},
								},
							},
						},
					},
				},
			},
			expected:    nil,
			expectedErr: fmt.Errorf("empty shardID given: {Services:map[redis:{Role:Master ReplicaType: Status:}] Tags:{Geo: Meta:{ShardID: SubClusterID: Roles:[redis_cluster]}}}"),
		},
		{
			name: "empty_shard_id_error",
			args: args{
				cluster: planner.Cluster{
					Hosts: map[string]planner.Host{
						"redis01": {
							Services: map[string]planner.Service{
								"redis": {
									Role: types.ServiceRoleMaster,
								},
							},
							Tags: tags.HostTags{
								Meta: tags.HostMeta{
									ShardID: "",
									Roles:   []string{string(hostRoleRedis)},
								},
							},
						},
					},
				},
			},
			expected:    nil,
			expectedErr: fmt.Errorf("empty shardID given: {Services:map[redis:{Role:Master ReplicaType: Status:}] Tags:{Geo: Meta:{ShardID: SubClusterID: Roles:[redis_cluster]}}}"),
		},
		{
			name: "unknown_service_error",
			args: args{
				cluster: planner.Cluster{
					Hosts: map[string]planner.Host{
						"redis01": {
							Services: map[string]planner.Service{
								"unknown_srv": {
									Role: types.ServiceRoleMaster,
								},
							},
							Tags: tags.HostTags{
								Meta: tags.HostMeta{
									ShardID: "shard01",
									Roles:   []string{string(hostRoleRedis)},
								},
							},
						},
					},
				},
			},
			expected:    nil,
			expectedErr: fmt.Errorf("no \"redis\", neither \"redis_cluster\" in \"redis01\" host services: map[unknown_srv:{Role:Master ReplicaType: Status:}]"),
		},
		{
			name: "sharded_sentinel_mix",
			args: args{
				cluster: planner.Cluster{
					Hosts: map[string]planner.Host{
						"redis02": NewRedisHost(types.ServiceRoleReplica, "shard01", "redis_cluster"),
						"redis01": NewRedisHost(types.ServiceRoleMaster, "shard01", "redis"),
					},
				},
			},
			expected:    nil,
			expectedErr: fmt.Errorf("unexpected sentinel\\sharded hosts mix: map[redis01:{Services:map[redis:{Role:Master ReplicaType: Status:}] Tags:{Geo: Meta:{ShardID:shard01 SubClusterID: Roles:[redis_cluster]}}} redis02:{Services:map[redis_cluster:{Role:Replica ReplicaType: Status:}] Tags:{Geo: Meta:{ShardID:shard01 SubClusterID: Roles:[redis_cluster]}}}]"),
		},
		{
			name: "sentinel_sharded_mix",
			args: args{
				cluster: planner.Cluster{
					Hosts: map[string]planner.Host{
						"redis01": NewRedisHost(types.ServiceRoleMaster, "shard01", "redis"),
						"redis02": NewRedisHost(types.ServiceRoleReplica, "shard01", "redis_cluster"),
					},
				},
			},
			expected:    nil,
			expectedErr: fmt.Errorf("unexpected sentinel\\sharded hosts mix: map[redis01:{Services:map[redis:{Role:Master ReplicaType: Status:}] Tags:{Geo: Meta:{ShardID:shard01 SubClusterID: Roles:[redis_cluster]}}} redis02:{Services:map[redis_cluster:{Role:Replica ReplicaType: Status:}] Tags:{Geo: Meta:{ShardID:shard01 SubClusterID: Roles:[redis_cluster]}}}]"),
		},
		{
			name: "sentinel_many_shards",
			args: args{
				cluster: planner.Cluster{
					Hosts: map[string]planner.Host{
						"redis01": NewRedisHost(types.ServiceRoleMaster, "shard01", "redis"),
						"redis02": NewRedisHost(types.ServiceRoleReplica, "shard02", "redis"),
					},
				},
			},
			expected:    nil,
			expectedErr: fmt.Errorf("unexpected shards number for cluster type: map[redis01:{Services:map[redis:{Role:Master ReplicaType: Status:}] Tags:{Geo: Meta:{ShardID:shard01 SubClusterID: Roles:[redis_cluster]}}} redis02:{Services:map[redis:{Role:Replica ReplicaType: Status:}] Tags:{Geo: Meta:{ShardID:shard02 SubClusterID: Roles:[redis_cluster]}}}]"),
		},
		{
			name: "sharded_single_shard",
			args: args{
				cluster: planner.Cluster{
					Hosts: map[string]planner.Host{
						"redis01": NewRedisHost(types.ServiceRoleMaster, "shard01", "redis_cluster"),
						"redis02": NewRedisHost(types.ServiceRoleReplica, "shard01", "redis_cluster"),
					},
				},
			},
			expected:    nil,
			expectedErr: fmt.Errorf("unexpected shards number for cluster type: map[redis01:{Services:map[redis_cluster:{Role:Master ReplicaType: Status:}] Tags:{Geo: Meta:{ShardID:shard01 SubClusterID: Roles:[redis_cluster]}}} redis02:{Services:map[redis_cluster:{Role:Replica ReplicaType: Status:}] Tags:{Geo: Meta:{ShardID:shard01 SubClusterID: Roles:[redis_cluster]}}}]"),
		},
		{
			name: "1_host_cluster_ok",
			args: args{
				cluster: planner.Cluster{
					Hosts: map[string]planner.Host{
						"redis01": NewRedisHost(types.ServiceRoleMaster, "shard1", "redis"),
					},
				},
			},
			expected:    [][]string{{"redis01"}},
			expectedErr: nil,
		},
		{
			name: "7_host_cluster_ok",
			args: args{
				cluster: planner.Cluster{
					Hosts: map[string]planner.Host{
						"redis01": NewRedisHost(types.ServiceRoleMaster, "shard01", "redis"),
						"redis02": NewRedisHost(types.ServiceRoleReplica, "shard01", "redis"),
						"redis03": NewRedisHost(types.ServiceRoleReplica, "shard01", "redis"),
						"redis04": NewRedisHost(types.ServiceRoleReplica, "shard01", "redis"),
						"redis05": NewRedisHost(types.ServiceRoleReplica, "shard01", "redis"),
						"redis06": NewRedisHost(types.ServiceRoleReplica, "shard01", "redis"),
						"redis07": NewRedisHost(types.ServiceRoleReplica, "shard01", "redis"),
					},
				},
			},
			expected:    [][]string{{"redis02"}, {"redis03", "redis04"}, {"redis05", "redis06", "redis07"}, {"redis01"}},
			expectedErr: nil,
		},
		{
			name: "sharded_cluster_ok",
			args: args{
				cluster: planner.Cluster{
					Hosts: map[string]planner.Host{
						"redis01": NewRedisHost(types.ServiceRoleMaster, "shard1", "redis_cluster"),
						"redis02": NewRedisHost(types.ServiceRoleReplica, "shard1", "redis_cluster"),
						"redis03": NewRedisHost(types.ServiceRoleReplica, "shard1", "redis_cluster"),
						"redis04": NewRedisHost(types.ServiceRoleReplica, "shard1", "redis_cluster"),
						"redis11": NewRedisHost(types.ServiceRoleMaster, "shard2", "redis_cluster"),
						"redis12": NewRedisHost(types.ServiceRoleReplica, "shard2", "redis_cluster"),
						"redis13": NewRedisHost(types.ServiceRoleReplica, "shard2", "redis_cluster"),
						"redis14": NewRedisHost(types.ServiceRoleReplica, "shard2", "redis_cluster"),
					},
				},
			},
			expected: [][]string{
				{"redis02"}, {"redis03", "redis04"}, {"redis01"},
				{"redis12"}, {"redis13", "redis14"}, {"redis11"},
			},
			expectedErr: nil,
		},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			got, err := Planner(tt.args.cluster)

			if tt.expectedErr != nil {
				require.EqualError(t, err, tt.expectedErr.Error())
				return
			}
			require.NoError(t, err)
			require.Equal(t, tt.expected, got)
		})
	}
}
