package sqlserver

import (
	"context"
	"encoding/json"
	"fmt"
	"sort"
	"strings"

	ssv1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/sqlserver/v1"
	ssv1console "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/sqlserver/v1/console"
	"a.yandex-team.ru/cloud/mdb/internal/compute/compute"
	"a.yandex-team.ru/cloud/mdb/internal/reflectutil"
	grpcapi "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc/common"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/console"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/sqlserver"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/sqlserver/ssmodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	consolemodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/console"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/slices"
)

// ClusterService implements DB-specific gRPC methods
type ConsoleClusterService struct {
	ssv1console.UnimplementedClusterServiceServer

	Console       console.Console
	ss            sqlserver.SQLServer
	saltEnvMapper grpcapi.SaltEnvMapper
}

var _ ssv1console.ClusterServiceServer = &ConsoleClusterService{}

func NewConsoleClusterService(cnsl console.Console, ss sqlserver.SQLServer, saltEnvsCfg logic.SaltEnvsConfig) *ConsoleClusterService {
	return &ConsoleClusterService{
		Console: cnsl,
		ss:      ss,
		saltEnvMapper: grpcapi.NewSaltEnvMapper(
			int64(ssv1.Cluster_PRODUCTION),
			int64(ssv1.Cluster_PRESTABLE),
			int64(ssv1.Cluster_ENVIRONMENT_UNSPECIFIED),
			saltEnvsCfg)}
}

func versionsToGRPC(versions []clusters.Version, allowDevVersions bool, allow17_19 bool) (res []*ssv1console.SQLServerClustersConfig_VersionInfo) {
	for _, version := range versions {
		if !versionIsAllowed(version, allowDevVersions, allow17_19) {
			continue
		}

		res = append(res, &ssv1console.SQLServerClustersConfig_VersionInfo{
			Id:          version.ID,
			Name:        version.Name,
			Deprecated:  version.Deprecated,
			UpdatableTo: version.UpdatableTo,
		})
	}
	return res
}

func versionsNamesToGRPC(versions []clusters.Version, allowDevVersions bool, allow17_19 bool) (res []string) {
	for _, version := range versions {
		if !versionIsAllowed(version, allowDevVersions, allow17_19) {
			continue
		}

		res = append(res, version.ID)
	}
	return res
}

var years17_19prefixes = map[string]struct{}{
	"2017": {},
	"2019": {},
}

func versionIsAllowed(version clusters.Version, allowDevVersions bool, allow17_19 bool) bool {
	// check is dev versions enabled
	if !allowDevVersions && ssmodels.VersionEdition(version.ID) == ssmodels.EditionDeveloper {
		return false
	}

	// check if 2017 and 2019 years enabled
	if !allow17_19 {
		for key := range years17_19prefixes {
			if strings.HasPrefix(version.ID, key) {
				return false
			}
		}
	}

	return true
}

