package provider

import (
	"context"
	"encoding/json"
	"fmt"
	"sort"
	"strings"
	"time"

	cheventspub "a.yandex-team.ru/cloud/bitbucket/public-api/yandex/cloud/events/mdb/clickhouse"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/s3"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/clickhouse"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/clickhouse/chmodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/clickhouse/provider/internal/chpillars"
	clusterslogic "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/sessions"
	taskslogic "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/tasks"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	bmodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/backups"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/environment"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/operations"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/pagination"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/pillars"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/slices"
)

type restoreClusterImplArgs struct {
	backups          []bmodels.Backup
	sourceCluster    cluster
	sourceSubCluster subCluster
	createClusterImplArgs
}

func (ch *ClickHouse) prepareMDBClusterRestore(
	ctx context.Context,
	session sessions.Session,
	creator clusterslogic.Creator,
	args clickhouse.RestoreMDBClusterArgs,
) (createClusterImplArgs, error) {
	if err := args.ValidateAndSane(); err != nil {
		return createClusterImplArgs{}, err
	}

	if !args.FolderExtID.Valid {
		args.FolderExtID = optional.NewString(session.FolderCoords.FolderExtID)
	}

	if err := ch.validateGeoBaseURI(ctx, args.ClusterSpec.Config.GeobaseURI); err != nil {
		return createClusterImplArgs{}, nil
	}

	v, err := ch.validateClickHouseVersion(session, args.ClusterSpec.Version)
	args.ClusterSpec.Version = v
	if err != nil {
		return createClusterImplArgs{}, err
	}

	hostSpecs, err := chmodels.SplitHostsByType(args.HostSpecs)
	if err != nil {
		return createClusterImplArgs{}, err
	}

	var hostGroups []clusterslogic.HostGroup
	for i := 0; i < len(args.ClusterSpec.ShardResources); i++ {
		clickHouseResources := args.ClusterSpec.ShardResources[i]
		if err := clickHouseResources.Validate(true); err != nil {
			return createClusterImplArgs{}, err
		}

		hostGroups = append(hostGroups, clusterslogic.HostGroup{
			Role:                   hosts.RoleClickHouse,
			NewResourcePresetExtID: clickHouseResources.ResourcePresetExtID,
			DiskTypeExtID:          clickHouseResources.DiskTypeExtID.Must(),
			NewDiskSize:            clickHouseResources.DiskSize,
			HostsToAdd:             chmodels.ToZoneHosts(hostSpecs.ClickHouseNodes),
			ShardName:              optional.NewString(args.ShardNames[i]),
		})
	}
	resolvedGroups, _, err := creator.ValidateResources(ctx, session, clusters.TypeClickHouse, hostGroups...)
	if err != nil {
		return createClusterImplArgs{}, err
	}

	if args.ServiceAccountID != "" {
		if resolvedGroups.Single().TargetResourcePreset().VType != environment.VTypeCompute {
			return createClusterImplArgs{}, semerr.FailedPrecondition("service accounts are not supported in porto clusters")
		}

		if err := session.ValidateServiceAccount(ctx, args.ServiceAccountID); err != nil {
			return createClusterImplArgs{}, err
		}
	}

	if args.ClusterSpec.EnableCloudStorage.Bool {
		if err := clickhouse.ValidateCloudStorageSupported(args.ClusterSpec.Version); err != nil {
			return createClusterImplArgs{}, err
		}
	}

	if args.ClusterSpec.EmbeddedKeeper {
		if err := clickhouse.ValidateClickHouseKeeperSupported(args.ClusterSpec.Version); err != nil {
			return createClusterImplArgs{}, err
		}
	}

	var shardSpecs []shardSpec
	for _, shardsName := range args.ShardNames {
		shardSpecs = append(shardSpecs, shardSpec{
			ShardName:   shardsName,
			ShardWeight: chmodels.DefaultShardWeight,
			ShardHosts:  hostSpecs.ClickHouseNodes,
		})
	}

	return createClusterImplArgs{
		FolderExtID:          args.FolderExtID.Must(),
		Name:                 args.Name,
		Description:          args.Description,
		Labels:               args.Labels,
		ClusterSpec:          args.ClusterSpec.Combine(),
		BackupWindowStart:    args.ClusterSpec.BackupWindowStart,
		Environment:          args.Environment,
		NetworkID:            args.NetworkID,
		SecurityGroupIDs:     args.SecurityGroupIDs,
		UserSpecs:            args.UserSpecs,
		DatabaseSpecs:        args.DatabaseSpecs,
		ClickHouseHostGroups: resolvedGroups,
		ClusterHosts:         hostSpecs,
		DeletionProtection:   args.DeletionProtection,
		MaintenanceWindow:    args.MaintenanceWindow,
		ServiceAccountID:     args.ServiceAccountID,
		Shards:               shardSpecs,
	}, nil
}

