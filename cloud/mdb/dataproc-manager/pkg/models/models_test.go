package models

import (
	"reflect"
	"testing"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/dataproc-manager/pkg/models/health"
	"a.yandex-team.ru/cloud/mdb/dataproc-manager/pkg/models/role"
	"a.yandex-team.ru/cloud/mdb/dataproc-manager/pkg/models/service"
)

func TestClusterHealth_HasService(t *testing.T) {
	type args struct {
		service service.Service
	}
	cluster := ClusterHealth{
		Services: map[service.Service]ServiceHealth{
			service.Yarn: BasicHealthService{},
			service.Hdfs: BasicHealthService{},
		},
	}
	tests := []struct {
		name string
		args args
		want bool
	}{
		{name: "Found hdfs", args: args{service: service.Hdfs}, want: true},
		{name: "Found yarn", args: args{service: service.Yarn}, want: true},
		{name: "Not found hbase", args: args{service: service.Hbase}, want: false},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			if got := cluster.HasService(tt.args.service); got != tt.want {
				t.Errorf("ClusterHealth.HasService() = %v, want %v", got, tt.want)
			}
		})
	}
}

func TestHostHealth_HasService(t *testing.T) {
	host := HostHealth{
		Services: map[service.Service]ServiceHealth{
			service.Hdfs: BasicHealthService{},
			service.Yarn: BasicHealthService{},
		},
	}
	type args struct {
		service service.Service
	}
	tests := []struct {
		name string
		args args
		want bool
	}{
		{name: "Found hdfs", args: args{service: service.Hdfs}, want: true},
		{name: "Found yarn", args: args{service: service.Yarn}, want: true},
		{name: "Not found hbase", args: args{service: service.Hbase}, want: false},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			if got := host.HasService(tt.args.service); got != tt.want {
				t.Errorf("HostHealth.HasService() = %v, want %v", got, tt.want)
			}
		})
	}
}

func TestClusterHealth_ServiceHealth(t *testing.T) {
	cluster := ClusterHealth{
		Services: map[service.Service]ServiceHealth{
			service.Yarn: BasicHealthService{Health: health.Alive},
			service.Hdfs: BasicHealthService{Health: health.Degraded},
			service.Hive: BasicHealthService{Health: health.Dead},
		},
	}
	type args struct {
		serviceType service.Service
	}
	tests := []struct {
		name string
		args args
		want health.Health
	}{
		{name: "Correct yarn health", args: args{serviceType: service.Yarn}, want: health.Alive},
		{name: "Correct hdfs health", args: args{serviceType: service.Hdfs}, want: health.Degraded},
		{name: "Correct hive health", args: args{serviceType: service.Hive}, want: health.Dead},
		// {name: "Correct zk health", args: args{serviceType: "zk"}, want: health.Unknown},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			if got := cluster.ServiceHealth(tt.args.serviceType); !reflect.DeepEqual(got, tt.want) {
				t.Errorf("ClusterHealth.ServiceHealth() = %v, want %v", got, tt.want)
			}
		})
	}
}

func TestHostHealth_ServiceHealth(t *testing.T) {
	host := HostHealth{
		Services: map[service.Service]ServiceHealth{
			service.Yarn: BasicHealthService{Health: health.Alive},
			service.Hdfs: BasicHealthService{Health: health.Degraded},
			service.Hive: BasicHealthService{Health: health.Dead},
		},
	}
	type args struct {
		serviceType service.Service
	}
	tests := []struct {
		name string
		args args
		want health.Health
	}{
		{name: "Correct yarn health", args: args{serviceType: service.Yarn}, want: health.Alive},
		{name: "Correct hdfs health", args: args{serviceType: service.Hdfs}, want: health.Degraded},
		{name: "Correct hive health", args: args{serviceType: service.Hive}, want: health.Dead},
		// {name: "Correct zk health", args: args{serviceType: "zk"}, want: health.Unknown},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			if got := host.ServiceHealth(tt.args.serviceType); !reflect.DeepEqual(got, tt.want) {
				t.Errorf("HostHealth.ServiceHealth() = %v, want %v", got, tt.want)
			}
		})
	}
}

