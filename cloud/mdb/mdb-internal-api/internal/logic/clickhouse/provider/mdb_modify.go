package provider

import (
	"context"

	cheventspub "a.yandex-team.ru/cloud/bitbucket/public-api/yandex/cloud/events/mdb/clickhouse"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/clickhouse"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/clickhouse/chmodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/common"
	clusterslogic "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/sessions"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	clustermodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/environment"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/operations"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/slices"
)

func (ch *ClickHouse) UpdateMDBCluster(ctx context.Context, args clickhouse.UpdateMDBClusterArgs) (operations.Operation, error) {
	if err := args.ValidateAndSane(); err != nil {
		return operations.Operation{}, err
	}

	return ch.operator.ModifyOnCluster(
		ctx,
		args.ClusterID,
		clustermodels.TypeClickHouse,
		func(
			ctx context.Context,
			session sessions.Session,
			reader clusterslogic.Reader,
			modifier clusterslogic.Modifier,
			cluster clusterslogic.Cluster,
		) (operations.Operation, error) {
			clusterChanges := newChanges()

			chSubCluster, err := ch.chSubCluster(ctx, reader, cluster.ClusterID)
			if err != nil {
				return operations.Operation{}, err
			}

			clusterChanges.HasMetadataChanges, err = modifier.ModifyClusterMetadata(ctx, cluster, args.Name, args.Labels)
			if err != nil {
				return operations.Operation{}, err
			}

			clusterChanges.HasMetaDBChanges, err = modifier.ModifyClusterMetadataParameters(ctx, cluster, args.Description, args.Labels, args.DeletionProtection, args.MaintenanceWindow)
			if err != nil {
				return operations.Operation{}, err
			}

			if args.SecurityGroupIDs.Valid {
				if !slices.EqualAnyOrderStrings(args.SecurityGroupIDs.Strings, cluster.SecurityGroupIDs) {
					if err := ch.compute.ValidateSecurityGroups(ctx, args.SecurityGroupIDs.Strings, cluster.NetworkID); err != nil {
						return operations.Operation{}, err
					}

					clusterChanges.mergeChanges(clickHouseChanges{
						ClusterChanges: common.ClusterChanges{
							HasChanges: true,
							TaskArgs: map[string]interface{}{
								"security_group_ids": args.SecurityGroupIDs,
							},
						},
					})
				}
			}

			if args.ServiceAccountID.Valid && args.ServiceAccountID.String == chSubCluster.Pillar.ServiceAccountID().String {

				h, err := reader.AnyHost(ctx, args.ClusterID)
				if err != nil {
					return operations.Operation{}, err
				}

				vtype, err := environment.ParseVType(h.VType)
				if err != nil {
					return operations.Operation{}, err
				}

				if vtype != environment.VTypeCompute {
					return operations.Operation{}, semerr.FailedPrecondition("service accounts are not supported in porto clusters")
				}

				if err := session.ValidateServiceAccount(ctx, args.ServiceAccountID.String); err != nil {
					return operations.Operation{}, err
				}

				chSubCluster.Pillar.SetServiceAccountID(args.ServiceAccountID.String)

				clusterChanges.mergeChanges(clickHouseChanges{
					ClusterChanges: common.ClusterChanges{
						HasChanges: true,
					},
					HasPillarChanges: true,
				})
			}
			clusterChanges.TaskArgs["service_account_id"] = chSubCluster.Pillar.Data.ServiceAccountID

			resourceChanges, err := ch.processMDBResourceChanges(ctx, session, reader, modifier, cluster, chSubCluster, args.ConfigSpec)
			if err != nil {
				return operations.Operation{}, err
			}
			clusterChanges.mergeChanges(resourceChanges)

			if args.ConfigSpec.Access.DataLens.Valid || args.ConfigSpec.Access.DataTransfer.Valid ||
				args.ConfigSpec.Access.Metrica.Valid || args.ConfigSpec.Access.WebSQL.Valid ||
				args.ConfigSpec.Access.Serverless.Valid || args.ConfigSpec.Access.YandexQuery.Valid {
				chSubCluster.Pillar.SetAccess(args.ConfigSpec.Access)
				clusterChanges.mergeChanges(clickHouseChanges{
					ClusterChanges: common.ClusterChanges{
						HasChanges: true,
					},
					HasPillarChanges: true,
				})
			}

			if clusterChanges.HasPillarChanges {
				if err := modifier.UpdateSubClusterPillar(ctx, cluster.ClusterID, chSubCluster.SubClusterID, cluster.Revision, chSubCluster.Pillar); err != nil {
					return operations.Operation{}, err
				}
			}

			op, err := ch.applyChanges(ctx, session, modifier, clusterChanges, cluster.Cluster, chSubCluster, args.SecurityGroupIDs)
			if err != nil {
				return operations.Operation{}, err
			}

			event := &cheventspub.UpdateCluster{
				Authentication:  ch.events.NewAuthentication(session.Subject),
				Authorization:   ch.events.NewAuthorization(session.Subject),
				RequestMetadata: ch.events.NewRequestMetadata(ctx),
				EventStatus:     cheventspub.UpdateCluster_STARTED,
				Details: &cheventspub.UpdateCluster_EventDetails{
					ClusterId: cluster.ClusterID,
				},
			}

			em, err := ch.events.NewEventMetadata(event, op, session.FolderCoords.CloudExtID, session.FolderCoords.FolderExtID)
			if err != nil {
				return operations.Operation{}, err
			}
			event.EventMetadata = em

			if err = ch.events.Store(ctx, event, op); err != nil {
				return operations.Operation{}, xerrors.Errorf("saving event for op %+v: %w", op, err)
			}

			if err := ch.search.StoreDoc(
				ctx,
				clickHouseService,
				session.FolderCoords.FolderExtID,
				session.FolderCoords.CloudExtID,
				op,
				searchAttributesExtractor(chSubCluster),
			); err != nil {
				return operations.Operation{}, err
			}

			return op, nil
		})
}