/*
// Can't use, because both contains Unmanaged
type targetPillar struct {
	chpillars.ClusterCHData
	chpillars.SubClusterCHData
}
*/
type targetPillar struct {
	// Cluster pillar
	pillars.ClusterData

	S3Bucket                  string              `json:"s3_bucket"`
	ClusterPrivateKey         pillars.CryptoKey   `json:"cluster_private_key"`
	MDBMetrics                *pillars.MDBMetrics `json:"mdb_metrics,omitempty"`
	UseYASMAgent              *bool               `json:"use_yasmagent,omitempty"`
	SuppressExternalYASMAgent bool                `json:"suppress_external_yasmagent,omitempty"`
	ShipLogs                  *bool               `json:"ship_logs,omitempty"`
	Billing                   *pillars.Billing    `json:"billing,omitempty"`
	MDBHealth                 *pillars.MDBHealth  `json:"mdb_health,omitempty"`

	// SubCluster pillar
	Access           *chpillars.AccessSettings    `json:"access,omitempty"`
	Backup           bmodels.BackupSchedule       `json:"backup"`
	CHBackup         json.RawMessage              `json:"ch_backup,omitempty"`
	ClickHouse       chpillars.SubClusterCHServer `json:"clickhouse"`
	ServiceAccountID *string                      `json:"service_account_id,omitempty"`
	CloudStorage     chpillars.CloudStorage       `json:"cloud_storage"`
	UseCHBackup      *bool                        `json:"use_ch_backup,omitempty"`
	MonrunConfig     json.RawMessage              `json:"monrun,omitempty"`
	Unmanaged        json.RawMessage              `json:"unmanaged,omitempty"`
	TestingRepos     *bool                        `json:"testing_repos,omitempty"`
}

func (ch *ClickHouse) RestoreDataCloudCluster(ctx context.Context, globalBackupID string, args clickhouse.CreateDataCloudClusterArgs) (operations.Operation, error) {
	sourceCid, backupID, err := bmodels.DecodeGlobalBackupID(globalBackupID)
	if err != nil {
		return operations.Operation{}, err
	}

	return ch.operator.Restore(ctx, args.ProjectID, func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader, restorer clusterslogic.Restorer) (clusters.Cluster, operations.Operation, error) {
		backups, sourceCluster, sourceSubCluster, err := ch.loadSourceClusterInfo(ctx, reader, restorer, sourceCid, []string{backupID})
		if err != nil {
			return clusters.Cluster{}, operations.Operation{}, err
		}

		resources, err := loadShardResourcesForMultipleShards(ctx, restorer, backups, args.ClusterSpec.ClickHouseResources, sourceCluster)
		if err != nil {
			return clusters.Cluster{}, operations.Operation{}, err
		}

		if err := ch.copyDataCloudSourceConfig(sourceCluster, sourceSubCluster, resources[0], &args); err != nil {
			return clusters.Cluster{}, operations.Operation{}, err
		}

		implArgs, err := ch.prepareDataCloudClusterCreate(ctx, session, restorer, args)
		if err != nil {
			return clusters.Cluster{}, operations.Operation{}, err
		}

		return ch.restoreClusterImpl(ctx, session, restorer, restoreClusterImplArgs{
			backups:               backups,
			sourceCluster:         sourceCluster,
			sourceSubCluster:      sourceSubCluster,
			createClusterImplArgs: implArgs,
		})
	})
}

func (ch *ClickHouse) RestoreMDBCluster(ctx context.Context, globalBackupIDs []string, args clickhouse.RestoreMDBClusterArgs) (operations.Operation, error) {
	backupIDs, sourceCid, err := validateBackupIDs(globalBackupIDs)
	if err != nil {
		return operations.Operation{}, err
	}

	return ch.operator.RestoreWithoutBackupService(ctx, args.FolderExtID, sourceCid, func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader, restorer clusterslogic.Restorer) (clusters.Cluster, operations.Operation, error) {
		backups, sourceCluster, sourceSubCluster, err := ch.loadSourceClusterInfo(ctx, reader, restorer, sourceCid, backupIDs)
		if err != nil {
			return clusters.Cluster{}, operations.Operation{}, err
		}

		if args.Name == "" {
			args.Name = fmt.Sprintf("%s_restored", sourceCluster.Name)
		}
		if args.Description == "" {
			args.Description = fmt.Sprintf("Restored from backup(s): %s", strings.Join(backupIDs, ", "))
		}

		if (!args.ClusterSpec.EnableCloudStorage.Valid || !args.ClusterSpec.EnableCloudStorage.Bool) && sourceSubCluster.Pillar.CloudStorageEnabled() {
			return clusters.Cluster{}, operations.Operation{}, semerr.FailedPreconditionf("Backup with hybrid storage can not be restored to cluster without hybrid storage")
		}

		shardResources, err := loadShardResourcesForMultipleShards(ctx, restorer, backups, args.ClusterSpec.ShardResources[0], sourceCluster)
		if err != nil {
			return clusters.Cluster{}, operations.Operation{}, err
		}

		if err := ch.copyMDBSourceConfig(ctx, reader, sourceSubCluster, shardResources, backups, &args); err != nil {
			return clusters.Cluster{}, operations.Operation{}, err
		}

		implArgs, err := ch.prepareMDBClusterRestore(ctx, session, restorer, args)
		if err != nil {
			return clusters.Cluster{}, operations.Operation{}, err
		}

		return ch.restoreClusterImpl(ctx, session, restorer, restoreClusterImplArgs{
			backups:               backups,
			sourceCluster:         sourceCluster,
			sourceSubCluster:      sourceSubCluster,
			createClusterImplArgs: implArgs,
		})
	})
}

