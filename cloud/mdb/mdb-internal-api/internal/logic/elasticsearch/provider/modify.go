package provider

import (
	"context"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/common"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/elasticsearch"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/elasticsearch/esmodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/elasticsearch/provider/internal/espillars"
	clusterslogic "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/sessions"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/environment"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/operations"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/resources"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/slices"
)

const (
	// updateClusterDefaultTimeout default timeout for cluster update operations,
	// heavy operation can increase this value, so final timeout will be greater.
	updateClusterDefaultTimeout = 15 * time.Minute
)

type resourceChanges struct {
	DiskUpscale     bool
	PresetDownscale bool
}

type workerFlags struct {
	reverseOrderData   bool
	reverseOrderMaster bool
	disableHealthCheck bool
}

func (es *ElasticSearch) ModifyCluster(ctx context.Context, args elasticsearch.ModifyClusterArgs) (operations.Operation, error) {
	return es.operator.ModifyOnCluster(ctx, args.ClusterID, clusters.TypeElasticSearch,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader, modifier clusterslogic.Modifier, cluster clusterslogic.Cluster) (operations.Operation, error) {
			clusterChanges := common.GetClusterChanges()
			clusterChanges.Timeout = updateClusterDefaultTimeout
			clusterPillar := espillars.NewCluster()
			if err := cluster.Pillar(clusterPillar); err != nil {
				return operations.Operation{}, err
			}

			upillar := espillars.NewPillarUpdater(clusterPillar)

			var changedSecurityGroupIDs optional.Strings

			if err := upillar.UpdateEdition(args.ConfigSpec.Edition, es.allowedEditions); err != nil {
				return operations.Operation{}, err
			}

			if err := upillar.UpdateAdminPassword(args.ConfigSpec.AdminPassword, es.cryptoProvider); err != nil {
				return operations.Operation{}, err
			}

			if args.ServiceAccountID.Valid {
				h, err := reader.AnyHost(ctx, cluster.ClusterID)
				if err != nil {
					return operations.Operation{}, err
				}

				vtype, err := environment.ParseVType(h.VType)
				if err != nil {
					return operations.Operation{}, err
				}

				if err := es.validateServiceAccountAccess(ctx, session, args.ServiceAccountID.String, vtype); err != nil {
					return operations.Operation{}, err
				}
			}

			if err := upillar.UpdateServiceAccountID(args.ServiceAccountID); err != nil {
				return operations.Operation{}, err
			}

			if err := upillar.UpdatePlugins(args.ConfigSpec.Elasticsearch.Plugins); err != nil {
				return operations.Operation{}, err
			}

			if args.SecurityGroupIDs.Valid && !slices.EqualAnyOrderStrings(args.SecurityGroupIDs.Strings, cluster.SecurityGroupIDs) {
				changedSecurityGroupIDs.Set(slices.DedupStrings(args.SecurityGroupIDs.Strings))
				if err := es.compute.ValidateSecurityGroups(ctx, changedSecurityGroupIDs.Strings, cluster.NetworkID); err != nil {
					return operations.Operation{}, err
				}
				clusterChanges.HasChanges = true
			}

			resChanges, resTimeout, flags, err := es.modifyResources(ctx, reader, modifier, cluster.Cluster, &args.ConfigSpec, session)
			if err != nil {
				return operations.Operation{}, err
			}
			clusterChanges.Timeout += resTimeout
			clusterChanges.HasChanges = clusterChanges.HasChanges || resChanges

			if args.ConfigSpec.Version.Valid && !args.ConfigSpec.Version.Value.Equal(clusterPillar.Data.ElasticSearch.Version) {
				if resChanges {
					return operations.Operation{}, semerr.InvalidInput("version update cannot be mixed with update of host resources")
				}
				if err := upillar.UpdateVersion(args.ConfigSpec.Version); err != nil {
					return operations.Operation{}, err
				}
				clusterChanges.NeedUpgrade = true
				clusterChanges.Timeout += 1 * time.Hour
			}

			if args.ConfigSpec.Elasticsearch.DataNode.Valid {
				dataNode := args.ConfigSpec.Elasticsearch.DataNode.Value
				err = dataNode.Validate(false)
				if err != nil {
					return operations.Operation{}, semerr.InvalidInputf("invalid data node config: %s", err)
				}
				dataPillar := &espillars.DataNodeSubCluster{}
				subCluster, err := reader.SubClusterByRole(ctx, cluster.ClusterID, hosts.RoleElasticSearchDataNode, dataPillar)
				if err != nil {
					return operations.Operation{}, err
				}
				if dataPillar.UpdateConfig(dataNode.Config) {
					err = modifier.UpdateSubClusterPillar(ctx, cluster.ClusterID, subCluster.SubClusterID, cluster.Revision, dataPillar)
					if err != nil {
						return operations.Operation{}, err
					}
					clusterChanges.HasChanges = true
				}
			}

			clusterChanges.HasMetadataChanges, err = modifier.ModifyClusterMetadata(ctx, cluster, args.Name, args.Labels)
			if err != nil {
				return operations.Operation{}, err
			}

			clusterChanges.HasMetaDBChanges, err = modifier.ModifyClusterMetadataParameters(ctx, cluster, args.Description, args.Labels, args.DeletionProtection, args.MaintenanceWindow)
			if err != nil {
				return operations.Operation{}, err
			}

			if args.ConfigSpec.Access.Valid {
				upillar.UpdateAccess(args.ConfigSpec.Access)
			}

			if upillar.HasChanges() {
				if err := modifier.UpdatePillar(ctx, cluster.ClusterID, cluster.Revision, upillar.Pillar()); err != nil {
					return operations.Operation{}, err
				}
				clusterChanges.HasChanges = true
			}

			clusterChanges.TaskArgs = map[string]interface{}{
				"restart":              true,
				"service_account_id":   clusterPillar.Data.ServiceAccountID,
				"disable_health_check": flags.disableHealthCheck,
				"reverse_order_master": flags.reverseOrderMaster,
				"reverse_order_data":   flags.reverseOrderData,
			}

			op, err := common.CreateClusterModifyOperation(es.tasks, ctx, session, cluster.Cluster, searchAttributesExtractor, clusterChanges, args.SecurityGroupIDs, getTaskCreationInfo())
			if err != nil {
				return operations.Operation{}, err
			}

			if clusterChanges.HasOnlyMetadbChanges() {
				if err := es.toSearchQueue(ctx, session.FolderCoords, op); err != nil {
					return operations.Operation{}, err
				}

				return op, nil
			}

			return op, nil
		},
		clusterslogic.WithPermission(esmodels.PermClustersUpdate),
	)
}

