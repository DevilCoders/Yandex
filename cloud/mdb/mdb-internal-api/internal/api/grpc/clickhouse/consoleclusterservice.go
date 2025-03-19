package clickhouse

import (
	"context"
	"encoding/json"

	chv1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/clickhouse/v1"
	chv1console "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/clickhouse/v1/console"
	grpcapi "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/clickhouse"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/clickhouse/chmodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/console"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
)

type ConsoleClusterService struct {
	chv1console.UnimplementedClusterServiceServer

	console       console.Console
	ch            clickhouse.ClickHouse
	saltEnvMapper grpcapi.SaltEnvMapper
}

var _ chv1console.ClusterServiceServer = &ConsoleClusterService{}

func NewConsoleClusterService(console console.Console, ch clickhouse.ClickHouse, saltEnvsCfg logic.SaltEnvsConfig) *ConsoleClusterService {
	return &ConsoleClusterService{
		console: console,
		ch:      ch,
		saltEnvMapper: grpcapi.NewSaltEnvMapper(
			int64(chv1.Cluster_PRODUCTION),
			int64(chv1.Cluster_PRESTABLE),
			int64(chv1.Cluster_ENVIRONMENT_UNSPECIFIED),
			saltEnvsCfg,
		),
	}
}

func (c *ConsoleClusterService) GetClustersConfig(ctx context.Context, req *chv1console.GetClustersConfigRequest) (*chv1console.ClustersConfig, error) {
	commonConfig, err := c.console.GetClusterConfig(ctx, req.GetFolderId(), clusters.TypeClickHouse)
	if err != nil {
		return nil, err
	}

	versionNames := c.ch.VersionIDs()
	allVersions := c.ch.Versions()
	defaultVersion := c.ch.DefaultVersion()

	return ClustersConfigToGRPC(chmodels.ExtendClustersConfig(commonConfig, versionNames, allVersions, defaultVersion)), nil
}

func (c *ConsoleClusterService) EstimateCreate(ctx context.Context, req *chv1.CreateClusterRequest) (*chv1console.BillingEstimate, error) {
	env, err := c.saltEnvMapper.FromGRPC(int64(req.GetEnvironment()))
	if err != nil {
		return nil, err
	}

	specs, err := UserSpecsFromGRPC(req.GetUserSpecs())
	if err != nil {
		return nil, err
	}

	hostSpecs, err := HostSpecsFromGRPC(req.GetHostSpecs())
	if err != nil {
		return nil, err
	}

	configSpec, err := ClusterConfigSpecFromGRPC(req.GetConfigSpec())
	if err != nil {
		return nil, err
	}

	window, err := MaintenanceWindowFromGRPC(req.GetMaintenanceWindow())
	if err != nil {
		return nil, err
	}

	createArgs := clickhouse.CreateMDBClusterArgs{
		FolderExtID:        req.GetFolderId(),
		Name:               req.GetName(),
		Environment:        env,
		ClusterSpec:        configSpec,
		DatabaseSpecs:      DatabaseSpecsFromGRPC(req.GetDatabaseSpecs()),
		UserSpecs:          specs,
		HostSpecs:          hostSpecs,
		NetworkID:          req.GetNetworkId(),
		Description:        req.GetDescription(),
		Labels:             req.GetLabels(),
		ShardName:          req.GetShardName(),
		MaintenanceWindow:  window,
		ServiceAccountID:   req.GetServiceAccountId(),
		SecurityGroupIDs:   req.GetSecurityGroupIds(),
		DeletionProtection: req.GetDeletionProtection(),
	}

	billingEstimate, err := c.ch.EstimateCreateMDBCluster(ctx, createArgs)
	if err != nil {
		return nil, err
	}

	return BillingEstimateToGRPC(billingEstimate), nil
}