// validateBackupIDs checks if all backups are made from the same cluster.
// Returns list of backup IDs to restore cluster from as well as its cluster ID.
// It ensures that returned list does not contain duplicate backups IDs.
func validateBackupIDs(globalBackupIDs []string) ([]string, string, error) {
	var backupIDs []string
	var foreighBackupIDs []string

	mainCid, _, err := bmodels.DecodeGlobalBackupID(globalBackupIDs[0])
	if err != nil {
		return nil, "", err
	}

	for _, globalBackupID := range globalBackupIDs {
		backupCid, backupID, err := bmodels.DecodeGlobalBackupID(globalBackupID)
		if err != nil {
			return nil, "", err
		}

		if backupCid != mainCid {
			foreighBackupIDs = append(foreighBackupIDs, fmt.Sprintf("%q", globalBackupID))
			continue
		}

		backupIDs = append(backupIDs, backupID)
	}

	if len(foreighBackupIDs) > 0 {
		return nil, "", semerr.FailedPreconditionf("backups [%s] does not belong to the same cluster with id %q", strings.Join(foreighBackupIDs, ", "), mainCid)
	}

	return slices.DedupStrings(backupIDs), mainCid, nil
}

func (ch *ClickHouse) loadSourceClusterInfo(
	ctx context.Context,
	reader clusterslogic.Reader,
	restorer clusterslogic.Restorer,
	cid string,
	backupIDs []string,
) ([]bmodels.Backup, cluster, subCluster, error) {
	var backups []bmodels.Backup
	shardBackups := map[string][]string{}

	useBackupAPI, err := reader.ClusterUsesBackupService(ctx, cid)
	if err != nil {
		return nil, cluster{}, subCluster{}, err
	}

	if useBackupAPI {
		for _, backupID := range backupIDs {
			backup, err := ch.backups.ManagedBackupByBackupID(ctx, backupID, BackupConverter{})
			if err != nil {
				return nil, cluster{}, subCluster{}, err
			}

			backups = append(backups, backup)
		}
	} else {
		backups, _, err = ch.backups.BackupsByClusterID(ctx, cid, func(ctx context.Context, s3client s3.Client, cid string) ([]bmodels.Backup, error) {
			return listS3ClusterBackups(ctx, reader, s3client, cid, backupIDs...)
		}, bmodels.BackupsPageToken{}, optional.NewInt64(int64(len(backupIDs))))
		if err != nil {
			return nil, cluster{}, subCluster{}, err
		}
	}

	for _, backup := range backups {
		if len(backup.SourceShardNames) != 1 {
			return nil, cluster{}, subCluster{}, xerrors.Errorf("malformed backup: there can be only one shard_name in backup %+v", backup)
		}

		shardBackups[backup.SourceShardNames[0]] = append(shardBackups[backup.SourceShardNames[0]], bmodels.EncodeGlobalBackupID(cid, backup.ID))
	}

	if err := validateBackupShards(shardBackups); err != nil {
		return nil, cluster{}, subCluster{}, err
	}

	sort.Slice(backups, func(i, j int) bool {
		return backups[i].CreatedAt.After(backups[j].CreatedAt)
	})

	sourceCluster, err := restorer.ClusterAtTime(ctx, cid, backups[0].CreatedAt, clusters.TypeClickHouse)
	if err != nil {
		return nil, cluster{}, subCluster{}, err
	}

	var sourceClusterPillar chpillars.ClusterCH
	if err := sourceCluster.Pillar(&sourceClusterPillar); err != nil {
		return nil, cluster{}, subCluster{}, err
	}

	sourceSubCluster, err := ch.chSubClusterAtRevision(ctx, reader, cid, sourceCluster.Revision)
	if err != nil {
		return nil, cluster{}, subCluster{}, err
	}

	return backups, cluster{
		Cluster: sourceCluster.Cluster,
		Pillar:  &sourceClusterPillar,
	}, sourceSubCluster, nil
}

