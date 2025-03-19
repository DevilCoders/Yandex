package provider

import (
	"context"
	"fmt"
	"time"

	cheventspub "a.yandex-team.ru/cloud/bitbucket/public-api/yandex/cloud/events/mdb/clickhouse"
	networkProvider "a.yandex-team.ru/cloud/mdb/internal/network"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/secret"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/clickhouse/chmodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/clickhouse/provider/internal/chpillars"
	clusterslogic "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/sessions"
	taskslogic "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/tasks"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/operations"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/tasks"
	"a.yandex-team.ru/library/go/core/xerrors"
)

const (
	clickHouseService = "managed-clickhouse"
)

func (ch ClickHouse) getClusterImpl(ctx context.Context, reader clusterslogic.Reader, extCluster clusters.ClusterExtended) (chmodels.Cluster, string, *chpillars.ClusterCH, *chpillars.SubClusterCH, error) {
	result := chmodels.Cluster{ClusterExtended: extCluster}

	chCluster, err := ch.chCluster(ctx, reader, extCluster.ClusterID)
	if err != nil {
		return chmodels.Cluster{}, "", nil, nil, err
	}

	result.CloudType = string(chCluster.Pillar.Data.CloudType)
	result.RegionID = chCluster.Pillar.Data.RegionID

	chSubCluster, err := ch.chSubCluster(ctx, reader, extCluster.ClusterID)
	if err != nil {
		return chmodels.Cluster{}, "", nil, nil, err
	}

	result.Version = chmodels.CutVersionToMajor(chSubCluster.Pillar.Data.ClickHouse.Version)

	result.ServiceAccountID = chSubCluster.Pillar.ServiceAccountID()

	result.MaintenanceInfo, err = reader.MaintenanceInfoByClusterID(ctx, extCluster.ClusterID)
	if err != nil {
		return chmodels.Cluster{}, "", nil, nil, err
	}

	return result, chSubCluster.SubClusterID, chCluster.Pillar, chSubCluster.Pillar, nil
}

func (ch *ClickHouse) MDBCluster(ctx context.Context, cid string) (chmodels.MDBCluster, error) {
	result := chmodels.MDBCluster{}
	var err error

	if err = ch.operator.ReadCluster(ctx, cid,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader) error {
			extCluster, err := reader.ClusterExtendedByClusterID(ctx, cid, clusters.TypeClickHouse, models.VisibilityVisible, session)
			if err != nil {
				return err
			}

			chCluster, _, _, userPillar, err := ch.getClusterImpl(ctx, reader, extCluster)
			if err != nil {
				return err
			}
			result.Cluster = chCluster

			defaultPillar := chpillars.NewSubClusterCHWithVersion(userPillar.Data.ClickHouse.Version)
			err = reader.ClusterTypePillar(ctx, clusters.TypeClickHouse, defaultPillar)
			if err != nil {
				return err
			}

			defaultConfig, err := defaultPillar.Data.ClickHouse.Config.ToModel()
			if err != nil {
				return err
			}
			userConfig, err := userPillar.Data.ClickHouse.Config.ToModel()
			if err != nil {
				return err
			}
			effectiveConfig, err := chmodels.MergeClickhouseConfigs(defaultConfig, userConfig)
			if err != nil {
				return err
			}

			result.Config = userPillar.ToClusterConfig()
			result.Config.ClickhouseConfigSet = chmodels.ClickhouseConfigSet{
				Default:   defaultConfig,
				User:      userConfig,
				Effective: effectiveConfig,
			}
			result.Config.ClickhouseResources, result.Config.ZooKeeperResources, err = ch.getMDBClusterResources(ctx, reader, cid)
			if err != nil {
				return err
			}

			if userPillar.Data.CloudStorage.Settings != nil {
				if userPillar.Data.CloudStorage.Settings.DataCacheEnabled != nil {
					result.Config.CloudStorageConfig.DataCacheEnabled = optional.NewBool(*userPillar.Data.CloudStorage.Settings.DataCacheEnabled)
				}

				if userPillar.Data.CloudStorage.Settings.DataCacheMaxSize != nil {
					result.Config.CloudStorageConfig.DataCacheMaxSize = optional.NewInt64(*userPillar.Data.CloudStorage.Settings.DataCacheMaxSize)
				}

				if userPillar.Data.CloudStorage.Settings.MoveFactor != nil {
					result.Config.CloudStorageConfig.MoveFactor = optional.NewFloat64(*userPillar.Data.CloudStorage.Settings.MoveFactor)
				}
			}

			return nil
		},
	); err != nil {
		return chmodels.MDBCluster{}, err
	}

	return result, err
}

