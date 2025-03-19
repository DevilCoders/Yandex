package elasticsearch

import (
	"context"
	"sort"

	esv1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/elasticsearch/v1"
	esv1console "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/elasticsearch/v1/console"
	grpcapi "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc/common"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/console"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/elasticsearch"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/elasticsearch/esmodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	consolemodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/console"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts"
)

// ClusterService implements DB-specific gRPC methods
type ConsoleClusterService struct {
	esv1console.UnimplementedClusterServiceServer

	Console       console.Console
	es            elasticsearch.ElasticSearch
	saltEnvMapper grpcapi.SaltEnvMapper
}

var _ esv1console.ClusterServiceServer = &ConsoleClusterService{}

func NewConsoleClusterService(cnsl console.Console, es elasticsearch.ElasticSearch, saltEnvsCfg logic.SaltEnvsConfig) *ConsoleClusterService {
	return &ConsoleClusterService{
		Console: cnsl,
		es:      es,
		saltEnvMapper: grpcapi.NewSaltEnvMapper(
			int64(esv1.Cluster_PRODUCTION),
			int64(esv1.Cluster_PRESTABLE),
			int64(esv1.Cluster_ENVIRONMENT_UNSPECIFIED),
			saltEnvsCfg)}
}

func (ccs *ConsoleClusterService) resourcePresetsToGRPC(resourcePresets []consolemodels.ResourcePreset, unlimitedHosts bool) ([]*esv1console.ElasticsearchClustersConfig_HostType, error) {
	var lastRecord *consolemodels.ResourcePreset
	aggregated := common.AggregateResourcePresets(resourcePresets)
	hostRolesGRPC := make([]*esv1console.ElasticsearchClustersConfig_HostType, 0, len(aggregated))
	for hostRole, hostRoleAgg := range aggregated {
		roleDefaultResources, err := ccs.Console.GetDefaultResourcesByClusterType(clusters.TypeElasticSearch, hosts.Role(hostRole))
		if err != nil {
			return nil, err
		}

		resourcesGRPC := make([]*esv1console.ElasticsearchClustersConfig_HostType_ResourcePreset, 0, len(hostRoleAgg))
		for _, presetAgg := range hostRoleAgg {
			zonesGRPC := make([]*esv1console.ElasticsearchClustersConfig_HostType_ResourcePreset_Zone, 0, len(presetAgg))
			for _, zoneAgg := range presetAgg {
				diskTypesGRPC := make([]*esv1console.ElasticsearchClustersConfig_HostType_ResourcePreset_Zone_DiskType, 0, len(zoneAgg))
				for _, record := range zoneAgg {
					maxHosts := record.MaxHosts
					if hostRole == hosts.RoleElasticSearchDataNode && unlimitedHosts {
						maxHosts = 100500
					}
					diskTypeGRPC := &esv1console.ElasticsearchClustersConfig_HostType_ResourcePreset_Zone_DiskType{
						DiskTypeId: record.DiskTypeExtID,
						MaxHosts:   maxHosts,
						MinHosts:   record.MinHosts,
					}
					// Providing non empty disk size field
					if len(record.DiskSizes) == 0 {
						diskTypeGRPC.DiskSize = &esv1console.ElasticsearchClustersConfig_HostType_ResourcePreset_Zone_DiskType_DiskSizeRange_{
							DiskSizeRange: &esv1console.ElasticsearchClustersConfig_HostType_ResourcePreset_Zone_DiskType_DiskSizeRange{
								Min: record.DiskSizeRange.Lower,
								Max: record.DiskSizeRange.Upper,
							},
						}
					} else {
						diskTypeGRPC.DiskSize = &esv1console.ElasticsearchClustersConfig_HostType_ResourcePreset_Zone_DiskType_DiskSizes_{
							DiskSizes: &esv1console.ElasticsearchClustersConfig_HostType_ResourcePreset_Zone_DiskType_DiskSizes{
								Sizes: record.DiskSizes,
							},
						}
					}
					diskTypesGRPC = append(diskTypesGRPC, diskTypeGRPC)
					lastRecord = &record
				}
				// Using last record to fill zone info
				zoneGRPC := esv1console.ElasticsearchClustersConfig_HostType_ResourcePreset_Zone{
					ZoneId:    lastRecord.Zone,
					DiskTypes: diskTypesGRPC,
				}
				zonesGRPC = append(zonesGRPC, &zoneGRPC)
			}
			sort.Slice(zonesGRPC, func(i, j int) bool {
				zoneA := zonesGRPC[i]
				zoneB := zonesGRPC[j]
				return zoneA.ZoneId < zoneB.ZoneId
			})
			// Using last record to fill preset info
			presetGRPC := &esv1console.ElasticsearchClustersConfig_HostType_ResourcePreset{
				PresetId:       lastRecord.ExtID,
				Type:           lastRecord.FlavorType,
				CpuLimit:       lastRecord.CPULimit,
				CpuFraction:    lastRecord.CPUFraction,
				MemoryLimit:    lastRecord.MemoryLimit,
				Zones:          zonesGRPC,
				Generation:     lastRecord.Generation,
				GenerationName: lastRecord.GenerationName,
				Decomissioning: lastRecord.Decommissioned,
			}
			resourcesGRPC = append(resourcesGRPC, presetGRPC)
		}
		hostRoleGRPC := &esv1console.ElasticsearchClustersConfig_HostType{
			Type:            HostRoleToGRPC(hostRole),
			ResourcePresets: resourcesGRPC,
			DefaultResources: &esv1console.ElasticsearchClustersConfig_HostType_DefaultResources{
				ResourcePresetId: roleDefaultResources.ResourcePresetExtID,
				DiskTypeId:       roleDefaultResources.DiskTypeExtID,
				DiskSize:         roleDefaultResources.DiskSize,
				Generation:       roleDefaultResources.Generation,
				GenerationName:   roleDefaultResources.GenerationName,
			},
		}
		hostRolesGRPC = append(hostRolesGRPC, hostRoleGRPC)
	}
	sort.Slice(hostRolesGRPC, func(i, j int) bool {
		itemA := hostRolesGRPC[i]
		itemB := hostRolesGRPC[j]
		return itemA.Type < itemB.Type
	})
	return hostRolesGRPC, nil
}