// validateBackupShards checks if each shard has only one backup to restore from
func validateBackupShards(shardBackups map[string][]string) error {
	var errorMsg []string

	for shardName, globalBackupIDs := range shardBackups {
		if len(globalBackupIDs) > 1 {
			for i := 0; i < len(globalBackupIDs); i++ {
				globalBackupIDs[i] = fmt.Sprintf("%q", globalBackupIDs[i])
			}
			sort.Strings(globalBackupIDs)
			errorMsg = append(errorMsg, fmt.Sprintf("shard %q: backup ids: [%s]", shardName, strings.Join(globalBackupIDs, ", ")))
		}
	}

	if len(errorMsg) > 0 {
		return semerr.FailedPreconditionf("two or more backups are associated to the same shard. there must be only one backup per shard. %s", strings.Join(errorMsg, "; "))
	}

	return nil
}

func loadShardResourcesForMultipleShards(ctx context.Context, restorer clusterslogic.Restorer, backups []bmodels.Backup, targetResources models.ClusterResourcesSpec, sourceCluster cluster) ([]models.ClusterResources, error) {
	var shardResources []models.ClusterResources

	for _, backup := range backups {
		resources, err := restorer.ShardResourcesAtRevision(ctx, backup.SourceShardNames[0], sourceCluster.ClusterID, sourceCluster.Revision)
		if err != nil {
			return nil, semerr.FailedPreconditionf(
				// format string
				"backups are incompatible. backup %q contains data for shard %q but this shard didn't exist at the time of backup %q creation",
				bmodels.EncodeGlobalBackupID(sourceCluster.ClusterID, backup.ID),
				backup.SourceShardNames[0],
				bmodels.EncodeGlobalBackupID(sourceCluster.ClusterID, backups[0].ID), // latest created backup
			)
		}

		newResources := models.ClusterResources{}

		if targetResources.ResourcePresetExtID.Valid {
			newResources.ResourcePresetExtID = targetResources.ResourcePresetExtID.Must()
		} else {
			newResources.ResourcePresetExtID = resources.ResourcePresetExtID
		}

		if targetResources.DiskSize.Valid {
			if resources.DiskSize > targetResources.DiskSize.Must() {
				return nil, semerr.InvalidInputf("insufficient disk_size, increase it to %d", resources.DiskSize)
			}

			newResources.DiskSize = targetResources.DiskSize.Must()
		} else {
			newResources.DiskSize = resources.DiskSize
		}

		if targetResources.DiskTypeExtID.Valid {
			newResources.DiskTypeExtID = targetResources.DiskTypeExtID.Must()
		} else {
			newResources.DiskTypeExtID = resources.DiskTypeExtID
		}

		shardResources = append(shardResources, newResources)
	}

	return shardResources, nil
}

func getOverlappedShardGroupsByBackups(shardGroups map[string]chpillars.ShardGroup, backups []bmodels.Backup) map[string]chpillars.ShardGroup {
	res := map[string]chpillars.ShardGroup{}

	for name, shardGroup := range shardGroups {
		var tmpShardGroup chpillars.ShardGroup

		for i := 0; i < len(backups); i++ {
			if slices.ContainsString(shardGroup.ShardNames, backups[i].SourceShardNames[0]) {
				tmpShardGroup.ShardNames = append(tmpShardGroup.ShardNames, backups[i].SourceShardNames[0])
			}
		}

		if len(tmpShardGroup.ShardNames) > 0 {
			tmpShardGroup.Description = shardGroup.Description

			sort.Strings(tmpShardGroup.ShardNames)

			res[name] = tmpShardGroup
		}
	}

	return res
}