func (es *ElasticSearch) modifyResources(ctx context.Context, reader clusterslogic.Reader, modifier clusterslogic.Modifier, cluster clusters.Cluster, configSpec *esmodels.ConfigSpecUpdate, session sessions.Session) (bool, time.Duration, workerFlags, error) {
	var hasChanges bool
	var timeout time.Duration
	var flags workerFlags

	if configSpec.Elasticsearch.DataNode.Valid {
		dataNode := configSpec.Elasticsearch.DataNode.Value
		if dataNode.Resources.IsSet() {
			changes, extraTimeout, dataResChanges, err := es.modifyResourcesForRole(ctx, reader, modifier, cluster, hosts.RoleElasticSearchDataNode, dataNode.Resources, session)
			if err != nil {
				return false, timeout, workerFlags{}, err
			}

			hasChanges = hasChanges || changes
			timeout += extraTimeout

			flags.disableHealthCheck = dataResChanges.DiskUpscale
			flags.reverseOrderData = dataResChanges.PresetDownscale
		}
	}

	if configSpec.Elasticsearch.MasterNode.Valid {
		masterNode := configSpec.Elasticsearch.MasterNode.Value
		changes, extraTimeout, masterResChanges, err := es.modifyResourcesForRole(ctx, reader, modifier, cluster, hosts.RoleElasticSearchMasterNode, masterNode.Resources, session)
		if err != nil {
			return false, timeout, workerFlags{}, err
		}

		hasChanges = hasChanges || changes
		timeout += extraTimeout

		flags.reverseOrderMaster = masterResChanges.PresetDownscale
	}
	return hasChanges, timeout, flags, nil
}