func (ch *ClickHouse) DataCloudCluster(ctx context.Context, cid string, sensitive bool) (chmodels.DataCloudCluster, error) {
	result := chmodels.DataCloudCluster{}

	var (
		subcid string
		err    error
	)

	if err = ch.operator.ReadCluster(ctx, cid,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader) error {
			extCluster, err := reader.ClusterExtendedByClusterID(ctx, cid, clusters.TypeClickHouse, models.VisibilityVisible, session)
			if err != nil {
				return err
			}

			chCluster, chSubClusterID, clusterCHPillar, subClusterCH, err := ch.getClusterImpl(ctx, reader, extCluster)
			if err != nil {
				return err
			}
			result.Cluster = chCluster
			result.NetworkID = chCluster.NetworkID

			subcid = chSubClusterID

			result.Resources, err = ch.getDataCloudClusterResources(ctx, reader, chCluster.ClusterID)
			if err != nil {
				return err
			}
			if subClusterCH.Data.Access != nil {
				result.Access = clusters.Access{}
				if subClusterCH.Data.Access.DataLens != nil {
					result.Access.DataLens = optional.NewBool(*subClusterCH.Data.Access.DataLens)
				}
				if subClusterCH.Data.Access.DataTransfer != nil {
					result.Access.DataTransfer = optional.NewBool(*subClusterCH.Data.Access.DataTransfer)
				}
				result.Access.Ipv4CidrBlocks = subClusterCH.Data.Access.Ipv4CidrBlocks

				result.Access.Ipv6CidrBlocks = subClusterCH.Data.Access.Ipv6CidrBlocks
			}

			if encPillar := clusterCHPillar.Data.Encryption; encPillar != nil {
				result.Encryption = clusters.Encryption{}
				if encPillar.Enabled != nil {
					result.Encryption.Enabled = optional.NewBool(*encPillar.Enabled)
					result.Encryption.Key = encPillar.Key
				}
			}
			return nil
		},
	); err != nil {
		return chmodels.DataCloudCluster{}, err
	}

	password := secret.String{}
	if sensitive {
		password, err = ch.pillarSecrets.GetSubClusterPillarSecret(ctx, cid, subcid, chmodels.AdminPasswordPath)
		if err != nil {
			if !semerr.IsAuthorization(err) {
				return chmodels.DataCloudCluster{}, err
			}
		}
	}

	result.ConnectionInfo, err = ch.getConnectionInfo(cid, password)

	return result, err
}