func (c *ConsoleClusterService) GetClustersStats(ctx context.Context, req *chv1console.GetClustersStatsRequest) (*chv1console.ClustersStats, error) {
	folderStats, err := c.console.FolderStats(ctx, req.GetFolderId())
	if err != nil {
		return nil, err
	}

	return &chv1console.ClustersStats{
		ClustersCount: folderStats.Clusters[clusters.TypeClickHouse],
	}, nil
}

func (c *ConsoleClusterService) GetClickhouseConfigSchema(_ context.Context, _ *chv1console.GetClickhouseConfigSchemaRequest) (*chv1console.JSONSchema, error) {
	jsonschema := `
		{
			"ClickhouseConfig": {
				"type": "object",
				"properties": {
					"mergeTree":                   { "$ref": "#/MergeTree" },
					"compression":                 { "type": "array", "items": { "$ref": "#/Compression" } },
					"graphiteRollup":              { "$ref": "#/GraphiteRollup" },
					"kafka":                       { "$ref": "#/Kafka" },
					"kafkaTopics":                 { "type": "array", "items": { "$ref": "#/KafkaTopic" } },
					"rabbitmq":                    { "$ref": "#/Rabbitmq" },
					"logLevel":                    { "$ref": "#/LogLevel" },
					"maxConnections":              { "$ref": "#/google.protobuf.Int64Value" },
					"maxConcurrentQueries":        { "$ref": "#/google.protobuf.Int64Value" },
					"keepAliveTimeout":            { "$ref": "#/google.protobuf.Int64Value" },
					"uncompressedCacheSize":       { "$ref": "#/google.protobuf.Int64Value" },
					"markCacheSize":               { "$ref": "#/google.protobuf.Int64Value" },
					"maxTableSizeToDrop":          { "$ref": "#/google.protobuf.Int64Value" },
					"maxPartitionSizeToDrop":      { "$ref": "#/google.protobuf.Int64Value" },
					"timezone":                    { "type": "string" },
					"geobaseUri":                  { "type": "string" },
					"queryLogRetentionSize":       { "$ref": "#/google.protobuf.Int64Value" },
					"queryLogRetentionTime":       { "$ref": "#/google.protobuf.Int64Value" },
					"queryThreadLogEnabled":       { "$ref": "#/google.protobuf.BoolValue" },
					"queryThreadLogRetentionSize": { "$ref": "#/google.protobuf.Int64Value" },
					"queryThreadLogRetentionTime": { "$ref": "#/google.protobuf.Int64Value" },
					"partLogRetentionSize":        { "$ref": "#/google.protobuf.Int64Value" },
					"partLogRetentionTime":        { "$ref": "#/google.protobuf.Int64Value" },
					"metricLogEnabled":            { "$ref": "#/google.protobuf.BoolValue" },
					"metricLogRetentionSize":      { "$ref": "#/google.protobuf.Int64Value" },
					"metricLogRetentionTime":      { "$ref": "#/google.protobuf.Int64Value" },
					"traceLogEnabled":             { "$ref": "#/google.protobuf.BoolValue" },
					"traceLogRetentionSize":       { "$ref": "#/google.protobuf.Int64Value" },
					"traceLogRetentionTime":       { "$ref": "#/google.protobuf.Int64Value" },
					"textLogEnabled":              { "$ref": "#/google.protobuf.BoolValue" },
					"textLogRetentionSize":        { "$ref": "#/google.protobuf.Int64Value" },
					"textLogRetentionTime":        { "$ref": "#/google.protobuf.Int64Value" },
					"textLogLevel":                { "$ref": "#/LogLevel" },
					"backgroundPoolSize":          { "$ref": "#/google.protobuf.Int64Value" },
					"backgroundSchedulePoolSize":  { "$ref": "#/google.protobuf.Int64Value" }
				}
			},
			"MergeTree": {
				"type": "object",
				"properties": {
					"replicatedDeduplicationWindow":                  { "$ref": "#/google.protobuf.Int64Value" },
					"replicatedDeduplicationWindowSeconds":           { "$ref": "#/google.protobuf.Int64Value" },
					"partsToDelayInsert":                             { "$ref": "#/google.protobuf.Int64Value" },
					"partsToThrowInsert":                             { "$ref": "#/google.protobuf.Int64Value" },
					"maxReplicatedMergesInQueue":                     { "$ref": "#/google.protobuf.Int64Value" },
					"numberOfFreeEntriesInPoolToLowerMaxSizeOfMerge": { "$ref": "#/google.protobuf.Int64Value" },
					"maxBytesToMergeAtMinSpaceInPool":                { "$ref": "#/google.protobuf.Int64Value" },
					"maxBytesToMergeAtMaxSpaceInPool":                { "$ref": "#/google.protobuf.Int64Value" }
				}
			},
			"Compression": {
				"type": "object",
				"properties": {
					"method": {
						"type": "string",
						"enum": [
							"METHOD_UNSPECIFIED",
							"LZ4",
							"ZSTD"
						],
						"description": {
							"METHOD_UNSPECIFIED": null,
							"LZ4": "lz4",
							"ZSTD": "zstd"
						}
					},
					"minPartSize": {
						"type": "integer"
					},
					"minPartSizeRatio": {
						"type": "number"
					}
				}
			},
			"GraphiteRollup": {
				"type": "object",
				"properties": {
					"name": {
						"type": "string"
					},
					"patterns": {
						"type": "array",
						"items": {
							"type": "object",
							"properties": {
								"regexp": {
									"type": "string"
								},
								"function": {
									"type": "string"
								},
								"retention": {
									"type": "array",
									"items": {
										"type": "object",
										"properties": {
											"age": {
												"type": "integer"
											},
											"precision": {
												"type": "integer"
											}
										}
									}
								}
							}
						}
					}
				}
			},
			"Kafka": {
				"type": "object",
				"properties": {
					"security_protocol": {
						"type": "string",
						"enum": [
							"SECURITY_PROTOCOL_UNSPECIFIED",
							"SECURITY_PROTOCOL_PLAINTEXT",
							"SECURITY_PROTOCOL_SSL",
							"SECURITY_PROTOCOL_SASL_PLAINTEXT",
							"SECURITY_PROTOCOL_SASL_SSL"
						],
						"description": {
							"SECURITY_PROTOCOL_UNSPECIFIED": null,
							"SECURITY_PROTOCOL_PLAINTEXT": "PLAINTEXT",
							"SECURITY_PROTOCOL_SSL": "SSL",
							"SECURITY_PROTOCOL_SASL_PLAINTEXT": "SASL_PLAINTEXT",
							"SECURITY_PROTOCOL_SASL_SSL": "SASL_SSL"
						}
					},
					"sasl_mechanism": {
						"type": "string",
						"enum": [
							"SASL_MECHANISM_UNSPECIFIED",
							"SASL_MECHANISM_GSSAPI",
							"SASL_MECHANISM_PLAIN",
							"SASL_MECHANISM_SCRAM_SHA_256",
							"SASL_MECHANISM_SCRAM_SHA_512"
						],
						"description": {
							"SASL_MECHANISM_UNSPECIFIED": null,
							"SASL_MECHANISM_GSSAPI": "GSSAPI",
							"SASL_MECHANISM_PLAIN": "PLAIN",
							"SASL_MECHANISM_SCRAM_SHA_256": "SCRAM_SHA_256",
							"SASL_MECHANISM_SCRAM_SHA_512": "SCRAM_SHA_512"
						}
					},
					"sasl_username": {
						"type": "string"
					},
					"sasl_password": {
						"type": "string"
					}
				}
			},
			"KafkaTopic": {
				"type": "object",
				"properties": {
					"name": {
						"type": "string"
					},
					"settings": {
						"$ref": "#/Kafka"
					}
				}
			},
			"Rabbitmq": {
				"type": "object",
				"properties": {
					"username": {
						"type": "string"
					},
					"password": {
						"type": "string"
					}
				}
			},
			"LogLevel": {
				"type": "string",
				"enum": [
					"LOG_LEVEL_UNSPECIFIED",
					"TRACE",
					"DEBUG",
					"INFORMATION",
					"WARNING",
					"ERROR"
				],
				"description": {
					"LOG_LEVEL_UNSPECIFIED": null,
					"TRACE": "trace",
					"DEBUG": "debug",
					"INFORMATION": "information",
					"WARNING": "warning",
					"ERROR": "error"
				}
			},
			"google.protobuf.Int64Value": {
				"type": "object",
				"properties": {
					"value": {
					    "type": "integer"
					}
				}
			},
			"google.protobuf.BoolValue": {
				"type": "object",
				"properties": {
					"value": {
					    "type": "integer"
					}
				}
			}
		}
		`
	m := make(map[string]interface{})
	err := json.Unmarshal([]byte(jsonschema), &m)
	if err != nil {
		return nil, err
	}
	b, err := json.Marshal(m)
	if err != nil {
		return nil, err
	}
	return &chv1console.JSONSchema{Schema: string(b)}, nil
}