func (es *ElasticSearch) modifyResourcesForRole(ctx context.Context, reader clusterslogic.Reader, modifier clusterslogic.Modifier, cluster clusters.Cluster, role hosts.Role, targetResources models.ClusterResourcesSpec, session sessions.Session) (bool, time.Duration, resourceChanges, error) {
	currentHosts, err := clusterslogic.ListAllHosts(ctx, reader, cluster.ClusterID)
	if err != nil {
		return false, 0, resourceChanges{}, err
	}

	hostsWithRole := clusterslogic.GetHostsWithRole(currentHosts, role)
	if len(hostsWithRole) == 0 {
		return false, 0, resourceChanges{}, xerrors.Errorf("failed to find host with role %q within cluster %q", role.String(), cluster.ClusterID)
	}
	anyHost := hostsWithRole[0]

	if targetResources.DiskTypeExtID.Valid && targetResources.DiskTypeExtID.String != anyHost.DiskTypeExtID {
		return false, 0, resourceChanges{}, semerr.InvalidInput("type of disk cannot be changed")
	}

	diskUpscale := false
	if targetResources.DiskSize.Valid {
		targetDiskSize := targetResources.DiskSize.Must()
		if targetDiskSize > anyHost.SpaceLimit {
			diskUpscale = true
		}

		isLocalDisk := resources.DiskTypes{}.IsLocalDisk(anyHost.DiskTypeExtID)
		vType, err := environment.ParseVType(anyHost.VType)
		if err != nil {
			return false, 0, resourceChanges{}, err
		}
		if targetDiskSize < anyHost.SpaceLimit && isLocalDisk && vType == environment.VTypeCompute {
			return false, 0, resourceChanges{}, semerr.InvalidInput("local disk cannot be sized down")
		}
	}

	zoneHostsByZone := make(map[string]*clusterslogic.ZoneHosts)
	for _, host := range hostsWithRole {
		zoneHosts, ok := zoneHostsByZone[host.ZoneID]
		if !ok {
			zoneHosts = &clusterslogic.ZoneHosts{ZoneID: host.ZoneID}
			zoneHostsByZone[host.ZoneID] = zoneHosts
		}
		zoneHosts.Count += 1
	}
	zoneHostsList := make([]clusterslogic.ZoneHosts, 0, len(zoneHostsByZone))
	for _, zoneHosts := range zoneHostsByZone {
		zoneHostsList = append(zoneHostsList, *zoneHosts)
	}

	hostGroup := clusterslogic.HostGroup{
		Role:                       role,
		CurrentResourcePresetExtID: optional.NewString(anyHost.ResourcePresetExtID),
		NewResourcePresetExtID:     targetResources.ResourcePresetExtID,
		DiskTypeExtID:              anyHost.DiskTypeExtID,
		CurrentDiskSize:            optional.NewInt64(anyHost.SpaceLimit),
		NewDiskSize:                targetResources.DiskSize,
		HostsCurrent:               zoneHostsList,
		SkipValidations: clusterslogic.SkipValidations{
			MaxHosts: role == hosts.RoleElasticSearchDataNode,
		},
	}

	resolvedHostGroups, hasChanges, err := modifier.ValidateResources(
		ctx,
		session,
		clusters.TypeElasticSearch,
		hostGroup,
	)
	if err != nil {
		return false, 0, resourceChanges{}, err
	}

	if !hasChanges {
		return false, 0, resourceChanges{}, nil
	}

	resolvedHostGroup := resolvedHostGroups.Single()

	var extraTimeout time.Duration
	for _, host := range hostsWithRole {
		args := models.ModifyHostArgs{
			ClusterID:        host.ClusterID,
			FQDN:             host.FQDN,
			Revision:         cluster.Revision,
			SpaceLimit:       resolvedHostGroup.TargetDiskSize(),
			ResourcePresetID: resolvedHostGroup.TargetResourcePreset().ID,
			DiskTypeExtID:    resolvedHostGroup.DiskTypeExtID,
		}

		err = modifier.ModifyHost(ctx, args)
		if err != nil {
			return false, 0, resourceChanges{}, err
		}

		// expect 1 GB to be transferred in 60 seconds
		extraTimeout += time.Duration(host.SpaceLimit*60/(1<<30)) * time.Second
	}

	return true, extraTimeout, resourceChanges{
		DiskUpscale:     diskUpscale,
		PresetDownscale: resolvedHostGroup.IsDownscale(),
	}, nil
}

func getTaskCreationInfo() common.TaskCreationInfo {
	return common.TaskCreationInfo{
		ClusterUpgradeTask:     esmodels.TaskTypeClusterUpgrade,
		ClusterModifyTask:      esmodels.TaskTypeClusterModify,
		ClusterModifyOperation: esmodels.OperationTypeClusterModify,

		MetadataUpdateTask:      esmodels.TaskTypeMetadataUpdate,
		MetadataUpdateOperation: esmodels.OperationTypeMetadataUpdate,

		SearchService: elasticSearchService,
	}
}