func (ch *ClickHouse) processMDBResourceChanges(
	ctx context.Context,
	session sessions.Session,
	reader clusterslogic.Reader,
	modifier clusterslogic.Modifier,
	cluster clusterslogic.Cluster,
	subcluster subCluster,
	configSpec chmodels.MDBConfigSpecUpdate,
) (clickHouseChanges, error) {
	configChanges := newChanges()

	allHosts, err := clusterslogic.ListAllHosts(ctx, reader, cluster.ClusterID)
	if err != nil {
		return clickHouseChanges{}, err
	}

	chHosts := clusterslogic.GetHostsWithRole(allHosts, hosts.RoleClickHouse)
	zkHosts := clusterslogic.GetHostsWithRole(allHosts, hosts.RoleZooKeeper)

	hostGroups := buildClusterHostGroups(chHosts, zkHosts, optional.String{}, nil, nil, nil, nil)
	for i, group := range hostGroups {
		res := configSpec.ClickHouseResources
		if group.Role == hosts.RoleZooKeeper {
			res = configSpec.ZookeeperResources
		}

		group.NewResourcePresetExtID = res.ResourcePresetExtID
		group.NewDiskSize = res.DiskSize
		hostGroups[i] = group
	}
	resolvedHostGroups, changed, err := modifier.ValidateResources(ctx, session, clustermodels.TypeClickHouse, hostGroups...)
	configChanges.HasChanges = configChanges.HasChanges || changed
	configChanges.HasResourceChanges = configChanges.HasResourceChanges || changed
	if err != nil || !changed {
		return clickHouseChanges{}, err
	}

	if len(zkHosts) != 0 {
		if err := ch.validateZookeeperCores(ctx, session, modifier, resolvedHostGroups, "update the cluster's resource preset"); err != nil {
			return clickHouseChanges{}, err
		}
	}

	isDownscale := false
	isClickHouseChanges := false
	chResolvedGroups := resolvedHostGroups.MustMapByShardName(hosts.RoleClickHouse)
	versionNeedRestart, err := chmodels.VersionGreaterOrEqual(subcluster.Pillar.Data.ClickHouse.Version, 20, 4)
	if err != nil {
		return clickHouseChanges{}, err
	}

	for _, host := range chHosts {
		group, ok := chResolvedGroups[host.ShardID.String]
		if !ok {
			return configChanges, xerrors.Errorf("host group for host %q not found", host.FQDN)
		}

		isClickHouseChanges = isClickHouseChanges || group.HasChanges()
		isDownscale = isDownscale || group.IsDownscale()
		if err := modifier.ModifyHost(ctx, models.ModifyHostArgs{
			FQDN:             host.FQDN,
			ClusterID:        cluster.ClusterID,
			Revision:         cluster.Revision,
			SpaceLimit:       group.TargetDiskSize(),
			ResourcePresetID: group.TargetResourcePreset().ID,
			DiskTypeExtID:    group.DiskTypeExtID,
		}); err != nil {
			return configChanges, err
		}
	}

	if zkGroup, ok := resolvedHostGroups.ByHostRole(hosts.RoleZooKeeper); ok && zkGroup.HasChanges() {
		if isClickHouseChanges && zkGroup.IsDownscale() != isDownscale {
			return configChanges, semerr.FailedPrecondition("upscale of one type of hosts cannot be mixed with downscale of another type of hosts")
		}

		for _, host := range zkHosts {
			if err := modifier.ModifyHost(ctx, models.ModifyHostArgs{
				FQDN:             host.FQDN,
				ClusterID:        cluster.ClusterID,
				Revision:         cluster.Revision,
				SpaceLimit:       zkGroup.TargetDiskSize(),
				ResourcePresetID: zkGroup.TargetResourcePreset().ID,
				DiskTypeExtID:    zkGroup.DiskTypeExtID,
			}); err != nil {
				return configChanges, err
			}
		}
	}

	if isClickHouseChanges && versionNeedRestart {
		configChanges.TaskArgs["restart"] = true
	}
	configChanges.TaskArgs["reverse_order"] = isDownscale
	configChanges.Timeout = resolvedHostGroups.ExtraTimeout()
	return configChanges, nil
}

