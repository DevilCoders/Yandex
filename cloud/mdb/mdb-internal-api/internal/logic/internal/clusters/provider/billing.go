package provider

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/sqlerrors"
	clusterslogic "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/console"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/environment"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/resources"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (c *Clusters) EstimateBilling(ctx context.Context, folderID string, clusterType clusters.Type, hosts []clusterslogic.HostBillingSpec, cloudType environment.CloudType) (console.BillingEstimate, error) {
	resourcePresets := map[string]resources.Preset{}
	metrics := make([]console.BillingMetric, 0, len(hosts))
	for _, host := range hosts {
		presetExtID := host.ClusterResources.ResourcePresetExtID
		preset, ok := resourcePresets[presetExtID]
		if !ok {
			var err error
			preset, err = c.metaDB.ResourcePresetByExtID(ctx, presetExtID)
			if err != nil {
				if xerrors.Is(err, sqlerrors.ErrNotFound) {
					return console.BillingEstimate{}, semerr.WrapWithInvalidInputf(err, "resource preset %q is not available", presetExtID)
				}

				return console.BillingEstimate{}, xerrors.Errorf("resource preset %q load: %w", presetExtID, err)
			}
			resourcePresets[presetExtID] = preset
		}

		publicIP := int64(0)
		if host.AssignPublicIP {
			publicIP = 1
		}

		onDedicatedHost := int64(0)
		if host.OnDedicatedHost {
			onDedicatedHost = 1
		}

		tags := console.BillingMetricTags{
			PublicIP:                        publicIP,
			DiskTypeID:                      host.ClusterResources.DiskTypeExtID,
			ClusterType:                     clusterType.Stringified(),
			DiskSize:                        host.ClusterResources.DiskSize,
			ResourcePresetID:                presetExtID,
			ResourcePresetType:              preset.Type,
			PlatformID:                      preset.PlatformID,
			Cores:                           int64(preset.CPULimit),
			CoreFraction:                    int64(preset.CPUFraction),
			Memory:                          preset.MemoryLimit,
			SoftwareAcceleratedNetworkCores: preset.IOCoresLimit,
			Roles:                           []string{host.HostRole.Stringified()},
			Online:                          1,
			OnDedicatedHost:                 onDedicatedHost,
		}
		metrics = append(metrics, console.BillingMetric{
			FolderID: folderID,
			Schema:   console.BillingSchemaForCloudType(cloudType),
			Tags:     tags,
		})
	}

	return console.BillingEstimate{Metrics: metrics}, nil
}
