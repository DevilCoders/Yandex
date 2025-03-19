package cloudstorage

import (
	"a.yandex-team.ru/cloud/mdb/billing/bookkeeper/internal/exporter/model"
	"a.yandex-team.ru/cloud/mdb/billing/bookkeeper/internal/invoicer"
	"a.yandex-team.ru/cloud/mdb/billing/bookkeeper/internal/invoicer/cloudstorage"
	"a.yandex-team.ru/cloud/mdb/internal/generator"
	"a.yandex-team.ru/library/go/core/xerrors"
)

const (
	DCCloudStorageSchema = "mdb.cloud_storage.dc.v1"
)

type CloudStorageMetricTags struct {
	ClusterType        string `json:"cluster_type"`
	CloudRegion        string `json:"cloud_region"`
	CloudProvider      string `json:"cloud_provider"`
	ResourcePresetType string `json:"resource_preset_type"`
}

func DCCloudStorageMetricsFromInvoice(i invoicer.Invoice, idgen generator.IDGenerator, sourceID string) ([]model.Metric, error) {
	var metrics []model.Metric
	for _, r := range i.Ranges {
		payload, ok := r.Payload.(*cloudstorage.CloudStoragePayload)
		if !ok {
			return nil, xerrors.Errorf("expected CloudStoragePayload, but given %T: %v", r.Payload, r.Payload)
		}

		id, err := idgen.Generate()
		if err != nil {
			return nil, err
		}

		m := &model.CloudMetric{
			ID:         id,
			Schema:     DCCloudStorageSchema,
			CloudID:    i.Resource.CloudID,
			FolderID:   i.Resource.FolderID,
			ResourceID: i.Resource.ClusterID,
			SourceID:   sourceID,
			SourceWT:   i.UntilTS.Unix(),
			Usage: model.MetricUsage{
				Type:     "delta",
				Quantity: payload.StorageSpace,
				Unit:     "mb*hour",
				Start:    r.FromTS.Unix(),
				Finish:   r.UntilTS.Unix(),
			},
			Tags: CloudStorageMetricTags{
				ClusterType:        string(i.Resource.ClusterType),
				CloudRegion:        i.Resource.CloudRegion,
				CloudProvider:      i.Resource.CloudProvider,
				ResourcePresetType: i.Resource.ResourcePresetType,
			},
		}
		metrics = append(metrics, m)
	}
	return metrics, nil
}
