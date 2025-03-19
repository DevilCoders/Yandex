package mongodb

import (
	"fmt"
	"testing"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/katan/internal/tags"
	"a.yandex-team.ru/cloud/mdb/katan/katan/internal/cluster/planner"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types"
)

func NewMongodHost(srv types.ServiceRole, shard string) planner.Host {
	return planner.Host{
		Services: map[string]planner.Service{
			"mongod": {
				Role: srv,
			},
		},
		Tags: tags.HostTags{
			Meta: tags.HostMeta{
				ShardID: shard,
				Roles:   []string{string(hostRoleMongod)},
			},
		},
	}
}

func NewMongoinfraHost(srv types.ServiceRole) planner.Host {
	return planner.Host{
		Services: map[string]planner.Service{
			"mongos": {
				Role: types.ServiceRoleMaster,
			},
			"mongocfg": {
				Role: srv,
			},
		},
		Tags: tags.HostTags{
			Meta: tags.HostMeta{
				Roles: []string{string(hostRoleMongoinfra)},
			},
		},
	}
}

func NewMongocfgHost(srv types.ServiceRole) planner.Host {
	return planner.Host{
		Services: map[string]planner.Service{
			"mongocfg": {
				Role: srv,
			},
		},
		Tags: tags.HostTags{
			Meta: tags.HostMeta{
				Roles: []string{string(hostRoleMongocfg)},
			},
		},
	}
}