func (ccs *ConsoleClusterService) Get(ctx context.Context, cfgReq *esv1console.GetElasticsearchClustersConfigRequest) (*esv1console.ElasticsearchClustersConfig, error) {
	resourcePresets, err := ccs.Console.GetResourcePresetsByClusterType(ctx, clusters.TypeElasticSearch, cfgReq.FolderId, true)
	if err != nil {
		return nil, err
	}
	featureFlags, err := ccs.Console.GetFeatureFlags(ctx, cfgReq.FolderId)
	if err != nil {
		return nil, err
	}

	var versions []string
	var availableVersions []*esv1console.ElasticsearchClustersConfig_VersionInfo
	supportedVersions := ccs.es.SupportedVersions(ctx)
	for _, ver := range supportedVersions {
		versions = append(versions, ver.ID)
		var updatableTo []string
		for _, uv := range ver.UpdatableTo {
			updatableTo = append(updatableTo, uv.ID)
		}
		availableVersions = append(availableVersions, &esv1console.ElasticsearchClustersConfig_VersionInfo{
			Id:          ver.ID,
			Name:        ver.ID,
			Deprecated:  ver.Deprecated,
			UpdatableTo: updatableTo,
			Plugins:     esmodels.AllowedPlugins(ver),
		})
	}
	defaultVersion, err := supportedVersions.DefaultVersion()
	if err != nil {
		return nil, err
	}

	editions, err := ccs.es.AllowedEditions(ctx)
	if err != nil {
		return nil, err
	}
	availableEditions := make([]*esv1console.ElasticsearchClustersConfig_EditionInfo, 0, len(editions))
	for _, ed := range editions {
		availableEditions = append(availableEditions, &esv1console.ElasticsearchClustersConfig_EditionInfo{
			Id:   ed.String(),
			Name: ed.String(),
		})
	}

	unlimitedHosts := featureFlags.Has(esmodels.ElasticSearchAllowUnlimitedHostsFeatureFlag)
	resourcePresetsGRPC, err := ccs.resourcePresetsToGRPC(resourcePresets, unlimitedHosts)
	if err != nil {
		return nil, err
	}

	connectionDomain, err := ccs.Console.GetConnectionDomain(ctx)
	if err != nil {
		return nil, err
	}

	return &esv1console.ElasticsearchClustersConfig{
		ClusterName: &esv1console.NameValidator{
			Regexp: models.DefaultClusterNamePattern,
			Min:    models.DefaultClusterNameMinLen,
			Max:    models.DefaultClusterNameMaxLen,
		},
		UserName: &esv1console.NameValidator{
			Regexp: models.DefaultUserNamePattern,
			Min:    models.DefaultUserNameMinLen,
			Max:    models.DefaultUserNameMaxLen,
		},
		Password: &esv1console.NameValidator{
			Regexp: models.DefaultUserPasswordPattern,
			Min:    models.DefaultUserPasswordMinLen,
			Max:    models.DefaultUserPasswordMaxLen,
		},
		HostTypes:         resourcePresetsGRPC,
		Versions:          versions,
		AvailableVersions: availableVersions,
		AvailableEditions: availableEditions,
		DefaultVersion:    defaultVersion.ID,
		ConnectionDomain:  connectionDomain,
	}, nil
}

