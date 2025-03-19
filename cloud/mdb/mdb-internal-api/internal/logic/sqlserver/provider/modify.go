package provider

import (
	"context"
	"reflect"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/common"
	clusterslogic "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/sessions"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/sqlserver"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/sqlserver/provider/internal/sspillars"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/sqlserver/ssmodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	bmodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/backups"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/operations"
	modelsoptional "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/optional"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/resources"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/slices"
)

func (ss *SQLServer) modifyResources(ctx context.Context, reader clusterslogic.Reader, modifier clusterslogic.Modifier, cluster clusters.Cluster, targetResources models.ClusterResourcesSpec, session sessions.Session) (bool, time.Duration, error) {
	var additionalTimeout time.Duration
	currentHosts, _, _, err := reader.ListHosts(ctx, cluster.ClusterID, 0, 0)
	if err != nil {
		return false, 0, err
	}

	currentHosts = clusterslogic.GetHostsWithRole(currentHosts, hosts.RoleSQLServer)

	hostCount := len(currentHosts)
	if hostCount == 0 {
		// should never happen,
		// but I decided to still protect code from the panic a few lines below
		return false, 0, semerr.InvalidInput("cluster has zero hosts")
	}

	currentHost := currentHosts[0]

	hostGroup := clusterslogic.HostGroup{
		Role:                       hosts.RoleSQLServer,
		CurrentResourcePresetExtID: optional.NewString(currentHost.ResourcePresetExtID),
		NewResourcePresetExtID:     targetResources.ResourcePresetExtID,
		DiskTypeExtID:              currentHost.DiskTypeExtID,
		CurrentDiskSize:            optional.NewInt64(currentHost.SpaceLimit),
		NewDiskSize:                targetResources.DiskSize,
		HostsCurrent:               make([]clusterslogic.ZoneHosts, 0, len(currentHosts)),
	}
	for _, host := range currentHosts {
		hostGroup.HostsCurrent = append(hostGroup.HostsCurrent, clusterslogic.ZoneHosts{ZoneID: host.ZoneID, Count: 1})
	}

	resolvedHostGroups, hasChanges, err := modifier.ValidateResources(
		ctx,
		session,
		clusters.TypeSQLServer,
		hostGroup,
	)
	if err != nil {
		return false, 0, err
	}

	if !hasChanges {
		return false, 0, nil
	}

	resolvedHostGroup := resolvedHostGroups.Single()

	for _, host := range currentHosts {
		args := models.ModifyHostArgs{
			ClusterID:        host.ClusterID,
			FQDN:             host.FQDN,
			Revision:         cluster.Revision,
			SpaceLimit:       resolvedHostGroup.TargetDiskSize(),
			ResourcePresetID: resolvedHostGroup.TargetResourcePreset().ID,
			// TODO: support DiskTypeExtID changes
			DiskTypeExtID: resolvedHostGroup.DiskTypeExtID,
		}

		err = modifier.ModifyHost(ctx, args)
		if err != nil {
			return false, 0, err
		}
		additionalTimeout += 15 * time.Minute
	}

	return true, additionalTimeout, nil
}

func (ss *SQLServer) getResourcesOnPreviousRevision(ctx context.Context, reader clusterslogic.Reader, cluster clusterslogic.Cluster) (models.ClusterResources, error) {
	return reader.ResourcesByClusterIDRoleAtRevision(ctx, cluster.ClusterID, cluster.Revision-1, hosts.RoleSQLServer)
}