func (ch *ClickHouse) MDBClusters(ctx context.Context, folderExtID string, pageSize int64, pageToken clusters.ClusterPageToken) ([]chmodels.MDBCluster, error) {
	var result []chmodels.MDBCluster

	if err := ch.operator.ReadOnFolder(ctx, folderExtID,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader) error {
			var err error

			extClusters, err := reader.ClustersExtended(ctx, models.ListClusterArgs{
				ClusterType:   clusters.TypeClickHouse,
				FolderID:      session.FolderCoords.FolderID,
				Limit:         optional.NewInt64(pageSize),
				PageTokenName: optional.NewString(pageToken.LastClusterName),
				Visibility:    models.VisibilityVisible,
			}, session)
			if err != nil {
				return err
			}

			for _, extCluster := range extClusters {
				chCluster, _, _, userPillar, err := ch.getClusterImpl(ctx, reader, extCluster)
				if err != nil {
					return err
				}

				mdbCluster := chmodels.MDBCluster{Cluster: chCluster}

				defaultPillar := chpillars.NewSubClusterCHWithVersion(userPillar.Data.ClickHouse.Version)
				err = reader.ClusterTypePillar(ctx, clusters.TypeClickHouse, defaultPillar)
				if err != nil {
					return err
				}

				defaultConfig, err := defaultPillar.Data.ClickHouse.Config.ToModel()
				if err != nil {
					return err
				}
				userConfig, err := userPillar.Data.ClickHouse.Config.ToModel()
				if err != nil {
					return err
				}
				effectiveConfig, err := chmodels.MergeClickhouseConfigs(defaultConfig, userConfig)
				if err != nil {
					return err
				}

				mdbCluster.Config = userPillar.ToClusterConfig()
				mdbCluster.Config.ClickhouseConfigSet = chmodels.ClickhouseConfigSet{
					Default:   defaultConfig,
					User:      userConfig,
					Effective: effectiveConfig,
				}
				mdbCluster.Config.ClickhouseResources, mdbCluster.Config.ZooKeeperResources, err = ch.getMDBClusterResources(ctx, reader, mdbCluster.ClusterID)
				if err != nil {
					return err
				}

				if userPillar.Data.CloudStorage.Settings != nil {
					if userPillar.Data.CloudStorage.Settings.DataCacheEnabled != nil {
						mdbCluster.Config.CloudStorageConfig.DataCacheEnabled = optional.NewBool(*userPillar.Data.CloudStorage.Settings.DataCacheEnabled)
					}

					if userPillar.Data.CloudStorage.Settings.DataCacheMaxSize != nil {
						mdbCluster.Config.CloudStorageConfig.DataCacheMaxSize = optional.NewInt64(*userPillar.Data.CloudStorage.Settings.DataCacheMaxSize)
					}

					if userPillar.Data.CloudStorage.Settings.MoveFactor != nil {
						mdbCluster.Config.CloudStorageConfig.MoveFactor = optional.NewFloat64(*userPillar.Data.CloudStorage.Settings.MoveFactor)
					}
				}

				result = append(result, mdbCluster)
			}
			return nil
		},
	); err != nil {
		return nil, err
	}

	return result, nil
}

func (ch *ClickHouse) DataCloudClusters(ctx context.Context, folderExtID string, pageSize int64, pageToken clusters.ClusterPageToken) ([]chmodels.DataCloudCluster, error) {
	var result []chmodels.DataCloudCluster

	if err := ch.operator.ReadOnFolder(ctx, folderExtID,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader) error {
			var err error
			extClusters, err := reader.ClustersExtended(ctx, models.ListClusterArgs{
				ClusterType:   clusters.TypeClickHouse,
				FolderID:      session.FolderCoords.FolderID,
				Limit:         optional.NewInt64(pageSize),
				PageTokenName: optional.NewString(pageToken.LastClusterName),
				Visibility:    models.VisibilityVisible,
			}, session)
			if err != nil {
				return err
			}

			for _, extCluster := range extClusters {
				chCluster, _, _, _, err := ch.getClusterImpl(ctx, reader, extCluster)
				if err != nil {
					return err
				}

				dcCluster := chmodels.DataCloudCluster{Cluster: chCluster}
				dcCluster.NetworkID = chCluster.NetworkID
				dcCluster.Resources, err = ch.getDataCloudClusterResources(ctx, reader, chCluster.ClusterID)
				if err != nil {
					return err
				}
				dcCluster.ConnectionInfo, err = ch.getConnectionInfo(dcCluster.ClusterID, secret.String{})
				if err != nil {
					return err
				}

				result = append(result, dcCluster)
			}
			return nil
		},
	); err != nil {
		return nil, err
	}

	return result, nil
}