func (ccs *ConsoleClusterService) EstimateCreate(ctx context.Context, req *esv1.CreateClusterRequest) (*esv1console.BillingEstimateResponse, error) {
	env, err := ccs.saltEnvMapper.FromGRPC(int64(req.GetEnvironment()))
	if err != nil {
		return nil, err
	}

	confSpec, err := ConfigFromGRPC(req.GetConfigSpec(), ccs.es.SupportedVersions(ctx))
	if err != nil {
		return nil, err
	}
	hostSpec, err := HostsFromGRPC(req.GetHostSpecs())
	if err != nil {
		return nil, err
	}

	estimate, err := ccs.es.EstimateCreateCluster(
		ctx,
		elasticsearch.CreateClusterArgs{
			FolderExtID:      req.GetFolderId(),
			Name:             req.GetName(),
			Description:      req.GetDescription(),
			Labels:           req.GetLabels(),
			UserSpecs:        UserSpecsFromGRPC(req.GetUserSpecs()),
			HostSpec:         hostSpec,
			Environment:      env,
			NetworkID:        req.GetNetworkId(),
			SecurityGroupIDs: req.GetSecurityGroupIds(),
			ConfigSpec:       confSpec,
		})
	if err != nil {
		return nil, err
	}

	return billingEstimateToGRPC(estimate), nil
}

func billingEstimateToGRPC(estimate consolemodels.BillingEstimate) *esv1console.BillingEstimateResponse {
	metrics := make([]*esv1console.BillingMetric, 0, len(estimate.Metrics))
	for _, metric := range estimate.Metrics {
		tags := &esv1console.BillingMetric_BillingTags{
			PublicIp:                        metric.Tags.PublicIP,
			DiskTypeId:                      metric.Tags.DiskTypeID,
			ClusterType:                     metric.Tags.ClusterType,
			DiskSize:                        metric.Tags.DiskSize,
			ResourcePresetId:                metric.Tags.ResourcePresetID,
			PlatformId:                      metric.Tags.PlatformID,
			Cores:                           metric.Tags.Cores,
			CoreFraction:                    metric.Tags.CoreFraction,
			Memory:                          metric.Tags.Memory,
			SoftwareAcceleratedNetworkCores: metric.Tags.SoftwareAcceleratedNetworkCores,
			Roles:                           metric.Tags.Roles,
			Online:                          metric.Tags.Online,
			OnDedicatedHost:                 metric.Tags.OnDedicatedHost,
			Edition:                         metric.Tags.Edition,
		}

		metrics = append(metrics, &esv1console.BillingMetric{
			FolderId: metric.FolderID,
			Schema:   metric.Schema,
			Tags:     tags,
		})
	}
	return &esv1console.BillingEstimateResponse{
		Metrics: metrics,
	}
}