func NewMongosHost() planner.Host {
	return planner.Host{
		Services: map[string]planner.Service{
			"mongos": {
				Role: types.ServiceRoleMaster,
			},
		},
		Tags: tags.HostTags{
			Meta: tags.HostMeta{
				Roles: []string{string(hostRoleMongos)},
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
						"mongod01": NewMongodHost(types.ServiceRoleReplica, "shard1"),
						"mongod02": NewMongodHost(types.ServiceRoleReplica, "shard1"),
						"mongod03": NewMongodHost(types.ServiceRoleReplica, "shard1"),
					},
				},
			},
			expected:    nil,
			expectedErr: fmt.Errorf("mongod \"shard1\" shard replset failure: primary not found"),
		},
		{
			name: "sharded_cluster_,one_shard_no_primary_error",
			args: args{
				cluster: planner.Cluster{
					Hosts: map[string]planner.Host{
						"mongod01":     NewMongodHost(types.ServiceRoleMaster, "shard1"),
						"mongod02":     NewMongodHost(types.ServiceRoleReplica, "shard1"),
						"mongod11":     NewMongodHost(types.ServiceRoleMaster, "shard2"),
						"mongod12":     NewMongodHost(types.ServiceRoleReplica, "shard2"),
						"mongod21":     NewMongodHost(types.ServiceRoleReplica, "shard3"),
						"mongod22":     NewMongodHost(types.ServiceRoleReplica, "shard3"),
						"mongoinfra01": NewMongoinfraHost(types.ServiceRoleMaster),
						"mongoinfra02": NewMongoinfraHost(types.ServiceRoleReplica),
						"mongoinfra03": NewMongoinfraHost(types.ServiceRoleReplica),
					},
				},
			},
			expected:    nil,
			expectedErr: fmt.Errorf("mongod \"shard3\" shard replset failure: primary not found"),
		},
		{
			name: "sharded_cluster_,no_infra_error",
			args: args{
				cluster: planner.Cluster{
					Hosts: map[string]planner.Host{
						"mongod01": NewMongodHost(types.ServiceRoleMaster, "shard1"),
						"mongod02": NewMongodHost(types.ServiceRoleReplica, "shard1"),
						"mongod11": NewMongodHost(types.ServiceRoleMaster, "shard2"),
						"mongod12": NewMongodHost(types.ServiceRoleReplica, "shard2"),
						"mongod21": NewMongodHost(types.ServiceRoleReplica, "shard3"),
						"mongod22": NewMongodHost(types.ServiceRoleMaster, "shard3"),
					},
				},
			},
			expected:    nil,
			expectedErr: fmt.Errorf("expected or mongos+mongocfg or mongoinfra hosts"),
		},
		{
			name: "sharded_cluster,infra_no_primary_error",
			args: args{
				cluster: planner.Cluster{
					Hosts: map[string]planner.Host{
						"mongod01":     NewMongodHost(types.ServiceRoleMaster, "shard1"),
						"mongod02":     NewMongodHost(types.ServiceRoleReplica, "shard1"),
						"mongoinfra01": NewMongoinfraHost(types.ServiceRoleReplica),
						"mongoinfra02": NewMongoinfraHost(types.ServiceRoleReplica),
						"mongoinfra03": NewMongoinfraHost(types.ServiceRoleReplica),
					},
				},
			},
			expected:    nil,
			expectedErr: fmt.Errorf("mongoinfra replset failure: primary not found"),
		},
		{
			name: "sharded_cluster,cfg_no_primary_error",
			args: args{
				cluster: planner.Cluster{
					Hosts: map[string]planner.Host{
						"mongod01":   NewMongodHost(types.ServiceRoleMaster, "shard1"),
						"mongod02":   NewMongodHost(types.ServiceRoleReplica, "shard1"),
						"mongocfg01": NewMongocfgHost(types.ServiceRoleReplica),
						"mongocfg02": NewMongocfgHost(types.ServiceRoleReplica),
						"mongocfg03": NewMongocfgHost(types.ServiceRoleReplica),
						"mongos01":   NewMongosHost(),
						"mongos02":   NewMongosHost(),
					},
				},
			},
			expected:    nil,
			expectedErr: fmt.Errorf("mongocfg replset failure: primary not found"),
		},
		{
			name: "sharded_cluster,cfg_no_mongos_error",
			args: args{
				cluster: planner.Cluster{
					Hosts: map[string]planner.Host{
						"mongod01":   NewMongodHost(types.ServiceRoleMaster, "shard1"),
						"mongod02":   NewMongodHost(types.ServiceRoleReplica, "shard1"),
						"mongocfg01": NewMongocfgHost(types.ServiceRoleMaster),
						"mongocfg02": NewMongocfgHost(types.ServiceRoleReplica),
						"mongocfg03": NewMongocfgHost(types.ServiceRoleReplica),
					},
				},
			},
			expected:    nil,
			expectedErr: fmt.Errorf("expected mongocfg with mongos hosts"),
		},
		{
			name: "sharded_cluster,infra_and_cfg_error",
			args: args{
				cluster: planner.Cluster{
					Hosts: map[string]planner.Host{
						"mongod01":     NewMongodHost(types.ServiceRoleMaster, "shard1"),
						"mongod02":     NewMongodHost(types.ServiceRoleReplica, "shard1"),
						"mongocfg01":   NewMongocfgHost(types.ServiceRoleMaster),
						"mongocfg02":   NewMongocfgHost(types.ServiceRoleReplica),
						"mongocfg03":   NewMongocfgHost(types.ServiceRoleReplica),
						"mongoinfra01": NewMongoinfraHost(types.ServiceRoleMaster),
						"mongoinfra02": NewMongoinfraHost(types.ServiceRoleReplica),
						"mongoinfra03": NewMongoinfraHost(types.ServiceRoleReplica),
					},
				},
			},
			expected:    nil,
			expectedErr: fmt.Errorf("expected or mongos+mongocfg or mongoinfra hosts"),
		},
		{
			name: "sharded_cluster,infra_and_mongos_error",
			args: args{
				cluster: planner.Cluster{
					Hosts: map[string]planner.Host{
						"mongod01":     NewMongodHost(types.ServiceRoleMaster, "shard1"),
						"mongod02":     NewMongodHost(types.ServiceRoleReplica, "shard1"),
						"mongos01":     NewMongosHost(),
						"mongos02":     NewMongosHost(),
						"mongoinfra01": NewMongoinfraHost(types.ServiceRoleMaster),
						"mongoinfra02": NewMongoinfraHost(types.ServiceRoleReplica),
						"mongoinfra03": NewMongoinfraHost(types.ServiceRoleReplica),
					},
				},
			},
			expected:    nil,
			expectedErr: fmt.Errorf("expected or mongos+mongocfg or mongoinfra hosts"),
		},
		{
			name: "sharded_cluster,infra_and_mongos_cfg_error",
			args: args{
				cluster: planner.Cluster{
					Hosts: map[string]planner.Host{
						"mongod01":     NewMongodHost(types.ServiceRoleMaster, "shard1"),
						"mongod02":     NewMongodHost(types.ServiceRoleReplica, "shard1"),
						"mongocfg01":   NewMongocfgHost(types.ServiceRoleMaster),
						"mongocfg02":   NewMongocfgHost(types.ServiceRoleReplica),
						"mongocfg03":   NewMongocfgHost(types.ServiceRoleReplica),
						"mongos01":     NewMongosHost(),
						"mongos02":     NewMongosHost(),
						"mongoinfra01": NewMongoinfraHost(types.ServiceRoleMaster),
						"mongoinfra02": NewMongoinfraHost(types.ServiceRoleReplica),
						"mongoinfra03": NewMongoinfraHost(types.ServiceRoleReplica),
					},
				},
			},
			expected:    nil,
			expectedErr: fmt.Errorf("expected or mongos+mongocfg or mongoinfra hosts"),
		},
		{
			name: "sharded_cluster,no_mongod_error",
			args: args{
				cluster: planner.Cluster{
					Hosts: map[string]planner.Host{
						"mongocfg01": NewMongocfgHost(types.ServiceRoleMaster),
						"mongocfg02": NewMongocfgHost(types.ServiceRoleReplica),
						"mongocfg03": NewMongocfgHost(types.ServiceRoleReplica),
						"mongos01":   NewMongosHost(),
						"mongos02":   NewMongosHost(),
					},
				},
			},
			expected:    nil,
			expectedErr: fmt.Errorf("empty mongod host list"),
		},
		{
			name: "unknown_service_role_error",
			args: args{
				cluster: planner.Cluster{
					Hosts: map[string]planner.Host{
						"mongod01": {
							Services: map[string]planner.Service{
								"mongod": {
									Role: types.ServiceRoleUnknown,
								},
							},
							Tags: tags.HostTags{
								Meta: tags.HostMeta{
									ShardID: "shard01",
									Roles:   []string{string(hostRoleMongod)},
								},
							},
						},
					},
				},
			},
			expected:    nil,
			expectedErr: fmt.Errorf("mongod \"shard01\" host failure: unsupported service role: \"Unknown\". On \"mongod01\""),
		},
		{
			name: "unknown_service_role_error",
			args: args{
				cluster: planner.Cluster{
					Hosts: map[string]planner.Host{
						"mongod01": {
							Services: map[string]planner.Service{
								"mongod": {
									Role: types.ServiceRoleMaster,
								},
							},
							Tags: tags.HostTags{
								Meta: tags.HostMeta{
									Roles: []string{string(hostRoleMongod)},
								},
							},
						},
					},
				},
			},
			expected:    nil,
			expectedErr: fmt.Errorf("empty shardID given: {Services:map[mongod:{Role:Master ReplicaType: Status:}] Tags:{Geo: Meta:{ShardID: SubClusterID: Roles:[mongodb_cluster.mongod]}}}"),
		},
		{
			name: "unknown_service_error",
			args: args{
				cluster: planner.Cluster{
					Hosts: map[string]planner.Host{
						"mongod01": {
							Services: map[string]planner.Service{
								"unknown_srv": {
									Role: types.ServiceRoleMaster,
								},
							},
							Tags: tags.HostTags{
								Meta: tags.HostMeta{
									ShardID: "shard01",
									Roles:   []string{string(hostRoleMongod)},
								},
							},
						},
					},
				},
			},
			expected:    nil,
			expectedErr: fmt.Errorf("no \"mongod\" in \"mongod01\" host services: map[unknown_srv:{Role:Master ReplicaType: Status:}]"),
		},
		{
			name: "unknown_tags_role_error",
			args: args{
				cluster: planner.Cluster{
					Hosts: map[string]planner.Host{
						"mongod01": {
							Tags: tags.HostTags{
								Meta: tags.HostMeta{
									ShardID: "shard01",
									Roles:   []string{"unknown_role"},
								},
							},
						},
					},
				},
			},
			expected:    nil,
			expectedErr: fmt.Errorf("unknown host role: unknown_role"),
		},
		{
			name: "multiple_tags_role_error",
			args: args{
				cluster: planner.Cluster{
					Hosts: map[string]planner.Host{
						"mongod01": {
							Tags: tags.HostTags{
								Meta: tags.HostMeta{
									ShardID: "shard01",
									Roles:   []string{string(hostRoleMongos), string(hostRoleMongocfg)},
								},
							},
						},
					},
				},
			},
			expected:    nil,
			expectedErr: fmt.Errorf("unexpected roles count: [mongodb_cluster.mongos mongodb_cluster.mongocfg]"),
		},
		{
			name: "1_host_cluster_ok",
			args: args{
				cluster: planner.Cluster{
					Hosts: map[string]planner.Host{
						"mongod01": NewMongodHost(types.ServiceRoleMaster, "shard1"),
					},
				},
			},
			expected:    [][]string{{"mongod01"}},
			expectedErr: nil,
		},
		{
			name: "4_host_cluster_ok",
			args: args{
				cluster: planner.Cluster{
					Hosts: map[string]planner.Host{
						"mongod01": NewMongodHost(types.ServiceRoleMaster, "shard01"),
						"mongod02": NewMongodHost(types.ServiceRoleReplica, "shard01"),
						"mongod03": NewMongodHost(types.ServiceRoleReplica, "shard01"),
						"mongod04": NewMongodHost(types.ServiceRoleReplica, "shard01"),
						"mongod05": NewMongodHost(types.ServiceRoleReplica, "shard01"),
						"mongod06": NewMongodHost(types.ServiceRoleReplica, "shard01"),
						"mongod07": NewMongodHost(types.ServiceRoleReplica, "shard01"),
					},
				},
			},
			expected:    [][]string{{"mongod02"}, {"mongod03", "mongod04"}, {"mongod05", "mongod06", "mongod07"}, {"mongod01"}},
			expectedErr: nil,
		},
		{
			name: "sharded_cluster,with_infra_ok",
			args: args{
				cluster: planner.Cluster{
					Hosts: map[string]planner.Host{
						"mongod01":     NewMongodHost(types.ServiceRoleMaster, "shard1"),
						"mongod02":     NewMongodHost(types.ServiceRoleReplica, "shard1"),
						"mongod03":     NewMongodHost(types.ServiceRoleReplica, "shard1"),
						"mongod04":     NewMongodHost(types.ServiceRoleReplica, "shard1"),
						"mongod11":     NewMongodHost(types.ServiceRoleMaster, "shard2"),
						"mongod12":     NewMongodHost(types.ServiceRoleReplica, "shard2"),
						"mongod13":     NewMongodHost(types.ServiceRoleReplica, "shard2"),
						"mongod14":     NewMongodHost(types.ServiceRoleReplica, "shard2"),
						"mongoinfra01": NewMongoinfraHost(types.ServiceRoleMaster),
						"mongoinfra02": NewMongoinfraHost(types.ServiceRoleReplica),
						"mongoinfra03": NewMongoinfraHost(types.ServiceRoleReplica),
					},
				},
			},
			expected: [][]string{
				{"mongod02"}, {"mongod03", "mongod04"}, {"mongod01"},
				{"mongod12"}, {"mongod13", "mongod14"}, {"mongod11"},
				{"mongoinfra02"}, {"mongoinfra03"}, {"mongoinfra01"},
			},
			expectedErr: nil,
		},
		{
			name: "sharded_cluster,with_infra_ok",
			args: args{
				cluster: planner.Cluster{
					Hosts: map[string]planner.Host{
						"mongod01":   NewMongodHost(types.ServiceRoleMaster, "shard1"),
						"mongod02":   NewMongodHost(types.ServiceRoleReplica, "shard1"),
						"mongod03":   NewMongodHost(types.ServiceRoleReplica, "shard1"),
						"mongod04":   NewMongodHost(types.ServiceRoleReplica, "shard1"),
						"mongod11":   NewMongodHost(types.ServiceRoleMaster, "shard2"),
						"mongod12":   NewMongodHost(types.ServiceRoleReplica, "shard2"),
						"mongod13":   NewMongodHost(types.ServiceRoleReplica, "shard2"),
						"mongod14":   NewMongodHost(types.ServiceRoleReplica, "shard2"),
						"mongocfg01": NewMongocfgHost(types.ServiceRoleMaster),
						"mongocfg02": NewMongocfgHost(types.ServiceRoleReplica),
						"mongocfg03": NewMongocfgHost(types.ServiceRoleReplica),
						"mongos01":   NewMongosHost(),
						"mongos02":   NewMongosHost(),
						"mongos03":   NewMongosHost(),
					},
				},
			},
			expected: [][]string{
				{"mongod02"}, {"mongod03", "mongod04"}, {"mongod01"},
				{"mongod12"}, {"mongod13", "mongod14"}, {"mongod11"},
				{"mongocfg02"}, {"mongocfg03"}, {"mongocfg01"},
				{"mongos01"}, {"mongos02", "mongos03"},
			},
			expectedErr: nil,
		},
	}
	for _, tt := range tests {
		// TODO: empty shard name, unknown role for sharded infra
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