func (ch *ClickHouse) getConnectionInfo(cid string, password secret.String) (chmodels.ConnectionInfo, error) {
	domain, err := ch.cfg.GetDomain(ch.cfg.EnvironmentVType)
	if err != nil {
		return chmodels.ConnectionInfo{}, err
	}

	hostname := fmt.Sprintf("rw.%s.%s", cid, domain)

	res := chmodels.ConnectionInfo{
		Hostname: hostname,
		// TODO change admin user name after branding
		Username: "admin",
		Password: password,

		HTTPSPort:     8443,
		TCPPortSecure: 9440,
	}

	res.NativeProtocol = fmt.Sprintf("%s:%d", hostname, res.TCPPortSecure)
	if password.Unmask() != "" {
		res.HTTPSURI = fmt.Sprintf("https://%s:%s@%s:%d", res.Username, password.Unmask(), res.Hostname, res.HTTPSPort)
		res.JDBCURI = fmt.Sprintf("jdbc:clickhouse://%s:%d/default?ssl=true&user=%s&password=%s", res.Hostname, res.HTTPSPort, res.Username, res.Password.Unmask())
		res.ODBCURI = fmt.Sprintf("https://%s:%s@%s:%d", res.Username, res.Password.Unmask(), res.Hostname, res.HTTPSPort)
	} else {
		res.HTTPSURI = fmt.Sprintf("https://%s:%d", res.Hostname, res.HTTPSPort)
		res.JDBCURI = fmt.Sprintf("jdbc:clickhouse://%s:%d/default?ssl=true", res.Hostname, res.HTTPSPort)
		res.ODBCURI = fmt.Sprintf("https://%s:%d", res.Hostname, res.HTTPSPort)
	}

	return res, nil
}

func (ch *ClickHouse) getMDBClusterResources(ctx context.Context, reader clusterslogic.Reader, cid string) (models.ClusterResources, models.ClusterResources, error) {
	currentHosts, err := clusterslogic.ListAllHosts(ctx, reader, cid)
	if err != nil {
		return models.ClusterResources{}, models.ClusterResources{}, err
	}

	if len(currentHosts) == 0 {
		return models.ClusterResources{}, models.ClusterResources{}, xerrors.Errorf("clickhouse cluster without ch hosts %q", cid)
	}

	var (
		chResources models.ClusterResources
		zkResources models.ClusterResources
		chFound     bool
		zkFound     bool
	)

	for _, host := range currentHosts {
		switch host.Roles[0] {
		case hosts.RoleClickHouse:
			if !chFound {
				chResources = models.ClusterResources{
					ResourcePresetExtID: host.ResourcePresetExtID,
					DiskSize:            host.SpaceLimit,
					DiskTypeExtID:       host.DiskTypeExtID,
				}
				chFound = true
			}
		case hosts.RoleZooKeeper:
			if !zkFound {
				zkResources = models.ClusterResources{
					ResourcePresetExtID: host.ResourcePresetExtID,
					DiskSize:            host.SpaceLimit,
					DiskTypeExtID:       host.DiskTypeExtID,
				}
				zkFound = true
			}
		default:
			return models.ClusterResources{}, models.ClusterResources{}, xerrors.Errorf("clickhouse cluster host has unknown role %v", host.Roles[0])
		}

		if chFound && zkFound {
			break
		}
	}

	return chResources, zkResources, nil
}

func (ch *ClickHouse) getDataCloudClusterResources(ctx context.Context, reader clusterslogic.Reader, cid string) (chmodels.DataCloudResources, error) {
	currentHosts, err := clusterslogic.ListAllHosts(ctx, reader, cid)
	if err != nil {
		return chmodels.DataCloudResources{}, err
	}

	if len(currentHosts) == 0 {
		return chmodels.DataCloudResources{}, xerrors.Errorf("clickhouse cluster without ch hosts %q", cid)
	}

	shardMap := map[string]int64{}
	for _, h := range currentHosts {
		shardMap[h.ShardID.String] += 1
	}
	chExample := &currentHosts[0]
	res := chmodels.DataCloudResources{
		ResourcePresetID: optional.NewString(chExample.ResourcePresetExtID),
		DiskSize:         optional.NewInt64(chExample.SpaceLimit),
		ReplicaCount:     optional.NewInt64(shardMap[chExample.ShardID.String]),
		ShardCount:       optional.NewInt64(int64(len(shardMap))),
	}

	return res, nil
}