func (ccs *ConsoleClusterService) GetCreateConfig(_ context.Context, _ *esv1console.CreateClusterConfigRequest) (*esv1console.JSONSchema, error) {
	jsonschema := `{
	  "ClusterCreate": {
		"type": "object",
		"properties": {
		  "folderId": {
			"type": "string"
		  },
		  "name": {
			"type": "string"
		  },
		  "description":  {
			"type": "string"
		  },
		  "labels": {
			"type": "object"
		  },
		  "environment": {
			"$ref": "#/ClusterEnvironment"
		  },
		  "configSpec": {
			"$ref": "#/ClusterConfigSpec"
		  },
		  "hostSpecs": {
			"type": "array",
			"items": {
			  "$ref": "#/HostSpec"
			}
		  },
		  "networkId": {
			"type": "string"
		  },
		  "securityGroupIds": {
			"type": "array",
			"items": {
			  "type": "string"
			}
		  }
		},
		"required": [
		  "configSpec",
		  "environment",
		  "folderId",
		  "hostSpecs",
		  "name"
		]
	  },
	  "ClusterConfigSpec": {
		"type": "object",
		"properties": {
		  "version": {
			"type": "string"
		  },
		  "elasticsearchSpec": {
			"$ref": "#/ElasticsearchSpec"
		  },
		  "edition": {
			"type": "string"
		  },
		  "adminPassword": {
			"type": "string"
		  }
		},
		"required": [
		  "adminPassword",
		  "edition",
		  "version",
		  "elasticsearchSpec"
		]
	  },
	  "ElasticsearchConfig7": {
		"type": "object",
		"properties": {
		  "fielddataCacheSize": {
			"type": "string",
			"description": "the percentage or absolute value (10%, 512mb) of heap space that is allocated to fielddata"
		  }
		}
	  },
	  "ElasticsearchSpec": {
		"type": "object",
		"properties": {
		  "dataNode": {
			"$ref": "#/ElasticsearchSpecDataNode"
		  },
		  "masterNode": {
			"$ref": "#/ElasticsearchSpecMasterNode"
		  },
		  "plugins": {
			"type": "array",
			"items": {
			  "type": "string"
			}
		  }
		},
		"required": [
		  "dataNode"
		]
	  },
	  "ElasticsearchSpecDataNode": {
		"type": "object",
		"properties": {
		  "resources": {
			"$ref": "#/Resources"
		  },
		  "elasticsearchConfig_7": {
			"$ref": "#/ElasticsearchConfig7"
		  }
		},
		"required": [
		  "resources"
		]
	  },
	  "ElasticsearchSpecMasterNode": {
		"type": "object",
		"properties": {
		  "resources": {
			"$ref": "#/Resources"
		  }
		},
		"required": [
		  "resources"
		]
	  },
	  "HostSpec": {
		"type": "object",
		"properties": {
		  "zoneId": {
			"type": "string"
		  },
		  "subnetId": {
			"type": "string"
		  },
		  "assignPublicIp": {
			"format": "boolean",
			"type": "boolean"
		  },
		  "type": {
			"$ref": "#/HostType"
		  },
		  "shardName": {
			"type": "string"
		  }
		},
		"required": [
		  "resources"
		]
	  },
	  "HostType": {
		"enum": [
		  "DATA_NODE",
		  "MASTER_NODE"
		],
		"type": "string"
	  },
	  "Resources": {
		"type": "object",
		"properties": {
		  "resourcePresetId": {
			"type": "string"
		  },
		  "diskSize": {
			"format": "int64",
			"type": "string"
		  },
		  "diskTypeId": {
			"type": "string"
		  }
		},
		"required": [
		  "resourcePresetId",
		  "diskSize",
		  "diskTypeId"
		]
	  }
	}`
	return &esv1console.JSONSchema{Schema: jsonschema}, nil
}