func TestClusterHealth_AllServicesAlive(t *testing.T) {
	type fields struct {
		Services map[service.Service]ServiceHealth
	}
	tests := []struct {
		name   string
		fields fields
		want   bool
	}{
		{name: "All alive", fields: fields{Services: map[service.Service]ServiceHealth{
			service.Yarn: BasicHealthService{Health: health.Alive},
			service.Hdfs: BasicHealthService{Health: health.Alive},
			service.Hive: BasicHealthService{Health: health.Alive},
		}}, want: true},
		{name: "One dead", fields: fields{Services: map[service.Service]ServiceHealth{
			service.Yarn: BasicHealthService{Health: health.Alive},
			service.Hdfs: BasicHealthService{Health: health.Dead},
			service.Hive: BasicHealthService{Health: health.Alive},
		}}, want: false},
		{name: "One unknown", fields: fields{Services: map[service.Service]ServiceHealth{
			service.Yarn: BasicHealthService{Health: health.Alive},
			service.Hdfs: BasicHealthService{Health: health.Alive},
			service.Hive: BasicHealthService{Health: health.Unknown},
		}}, want: false},
		{name: "Empty", fields: fields{Services: map[service.Service]ServiceHealth{}}, want: true},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			cluster := ClusterHealth{
				Services: tt.fields.Services,
			}
			if got := cluster.AllServicesAlive(); got != tt.want {
				t.Errorf("ClusterHealth.AllServicesAlive() = %v, want %v", got, tt.want)
			}
		})
	}
}

func TestHostHealth_AllServicesAlive(t *testing.T) {
	type fields struct {
		Services map[service.Service]ServiceHealth
	}
	tests := []struct {
		name   string
		fields fields
		want   bool
	}{
		{name: "All alive", fields: fields{Services: map[service.Service]ServiceHealth{
			service.Yarn: BasicHealthService{Health: health.Alive},
			service.Hdfs: BasicHealthService{Health: health.Alive},
			service.Hive: BasicHealthService{Health: health.Alive},
		}}, want: true},
		{name: "One dead", fields: fields{Services: map[service.Service]ServiceHealth{
			service.Yarn: BasicHealthService{Health: health.Alive},
			service.Hdfs: BasicHealthService{Health: health.Dead},
			service.Hive: BasicHealthService{Health: health.Alive},
		}}, want: false},
		{name: "One unknown", fields: fields{Services: map[service.Service]ServiceHealth{
			service.Yarn: BasicHealthService{Health: health.Alive},
			service.Hdfs: BasicHealthService{Health: health.Alive},
			service.Hive: BasicHealthService{Health: health.Unknown},
		}}, want: false},
		{name: "Empty", fields: fields{Services: map[service.Service]ServiceHealth{}}, want: true},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			host := HostHealth{
				Services: tt.fields.Services,
			}
			if got := host.AllServicesAlive(); got != tt.want {
				t.Errorf("HostHealth.AllServicesAlive() = %v, want %v", got, tt.want)
			}
		})
	}
}

func TestClusterHealth_DeduceHealth(t *testing.T) {
	type fields struct {
		Services map[service.Service]ServiceHealth
	}
	tests := []struct {
		name          string
		fields        fields
		clusterHealth health.Health
		explanation   string
	}{
		{name: "All alive", fields: fields{Services: map[service.Service]ServiceHealth{
			service.Yarn: BasicHealthService{Health: health.Alive},
			service.Hdfs: BasicHealthService{Health: health.Alive},
			service.Hive: BasicHealthService{Health: health.Alive},
		}}, clusterHealth: health.Alive},
		{name: "Yarn dead", fields: fields{Services: map[service.Service]ServiceHealth{
			service.Yarn: BasicHealthService{Health: health.Dead},
			service.Hdfs: BasicHealthService{Health: health.Alive},
			service.Hive: BasicHealthService{Health: health.Alive},
		}}, clusterHealth: health.Dead, explanation: "some critical services are Dead: YARN is Dead"},
		{name: "All dead", fields: fields{Services: map[service.Service]ServiceHealth{
			service.Yarn: BasicHealthService{Health: health.Dead},
			service.Hdfs: BasicHealthService{Health: health.Dead},
			service.Hive: BasicHealthService{Health: health.Dead},
		}}, clusterHealth: health.Dead, explanation: "all services are Dead: HDFS is Dead, Hive is Dead, YARN is Dead"},
		{name: "Hive dead", fields: fields{Services: map[service.Service]ServiceHealth{
			service.Yarn: BasicHealthService{Health: health.Alive},
			service.Hdfs: BasicHealthService{Health: health.Alive},
			service.Hive: BasicHealthService{Health: health.Dead},
		}}, clusterHealth: health.Degraded, explanation: "some services are not Alive: Hive is Dead"},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			cluster := ClusterHealth{
				Services: tt.fields.Services,
			}
			gotHealth, gotExplanation := cluster.DeduceHealth()
			if !reflect.DeepEqual(gotHealth, tt.clusterHealth) {
				t.Errorf("ClusterHealth.DeduceHealth() = %v, want %v", gotHealth, tt.clusterHealth)
			}
			require.Equal(t, tt.explanation, gotExplanation)
		})
	}
}

