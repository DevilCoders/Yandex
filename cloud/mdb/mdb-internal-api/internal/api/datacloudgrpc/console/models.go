package console

import (
	"strconv"
	"strings"

	consolev1 "a.yandex-team.ru/cloud/mdb/datacloud/private_api/datacloud/console/v1"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/console"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/x/math"
)

func GenerateClusterNameHint(clusterType clusters.Type, existingClusters []console.Cluster) (string, error) {
	if clusterType == clusters.TypeUnknown {
		return "", xerrors.Errorf("generating cluster name hint for unknown cluster type")
	}

	clusterPrefix := strings.ToLower(clusterType.String())

	maxIndex := 0
	for _, cluster := range existingClusters {
		if cluster.Type == clusterType && strings.HasPrefix(cluster.Name, clusterPrefix) {
			index, err := strconv.Atoi(cluster.Name[len(clusterPrefix):])
			if err != nil {
				// User-created cluster name. We don't mess with it
				continue
			}

			maxIndex = math.MaxInt(maxIndex, index)
		}
	}

	return clusterPrefix + strconv.Itoa(maxIndex+1), nil
}

func BillingEstimateToGRPC(estimate console.BillingEstimate) *consolev1.BillingEstimate {
	metrics := make([]*consolev1.BillingMetric, len(estimate.Metrics))
	for i, metric := range estimate.Metrics {
		metrics[i] = &consolev1.BillingMetric{
			FolderId: metric.FolderID,
			Schema:   metric.Schema,
			Tags:     BillingMetricTagsToGRPC(metric.Tags),
		}
	}

	return &consolev1.BillingEstimate{
		Metrics: metrics,
	}
}

func BillingMetricTagsToGRPC(tags console.BillingMetricTags) *consolev1.BillingMetric_BillingTags {
	return &consolev1.BillingMetric_BillingTags{
		PublicIp:                        tags.PublicIP,
		DiskTypeId:                      tags.DiskTypeID,
		ClusterType:                     tags.ClusterType,
		DiskSize:                        tags.DiskSize,
		ResourcePresetId:                tags.ResourcePresetID,
		PlatformId:                      tags.PlatformID,
		Cores:                           tags.Cores,
		CoreFraction:                    tags.CoreFraction,
		Memory:                          tags.Memory,
		SoftwareAcceleratedNetworkCores: tags.SoftwareAcceleratedNetworkCores,
		Roles:                           tags.Roles,
		Online:                          tags.Online,
		OnDedicatedHost:                 tags.OnDedicatedHost,
		CloudProvider:                   tags.CloudProvider,
		CloudRegion:                     tags.CloudRegion,
		ResourcePresetType:              tags.ResourcePresetType,
	}
}
