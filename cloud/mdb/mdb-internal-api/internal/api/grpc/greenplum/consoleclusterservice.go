package greenplum

import (
	"context"
	"encoding/json"
	"fmt"
	"sort"

	gpv1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/greenplum/v1"
	gpv1console "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/greenplum/v1/console"
	"a.yandex-team.ru/cloud/mdb/internal/compute/compute"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	grpcapi "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc/common"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/console"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/greenplum"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	consolemodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/console"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts"
	"a.yandex-team.ru/library/go/slices"
)

// ConsoleClusterService implements DB-specific gRPC methods
type ConsoleClusterService struct {
	gpv1console.UnimplementedClusterServiceServer

	Console       console.Console
	gp            greenplum.Greenplum
	saltEnvMapper grpcapi.SaltEnvMapper
}

var _ gpv1console.ClusterServiceServer = &ConsoleClusterService{}

func NewConsoleClusterService(cnsl console.Console, gp greenplum.Greenplum, saltEnvsCfg logic.SaltEnvsConfig) *ConsoleClusterService {
	return &ConsoleClusterService{
		Console: cnsl,
		gp:      gp,
		saltEnvMapper: grpcapi.NewSaltEnvMapper(
			int64(gpv1.Cluster_PRODUCTION),
			int64(gpv1.Cluster_PRESTABLE),
			int64(gpv1.Cluster_ENVIRONMENT_UNSPECIFIED),
			saltEnvsCfg)}
}