func TestHostHealth_DeduceHealth(t *testing.T) {
	type fields struct {
		Services map[service.Service]ServiceHealth
	}
	tests := []struct {
		name   string
		fields fields
		want   health.Health
	}{
		{name: "All alive", fields: fields{Services: map[service.Service]ServiceHealth{
			service.Yarn: BasicHealthService{Health: health.Alive},
			service.Hdfs: BasicHealthService{Health: health.Alive},
			service.Hive: BasicHealthService{Health: health.Alive},
		}}, want: health.Alive},
		{name: "Yarn dead", fields: fields{Services: map[service.Service]ServiceHealth{
			service.Yarn: BasicHealthService{Health: health.Dead},
			service.Hdfs: BasicHealthService{Health: health.Alive},
			service.Hive: BasicHealthService{Health: health.Alive},
		}}, want: health.Degraded},
		{name: "All dead", fields: fields{Services: map[service.Service]ServiceHealth{
			service.Yarn: BasicHealthService{Health: health.Dead},
			service.Hdfs: BasicHealthService{Health: health.Dead},
			service.Hive: BasicHealthService{Health: health.Dead},
		}}, want: health.Dead},
		{name: "Hive dead", fields: fields{Services: map[service.Service]ServiceHealth{
			service.Yarn: BasicHealthService{Health: health.Alive},
			service.Hdfs: BasicHealthService{Health: health.Alive},
			service.Hive: BasicHealthService{Health: health.Dead},
		}}, want: health.Degraded},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			host := HostHealth{
				Services: tt.fields.Services,
			}
			if got := host.DeduceHealth(); !reflect.DeepEqual(got, tt.want) {
				t.Errorf("HostHealth.DeduceHealth() = %v, want %v", got, tt.want)
			}
		})
	}
}

func TestHostHealth_UnknownHealthConstructor(t *testing.T) {
	host := NewHostUnknownHealth()
	require.Equal(t, host.Health, health.Unknown)
}

func Test_DeduceRoleByNodeFQDN(t *testing.T) {
	deducedRole, err := role.DeduceRoleByNodeFQDN("rc1b-dataproc-g-7ctkvhx7kdzk7v1c-1.mdb.cloud-preprod.yandex.net")
	require.Equal(t, deducedRole, role.ComputeAutoScaling)
	require.NoError(t, err)
	deducedRole, err = role.DeduceRoleByNodeFQDN("rc1b-dataproc-c-7ctkvhx7kdzk7v1c.mdb.cloud-preprod.yandex.net")
	require.Equal(t, deducedRole, role.Compute)
	require.NoError(t, err)
	deducedRole, err = role.DeduceRoleByNodeFQDN("rc1b-dataproc-d-7ctkvhx7kdzk7v1c.mdb.cloud-preprod.yandex.net")
	require.Equal(t, deducedRole, role.Data)
	require.NoError(t, err)
	deducedRole, err = role.DeduceRoleByNodeFQDN("rc1b-dataproc-m-c7ih78pk129gg4xi.mdb.cloud-preprod.yandex.net")
	require.Equal(t, deducedRole, role.Main)
	require.NoError(t, err)
	deducedRole, err = role.DeduceRoleByNodeFQDN("rc1b-dataproc-z-c7ih78pk129gg4xi.mdb.cloud-preprod.yandex.net")
	require.Equal(t, deducedRole, role.Unknown)
	require.Error(t, err)
	deducedRole, err = role.DeduceRoleByNodeFQDN("mdb.cloud-preprod.yandex.net")
	require.Equal(t, deducedRole, role.Unknown)
	require.Error(t, err)
}