func resourcePresetsToGRPC(resourcePresets []consolemodels.ResourcePreset, hostGroupHostType map[string]compute.HostGroupHostType) ([]*ssv1console.SQLServerClustersConfig_ResourcePreset, error) {
	var lastRecord *consolemodels.ResourcePreset
	roleAggregated := common.AggregateResourcePresets(resourcePresets)
	roles := make([]hosts.Role, 0, len(roleAggregated))
	var minCoresInHostGroups int64
	var minMemoryInHostGroups int64

	for r := range roleAggregated {
		if r.String() != hosts.RoleWindowsWitnessNode.Info().Name {
			roles = append(roles, r)
		}
	}
	if len(roles) > 1 {
		return nil, xerrors.Errorf("Single host role expected, but multiple found: %v", roles)
	}
	if len(roles) < 1 {
		return nil, nil
	}

	aggregated := roleAggregated[roles[0]]
	resourcesGRPC := make([]*ssv1console.SQLServerClustersConfig_ResourcePreset, 0, len(aggregated))
	for _, presetAgg := range aggregated {
		zonesGRPC := make([]*ssv1console.SQLServerClustersConfig_ResourcePreset_Zone, 0, len(presetAgg))

		for _, zoneAgg := range presetAgg {
			diskTypesGRPC := make([]*ssv1console.SQLServerClustersConfig_ResourcePreset_Zone_DiskType, 0, len(zoneAgg))
			for _, record := range zoneAgg {
				diskTypeGRPC := &ssv1console.SQLServerClustersConfig_ResourcePreset_Zone_DiskType{
					DiskTypeId: record.DiskTypeExtID,
					MaxHosts:   record.MaxHosts,
					MinHosts:   record.MinHosts,
				}
				// Providing non empty disk size field
				_, _, minDisks, diskSize, _ := compute.GetMinHostGroupResources(hostGroupHostType, record.Zone)
				if record.DiskTypeExtID == "local-ssd" && len(hostGroupHostType) > 0 && diskSize > 0 {
					// Fill local-ssd sizes according host group
					var diskSizes []int64
					for i := diskSize; i <= minDisks*diskSize; i = i + diskSize {
						diskSizes = append(diskSizes, i)
					}
					record.DiskSizes = diskSizes
					diskTypeGRPC.DiskSize = &ssv1console.SQLServerClustersConfig_ResourcePreset_Zone_DiskType_DiskSizes_{
						DiskSizes: &ssv1console.SQLServerClustersConfig_ResourcePreset_Zone_DiskType_DiskSizes{
							Sizes: record.DiskSizes,
						},
					}
				} else if len(record.DiskSizes) == 0 {
					diskTypeGRPC.DiskSize = &ssv1console.SQLServerClustersConfig_ResourcePreset_Zone_DiskType_DiskSizeRange_{
						DiskSizeRange: &ssv1console.SQLServerClustersConfig_ResourcePreset_Zone_DiskType_DiskSizeRange{
							Min: record.DiskSizeRange.Lower,
							Max: record.DiskSizeRange.Upper,
						},
					}
				} else {
					diskTypeGRPC.DiskSize = &ssv1console.SQLServerClustersConfig_ResourcePreset_Zone_DiskType_DiskSizes_{
						DiskSizes: &ssv1console.SQLServerClustersConfig_ResourcePreset_Zone_DiskType_DiskSizes{
							Sizes: record.DiskSizes,
						},
					}
				}
				diskTypesGRPC = append(diskTypesGRPC, diskTypeGRPC)
				lastRecord = &record
			} /* foreach zoneAgg */
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
			zoneGRPC := ssv1console.SQLServerClustersConfig_ResourcePreset_Zone{
				ZoneId:    lastRecord.Zone,
				DiskTypes: diskTypesGRPC,
			}
			zonesGRPC = append(zonesGRPC, &zoneGRPC)
		} /* foreach presetAgg */
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
		// Using last record to fill preset info
		presetGRPC := &ssv1console.SQLServerClustersConfig_ResourcePreset{
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
	} /* foreach aggregated */
	sort.Slice(resourcesGRPC, func(i, j int) bool {
		ri := resourcePresets[i]
		rj := resourcePresets[j]
		if ri.FlavorType == rj.FlavorType {
			if ri.Generation == rj.Generation {
				if ri.CPULimit == rj.CPULimit {
					if ri.CPUFraction == rj.CPUFraction {
						if ri.MemoryLimit < rj.MemoryLimit {
							if ri.ExtID == rj.ExtID {
								return false
							}
							return ri.ExtID < rj.ExtID
						}
						return ri.MemoryLimit < rj.MemoryLimit
					}
					return ri.CPUFraction < rj.CPUFraction
				}
				return ri.CPULimit < rj.CPULimit
			}
			return ri.Generation < rj.Generation
		}
		return ri.FlavorType < rj.FlavorType
	})
	return resourcesGRPC, nil
}

func (ccs *ConsoleClusterService) Get(ctx context.Context, cfgReq *ssv1console.GetSQLServerClustersConfigRequest) (*ssv1console.SQLServerClustersConfig, error) {
	resourcePresets, err := ccs.Console.GetResourcePresetsByClusterType(ctx, clusters.TypeSQLServer, cfgReq.FolderId, true)
	if err != nil {
		return nil, err
	}
	hostGroupHostType, err := ccs.ss.GetHostGroupType(ctx, slices.DedupStrings(cfgReq.GetHostGroupIds()))
	if err != nil {
		return nil, err
	}
	defaultResources, err := ccs.Console.GetDefaultResourcesByClusterType(clusters.TypeSQLServer, hosts.RoleSQLServer)
	if err != nil {
		return nil, err
	}
	presetsGRPC, err := resourcePresetsToGRPC(resourcePresets, hostGroupHostType)
	if err != nil {
		return nil, err
	}
	featureFlags, err := ccs.Console.GetFeatureFlags(ctx, cfgReq.FolderId)
	if err != nil {
		return nil, err
	}

	allowDevVersions := featureFlags.Has(ssmodels.SQLServerAllowDevFeatureFlag)
	allow17_19 := featureFlags.Has(ssmodels.SQLServerAllow17_19FeatureFlag)

	return &ssv1console.SQLServerClustersConfig{
		ClusterName: &ssv1console.NameValidator{
			Regexp: models.DefaultClusterNamePattern,
			Min:    models.DefaultClusterNameMinLen,
			Max:    models.DefaultClusterNameMaxLen,
		},
		DbName: &ssv1console.NameValidator{
			Regexp:    models.DefaultDatabaseNamePattern,
			Min:       models.DefaultDatabaseNameMinLen,
			Max:       models.DefaultDatabaseNameMaxLen,
			Blacklist: ssmodels.DatabaseNameBlackList,
		},
		UserName: &ssv1console.NameValidator{
			Regexp:    models.DefaultUserNamePattern,
			Min:       models.DefaultUserNameMinLen,
			Max:       models.DefaultUserNameMaxLen,
			Blacklist: ssmodels.UserNameBlackList,
		},
		Password: &ssv1console.NameValidator{
			Regexp: ssmodels.SQLServerUserPasswordPattern,
			Min:    models.DefaultUserPasswordMinLen,
			Max:    models.DefaultUserPasswordMaxLen,
		},
		HostCountLimits: &ssv1console.SQLServerClustersConfig_HostCountLimits{
			MinHostCount: 1,
			MaxHostCount: 32,
		},
		ResourcePresets: presetsGRPC,
		DefaultResources: &ssv1console.SQLServerClustersConfig_DefaultResources{
			ResourcePresetId: defaultResources.ResourcePresetExtID,
			DiskTypeId:       defaultResources.DiskTypeExtID,
			DiskSize:         defaultResources.DiskSize,
			Generation:       defaultResources.Generation,
			GenerationName:   defaultResources.GenerationName,
		},
		Versions:          versionsNamesToGRPC(ssmodels.Versions, allowDevVersions, allow17_19),
		AvailableVersions: versionsToGRPC(ssmodels.Versions, allowDevVersions, allow17_19),
		DefaultVersion:    ssmodels.DefaultVersion,
	}, nil
}

func (ccs *ConsoleClusterService) EstimateCreate(ctx context.Context, req *ssv1.CreateClusterRequest) (*ssv1console.BillingEstimateResponse, error) {
	env, err := ccs.saltEnvMapper.FromGRPC(int64(req.GetEnvironment()))
	if err != nil {
		return nil, err
	}
	configSpec, err := clusterConfigSpecFromGRPC(req.GetConfigSpec(), grpcapi.AllPaths())
	if err != nil {
		return nil, err
	}

	estimate, err := ccs.ss.EstimateCreateCluster(
		ctx,
		sqlserver.CreateClusterArgs{
			NewClusterArgs: sqlserver.NewClusterArgs{
				FolderExtID:       req.GetFolderId(),
				Name:              req.GetName(),
				Description:       req.GetDescription(),
				Labels:            req.GetLabels(),
				Environment:       env,
				NetworkID:         req.GetNetworkId(),
				ClusterConfigSpec: configSpec,
				HostSpecs:         hostSpecsFromGRPC(req.GetHostSpecs()),
			},
			UserSpecs:     userSpecsFromGRPC(req.GetUserSpecs()),
			DatabaseSpecs: databaseSpecsFromGRPC(req.GetDatabaseSpecs()),
		})
	if err != nil {
		return nil, err
	}

	return billingEstimateToGRPC(estimate), nil
}

func billingEstimateToGRPC(estimate consolemodels.BillingEstimate) *ssv1console.BillingEstimateResponse {
	metrics := make([]*ssv1console.BillingMetric, 0, len(estimate.Metrics))
	for _, metric := range estimate.Metrics {
		tags := &ssv1console.BillingMetric_BillingTags{}
		reflectutil.CopyStructFields(&metric.Tags, tags, reflectutil.CopyStructFieldsConfig{
			IsValidField: reflectutil.AllFieldsAreValid,
			Convert:      reflectutil.CopyFieldsAsIs,
			PanicOnMissingField: func(missingInSrc, missingInDst bool) bool {
				return missingInSrc
			},
			CaseInsensitive: true,
		})
		metrics = append(metrics, &ssv1console.BillingMetric{
			FolderId: metric.FolderID,
			Schema:   metric.Schema,
			Tags:     tags,
		})
	}
	return &ssv1console.BillingEstimateResponse{
		Metrics: metrics,
	}
}

func restoreHintsToGRPC(hints ssmodels.RestoreHints, saltEnvMapper grpcapi.SaltEnvMapper) *ssv1console.RestoreHints {
	return &ssv1console.RestoreHints{
		Environment: ssv1.Cluster_Environment(saltEnvMapper.ToGRPC(hints.Environment)),
		Version:     hints.Version,
		NetworkId:   hints.NetworkID,
		Time:        grpcapi.TimeToGRPC(hints.Time),
		Resources: &ssv1console.RestoreResources{
			ResourcePresetId: hints.Resources.ResourcePresetID,
			DiskSize:         hints.Resources.DiskSize,
		},
	}
}

func (ccs *ConsoleClusterService) GetRestoreHints(ctx context.Context, req *ssv1console.GetRestoreHintsRequest) (*ssv1console.RestoreHints, error) {
	hints, err := ccs.ss.RestoreHints(ctx, req.BackupId)
	if err != nil {
		return nil, err
	}
	return restoreHintsToGRPC(hints, ccs.saltEnvMapper), nil
}

func SQLCollationsHintToGRPC(hints map[string]interface{}) *ssv1console.SQLCollationsHint {
	collations := make([]string, 0, len(hints))
	for key := range hints {
		collations = append(collations, key)
	}
	sort.Strings(collations)
	return &ssv1console.SQLCollationsHint{
		Sqlcollation: collations,
	}
}

func (ccs *ConsoleClusterService) GetSQLCollationsHint(ctx context.Context, req *ssv1console.GetSQLCollationsHintRequest) (*ssv1console.SQLCollationsHint, error) {
	hints, ok := sqlserver.AllowedCollations[req.SqlserverVersion]
	if !ok {
		return nil, xerrors.Errorf("No collations known for SQL Server version %q", req.SqlserverVersion)
	}
	return SQLCollationsHintToGRPC(hints), nil
}

func (ccs *ConsoleClusterService) GetCreateConfig(_ context.Context, _ *ssv1console.CreateClusterConfigRequest) (*ssv1console.JSONSchema, error) {
	defaultSQLServerConfig := `{
        "type": "object",
        "properties": {
            "maxDegreeOfParallelism": {
                "type": "integer",
                "format": "int16",
                "minimum": 0,
                "maximum": 32767
            },
            "costThresholdForParallelism": {
                "type": "integer",
                "format": "int16",
                "minimum": 5,
                "maximum": 32767
            },
            "auditLevel": {
                "type": "integer",
                "format": "int16",
                "minimum": 0,
                "maximum": 3
            },
            "fillFactorPercent": {
                "type": "integer",
                "format": "int16",
                "minimum": 0,
                "maximum": 100
            },
            "optimizeForAdHocWorkloads": {
                "type": "boolean"
            }
        }
    }`

	jsonschema := fmt.Sprintf(`{
    "ClusterCreate": {
        "type": "object",
        "properties": {
            "description": {
                "type": "string",
                "minLength": 0,
                "maxLength": 256,
                "description": "Description."
            },
            "name": {
                "type": "string"
            },
            "databaseSpecs": {
                "type": "array",
                "items": {
                    "$ref": "#/SQLServerDatabaseSpecSchemaV1"
                }
            },
            "labels": {
                "type": "object",
                "minLength": 0,
                "maxLength": 64,
                "description": "Labels."
            },
            "userSpecs": {
                "type": "array",
                "items": {
                    "$ref": "#/SQLServerUserSpecSchemaV1"
                }
            },
            "networkId": {
                "type": "string",
                "default": ""
            },
            "folderId": {
                "type": "string",
                "description": "Folder ID."
            },
            "hostSpecs": {
                "type": "array",
                "items": {
                    "$ref": "#/SQLServerHostSpecSchemaV1"
                }
            },
            "environment": {
                "type": "string",
                "enum": [
                    "PRESTABLE",
                    "PRODUCTION"
                ]
            },
            "configSpec": {
                "$ref": "#/SQLServerClusterConfigSpecSchemaV1"
            }
		},
        "required": [
            "configSpec",
            "databaseSpecs",
            "environment",
            "folderId",
            "hostSpecs",
            "name",
            "userSpecs"
        ]
	},
    "SQLServerClusterConfigSpecSchemaV1": {
        "type": "object",
        "properties": {
            "resources": {
                "$ref": "#/ResourcesSchemaV1"
            },
            "backupWindowStart": {
                "$ref": "#/TimeOfDay"
            },
            "sqlserverConfig_2016sp2dev": {
                "$ref": "#/SQLServer2016sp2devConfig"
            },
            "sqlserverConfig_2016sp2std": {
                "$ref": "#/SQLServer2016sp2stdConfig"
            },
            "sqlserverConfig_2016sp2ent": {
                "$ref": "#/SQLServer2016sp2entConfig"
            },
            "sqlserverConfig_2017dev": {
                "$ref": "#/SQLServer2017devConfig"
            },
            "sqlserverConfig_2017std": {
                "$ref": "#/SQLServer2017stdConfig"
            },
            "sqlserverConfig_2017ent": {
                "$ref": "#/SQLServer2017entConfig"
            },
            "sqlserverConfig_2019dev": {
                "$ref": "#/SQLServer2019devConfig"
            },
            "sqlserverConfig_2019std": {
                "$ref": "#/SQLServer2019stdConfig"
            },
            "sqlserverConfig_2019ent": {
                "$ref": "#/SQLServer2019entConfig"
            },
            "version": {
                "type": "string"
            }
        },
        "required": [
            "resources"
        ]
    },
    "SQLServer2016sp2devConfig": %s,
    "SQLServer2016sp2stdConfig": %s,
    "SQLServer2016sp2entConfig": %s,
    "SQLServer2017devConfig": %s,
    "SQLServer2017stdConfig": %s,
    "SQLServer2017entConfig": %s,
    "SQLServer2019devConfig": %s,
    "SQLServer2019stdConfig": %s,
    "SQLServer2019entConfig": %s,
    "TimeOfDay": {
        "type": "object",
        "properties": {
            "seconds": {
                "type": "integer",
                "format": "int32",
                "default": 0,
                "minimum": 0,
                "maximum": 59
            },
            "nanos": {
                "type": "integer",
                "format": "int32",
                "default": 0,
                "minimum": 0,
                "maximum": 999999999
            },
            "minutes": {
                "type": "integer",
                "format": "int32",
                "default": 0,
                "minimum": 0,
                "maximum": 59
            },
            "hours": {
                "type": "integer",
                "format": "int32",
                "default": 0,
                "minimum": 0,
                "maximum": 23
            }
        }
    },
    "ResourcesSchemaV1": {
        "type": "object",
        "properties": {
            "diskSize": {
                "type": "integer",
                "format": "int32"
            },
            "resourcePresetId": {
                "type": "string"
            },
            "diskTypeId": {
                "type": "string"
            }
        },
        "required": [
            "diskSize",
            "diskTypeId",
            "resourcePresetId"
        ]
    },
    "SQLServerHostSpecSchemaV1": {
        "type": "object",
        "properties": {
            "zoneId": {
                "type": "string",
                "description": "ID of availability zone."
            },
            "assignPublicIp": {
                "type": "boolean",
                "default": false
            },
            "subnetId": {
                "type": "string"
            },
            "configSpec": {
                "$ref": "#/SQLServerHostConfigUpdateSpecSchemaV1"
            }
        },
        "required": [
            "zoneId"
        ]
    },
    "SQLServerUserSpecSchemaV1": {
        "type": "object",
        "properties": {
            "name": {
                "type": "string"
            },
            "permissions": {
                "type": "array",
                "items": {
                    "$ref": "#/SQLServerPermissionSchemaV1"
                }
            },
            "password": {
                "type": "string"
            }
        },
        "required": [
            "name",
            "password"
        ]
    },
    "SQLServerPermissionSchemaV1": {
        "type": "object",
        "properties": {
            "databaseName": {
                "type": "string"
            },
            "roles": {
                "type": "array",
                "items": {
                    "$ref": "#/SQLServerDatabaseRoleV1"
                }
            }
        },
        "required": [
            "databaseName"
        ]
    },
    "SQLServerDatabaseRoleV1": {
        "type": "string",
		"enum": [
			"db_owner",
			"db_securityadmin",
			"db_accessadmin",
			"db_backupoperator",
			"db_ddladmin",
			"db_datawriter",
			"db_datareader",
			"db_denydatawriter",
			"db_denydatareader"
		]
    },
    "SQLServerDatabaseSpecSchemaV1": {
        "type": "object",
        "properties": {
            "name": {
                "type": "string"
			}
        },
		"required": [
            "name"
        ]
    }
 }`,
		// 2016sp2
		defaultSQLServerConfig,
		defaultSQLServerConfig,
		defaultSQLServerConfig,
		// 2017
		defaultSQLServerConfig,
		defaultSQLServerConfig,
		defaultSQLServerConfig,
		// 2019
		defaultSQLServerConfig,
		defaultSQLServerConfig,
		defaultSQLServerConfig)

	m := make(map[string]interface{})
	err := json.Unmarshal([]byte(jsonschema), &m)
	if err != nil {
		return nil, err
	}
	b, err := json.Marshal(m)
	if err != nil {
		return nil, err
	}
	return &ssv1console.JSONSchema{Schema: string(b)}, nil
}