func (ss *SQLServer) ModifyCluster(ctx context.Context, args sqlserver.ModifyClusterArgs) (operations.Operation, error) {
	return ss.operator.ModifyOnCluster(ctx, args.ClusterID, clusters.TypeSQLServer,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader, modifier clusterslogic.Modifier, cluster clusterslogic.Cluster) (operations.Operation, error) {
			clusterChanges := common.GetClusterChanges()
			clusterChanges.Timeout = 15 * time.Minute
			var err error
			var pillarChanges bool
			var changedSecurityGroupIDs optional.Strings
			// TODO: get resource preset from new config or old from metadb, when it'll be useful in validation
			resourcePreset := resources.Preset{}
			if err = args.Validate(resourcePreset); err != nil {
				return operations.Operation{}, err
			}

			clusterChanges.HasMetadataChanges, err = modifier.ModifyClusterMetadata(ctx, cluster, args.Name, args.Labels)
			if err != nil {
				return operations.Operation{}, err
			}

			/* modify metadb-only parameters */
			clusterChanges.HasMetaDBChanges, err = modifier.ModifyClusterMetadataParameters(ctx, cluster, args.Description, args.Labels, args.DeletionProtection, modelsoptional.MaintenanceWindow{})
			if err != nil {
				return operations.Operation{}, err
			}

			pillar := sspillars.NewCluster()
			if err := cluster.Pillar(pillar); err != nil {
				return operations.Operation{}, err
			}

			if args.SecurityGroupsIDs.Valid && !slices.EqualAnyOrderStrings(args.SecurityGroupsIDs.Strings, cluster.SecurityGroupIDs) {
				changedSecurityGroupIDs.Set(slices.DedupStrings(args.SecurityGroupsIDs.Strings))
				if err := ss.compute.ValidateSecurityGroups(ctx, changedSecurityGroupIDs.Strings, cluster.NetworkID); err != nil {
					return operations.Operation{}, err
				}
				clusterChanges.HasChanges = true
			}

			if args.ServiceAccountID.Valid && args.ServiceAccountID.String != pillar.Data.ServiceAccountID {
				if err := session.ValidateServiceAccount(ctx, args.ServiceAccountID.String); err != nil {
					return operations.Operation{}, err
				}
				pillar.Data.ServiceAccountID = args.ServiceAccountID.String
				pillarChanges = true
			}
			/* handle config changes */
			config := args.ClusterConfigSpec
			if config.Version.Valid {
				if v := config.Version.String; v != pillar.Data.SQLServer.Version.MajorHuman {
					return operations.Operation{}, semerr.NotImplemented("version upgrade is not implemented yet")
				}
			}

			/* handle resource changes if requested */
			if config.Resources.IsSet() {
				changes, addTimeout, err := ss.modifyResources(ctx, reader, modifier, cluster.Cluster, config.Resources, session)
				if err != nil {
					return operations.Operation{}, err
				}

				clusterChanges.Timeout = clusterChanges.Timeout + addTimeout

				clusterChanges.HasChanges = clusterChanges.HasChanges || changes
			}

			if config.Config.Valid {
				mergedConfig, err := ssmodels.ClusterConfigMergeByFields(
					pillar.Data.SQLServer.Config,
					config.Config.Value,
					config.Config.Fields,
				)
				if err != nil {
					return operations.Operation{}, err
				}
				if !reflect.DeepEqual(mergedConfig, pillar.Data.SQLServer.Config) {
					pillar.Data.SQLServer.Config = mergedConfig
					pillarChanges = true
				}
			}
			if config.Access.DataLens.Valid && config.Access.DataLens.Bool != pillar.Data.Access.DataLens {
				pillar.Data.Access.DataLens = config.Access.DataLens.Bool
				pillarChanges = true
			}
			if config.Access.WebSQL.Valid && config.Access.WebSQL.Bool != pillar.Data.Access.WebSQL {
				pillar.Data.Access.WebSQL = config.Access.WebSQL.Bool
				pillarChanges = true
			}
			if config.Access.DataTransfer.Valid && config.Access.DataTransfer.Bool != pillar.Data.Access.DataTransfer {
				pillar.Data.Access.DataTransfer = config.Access.DataTransfer.Bool
				pillarChanges = true
			}
			if config.SecondaryConnections.Valid && ssmodels.SecondaryConnectionsToUR(config.SecondaryConnections.Value) != pillar.Data.SQLServer.UnreadableReplicas {
				pillar.Data.SQLServer.UnreadableReplicas = ssmodels.SecondaryConnectionsToUR(config.SecondaryConnections.Value)
				pillarChanges = true
			}
			if pillarChanges {
				err = modifier.UpdatePillar(ctx, cluster.ClusterID, cluster.Revision, pillar)
				if err != nil {
					return operations.Operation{}, xerrors.Errorf("failed to update pillar: %w", err)
				}
				clusterChanges.HasChanges = true
			}

			if config.BackupWindowStart.Valid && !reflect.DeepEqual(config.BackupWindowStart.Value, cluster.BackupSchedule.Start) {
				backupSchedule := bmodels.GetBackupSchedule(cluster.BackupSchedule, args.ClusterConfigSpec.BackupWindowStart)
				err := ss.backups.AddBackupSchedule(ctx, cluster.ClusterID, backupSchedule, cluster.Revision)
				if err != nil {
					return operations.Operation{}, xerrors.Errorf("failed to update backup schedule: %w", err)
				}
				clusterChanges.HasChanges = true
			}

			clusterChanges.TaskArgs["major_version"] = pillar.Data.SQLServer.Version.MajorHuman
			clusterChanges.TaskArgs["service_account_id"] = pillar.Data.ServiceAccountID

			op, err := common.CreateClusterModifyOperation(ss.tasks, ctx, session, cluster.Cluster, searchAttributesExtractor, clusterChanges, changedSecurityGroupIDs, getTaskCreationInfo())
			if err != nil {
				return operations.Operation{}, err
			}

			if clusterChanges.HasOnlyMetadbChanges() {
				if err := ss.toSearchQueue(ctx, session.FolderCoords, op); err != nil {
					return operations.Operation{}, err
				}

				return op, nil
			}

			return op, nil
		})
}

func getTaskCreationInfo() common.TaskCreationInfo {
	return common.TaskCreationInfo{
		ClusterModifyTask:      ssmodels.TaskTypeClusterModify,
		ClusterModifyOperation: ssmodels.OperationTypeClusterModify,

		MetadataUpdateTask:      ssmodels.TaskTypeMetadataUpdate,
		MetadataUpdateOperation: ssmodels.OperationTypeMetadataUpdate,

		SearchService: sqlServerService,
	}
}
