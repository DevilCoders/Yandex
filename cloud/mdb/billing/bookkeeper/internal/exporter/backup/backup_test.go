package backup

import (
	"testing"
	"time"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/billing/bookkeeper/internal/exporter/model"
	"a.yandex-team.ru/cloud/mdb/billing/bookkeeper/internal/invoicer"
	"a.yandex-team.ru/cloud/mdb/billing/bookkeeper/internal/invoicer/simplebackup"
	"a.yandex-team.ru/cloud/mdb/billing/internal/billingdb"
	"a.yandex-team.ru/cloud/mdb/internal/generator"
)

func TestYCBackupMetricsFromInvoice(t *testing.T) {
	type args struct {
		i        invoicer.Invoice
		idgen    generator.IDGenerator
		sourceID string
	}
	tests := []struct {
		name    string
		args    args
		want    []model.Metric
		wantErr bool
	}{
		{
			name: "with_hole",
			args: args{
				i: invoicer.Invoice{
					Resource: invoicer.Resource{
						ClusterID:   "cid1",
						FolderID:    "folder1",
						CloudID:     "cloud1",
						ClusterType: "postgresql_cluster",
					},
					BillType: billingdb.BillTypeBackup,
					FromTS:   time.Date(2022, 01, 01, 14, 00, 00, 0, time.UTC),
					UntilTS:  time.Date(2022, 01, 01, 15, 00, 01, 0, time.UTC),
					Ranges: []invoicer.Range{
						{
							FromTS:  time.Date(2022, 01, 01, 14, 00, 00, 0, time.UTC),
							UntilTS: time.Date(2022, 01, 01, 14, 15, 30, 0, time.UTC),
							Payload: simplebackup.BackupPayload{StorageSpace: 1000},
						},
						{
							FromTS:  time.Date(2022, 01, 01, 14, 50, 00, 0, time.UTC),
							UntilTS: time.Date(2022, 01, 01, 15, 00, 01, 0, time.UTC),
							Payload: simplebackup.BackupPayload{StorageSpace: 2000},
						},
					},
				},
			},
			want: []model.Metric{
				&model.CloudMetric{
					CloudID:    "cloud1",
					FolderID:   "folder1",
					ResourceID: "cid1",
					Tags: BackupMetricTags{
						ClusterType: "postgresql_cluster",
					},
					ID: "1",
					Usage: model.MetricUsage{
						Type:     "delta",
						Quantity: 930000,
						Unit:     "byte*second",
						Start:    time.Date(2022, 01, 01, 14, 00, 00, 0, time.UTC).Unix(),
						Finish:   time.Date(2022, 01, 01, 14, 15, 30, 0, time.UTC).Unix(),
					},
					Version:  "v1alpha1",
					SourceID: "fqdn",
					Schema:   "mdb.db.backup.v1",
					Labels:   map[string]string{},
					SourceWT: time.Date(2022, 01, 01, 15, 00, 01, 0, time.UTC).Unix(),
				}, &model.CloudMetric{
					CloudID:    "cloud1",
					FolderID:   "folder1",
					ResourceID: "cid1",
					Tags: BackupMetricTags{
						ClusterType: "postgresql_cluster",
					},
					ID: "2",
					Usage: model.MetricUsage{
						Type:     "delta",
						Quantity: 1200000,
						Unit:     "byte*second",
						Start:    time.Date(2022, 01, 01, 14, 50, 00, 0, time.UTC).Unix(),
						Finish:   time.Date(2022, 01, 01, 15, 00, 00, 0, time.UTC).Unix(),
					},
					Version:  "v1alpha1",
					SourceID: "fqdn",
					Schema:   "mdb.db.backup.v1",
					Labels:   map[string]string{},
					SourceWT: time.Date(2022, 01, 01, 15, 00, 01, 0, time.UTC).Unix(),
				}, &model.CloudMetric{
					CloudID:    "cloud1",
					FolderID:   "folder1",
					ResourceID: "cid1",
					Tags: BackupMetricTags{
						ClusterType: "postgresql_cluster",
					},
					ID: "3",
					Usage: model.MetricUsage{
						Type:     "delta",
						Quantity: 2000,
						Unit:     "byte*second",
						Start:    time.Date(2022, 01, 01, 15, 00, 00, 0, time.UTC).Unix(),
						Finish:   time.Date(2022, 01, 01, 15, 00, 01, 0, time.UTC).Unix(),
					},
					Version:  "v1alpha1",
					SourceID: "fqdn",
					Schema:   "mdb.db.backup.v1",
					Labels:   map[string]string{},
					SourceWT: time.Date(2022, 01, 01, 15, 00, 01, 0, time.UTC).Unix(),
				},
			},
			wantErr: false,
		},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			got, err := YCBackupMetricsFromInvoice(tt.args.i, generator.NewSequentialGenerator(1), "fqdn")
			if (err != nil) != tt.wantErr {
				t.Errorf("YCBackupMetricsFromInvoice() error = %v, wantErr %v", err, tt.wantErr)
				return
			}
			require.Equal(t, tt.want, got)
		})
	}
}