func (ch *ClickHouse) DeleteCluster(ctx context.Context, cid string) (operations.Operation, error) {
	return ch.operator.Delete(ctx, cid, clusters.TypeClickHouse,
		func(ctx context.Context, session sessions.Session, cluster clusterslogic.Cluster, reader clusterslogic.Reader) (operations.Operation, error) {
			var pillar chpillars.ClusterCH
			err := cluster.Pillar(&pillar)
			if err != nil {
				return operations.Operation{}, err
			}

			s3Bucket := optional.NewString(ch.cfg.S3BucketName(cluster.ClusterID))
			if len(pillar.Data.S3Bucket) > 0 {
				if pillar.Data.S3Bucket == "dbaas" {
					return operations.Operation{}, xerrors.Errorf("deleting %q bucket", pillar.Data.S3Bucket)
				}
				s3Bucket.Set(pillar.Data.S3Bucket)
			}

			subCluster, err := ch.chSubCluster(ctx, reader, cid)
			if err != nil {
				return operations.Operation{}, err
			}

			s3Buckets := taskslogic.DeleteClusterS3Buckets{
				"backup": s3Bucket.String,
			}
			if subCluster.Pillar.CloudStorageEnabled() {
				s3Buckets["cloud_storage"] = subCluster.Pillar.Data.CloudStorage.S3.Bucket
			}

			op, err := ch.tasks.DeleteCluster(
				ctx,
				session,
				cluster.ClusterID,
				cluster.Revision,
				taskslogic.DeleteClusterTaskTypes{
					Delete:   chmodels.TaskTypeClusterDelete,
					Metadata: chmodels.TaskTypeClusterDeleteMetadata,
					Purge:    chmodels.TaskTypeClusterPurge,
				},
				chmodels.OperationTypeClusterDelete,
				s3Buckets,
			)
			if err != nil {
				return operations.Operation{}, err
			}

			if err := ch.search.StoreDocDelete(ctx, clickHouseService, session.FolderCoords.FolderExtID, session.FolderCoords.CloudExtID, op); err != nil {
				return operations.Operation{}, err
			}

			event := &cheventspub.DeleteCluster{
				Authentication:  ch.events.NewAuthentication(session.Subject),
				Authorization:   ch.events.NewAuthorization(session.Subject),
				RequestMetadata: ch.events.NewRequestMetadata(ctx),
				EventStatus:     cheventspub.DeleteCluster_STARTED,
				Details: &cheventspub.DeleteCluster_EventDetails{
					ClusterId: cid,
				},
			}
			em, err := ch.events.NewEventMetadata(event, op, session.FolderCoords.CloudExtID, session.FolderCoords.FolderExtID)
			if err != nil {
				return operations.Operation{}, err
			}
			event.EventMetadata = em

			if err = ch.events.Store(ctx, event, op); err != nil {
				return operations.Operation{}, xerrors.Errorf("saving event for operation %+v. %w", op, err)
			}

			return op, nil
		},
	)
}

