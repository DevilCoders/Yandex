package common

import (
	"testing"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/quota"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/common"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/metadb"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func Test_metricLimitToResources(t *testing.T) {
	type args struct {
		limits []*quota.MetricLimit
	}
	tests := []struct {
		name string
		args args
		want common.BatchUpdateResources
		err  error
	}{
		{
			name: "error",
			args: args{
				limits: []*quota.MetricLimit{
					{
						Name:  "test-unused",
						Limit: 1,
					},
				},
			},
			err: ErrUnknownMetric,
		},
		{
			name: "hdd",
			args: args{
				limits: []*quota.MetricLimit{
					{
						Name:  "mdb.hdd.size",
						Limit: 1,
					},
				},
			},
			want: common.BatchUpdateResources{HDDSpace: optional.NewInt64(1)},
		},
		{
			name: "mem",
			args: args{
				limits: []*quota.MetricLimit{
					{
						Name:  "mdb.memory.size",
						Limit: 1,
					},
				},
			},
			want: common.BatchUpdateResources{Memory: optional.NewInt64(1)},
		},
		{
			name: "cpu",
			args: args{
				limits: []*quota.MetricLimit{
					{
						Name:  "mdb.cpu.count",
						Limit: 1,
					},
				},
			},
			want: common.BatchUpdateResources{CPU: optional.NewFloat64(1)},
		},
		{
			name: "gpu",
			args: args{
				limits: []*quota.MetricLimit{
					{
						Name:  "mdb.gpu.count",
						Limit: 1,
					},
				},
			},
			want: common.BatchUpdateResources{GPU: optional.NewInt64(1)},
		},
		{
			name: "ssd",
			args: args{
				limits: []*quota.MetricLimit{
					{
						Name:  "mdb.ssd.size",
						Limit: 1,
					},
				},
			},
			want: common.BatchUpdateResources{SSDSpace: optional.NewInt64(1)},
		},
		{
			name: "disk-with-zero",
			args: args{
				limits: []*quota.MetricLimit{
					{
						Name:  "mdb.ssd.size",
						Limit: 1,
					},
					{
						Name:  "mdb.hdd.size",
						Limit: 0,
					},
				},
			},
			want: common.BatchUpdateResources{SSDSpace: optional.NewInt64(1), HDDSpace: optional.NewInt64(0)},
		},
		{
			name: "empty",
		},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			got, err := metricLimitToResources(tt.args.limits)
			if tt.err != nil {
				require.Error(t, err)
				require.True(t, xerrors.Is(err, tt.err))
			} else {
				require.NoError(t, err)
				require.Equal(t, tt.want, got)
			}
		})
	}
}

func TestCloudToMetrics(t *testing.T) {
	type args struct {
		cloud metadb.Cloud
	}
	tests := []struct {
		name string
		args args
		want []*quota.QuotaMetric
	}{
		{
			name: "happy path",
			args: args{
				cloud: metadb.Cloud{
					Quota: metadb.Resources{
						CPU:      1.0,
						GPU:      2,
						Memory:   3,
						SSDSpace: 4,
						HDDSpace: 5,
						Clusters: 6,
					},
					Used: metadb.Resources{
						CPU:      10.0,
						GPU:      20,
						Memory:   30,
						SSDSpace: 40,
						HDDSpace: 50,
						Clusters: 60,
					},
				},
			},
			want: []*quota.QuotaMetric{
				{
					Name:  "mdb.hdd.size",
					Limit: 5,
					Usage: 50,
				},
				{
					Name:  "mdb.clusters.count",
					Limit: 6,
					Usage: 60,
				},
				{
					Name:  "mdb.memory.size",
					Limit: 3,
					Usage: 30,
				},
				{
					Name:  "mdb.cpu.count",
					Limit: 1,
					Usage: 10,
				},
				{
					Name:  "mdb.gpu.count",
					Limit: 2,
					Usage: 20,
				},
				{
					Name:  "mdb.ssd.size",
					Limit: 4,
					Usage: 40,
				},
			},
		},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			require.Equal(t, tt.want, CloudToMetrics(tt.args.cloud))
		})
	}
}
