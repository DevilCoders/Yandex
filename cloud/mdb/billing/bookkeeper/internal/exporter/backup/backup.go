package backup

import (
	"a.yandex-team.ru/cloud/mdb/billing/bookkeeper/internal/exporter/model"
	"a.yandex-team.ru/cloud/mdb/billing/bookkeeper/internal/invoicer"
	"a.yandex-team.ru/cloud/mdb/billing/bookkeeper/internal/invoicer/simplebackup"
	"a.yandex-team.ru/cloud/mdb/internal/generator"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type BackupMetricTags struct {
	ClusterType string `json:"cluster_type"`
}

func YCBackupMetricsFromInvoice(i invoicer.Invoice, idgen generator.IDGenerator, sourceID string) ([]model.Metric, error) {
	var metrics []model.Metric

	var ranges []invoicer.Range
	for _, ir := range i.Ranges {
		ranges = append(ranges, invoicer.SplitRangeByHours(ir)...)
	}

	for _, r := range ranges {
		backupPayload, ok := r.Payload.(simplebackup.BackupPayload)
		if !ok {
			return nil, xerrors.Errorf("expected BackupPayload, but given %T: %v", r.Payload, r.Payload)
		}
		id, err := idgen.Generate()
		if err != nil {
			return nil, err
		}
		start := r.FromTS.Unix()
		finish := r.UntilTS.Unix()
		quantity := backupPayload.StorageSpace * (finish - start)
		m := &model.CloudMetric{
			CloudID:    i.Resource.CloudID,
			FolderID:   i.Resource.FolderID,
			ResourceID: i.Resource.ClusterID,
			Tags: BackupMetricTags{
				ClusterType: string(i.Resource.ClusterType),
			},
			ID: id,
			Usage: model.MetricUsage{
				Type:     "delta",
				Quantity: quantity,
				Unit:     "byte*second",
				Start:    start,
				Finish:   finish,
			},
			Version:  "v1alpha1",
			SourceID: sourceID,
			Schema:   "mdb.db.backup.v1",
			Labels:   map[string]string{},
			SourceWT: i.UntilTS.Unix(),
		}
		metrics = append(metrics, m)
	}
	return metrics, nil
}