func (ch *ClickHouse) AddZookeeper(ctx context.Context, cid string, resources models.ClusterResourcesSpec, hostSpecs []chmodels.HostSpec) (operations.Operation, error) {
	return ch.operator.ModifyOnNotStoppedCluster(ctx, cid, clusters.TypeClickHouse,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader, modifier clusterslogic.Modifier, cluster clusterslogic.Cluster) (operations.Operation, error) {
			currentHosts, err := clusterslogic.ListAllHosts(ctx, reader, cid)
			if err != nil {
				return operations.Operation{}, err
			}

			chCluster, err := ch.chCluster(ctx, reader, cid)
			if err != nil {
				return operations.Operation{}, err
			}

			chSubCluster, err := ch.chSubCluster(ctx, reader, cid)
			if err != nil {
				return operations.Operation{}, err
			}

			chHosts := clusterslogic.GetHostsWithRole(currentHosts, hosts.RoleClickHouse)

			if len(chHosts) != len(currentHosts) || len(chSubCluster.Pillar.Data.ClickHouse.ZKHosts) > 0 || len(chSubCluster.Pillar.Data.ClickHouse.KeeperHosts) > 0 {
				return operations.Operation{}, semerr.FailedPrecondition("ZooKeeper has been already configured for this cluster")
			}

			resolvedCHHostGroups, _, err := modifier.ValidateResources(ctx, session, clusters.TypeClickHouse,
				buildClickHouseHostGroups(chHosts, "", clusterslogic.ZoneHostsList{}, clusterslogic.ZoneHostsList{})...)
			if err != nil {
				return operations.Operation{}, err
			}

			_, subnets, err := ch.compute.NetworkAndSubnets(ctx, cluster.NetworkID)
			if err != nil {
				return operations.Operation{}, err
			}

			zkResources, hostSpecs, err := ch.buildZookeeperResources(
				ctx,
				session,
				modifier,
				resolvedCHHostGroups.MustGroupsByHostRole(hosts.RoleClickHouse),
				chmodels.ClusterHosts{ZooKeeperNodes: hostSpecs},
				currentHosts,
				resources,
				optional.String{},
				subnets,
			)
			if err != nil {
				return operations.Operation{}, err
			}

			resolvedZKHostGroups, _, err := modifier.ValidateResources(ctx, session, clusters.TypeClickHouse, clusterslogic.HostGroup{
				Role:                       hosts.RoleZooKeeper,
				CurrentResourcePresetExtID: optional.NewString(zkResources.ResourcePresetExtID),
				DiskTypeExtID:              zkResources.DiskTypeExtID,
				CurrentDiskSize:            optional.NewInt64(zkResources.DiskSize),
				HostsToAdd:                 chmodels.ToZoneHosts(hostSpecs),
			})
			if err != nil {
				return operations.Operation{}, err
			}

			if err := ch.validateZookeeperCores(ctx, session, modifier,
				clusterslogic.NewResolvedHostGroups(append(resolvedCHHostGroups.MustGroupsByHostRole(hosts.RoleClickHouse), resolvedZKHostGroups.Single())), ""); err != nil {
				return operations.Operation{}, err
			}

			if err := ch.createZKSubCluster(ctx, session, modifier, chCluster, hostSpecs, resolvedZKHostGroups.Single(), subnets); err != nil {
				return operations.Operation{}, err
			}

			op, err := ch.tasks.CreateTask(
				ctx,
				session,
				tasks.CreateTaskArgs{
					ClusterID:     cid,
					FolderID:      session.FolderCoords.FolderID,
					Auth:          session.Subject,
					TaskType:      chmodels.TaskTypeAddZookeeper,
					OperationType: chmodels.OperationTypeAddZookeeper,
					Revision:      cluster.Revision,
					Metadata:      chmodels.MetadataAddZookeeper{},
				})
			if err != nil {
				return operations.Operation{}, xerrors.Errorf("failed to create operation: %w", err)
			}

			event := &cheventspub.AddClusterZookeeper{
				Authentication:  ch.events.NewAuthentication(session.Subject),
				Authorization:   ch.events.NewAuthorization(session.Subject),
				RequestMetadata: ch.events.NewRequestMetadata(ctx),
				EventStatus:     cheventspub.AddClusterZookeeper_STARTED,
				Details: &cheventspub.AddClusterZookeeper_EventDetails{
					ClusterId: cid,
				},
			}
			em, err := ch.events.NewEventMetadata(event, op, session.FolderCoords.CloudExtID, session.FolderCoords.FolderExtID)
			if err != nil {
				return operations.Operation{}, err
			}
			event.EventMetadata = em

			if err = ch.events.Store(ctx, event, op); err != nil {
				return operations.Operation{}, xerrors.Errorf("failed to save event for op %+v: %w", op, err)
			}

			return op, nil
		})
}