func (ch *ClickHouse) copyMDBSourceConfig(
	ctx context.Context,
	reader clusterslogic.Reader,
	sourceChSubCluster subCluster,
	shardResources []models.ClusterResources,
	backups []bmodels.Backup,
	restoreArgs *clickhouse.RestoreMDBClusterArgs,
) error {
	for _, backup := range backups {
		restoreArgs.ShardNames = append(restoreArgs.ShardNames, backup.SourceShardNames[0])
	}

	if restoreArgs.ServiceAccountID == "" {
		restoreArgs.ServiceAccountID = sourceChSubCluster.Pillar.ServiceAccountID().String
	}

	if !restoreArgs.ClusterSpec.BackupWindowStart.Valid {
		restoreArgs.ClusterSpec.BackupWindowStart = bmodels.OptionalBackupWindowStart{
			Value: sourceChSubCluster.Pillar.Data.Backup.Start,
			Valid: true,
		}
	}

	if access := sourceChSubCluster.Pillar.Data.Access; access != nil {
		if !restoreArgs.ClusterSpec.Access.DataLens.Valid && access.DataLens != nil {
			restoreArgs.ClusterSpec.Access.DataLens = optional.NewBool(*sourceChSubCluster.Pillar.Data.Access.DataLens)
		}
		if !restoreArgs.ClusterSpec.Access.WebSQL.Valid && access.WebSQL != nil {
			restoreArgs.ClusterSpec.Access.WebSQL = optional.NewBool(*sourceChSubCluster.Pillar.Data.Access.WebSQL)
		}
		if !restoreArgs.ClusterSpec.Access.Metrica.Valid && access.Metrika != nil {
			restoreArgs.ClusterSpec.Access.Metrica = optional.NewBool(*sourceChSubCluster.Pillar.Data.Access.Metrika)
		}
		if !restoreArgs.ClusterSpec.Access.Serverless.Valid && access.Serverless != nil {
			restoreArgs.ClusterSpec.Access.Serverless = optional.NewBool(*sourceChSubCluster.Pillar.Data.Access.Serverless)
		}
		if !restoreArgs.ClusterSpec.Access.DataTransfer.Valid && access.DataTransfer != nil {
			restoreArgs.ClusterSpec.Access.DataTransfer = optional.NewBool(*sourceChSubCluster.Pillar.Data.Access.DataTransfer)
		}
		if !restoreArgs.ClusterSpec.Access.YandexQuery.Valid && access.YandexQuery != nil {
			restoreArgs.ClusterSpec.Access.YandexQuery = optional.NewBool(*sourceChSubCluster.Pillar.Data.Access.YandexQuery)
		}
	}

	if !restoreArgs.ClusterSpec.SQLUserManagement.Valid {
		restoreArgs.ClusterSpec.SQLUserManagement = optional.NewBool(sourceChSubCluster.Pillar.SQLUserManagement())
	}
	if !restoreArgs.ClusterSpec.SQLDatabaseManagement.Valid {
		restoreArgs.ClusterSpec.SQLDatabaseManagement = optional.NewBool(sourceChSubCluster.Pillar.SQLDatabaseManagement())
	}
	if !restoreArgs.ClusterSpec.EnableCloudStorage.Valid {
		restoreArgs.ClusterSpec.EnableCloudStorage = optional.NewBool(sourceChSubCluster.Pillar.CloudStorageEnabled())
	}

	//TODO check not set when keeper, mysql, psql added in spec
	restoreArgs.ClusterSpec.EmbeddedKeeper = sourceChSubCluster.Pillar.EmbeddedKeeper()
	restoreArgs.ClusterSpec.PostgreSQLProtocol = pillars.MapPtrBoolToOptionalBool(sourceChSubCluster.Pillar.Data.ClickHouse.PostgresqlProtocol)
	restoreArgs.ClusterSpec.MySQLProtocol = pillars.MapPtrBoolToOptionalBool(sourceChSubCluster.Pillar.Data.ClickHouse.MysqlProtocol)

	if restoreArgs.ClusterSpec.Version == "" {
		var err error
		restoreArgs.ClusterSpec.Version, err = ch.getSourceOrMinimalVersion(sourceChSubCluster.Pillar.Data.ClickHouse.Version)
		if err != nil {
			return err
		}
	}

	var resources []models.ClusterResourcesSpec
	for i := 0; i < len(shardResources); i++ {
		resources = append(resources, models.ClusterResourcesSpec{
			ResourcePresetExtID: optional.NewString(shardResources[i].ResourcePresetExtID),
			DiskSize:            optional.NewInt64(shardResources[i].DiskSize),
			DiskTypeExtID:       optional.NewString(shardResources[i].DiskTypeExtID),
		})
	}
	restoreArgs.ClusterSpec.ShardResources = resources

	var zkHosts []hosts.HostExtended
	if len(restoreArgs.HostSpecs) == 0 {
		var (
			clusterHosts []hosts.HostExtended
			h            []hosts.HostExtended
			offset       int64 = 0
			hasMore            = true
			err          error
		)
		for hasMore {
			h, offset, hasMore, err = reader.ListHosts(ctx, sourceChSubCluster.ClusterID, pagination.MaxPageSize, offset)
			if err != nil {
				return err
			}
			clusterHosts = append(clusterHosts, h...)
		}

		var hostSpecs []chmodels.HostSpec
		hostSpecs, zkHosts, err = ch.restoreBackupHostsFromSourceCluster(ctx, reader, clusterHosts, backups)
		if err != nil {
			return err
		}
		restoreArgs.HostSpecs = hostSpecs
	}
	if !restoreArgs.ClusterSpec.ZookeeperResources.IsSet() && len(zkHosts) > 0 {
		zkHost := zkHosts[0]
		restoreArgs.ClusterSpec.ZookeeperResources = models.ClusterResourcesSpec{
			ResourcePresetExtID: optional.NewString(zkHost.ResourcePresetExtID),
			DiskSize:            optional.NewInt64(zkHost.SpaceLimit),
			DiskTypeExtID:       optional.NewString(zkHost.DiskTypeExtID),
		}
	}

	return nil
}