func (ccs *ConsoleClusterService) resourcePresetsToGRPC(resourcePresets []consolemodels.ResourcePreset, allowLowMem bool, hostGroupHostType map[string]compute.HostGroupHostType) ([]*gpv1console.GreenplumClustersConfig_HostType, error) {
	hostGroupGenerations := make(map[int64]bool)
	for _, hght := range hostGroupHostType {
		generation := hght.Generation()
		hostGroupGenerations[generation] = true
	}

	var lastRecord *consolemodels.ResourcePreset
	aggregated := common.AggregateResourcePresets(resourcePresets)
	hostRolesGRPC := make([]*gpv1console.GreenplumClustersConfig_HostType, 0, len(aggregated))
	var minCoresInHostGroups int64
	var minMemoryInHostGroups int64
	for hostRole, hostRoleAgg := range aggregated {
		roleDefaultResources, err := ccs.Console.GetDefaultResourcesByClusterType(clusters.TypeGreenplumCluster, hosts.Role(hostRole))
		if err != nil {
			return nil, err
		}

		resourcesGRPC := make([]*gpv1console.GreenplumClustersConfig_HostType_ResourcePreset, 0, len(hostRoleAgg))
		for _, presetAgg := range hostRoleAgg {
			zonesGRPC := make([]*gpv1console.GreenplumClustersConfig_HostType_ResourcePreset_Zone, 0, len(presetAgg))
			for _, zoneAgg := range presetAgg {
				diskTypesGRPC := make([]*gpv1console.GreenplumClustersConfig_HostType_ResourcePreset_Zone_DiskType, 0, len(zoneAgg))
				for _, record := range zoneAgg {
					diskTypeGRPC := &gpv1console.GreenplumClustersConfig_HostType_ResourcePreset_Zone_DiskType{
						DiskTypeId: record.DiskTypeExtID,
						MaxHosts:   record.MaxHosts,
						MinHosts:   record.MinHosts,
					}
					if hostRole == hosts.RoleGreenplumSegmentNode {
						diskTypeGRPC.HostCountDivider = 2
						diskTypeGRPC.MaxSegmentInHostCount = greenplum.MaxSegmentInHostCountCalc(record.MemoryLimit, allowLowMem)
					} else {
						diskTypeGRPC.HostCountDivider = 1
					}
					// Providing non empty disk size field
					_, _, minDisks, diskSize, _ := compute.GetMinHostGroupResources(hostGroupHostType, record.Zone)
					if record.DiskTypeExtID == "local-ssd" && len(hostGroupHostType) > 0 && diskSize > 0 {
						// Fill local-ssd sizes according host group
						var diskSizes []int64
						for i := diskSize; i <= minDisks*diskSize; i = i + diskSize {
							diskSizes = append(diskSizes, i)
						}
						diskTypeGRPC.DiskSize = &gpv1console.GreenplumClustersConfig_HostType_ResourcePreset_Zone_DiskType_DiskSizes_{
							DiskSizes: &gpv1console.GreenplumClustersConfig_HostType_ResourcePreset_Zone_DiskType_DiskSizes{
								Sizes: diskSizes,
							},
						}
					} else if len(record.DiskSizes) == 0 {
						diskTypeGRPC.DiskSize = &gpv1console.GreenplumClustersConfig_HostType_ResourcePreset_Zone_DiskType_DiskSizeRange_{
							DiskSizeRange: &gpv1console.GreenplumClustersConfig_HostType_ResourcePreset_Zone_DiskType_DiskSizeRange{
								Min: record.DiskSizeRange.Lower,
								Max: record.DiskSizeRange.Upper,
							},
						}
					} else {
						diskTypeGRPC.DiskSize = &gpv1console.GreenplumClustersConfig_HostType_ResourcePreset_Zone_DiskType_DiskSizes_{
							DiskSizes: &gpv1console.GreenplumClustersConfig_HostType_ResourcePreset_Zone_DiskType_DiskSizes{
								Sizes: record.DiskSizes,
							},
						}
					}
					diskTypesGRPC = append(diskTypesGRPC, diskTypeGRPC)
					lastRecord = &record
				}
				// Filter zones according host group
				if len(hostGroupHostType) > 0 {
					minCores, minMemory, _, _, _ := compute.GetMinHostGroupResources(hostGroupHostType, lastRecord.Zone)
					if minCores == 0 {
						continue
					}
					if minCores > minCoresInHostGroups {
						minCoresInHostGroups = minCores
					}
					if minMemory > minMemoryInHostGroups {
						minMemoryInHostGroups = minMemory
					}
				}
				// Using last record to fill zone info
				zoneGRPC := gpv1console.GreenplumClustersConfig_HostType_ResourcePreset_Zone{
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
			// Filter preset info according host group
			if len(hostGroupHostType) > 0 {
				if lastRecord.CPULimit > minCoresInHostGroups {
					continue
				}
				if lastRecord.MemoryLimit > minMemoryInHostGroups {
					continue
				}
			}

			_, equalGenerations := hostGroupGenerations[lastRecord.Generation]
			// If there is more than one generation in host groups
			// or just one, but not equal to current flavor generation
			// then skip this flavor

			if (len(hostGroupGenerations) > 1) ||
				(len(hostGroupGenerations) == 1 && !equalGenerations) {
				continue
			}

			// Using last record to fill preset info
			presetGRPC := &gpv1console.GreenplumClustersConfig_HostType_ResourcePreset{
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
		sort.Slice(resourcesGRPC, func(i, j int) bool {
			if resourcesGRPC[i].Generation != resourcesGRPC[j].Generation {
				return resourcesGRPC[i].Generation < resourcesGRPC[j].Generation
			}
			if resourcesGRPC[i].CpuLimit != resourcesGRPC[j].CpuLimit {
				return resourcesGRPC[i].CpuLimit < resourcesGRPC[j].CpuLimit
			}
			if resourcesGRPC[i].MemoryLimit != resourcesGRPC[j].MemoryLimit {
				return resourcesGRPC[i].MemoryLimit < resourcesGRPC[j].MemoryLimit
			}
			return false
		})

		hostRoleGRPC := &gpv1console.GreenplumClustersConfig_HostType{
			Type:            HostRoleToGRPC(hostRole),
			ResourcePresets: resourcesGRPC,
			DefaultResources: &gpv1console.GreenplumClustersConfig_HostType_DefaultResources{
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

	sort.Slice(hostRolesGRPC, func(i, j int) bool {
		return hostRolesGRPC[i].Type < hostRolesGRPC[j].Type
	})

	return hostRolesGRPC, nil
}

func (ccs *ConsoleClusterService) Get(ctx context.Context, cfgReq *gpv1console.GetGreenplumClustersConfigRequest) (*gpv1console.GreenplumClustersConfig, error) {
	resourcePresets, err := ccs.Console.GetResourcePresetsByClusterType(ctx, clusters.TypeGreenplumCluster, cfgReq.FolderId, true)
	if err != nil {
		return nil, err
	}
	hostGroupHostType, err := ccs.gp.GetHostGroupType(ctx, slices.DedupStrings(cfgReq.GetHostGroupIds()))
	if err != nil {
		return nil, err
	}
	clusterVersions, err := ccs.gp.GetDefaultVersions(ctx)
	if err != nil {
		return nil, err
	}

	var availableVersions []*gpv1console.GreenplumClustersConfig_VersionInfo
	for _, ver := range clusterVersions {
		if ver.IsDeprecated {
			continue
		}
		availableVersions = append(availableVersions, ccs.versionsToGRPC(ver))
	}

	allowLowMem := false
	if cfgReq.FolderId != "" {
		allowLowMem, err = ccs.gp.IsLowMemSegmentAllowed(ctx, cfgReq.FolderId)
		if err != nil {
			return nil, err
		}
	}

	resourcePresetsGRPC, err := ccs.resourcePresetsToGRPC(resourcePresets, allowLowMem, hostGroupHostType)
	if err != nil {
		return nil, err
	}

	return &gpv1console.GreenplumClustersConfig{
		ClusterName: &gpv1console.NameValidator{
			Regexp: models.DefaultClusterNamePattern,
			Min:    models.DefaultClusterNameMinLen,
			Max:    models.DefaultClusterNameMaxLen,
		},
		UserName: &gpv1console.NameValidator{
			Regexp: models.DefaultUserNamePattern,
			Min:    models.DefaultUserNameMinLen,
			Max:    models.DefaultUserNameMaxLen,
		},
		Password: &gpv1console.NameValidator{
			Regexp: models.DefaultUserPasswordPattern,
			Min:    models.DefaultUserPasswordMinLen,
			Max:    models.DefaultUserPasswordMaxLen,
		},
		HostTypes:         resourcePresetsGRPC,
		Versions:          common.CollectAllVersionNames(clusterVersions),
		AvailableVersions: availableVersions,
		DefaultVersion:    common.CollectDefaultVersion(clusterVersions),
	}, nil
}

func (ccs *ConsoleClusterService) versionsToGRPC(ver consolemodels.DefaultVersion) *gpv1console.GreenplumClustersConfig_VersionInfo {
	return &gpv1console.GreenplumClustersConfig_VersionInfo{
		Id:         ver.MajorVersion,
		Name:       ver.Name,
		Deprecated: ver.IsDeprecated,
	}
}

func (ccs *ConsoleClusterService) EstimateCreate(ctx context.Context, req *gpv1.CreateClusterRequest) (*gpv1console.BillingEstimateResponse, error) {
	env, err := ccs.saltEnvMapper.FromGRPC(int64(req.GetEnvironment()))
	if err != nil {
		return nil, err
	}

	config, err := clusterConfigSpecFromGRPC(req.GetConfig(), grpcapi.AllPaths(),
		req.GetMasterConfig(), req.GetSegmentConfig())
	if err != nil {
		return nil, err
	}
	configMaster, err := clusterMasterConfigSpecFromGRPC(req.GetMasterConfig(), grpcapi.AllPaths())
	if err != nil {
		return nil, err
	}
	configSegment, err := clusterSegmentConfigSpecFromGRPC(req.GetSegmentConfig(), grpcapi.AllPaths())
	if err != nil {
		return nil, err
	}

	estimate, err := ccs.gp.EstimateCreateCluster(
		ctx,
		greenplum.CreateClusterArgs{
			FolderExtID:      req.GetFolderId(),
			Name:             req.GetName(),
			Description:      req.GetDescription(),
			Labels:           req.GetLabels(),
			Environment:      env,
			NetworkID:        req.GetNetworkId(),
			SecurityGroupIDs: req.GetSecurityGroupIds(),
			HostGroupIDs:     req.GetHostGroupIds(),

			Config:        config,
			MasterConfig:  configMaster,
			SegmentConfig: configSegment,

			MasterHostCount:  int(req.GetMasterHostCount()),
			SegmentHostCount: int(req.GetSegmentHostCount()),
			SegmentInHost:    int(req.GetSegmentInHost()),

			UserName:     req.GetUserName(),
			UserPassword: req.GetUserPassword(),
		})
	if err != nil {
		return nil, err
	}

	return billingEstimateToGRPC(estimate), nil
}

func billingEstimateToGRPC(estimate consolemodels.BillingEstimate) *gpv1console.BillingEstimateResponse {
	metrics := make([]*gpv1console.BillingMetric, 0, len(estimate.Metrics))
	for _, metric := range estimate.Metrics {
		tags := &gpv1console.BillingMetric_BillingTags{
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
		}

		metrics = append(metrics, &gpv1console.BillingMetric{
			FolderId: metric.FolderID,
			Schema:   metric.Schema,
			Tags:     tags,
		})
	}
	return &gpv1console.BillingEstimateResponse{
		Metrics: metrics,
	}
}

func (ccs *ConsoleClusterService) GetCreateConfig(_ context.Context, _ *gpv1console.CreateClusterConfigRequest) (*gpv1console.JSONSchema, error) {
	greenplumDefaultConfig := `{
		"type": "object",
		"properties": {
		"maxConnections": {
			"type": "integer",
			"format": "int32"
		},
		"maxSlotWalKeepSize": {
			"type": "integer",
			"format": "int32"
		},
		"gpWorkfileLimitPerSegment": {
			"type": "integer",
			"format": "int32"
		},
		"gpWorkfileLimitPerQuery": {
			"type": "integer",
			"format": "int32"
		},
		"gpWorkfileLimitFilesPerQuery": {
			"type": "integer",
			"format": "int32"
		},
		"maxPreparedTransactions": {
			"type": "integer",
			"format": "int32"
		},
		"gpWorkfileCompression": {
			"type": "boolean",
			"default": false
		},
		"maxStatementMem": {
			"type": "integer",
			"format": "int32"
		},
		"logStatement": {
		    "type": "string",
			"enum": [
				"none",
				"ddl",
				"mod",
				"all"
			  ]
			}
		}}`
	jsonschema := fmt.Sprintf(`{"CreateClusterRequest": {
    "type": "object",
      "properties": {
         "folderId": {
            "type": "string"
         },
         "name": {
            "type": "string"
         },
         "description": {
            "type": "string"
         },
         "labels": {
            "type": "object",
            "additionalProperties": {
               "type": "string"
            }
         },
         "environment": {
            "$ref": "#/ClusterEnvironment"
         },
         "config": {
            "$ref": "#/GreenplumConfig"
         },
         "masterConfig": {
            "$ref": "#/MasterSubclusterConfigSpec"
         },
         "segmentConfig": {
            "$ref": "#/SegmentSubclusterConfigSpec"
         },
         "configSpec": {
            "$ref": "#/ConfigSpec"
         },
         "masterHostCount": {
            "type": "string",
            "format": "int64"
         },
         "segmentInHost": {
            "type": "string",
            "format": "int64"
         },
         "segmentHostCount": {
            "type": "string",
            "format": "int64"
         },
         "userName": {
            "type": "string"
         },
         "userPassword": {
            "type": "string"
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
      }
    },
	"ClusterEnvironment": {
		"type": "string",
		"enum": [
			"PRODUCTION",
			"PRESTABLE"
		]
	},
	"GreenplumConfig": {
		"type": "object",
		"properties": {
			"version": {
				"type": "string"
			},
         "backupWindowStart": {
            "$ref": "#/TimeOfDay"
         },
         "access": {
            "$ref": "#/Access"
         },
         "zoneId": {
            "type": "string"
         },
         "subnetId": {
            "type": "string"
         },
         "assignPublicIp": {
            "type": "boolean",
            "format": "boolean"
         }
      }
   },
   "TimeOfDay": {
      "type": "object",
      "properties": {
         "hours": {
            "type": "integer",
            "format": "int32"
         },
         "minutes": {
            "type": "integer",
            "format": "int32"
         },
         "seconds": {
            "type": "integer",
            "format": "int32"
         },
         "nanos": {
            "type": "integer",
            "format": "int32"
         }
      }
   },
   "Access": {
      "type": "object",
      "properties": {
         "dataLens": {
            "type": "boolean",
            "format": "boolean"
         },
         "webSql": {
            "type": "boolean",
            "format": "boolean"
         },
         "dataTransfer": {
            "type": "boolean",
            "format": "boolean"
         },
         "serverless": {
            "type": "boolean",
            "format": "boolean"
         }
      }
   },
   "MasterSubclusterConfigSpec": {
      "type": "object",
      "properties": {
         "resources": {
            "$ref": "#/Resources"
         }
      }
   },
   "SegmentSubclusterConfigSpec": {
	"type": "object",
	"properties": {
	   "resources": {
		  "$ref": "#/Resources"
	   }
	}
   },
   "Resources": {
      "type": "object",
      "properties": {
         "resourcePresetId": {
            "type": "string"
         },
         "diskSize": {
            "type": "string",
            "format": "int64"
         },
         "diskTypeId": {
            "type": "string"
         }
      }
   },
   "ConfigSpec": {
	"type": "object",
	"properties": {
	   "config": {
		  "$ref": "#/GreenplumClusterConfigSpecSchemaV1"
	   },
	   "pool": {
		  "$ref": "#/PoolConfigSpecSchemaV1"
	   }
	}
    },
	"PoolConfigSpecSchemaV1": {
		"type": "object",
		"properties": {
		   "clientIdleTimeout": {
			  "type": "integer",
			  "format": "int32"
		   },
		   "size": {
			  "type": "integer",
			  "format": "int32"
		   },
		   "mode": {
			  "type": "string",
			  "enum": [
				"SESSION",
				"TRANSACTION"
			  ]
		    }
		}
	},
   "GreenplumClusterConfigSpecSchemaV1": {
	   "type": "object",
	   "greenplumConfig_6_17": {
			"$ref": "#/Greenplum617Config"
		},
		"greenplumConfig_6_19": {
			"$ref": "#/Greenplum619Config"
		}
   },
   "Greenplum617Config": %s,
   "Greenplum619Config": %s}`, greenplumDefaultConfig, greenplumDefaultConfig)
	m := make(map[string]interface{})
	err := json.Unmarshal([]byte(jsonschema), &m)
	if err != nil {
		return nil, err
	}
	b, err := json.Marshal(m)
	if err != nil {
		return nil, err
	}
	return &gpv1console.JSONSchema{Schema: string(b)}, nil
}

func GetRecommendedConfigArgsFromGRPC(req *gpv1console.GetRecommendedConfigRequest) (*greenplum.RecommendedConfigArgs, error) {
	databaseSize := req.GetDatabaseSize()
	if databaseSize <= 0 {
		return nil, semerr.InvalidInputf("database_size should be > 0 , got %d", req.GetDatabaseSize())
	}
	if req.GetType() == "" {
		return nil, semerr.InvalidInputf("no flavor_type specified")
	}

	flavorType := req.GetType()
	if flavorType != "standard" && flavorType != "io-optimized" {
		return nil, semerr.InvalidInputf("unknown flavor_type expected: [standard, io-optimized] got %s", flavorType)
	}

	diskType := req.GetDiskTypeId()
	if diskType != "local-ssd" && diskType != "network-ssd-nonreplicated" {
		return nil, semerr.InvalidInputf("unknown disk_type expected: [local-ssd, network-ssd-nonreplicated] got %s", diskType)
	}

	folderID := req.GetFolderId()
	if folderID == "" {
		return nil, semerr.InvalidInputf("no folder_id specified")
	}
	return &greenplum.RecommendedConfigArgs{
		DataSize:          databaseSize,
		DiskTypeID:        diskType,
		UseDedicatedHosts: req.GetNeedDedicatedHosts(),
		FlavorType:        flavorType,
		FolderID:          folderID,
	}, nil
}

func (ccs *ConsoleClusterService) GetRecommendedConfig(ctx context.Context, req *gpv1console.GetRecommendedConfigRequest) (*gpv1console.GetRecommendedConfigResponse, error) {
	/*
		1. Deserialize user request from grpc into RecommendedConfigArgs
		2. Calculate recommended configuration in GetRecommendedConfig
		3. Serialize response to grpc
	*/

	args, err := GetRecommendedConfigArgsFromGRPC(req)
	if err != nil {
		return nil, err
	}
	responseData, err := ccs.gp.GetRecommendedConfig(ctx, args)
	if err != nil {
		return nil, err
	}

	return &gpv1console.GetRecommendedConfigResponse{
		MasterHosts:        responseData.MasterHosts,
		NeedDedicatedHosts: responseData.UseDedicatedHosts,
		SegmentHosts:       responseData.SegmentHosts,
		SegmentsPerHost:    responseData.SegmentsPerHost,
		SegmentConfig: &gpv1console.RecommendedConfig{
			Resources: &gpv1console.SubClusterResources{
				DiskTypeId:       responseData.SegmentConfig.Resource.DiskTypeExtID,
				DiskSize:         responseData.SegmentConfig.Resource.DiskSize,
				ResourcePresetId: responseData.SegmentConfig.Resource.ResourcePresetExtID,
				Type:             responseData.SegmentConfig.Resource.Type,
				Generation:       responseData.SegmentConfig.Resource.Generation,
			},
		},
		MasterConfig: &gpv1console.RecommendedConfig{
			Resources: &gpv1console.SubClusterResources{
				DiskTypeId:       responseData.MasterConfig.Resource.DiskTypeExtID,
				DiskSize:         responseData.MasterConfig.Resource.DiskSize,
				ResourcePresetId: responseData.MasterConfig.Resource.ResourcePresetExtID,
				Type:             responseData.MasterConfig.Resource.Type,
				Generation:       responseData.MasterConfig.Resource.Generation,
			},
		},
	}, nil
}