func (ch *ClickHouse) upgradeClickHouseVersion(session sessions.Session, version optional.String, chSubCluster *subCluster) (clickHouseChanges, error) {
	changes := newChanges()
	if version.Valid && version.String != chmodels.CutVersionToMajor(chSubCluster.Pillar.Data.ClickHouse.Version) {
		v, err := ch.validateClickHouseVersion(session, version.String)
		if err != nil {
			return changes, err
		}
		chSubCluster.Pillar.Data.ClickHouse.Version = v
		changes.NeedUpgrade = true
		changes.HasChanges = true
		changes.HasPillarChanges = true
		changes.TaskArgs["restart"] = true
	}

	return changes, nil
}

func (ch *ClickHouse) applyChanges(
	ctx context.Context,
	session sessions.Session,
	modifier clusterslogic.Modifier,
	changes clickHouseChanges,
	cluster clustermodels.Cluster,
	subcluster subCluster,
	securityGroupIDS optional.Strings,
) (operations.Operation, error) {
	if changes.HasPillarChanges {
		if err := modifier.UpdateSubClusterPillar(ctx, cluster.ClusterID, subcluster.SubClusterID, cluster.Revision, subcluster.Pillar); err != nil {
			return operations.Operation{}, err
		}
	}

	if changes.HasResourceChanges && changes.NeedUpgrade {
		return operations.Operation{}, semerr.InvalidInputf("version update cannot be mixed with update of host resources")
	}

	return common.CreateClusterModifyOperation(ch.tasks, ctx, session, cluster, searchAttributesExtractor(subcluster), changes.ClusterChanges, securityGroupIDS, common.TaskCreationInfo{
		ClusterUpgradeTask:     chmodels.TaskTypeClusterUpgrade,
		ClusterModifyTask:      chmodels.TaskTypeClusterModify,
		ClusterModifyOperation: chmodels.OperationTypeClusterModify,

		MetadataUpdateTask:      chmodels.TaskTypeMetadataUpdate,
		MetadataUpdateOperation: chmodels.OperationTypeMetadataUpdate,

		SearchService: chmodels.ClickHouseSearchService,
	})
}