// restoreBackupHostsFromSourceCluster restores ZK configuration from source cluster and adds its hosts to host specs
// as well as restores specs for CH hosts and adds to host specs only those, that are present in backups
func (ch ClickHouse) restoreBackupHostsFromSourceCluster(ctx context.Context, reader clusterslogic.Reader, sourceHosts []hosts.HostExtended, backups []bmodels.Backup) (hostSpecs []chmodels.HostSpec, zkHosts []hosts.HostExtended, err error) {
	var (
		hostSpecsByShard = map[string][]chmodels.HostSpec{}
		hostCountByShard = map[string]int64{}
	)
	for _, sourceHost := range sourceHosts {
		if sourceHost.Roles[0] == hosts.RoleZooKeeper {
			zkHosts = append(zkHosts, sourceHost)
			continue
		}

		shard, err := ch.chShard(ctx, reader, sourceHost.ShardID.Must())
		if err != nil {
			return nil, nil, err
		}

		hostSpecsByShard[shard.Name] = append(hostSpecsByShard[shard.Name], chmodels.HostSpec{
			ZoneID:         sourceHost.ZoneID,
			HostRole:       sourceHost.Roles[0],
			ShardName:      shard.Name,
			AssignPublicIP: sourceHost.AssignPublicIP,
		})
	}

	for _, backup := range backups {
		hostCountByShard[backup.SourceShardNames[0]] += 1
	}

	for shardName, hostCount := range hostCountByShard {
		hostSpecs = append(hostSpecs, hostSpecsByShard[shardName][:hostCount]...)
	}
	for _, zkHost := range zkHosts {
		hostSpecs = append(hostSpecs, chmodels.HostSpec{
			ZoneID:         zkHost.ZoneID,
			HostRole:       hosts.RoleZooKeeper,
			AssignPublicIP: zkHost.AssignPublicIP,
		})
	}

	return hostSpecs, zkHosts, nil
}

func (ch *ClickHouse) copyDataCloudSourceConfig(
	chCluster cluster,
	chSubCluster subCluster,
	chResources models.ClusterResources,
	args *clickhouse.CreateDataCloudClusterArgs,
) error {
	if args.ClusterSpec.Version == "" {
		var err error
		args.ClusterSpec.Version, err = ch.getSourceOrMinimalVersion(chSubCluster.Pillar.Data.ClickHouse.Version)
		if err != nil {
			return err
		}
	}

	args.CloudType = chCluster.Pillar.Data.CloudType

	if args.RegionID == "" {
		args.RegionID = chCluster.Pillar.Data.RegionID
	}

	if !args.ClusterSpec.ClickHouseResources.ResourcePresetExtID.Valid {
		args.ClusterSpec.ClickHouseResources.ResourcePresetExtID = optional.NewString(chResources.ResourcePresetExtID)
	}

	return nil
}