func (ch *ClickHouse) createZKSubCluster(
	ctx context.Context,
	session sessions.Session,
	modifier creatorModifier,
	cluster cluster,
	zkHostSpecs []chmodels.HostSpec,
	group clusterslogic.ResolvedHostGroup,
	subnets []networkProvider.Subnet) error {
	sc, err := modifier.CreateSubCluster(ctx, models.CreateSubClusterArgs{
		ClusterID: cluster.ClusterID,
		Name:      chmodels.ZKSubClusterName,
		Roles:     []hosts.Role{hosts.RoleZooKeeper},
		Revision:  cluster.Revision,
	})
	if err != nil {
		return err
	}

	zkHosts, err := ch.createHosts(ctx, session, modifier, cluster, sc.SubClusterID, optional.String{}, chmodels.ZooKeeperShardName, zkHostSpecs, subnets, group)
	if err != nil {
		return err
	}
	pillar := chpillars.NewSubClusterZK()
	if err := pillar.AddUsers(ch.cryptoProvider); err != nil {
		return err
	}
	for _, host := range zkHosts {
		if _, err := pillar.AddNode(host.FQDN); err != nil {
			return err
		}
	}
	return modifier.AddSubClusterPillar(ctx, sc.ClusterID, sc.SubClusterID, cluster.Revision, pillar)
}

func (ch *ClickHouse) StartCluster(ctx context.Context, cid string) (operations.Operation, error) {
	return ch.operator.ModifyOnNotRunningCluster(ctx, cid, clusters.TypeClickHouse,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader, modifier clusterslogic.Modifier, cluster clusterslogic.Cluster) (operations.Operation, error) {
			taskArgs := map[string]interface{}{
				"time_limit": optional.NewDuration(3 * time.Hour),
			}

			op, err := ch.tasks.StartCluster(
				ctx,
				session,
				cid,
				cluster.Revision,
				chmodels.TaskTypeClusterStart,
				chmodels.OperationTypeClusterStart,
				chmodels.MetadataStartCluster{},
				taskslogic.StartClusterTaskArgs(taskArgs),
			)
			if err != nil {
				return operations.Operation{}, err
			}

			event := &cheventspub.StartCluster{
				Authentication:  ch.events.NewAuthentication(session.Subject),
				Authorization:   ch.events.NewAuthorization(session.Subject),
				RequestMetadata: ch.events.NewRequestMetadata(ctx),
				EventStatus:     cheventspub.StartCluster_STARTED,
				Details: &cheventspub.StartCluster_EventDetails{
					ClusterId: cid,
				},
			}
			em, err := ch.events.NewEventMetadata(event, op, session.FolderCoords.CloudExtID, session.FolderCoords.FolderExtID)
			if err != nil {
				return operations.Operation{}, err
			}
			event.EventMetadata = em

			if err = ch.events.Store(ctx, event, op); err != nil {
				return operations.Operation{}, xerrors.Errorf("failed to save event for op %+v: %w", op, err)
			}

			return op, nil
		})
}

func (ch *ClickHouse) StopCluster(ctx context.Context, cid string) (operations.Operation, error) {
	return ch.operator.ModifyOnNotStoppedCluster(ctx, cid, clusters.TypeClickHouse,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader, modifier clusterslogic.Modifier, cluster clusterslogic.Cluster) (operations.Operation, error) {
			op, err := ch.tasks.StopCluster(
				ctx,
				session,
				cid,
				cluster.Revision,
				chmodels.TaskTypeClusterStop,
				chmodels.OperationTypeClusterStop,
				chmodels.MetadataStopCluster{},
			)
			if err != nil {
				return operations.Operation{}, err
			}

			event := &cheventspub.StopCluster{
				Authentication:  ch.events.NewAuthentication(session.Subject),
				Authorization:   ch.events.NewAuthorization(session.Subject),
				RequestMetadata: ch.events.NewRequestMetadata(ctx),
				EventStatus:     cheventspub.StopCluster_STARTED,
				Details: &cheventspub.StopCluster_EventDetails{
					ClusterId: cid,
				},
			}
			em, err := ch.events.NewEventMetadata(event, op, session.FolderCoords.CloudExtID, session.FolderCoords.FolderExtID)
			if err != nil {
				return operations.Operation{}, err
			}
			event.EventMetadata = em

			if err = ch.events.Store(ctx, event, op); err != nil {
				return operations.Operation{}, xerrors.Errorf("failed to save event for op %+v: %w", op, err)
			}

			return op, nil
		})
}