func (c *ConsoleClusterService) GetUserSettingsSchema(_ context.Context, _ *chv1console.GetUserSettingsSchemaRequest) (*chv1console.JSONSchema, error) {
	jsonschema := `
		{
			"UserSettings": {
				"type": "object",
				"properties": {
					"readonly":                                     { "$ref": "#/google.protobuf.Int64Value" },
					"allowDdl":                                     { "$ref": "#/google.protobuf.BoolValue" },

					"connectTimeout":                               { "$ref": "#/google.protobuf.Int64Value" },
					"receiveTimeout":                               { "$ref": "#/google.protobuf.Int64Value" },
					"sendTimeout":                                  { "$ref": "#/google.protobuf.Int64Value" },

					"insertQuorum":                                 { "$ref": "#/google.protobuf.Int64Value" },
					"insertQuorumTimeout":                          { "$ref": "#/google.protobuf.Int64Value" },
					"selectSequentialConsistency":                  { "$ref": "#/google.protobuf.BoolValue" },
					"replicationAlterPartitionsSync":               { "$ref": "#/google.protobuf.Int64Value" },

					"maxReplicaDelayForDistributedQueries":         { "$ref": "#/google.protobuf.Int64Value" },
					"fallbackToStaleReplicasForDistributedQueries": { "$ref": "#/google.protobuf.BoolValue" },
					"distributedProductMode":                       { "$ref": "#/DistributedProductMode" },
					"distributedAggregationMemoryEfficient":        { "$ref": "#/google.protobuf.BoolValue" },
					"distributedDdlTaskTimeout":                    { "$ref": "#/google.protobuf.Int64Value" },
					"skipUnavailableShards":                        { "$ref": "#/google.protobuf.BoolValue" },

					"compileExpressions":                           { "$ref": "#/google.protobuf.BoolValue" },
					"minCountToCompileExpression":                  { "$ref": "#/google.protobuf.Int64Value" },

					"maxBlockSize":                                 { "$ref": "#/google.protobuf.Int64Value" },
					"minInsertBlockSizeRows":                       { "$ref": "#/google.protobuf.Int64Value" },
					"minInsertBlockSizeBytes":                      { "$ref": "#/google.protobuf.Int64Value" },
					"maxInsertBlockSize":                           { "$ref": "#/google.protobuf.Int64Value" },
					"minBytesToUseDirectIo":                        { "$ref": "#/google.protobuf.Int64Value" },
					"useUncompressedCache":                         { "$ref": "#/google.protobuf.BoolValue" },
					"mergeTreeMaxRowsToUseCache":                   { "$ref": "#/google.protobuf.Int64Value" },
					"mergeTreeMaxBytesToUseCache":                  { "$ref": "#/google.protobuf.Int64Value" },
					"mergeTreeMinRowsForConcurrentRead":            { "$ref": "#/google.protobuf.Int64Value" },
					"mergeTreeMinBytesForConcurrentRead":           { "$ref": "#/google.protobuf.Int64Value" },
					"maxBytesBeforeExternalGroupBy":                { "$ref": "#/google.protobuf.Int64Value" },
					"maxBytesBeforeExternalSort":                   { "$ref": "#/google.protobuf.Int64Value" },
					"groupByTwoLevelThreshold":                     { "$ref": "#/google.protobuf.Int64Value" },
					"groupByTwoLevelThresholdBytes":                { "$ref": "#/google.protobuf.Int64Value" },

					"priority":                                     { "$ref": "#/google.protobuf.Int64Value" },
					"maxThreads":                                   { "$ref": "#/google.protobuf.Int64Value" },
					"maxMemoryUsage":                               { "$ref": "#/google.protobuf.Int64Value" },
					"maxMemoryUsageForUser":                        { "$ref": "#/google.protobuf.Int64Value" },
					"maxNetworkBandwidth":                          { "$ref": "#/google.protobuf.Int64Value" },
					"maxNetworkBandwidthForUser":                   { "$ref": "#/google.protobuf.Int64Value" },

					"forceIndexByDate":                             { "$ref": "#/google.protobuf.BoolValue" },
					"forcePrimaryKey":                              { "$ref": "#/google.protobuf.BoolValue" },
					"maxRowsToRead":                                { "$ref": "#/google.protobuf.Int64Value" },
					"maxBytesToRead":                               { "$ref": "#/google.protobuf.Int64Value" },
					"readOverflowMode":                             { "$ref": "#/OverflowMode" },
					"maxRowsToGroupBy":                             { "$ref": "#/google.protobuf.Int64Value" },
					"groupByOverflowMode":                          { "$ref": "#/GroupByOverflowMode" },
					"maxRowsToSort":                                { "$ref": "#/google.protobuf.Int64Value" },
					"maxBytesToSort":                               { "$ref": "#/google.protobuf.Int64Value" },
					"sortOverflowMode":                             { "$ref": "#/OverflowMode" },
					"maxResultRows":                                { "$ref": "#/google.protobuf.Int64Value" },
					"maxResultBytes":                               { "$ref": "#/google.protobuf.Int64Value" },
					"resultOverflowMode":                           { "$ref": "#/OverflowMode" },
					"maxRowsInDistinct":                            { "$ref": "#/google.protobuf.Int64Value" },
					"maxBytesInDistinct":                           { "$ref": "#/google.protobuf.Int64Value" },
					"distinctOverflowMode":                         { "$ref": "#/OverflowMode" },
					"maxRowsToTransfer":                            { "$ref": "#/google.protobuf.Int64Value" },
					"maxBytesToTransfer":                           { "$ref": "#/google.protobuf.Int64Value" },
					"transferOverflowMode":                         { "$ref": "#/OverflowMode" },
					"maxExecutionTime":                             { "$ref": "#/google.protobuf.Int64Value" },
					"timeoutOverflowMode":                          { "$ref": "#/OverflowMode" },
					"maxRowsInSet":                                 { "$ref": "#/google.protobuf.Int64Value" },
					"maxBytesInSet":                                { "$ref": "#/google.protobuf.Int64Value" },
					"setOverflowMode":                              { "$ref": "#/OverflowMode" },
					"maxRowsInJoin":                                { "$ref": "#/google.protobuf.Int64Value" },
					"maxBytesInJoin":                               { "$ref": "#/google.protobuf.Int64Value" },
					"joinOverflowMode":                             { "$ref": "#/OverflowMode" },
					"maxColumnsToRead":                             { "$ref": "#/google.protobuf.Int64Value" },
					"maxTemporaryColumns":                          { "$ref": "#/google.protobuf.Int64Value" },
					"maxTemporaryNonConstColumns":                  { "$ref": "#/google.protobuf.Int64Value" },
					"maxQuerySize":                                 { "$ref": "#/google.protobuf.Int64Value" },
					"maxAstDepth":                                  { "$ref": "#/google.protobuf.Int64Value" },
					"maxAstElements":                               { "$ref": "#/google.protobuf.Int64Value" },
					"maxExpandedAstElements":                       { "$ref": "#/google.protobuf.Int64Value" },
					"minExecutionSpeed":                            { "$ref": "#/google.protobuf.Int64Value" },
					"minExecutionSpeedBytes":                       { "$ref": "#/google.protobuf.Int64Value" },

					"inputFormatValuesInterpretExpressions":        { "$ref": "#/google.protobuf.BoolValue" },
					"inputFormatDefaultsForOmittedFields":          { "$ref": "#/google.protobuf.BoolValue" },
					"outputFormatJsonQuote_64bitIntegers":          { "$ref": "#/google.protobuf.BoolValue" },
					"outputFormatJsonQuoteDenormals":               { "$ref": "#/google.protobuf.BoolValue" },
					"lowCardinalityAllowInNativeFormat":            { "$ref": "#/google.protobuf.BoolValue" },
					"emptyResultForAggregationByEmptySet":          { "$ref": "#/google.protobuf.BoolValue" },

					"httpConnectionTimeout":                        { "$ref": "#/google.protobuf.Int64Value" },
					"httpReceiveTimeout":                           { "$ref": "#/google.protobuf.Int64Value" },
					"httpSendTimeout":                              { "$ref": "#/google.protobuf.Int64Value" },
					"enableHttpCompression":                        { "$ref": "#/google.protobuf.BoolValue" },
					"sendProgressInHttpHeaders":                    { "$ref": "#/google.protobuf.BoolValue" },
					"httpHeadersProgressInterval":                  { "$ref": "#/google.protobuf.Int64Value" },
					"addHttpCorsHeader":                            { "$ref": "#/google.protobuf.BoolValue" },

					"quotaMode":                                    { "$ref": "#/QuotaMode" },

					"countDistinctImplementation":                  { "$ref": "#/CountDistinctImplementation" },
					"joinedSubqueryRequiresAlias":                  { "$ref": "#/google.protobuf.BoolValue" },
					"joinUseNulls":                                 { "$ref": "#/google.protobuf.BoolValue" },
					"transformNullIn":                              { "$ref": "#/google.protobuf.BoolValue" }
				}
			},
			"UserQuota": {
				"type": "object",
				"properties": {
					"intervalDuration":                             { "$ref": "#/google.protobuf.Int64Value" },
					"queries":                                      { "$ref": "#/google.protobuf.Int64Value" },
					"errors":                                       { "$ref": "#/google.protobuf.Int64Value" },
					"resultRows":                                   { "$ref": "#/google.protobuf.Int64Value" },
					"readRows":                                     { "$ref": "#/google.protobuf.Int64Value" },
					"executionTime":                                { "$ref": "#/google.protobuf.Int64Value" }
				}
			},
			"OverflowMode": {
				"type": "string",
				"enum": [
					"OVERFLOW_MODE_UNSPECIFIED",
					"OVERFLOW_MODE_THROW",
					"OVERFLOW_MODE_BREAK"
				],
				"description": {
					"OVERFLOW_MODE_UNSPECIFIED": null,
					"OVERFLOW_MODE_THROW": "throw",
					"OVERFLOW_MODE_BREAK": "break"
				}
			},
			"GroupByOverflowMode": {
				"type": "string",
				"enum": [
					"GROUP_BY_OVERFLOW_MODE_UNSPECIFIED",
					"GROUP_BY_OVERFLOW_MODE_THROW",
					"GROUP_BY_OVERFLOW_MODE_BREAK",
					"GROUP_BY_OVERFLOW_MODE_ANY"
				],
				"description": {
					"GROUP_BY_OVERFLOW_MODE_UNSPECIFIED": null,
					"GROUP_BY_OVERFLOW_MODE_THROW": "throw",
					"GROUP_BY_OVERFLOW_MODE_BREAK": "break",
					"GROUP_BY_OVERFLOW_MODE_ANY": "any"
				}
			},
			"DistributedProductMode": {
				"type": "string",
				"enum": [
					"DISTRIBUTED_PRODUCT_MODE_UNSPECIFIED",
					"DISTRIBUTED_PRODUCT_MODE_DENY",
					"DISTRIBUTED_PRODUCT_MODE_LOCAL",
					"DISTRIBUTED_PRODUCT_MODE_GLOBAL",
					"DISTRIBUTED_PRODUCT_MODE_ALLOW"
				],
				"description": {
					"DISTRIBUTED_PRODUCT_MODE_UNSPECIFIED": null,
					"DISTRIBUTED_PRODUCT_MODE_DENY": "deny",
					"DISTRIBUTED_PRODUCT_MODE_LOCAL": "local",
					"DISTRIBUTED_PRODUCT_MODE_GLOBAL": "global",
					"DISTRIBUTED_PRODUCT_MODE_ALLOW": "allow"
				}
			},
			"QuotaMode": {
				"type": "string",
				"enum": [
					"QUOTA_MODE_UNSPECIFIED",
					"QUOTA_MODE_DEFAULT",
					"QUOTA_MODE_KEYED",
					"QUOTA_MODE_KEYED_BY_IP"
				],
				"description": {
					"QUOTA_MODE_UNSPECIFIED": null,
					"QUOTA_MODE_DEFAULT": "default",
					"QUOTA_MODE_KEYED": "keyed",
					"QUOTA_MODE_KEYED_BY_IP": "keyed_by_ip"
				}
			},
			"CountDistinctImplementation": {
				"type": "string",
				"enum": [
					"COUNT_DISTINCT_IMPLEMENTATION_UNSPECIFIED",
					"COUNT_DISTINCT_IMPLEMENTATION_UNIQ",
					"COUNT_DISTINCT_IMPLEMENTATION_UNIQ_COMBINED",
					"COUNT_DISTINCT_IMPLEMENTATION_UNIQ_COMBINED_64",
					"COUNT_DISTINCT_IMPLEMENTATION_UNIQ_HLL_12",
					"COUNT_DISTINCT_IMPLEMENTATION_UNIQ_EXACT"
				],
				"description": {
					"COUNT_DISTINCT_IMPLEMENTATION_UNSPECIFIED": null,
					"COUNT_DISTINCT_IMPLEMENTATION_UNIQ": "uniq",
					"COUNT_DISTINCT_IMPLEMENTATION_UNIQ_COMBINED": "uniqCombined",
					"COUNT_DISTINCT_IMPLEMENTATION_UNIQ_COMBINED_64": "uniqCombined64",
					"COUNT_DISTINCT_IMPLEMENTATION_UNIQ_HLL_12": "uniqHLL12",
					"COUNT_DISTINCT_IMPLEMENTATION_UNIQ_EXACT": "uniqExact"
				}
			},
			"google.protobuf.Int64Value": {
				"type": "object",
				"properties": {
					"value": {
					    "type": "integer"
					}
				}
			},
			"google.protobuf.BoolValue": {
				"type": "object",
				"properties": {
					"value": {
					    "type": "integer"
					}
				}
			}
		}
		`
	m := make(map[string]interface{})
	err := json.Unmarshal([]byte(jsonschema), &m)
	if err != nil {
		return nil, err
	}
	b, err := json.Marshal(m)
	if err != nil {
		return nil, err
	}
	return &chv1console.JSONSchema{Schema: string(b)}, nil
}