func (ch *ClickHouse) restoreClusterImpl(
	ctx context.Context,
	session sessions.Session,
	restorer clusterslogic.Restorer,
	args restoreClusterImplArgs,
) (clusters.Cluster, operations.Operation, error) {
	network, subnets, err := ch.compute.NetworkAndSubnets(ctx, args.NetworkID)
	if err != nil {
		return clusters.Cluster{}, operations.Operation{}, err
	}

	if len(args.SecurityGroupIDs) > 0 {
		args.SecurityGroupIDs = slices.DedupStrings(args.SecurityGroupIDs)
		if err := ch.compute.ValidateSecurityGroups(ctx, args.SecurityGroupIDs, network.ID); err != nil {
			return clusters.Cluster{}, operations.Operation{}, err
		}
	}

	chCluster, err := ch.createCluster(ctx, session, restorer, args.createClusterImplArgs, network)
	if err != nil {
		return clusters.Cluster{}, operations.Operation{}, err
	}

	backupSchedule := bmodels.GetBackupSchedule(ch.cfg.ClickHouse.Backup.BackupSchedule, args.BackupWindowStart)
	if err := ch.backups.AddBackupSchedule(ctx, chCluster.ClusterID, backupSchedule, chCluster.Revision); err != nil {
		return clusters.Cluster{}, operations.Operation{}, xerrors.Errorf("couldn't create chCluster during adding backup schedule: %w ", err)
	}

	pillar, err := ch.createPillar(session, chCluster, args.createClusterImplArgs, backupSchedule, createPillarOptions{AdminPassword: args.sourceSubCluster.Pillar.Data.ClickHouse.AdminPassword})
	if err != nil {
		return clusters.Cluster{}, operations.Operation{}, err
	}

	if err := ch.copyPillarSettings(pillar, args.sourceSubCluster.Pillar, args); err != nil {
		return clusters.Cluster{}, operations.Operation{}, err
	}

	s3Buckets := map[string]string{"backup": ch.cfg.S3BucketName(chCluster.ClusterID)}
	if args.ClusterSpec.EnableCloudStorage.Bool {
		s3Buckets["cloud_storage"] = ch.cfg.ClickHouse.CloudStorageBucketName(chCluster.ClusterID)
	}

	resolvedGroups := args.ClickHouseHostGroups
	if !args.ClusterSpec.EmbeddedKeeper && args.ClusterHosts.NeedZookeeper() {
		zkResources, zkHostSpecs, err := ch.buildZookeeperResources(ctx, session, restorer, args.ClickHouseHostGroups.MustGroupsByHostRole(hosts.RoleClickHouse),
			args.ClusterHosts, []hosts.HostExtended{}, args.ClusterSpec.ZookeeperResources, args.RegionID, subnets)
		if err != nil {
			return clusters.Cluster{}, operations.Operation{}, err
		}

		args.ClusterHosts.ZooKeeperNodes = zkHostSpecs

		resolvedZkGroups, _, err := restorer.ValidateResources(ctx, session, clusters.TypeClickHouse, clusterslogic.HostGroup{
			Role:                   hosts.RoleZooKeeper,
			NewResourcePresetExtID: optional.NewString(zkResources.ResourcePresetExtID),
			DiskTypeExtID:          zkResources.DiskTypeExtID,
			NewDiskSize:            optional.NewInt64(zkResources.DiskSize),
			HostsToAdd:             chmodels.ToZoneHosts(args.ClusterHosts.ZooKeeperNodes),
		})
		if err != nil {
			return clusters.Cluster{}, operations.Operation{}, err
		}

		resolvedGroups = clusterslogic.NewResolvedHostGroups(append(args.ClickHouseHostGroups.MustGroupsByHostRole(hosts.RoleClickHouse), resolvedZkGroups.Single()))

		if err := ch.validateZookeeperCores(ctx, session, restorer, resolvedGroups, ""); err != nil {
			return clusters.Cluster{}, operations.Operation{}, err
		}

		if err := ch.createZKSubCluster(ctx, session, restorer, chCluster, args.ClusterHosts.ZooKeeperNodes, resolvedZkGroups.Single(), subnets); err != nil {
			return clusters.Cluster{}, operations.Operation{}, err
		}
	}

	chSubCluster, err := ch.createCHSubCluster(ctx, session, restorer, chCluster, pillar, resolvedGroups.MustMapByShardName(hosts.RoleClickHouse), subnets, args.Shards)
	if err != nil {
		return clusters.Cluster{}, operations.Operation{}, err
	}

	targetPillarID, err := restorer.AddTargetPillar(ctx, chCluster.ClusterID, pillars.MakeTargetPillar(targetPillar{
		// Cluster data
		ClusterData:               args.sourceCluster.Pillar.Data.ClusterData,
		S3Bucket:                  args.sourceCluster.Pillar.Data.S3Bucket,
		ClusterPrivateKey:         args.sourceCluster.Pillar.Data.ClusterPrivateKey,
		MDBMetrics:                args.sourceCluster.Pillar.Data.MDBMetrics,
		UseYASMAgent:              args.sourceCluster.Pillar.Data.UseYASMAgent,
		SuppressExternalYASMAgent: args.sourceCluster.Pillar.Data.SuppressExternalYASMAgent,
		ShipLogs:                  args.sourceCluster.Pillar.Data.ShipLogs,
		Billing:                   args.sourceCluster.Pillar.Data.Billing,
		MDBHealth:                 args.sourceCluster.Pillar.Data.MDBHealth,
		// SubCluster data
		Access:           args.sourceSubCluster.Pillar.Data.Access,
		Backup:           args.sourceSubCluster.Pillar.Data.Backup,
		CHBackup:         args.sourceSubCluster.Pillar.Data.CHBackup,
		ClickHouse:       args.sourceSubCluster.Pillar.Data.ClickHouse,
		ServiceAccountID: args.sourceSubCluster.Pillar.Data.ServiceAccountID,
		CloudStorage:     args.sourceSubCluster.Pillar.Data.CloudStorage,
		UseCHBackup:      args.sourceSubCluster.Pillar.Data.UseCHBackup,
		MonrunConfig:     args.sourceSubCluster.Pillar.Data.MonrunConfig,
		Unmanaged:        args.sourceSubCluster.Pillar.Data.Unmanaged,
		TestingRepos:     args.sourceSubCluster.Pillar.Data.TestingRepos,
	}))
	if err != nil {
		return clusters.Cluster{}, operations.Operation{}, err
	}

	var backupIDs []string
	var globalBackupIDs []string
	for _, backup := range args.backups {
		backupIDs = append(backupIDs, backup.ID)
		globalBackupIDs = append(globalBackupIDs, bmodels.EncodeGlobalBackupID(backup.SourceClusterID, backup.ID))
	}

	var restoreFromBackups []map[string]string
	for i := 0; i < len(backupIDs); i++ {
		restoreFromBackups = append(
			restoreFromBackups,
			map[string]string{
				"backup-id":  backupIDs[i],
				"s3-path":    args.backups[i].S3Path,
				"shard-name": args.backups[i].SourceShardNames[0],
			},
		)
	}

	taskArgs := map[string]interface{}{
		"target-pillar-id": targetPillarID,
		"restore-from": map[string]interface{}{
			"cid":     args.sourceCluster.ClusterID,
			"backups": restoreFromBackups,
		},
		"source_s3_bucket":               ch.cfg.S3BucketName(args.sourceCluster.ClusterID),
		"source_cloud_storage_s3_bucket": args.sourceSubCluster.Pillar.CloudStorageBucket(),
		"update-geobase":                 args.ClusterSpec.Config.GeobaseURI.Valid,
		"service_account_id":             chSubCluster.Pillar.Data.ServiceAccountID,
		"s3_buckets":                     s3Buckets,
	}

	op, err := ch.tasks.CreateCluster(
		ctx,
		session,
		chCluster.ClusterID,
		chCluster.Revision,
		chmodels.TaskTypeClusterRestore,
		chmodels.OperationTypeClusterRestore,
		chmodels.MetadataRestoreCluster{
			SourceClusterID: args.sourceCluster.ClusterID,
			BackupID:        strings.Join(backupIDs, chmodels.DefaultBackupIDSeparator),
		},
		optional.String{}, // We pass additional buckets via options
		args.SecurityGroupIDs,
		clickHouseService,
		searchAttributesExtractor(chSubCluster),
		func(options *taskslogic.CreateClusterOptions) {
			options.TaskArgs = taskArgs
			options.Timeout = optional.NewDuration(getTimeout(optional.NewDuration(time.Hour*3), options.TaskArgs, resolvedGroups))
		},
	)
	if err != nil {
		return clusters.Cluster{}, operations.Operation{}, err
	}

	event := &cheventspub.RestoreCluster{
		Authentication:  ch.events.NewAuthentication(session.Subject),
		Authorization:   ch.events.NewAuthorization(session.Subject),
		RequestMetadata: ch.events.NewRequestMetadata(ctx),
		EventStatus:     cheventspub.RestoreCluster_STARTED,
		Details: &cheventspub.RestoreCluster_EventDetails{
			ClusterId: chCluster.ClusterID,
			BackupId:  strings.Join(globalBackupIDs, chmodels.DefaultBackupIDSeparator),
		},
	}

	em, err := ch.events.NewEventMetadata(event, op, session.FolderCoords.CloudExtID, session.FolderCoords.FolderExtID)
	if err != nil {
		return clusters.Cluster{}, operations.Operation{}, err
	}
	event.EventMetadata = em

	if err = ch.events.Store(ctx, event, op); err != nil {
		return clusters.Cluster{}, operations.Operation{}, xerrors.Errorf("saving event for op %+v: %w", op, err)
	}

	return chCluster.Cluster, op, nil
}