func (ch *ClickHouse) MoveCluster(ctx context.Context, cid, destinationFolderID string) (operations.Operation, error) {
	return ch.operator.MoveCluster(ctx, cid, destinationFolderID, clusters.TypeClickHouse,
		func(ctx context.Context, session sessions.Session, modifier clusterslogic.Modifier, cluster clusterslogic.Cluster) (operations.Operation, error) {
			err := modifier.UpdateClusterFolder(ctx, cluster, destinationFolderID)
			if err != nil {
				return operations.Operation{}, err
			}

			op, err := ch.tasks.MoveCluster(
				ctx,
				session,
				cid,
				cluster.Revision,
				chmodels.OperationTypeClusterMove,
				chmodels.MetadataMoveCluster{
					SourceFolderID:      session.FolderCoords.FolderExtID,
					DestinationFolderID: destinationFolderID,
				},
				func(options *taskslogic.MoveClusterOptions) {
					options.TaskArgs = map[string]interface{}{}
					options.SrcFolderTaskType = chmodels.TaskTypeClusterMove
					options.DstFolderTaskType = chmodels.TaskTypeClusterMoveNoOp
					options.SrcFolderCoords = session.FolderCoords
					options.DstFolderExtID = destinationFolderID
				},
			)
			if err != nil {
				return operations.Operation{}, err
			}

			event := &cheventspub.MoveCluster{
				Authentication:  ch.events.NewAuthentication(session.Subject),
				Authorization:   ch.events.NewAuthorization(session.Subject),
				RequestMetadata: ch.events.NewRequestMetadata(ctx),
				EventStatus:     cheventspub.MoveCluster_STARTED,
				Details: &cheventspub.MoveCluster_EventDetails{
					ClusterId: cid,
				},
			}
			em, err := ch.events.NewEventMetadata(event, op, session.FolderCoords.CloudExtID, session.FolderCoords.FolderExtID)
			if err != nil {
				return operations.Operation{}, err
			}
			event.EventMetadata = em

			if err = ch.events.Store(ctx, event, op); err != nil {
				return operations.Operation{}, xerrors.Errorf("failed to save event for op %+v: %w", op, err)
			}

			return op, nil
		})
}

func (ch *ClickHouse) RescheduleMaintenance(ctx context.Context, cid string, rescheduleType clusters.RescheduleType, delayedUntil optional.Time) (operations.Operation, error) {
	f := func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader, modifier clusterslogic.Modifier, cluster clusterslogic.Cluster) (operations.Operation, error) {
		mntTime, err := modifier.RescheduleMaintenance(ctx, cluster.ClusterID, rescheduleType, delayedUntil)
		if err != nil {
			return operations.Operation{}, err
		}

		op, err := ch.tasks.CreateFinishedTask(
			ctx,
			session,
			cluster.ClusterID,
			cluster.Revision,
			chmodels.OperationTypeMaintenanceReschedule,
			chmodels.MetadataRescheduleMaintenance{
				DelayedUntil: mntTime,
			},
			true,
		)
		if err != nil {
			return operations.Operation{}, err
		}

		return op, nil
	}

	return ch.operator.ModifyOnClusterWithoutRevChanging(ctx, cid, clusters.TypeClickHouse, f)
}