func (ch *ClickHouse) copyPillarSettings(
	targetPillar *chpillars.SubClusterCH,
	sourcePillar *chpillars.SubClusterCH,
	args restoreClusterImplArgs,
) error {
	if sourcePillar.Data.ClickHouse.ClusterName != nil && *sourcePillar.Data.ClickHouse.ClusterName != "" {
		targetPillar.Data.ClickHouse.ClusterName = sourcePillar.Data.ClickHouse.ClusterName
	} else {
		targetPillar.Data.ClickHouse.ClusterName = &args.sourceCluster.ClusterID
	}

	if !targetPillar.SQLUserManagement() {
		targetPillar.Data.ClickHouse.Users = sourcePillar.Data.ClickHouse.Users
	}

	if !targetPillar.SQLDatabaseManagement() {
		targetPillar.Data.ClickHouse.DBs = sourcePillar.Data.ClickHouse.DBs
	}

	if targetPillar.Data.ClickHouse.AdminPassword == nil {
		targetPillar.Data.ClickHouse.AdminPassword = sourcePillar.Data.ClickHouse.AdminPassword
	}

	if len(targetPillar.Data.ClickHouse.Users) == 0 {
		targetPillar.Data.ClickHouse.Users = sourcePillar.Data.ClickHouse.Users
	}

	targetPillar.Data.ClickHouse.FormatSchemas = sourcePillar.Data.ClickHouse.FormatSchemas
	targetPillar.Data.ClickHouse.MLModels = sourcePillar.Data.ClickHouse.MLModels

	targetPillar.Data.ClickHouse.ShardGroups = getOverlappedShardGroupsByBackups(
		sourcePillar.Data.ClickHouse.ShardGroups,
		args.backups,
	)

	targetPillar.Data.ClickHouse.Config = sourcePillar.Data.ClickHouse.Config
	if err := targetPillar.SetClickHouseConfig(ch.cryptoProvider, args.ClusterSpec.Config); err != nil {
		return err
	}

	return nil
}
